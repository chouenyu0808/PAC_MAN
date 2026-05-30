/**
 * PAC-MAN Multi-Screen Edition (With Sound)
 * ----------------------------
 * 此程式支援三機連動模式。燒錄前請根據板子的位置修改下方 ROLE 定義：
 * 1 = Master (中間螢幕 / 遊戲邏輯 / 音效中心)
 * 2 = Slave Left (左邊螢幕 / 玩家 1 輸入)
 * 3 = Slave Right (右邊螢幕 / 玩家 2 輸入)
 */

#define ROLE 1  // <--- 修改這裡：1:中, 2:左, 3:右

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SoftwareSerial.h>
#include <TM1638plus.h>

// --- 硬體腳位定義 ---
#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  8
#define TM_STB   7
#define TM_CLK   6
#define TM_DIO   5

// --- 通訊與喇叭腳位 ---
#if ROLE == 1
  SoftwareSerial SlaveL(2, 3);
  SoftwareSerial SlaveR(4, A5);
  #define BUZZER_PIN A4  // Master 的喇叭接在 A4
#else
  SoftwareSerial Comm(2, 3); // Slave 統一用 2,3 與 Master 溝通
#endif

// --- 遊戲常數 ---
#define SCREEN_W 320
#define TOTAL_W  960
#define PLAYER_SIZE 8

// ==========================================
// --- 音樂資料與非阻塞播放引擎 (僅 Master 使用) ---
// ==========================================
#if ROLE == 1
  #define NOTE_B4  494
  #define NOTE_C5  523
  #define NOTE_D5  587
  #define NOTE_DS5 622
  #define NOTE_E5  659
  #define NOTE_F5  698
  #define NOTE_FS5 740
  #define NOTE_G5  784
  #define NOTE_GS5 831
  #define NOTE_A5  880
  #define NOTE_B5  988
  #define NOTE_C6  1047

  // 經典開場音樂 (儲存於 Flash 記憶體節省 RAM)
  const int introMelody[] PROGMEM = {
    NOTE_B4, NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_B5, NOTE_FS5, NOTE_DS5,
    NOTE_C5, NOTE_C6, NOTE_G5, NOTE_E5, NOTE_C6, NOTE_G5, NOTE_E5,
    NOTE_B4, NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_B5, NOTE_FS5, NOTE_DS5,
    NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_B5
  };
  const int introDurations[] PROGMEM = {
    150, 150, 150, 150, 75, 75, 150,
    150, 150, 150, 150, 75, 75, 150,
    150, 150, 150, 150, 75, 75, 150,
    75, 75, 75, 75, 75, 75, 75, 75, 75, 300
  };

  // 死亡音效
  const int deathMelody[] PROGMEM = { NOTE_B5, NOTE_F5, NOTE_D5, NOTE_B4 };
  const int deathDurations[] PROGMEM = { 150, 150, 150, 300 };

  unsigned long noteStartTime = 0;
  int currentNote = 0;
  bool isPlayingMusic = false;
  int melodyLength = 0;
  const int* currentMelody;
  const int* currentDurations;

  void playMelody(const int* melody, const int* durations, int length) {
    currentMelody = melody;
    currentDurations = durations;
    melodyLength = length;
    currentNote = 0;
    isPlayingMusic = true;
    noteStartTime = millis();
    int freq = pgm_read_word(&currentMelody[0]);
    if (freq > 0) tone(BUZZER_PIN, freq);
  }

  void updateMusic() {
    if (!isPlayingMusic) return;
    unsigned long currentMillis = millis();
    int duration = pgm_read_word(&currentDurations[currentNote]);
    int playTime = duration * 0.8; 

    if (currentMillis - noteStartTime >= duration) {
      currentNote++;
      if (currentNote >= melodyLength) {
        noTone(BUZZER_PIN);
        isPlayingMusic = false;
      } else {
        int freq = pgm_read_word(&currentMelody[currentNote]);
        if (freq > 0) tone(BUZZER_PIN, freq);
        noteStartTime = currentMillis;
      }
    } else if (currentMillis - noteStartTime >= playTime) {
      noTone(BUZZER_PIN);
    }
  }
#endif
// ==========================================

// --- 顯示器與計分板初始化 ---
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
TM1638plus tm(TM_STB, TM_CLK, TM_DIO);

// --- 全域遊戲變數 ---
int p1X = 100, p1Y = 120; // 玩家 1 (Pac-Man)
int p2X = 860, p2Y = 120; // 玩家 2 (Ghost)
int p1Dir = 0, p2Dir = 0;
int score1 = 0, score2 = 0;

// 舊座標 (用於局部重繪，避免閃爍)
int lp1X, lp1Y, lp2X, lp2Y;

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  
  tm.displayBegin();
  tm.displayText("READY");

  #if ROLE == 1
    SlaveL.begin(38400);
    SlaveR.begin(38400);
    pinMode(BUZZER_PIN, OUTPUT);
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(100, 110);
    tft.print("MASTER UNIT");
    
    // 播放開場音樂
    playMelody(introMelody, introDurations, 31);
  #else
    Comm.begin(38400);
    // Slave 的按鈕腳位 (上, 下, 左, 右)
    pinMode(A0, INPUT_PULLUP);
    pinMode(A1, INPUT_PULLUP);
    pinMode(A2, INPUT_PULLUP);
    pinMode(A3, INPUT_PULLUP);
  #endif
}

void loop() {
  #if ROLE == 1
    // --- MASTER 邏輯 ---
    handleInputs();
    updateLogic();
    syncSlaves();
    render(320); // 中間螢幕 Offset 是 320
    updateMusic(); // 更新音樂狀態
  #else
    // --- SLAVE 邏輯 ---
    sendInput();
    receiveAndRender();
  #endif
  
  delay(10); 
}

// --- Master 專用函式 ---
#if ROLE == 1
void handleInputs() {
  if (SlaveL.available()) p1Dir = SlaveL.read();
  if (SlaveR.available()) p2Dir = SlaveR.read();
}

void updateLogic() {
  auto move = [](int &x, int &y, int dir) {
    if (dir == 1) y -= 4;
    if (dir == 2) y += 4;
    if (dir == 3) x -= 4;
    if (dir == 4) x += 4;
    x = constrain(x, 10, TOTAL_W - 10);
    y = constrain(y, 10, 230);
  };
  
  move(p1X, p1Y, p1Dir);
  move(p2X, p2Y, p2Dir);

  // 碰撞偵測
  if (abs(p1X - p2X) < 15 && abs(p1Y - p2Y) < 15) {
    score2++; // 幽靈得分
    p1X = 100; p2X = 860; // 重置
    tm.displayIntNum(score2, false);
    
    // 播放被抓到的音效
    playMelody(deathMelody, deathDurations, 4);
  }
}

void syncSlaves() {
  // 格式: p1X,p1Y,p2X,p2Y,s1,s2
  String packet = String(p1X) + "," + String(p1Y) + "," + String(p2X) + "," + String(p2Y) + "," + String(score1) + "," + String(score2) + "\n";
  SlaveL.print(packet);
  SlaveR.print(packet);
}
#endif

// --- Slave 專用函式 ---
#if ROLE != 1
void sendInput() {
  int dir = 0;
  if (digitalRead(A0) == LOW) dir = 1;
  else if (digitalRead(A1) == LOW) dir = 2;
  else if (digitalRead(A2) == LOW) dir = 3;
  else if (digitalRead(A3) == LOW) dir = 4;
  
  if (dir != 0) Comm.write(dir);
}

void receiveAndRender() {
  if (Comm.available()) {
    String data = Comm.readStringUntil('\n');
    parseData(data);
    
    #if ROLE == 2
      render(0);   // 左屏 Offset
      tm.displayIntNum(score1, false);
    #else
      render(640); // 右屏 Offset
      tm.displayIntNum(score2, false);
    #endif
  }
}

void parseData(String data) {
  // 簡易解析
  int vals[6];
  int count = 0;
  int lastPos = 0;
  for (int i = 0; i < data.length(); i++) {
    if (data[i] == ',' || data[i] == '\n') {
      vals[count++] = data.substring(lastPos, i).toInt();
      lastPos = i + 1;
      if (count >= 6) break;
    }
  }
  p1X = vals[0]; p1Y = vals[1]; p2X = vals[2]; p2Y = vals[3];
  score1 = vals[4]; score2 = vals[5];
}
#endif

// --- 共用繪圖函式 ---
void render(int offset) {
  // 擦除舊位置
  auto erase = [&](int x, int y) {
    int lx = x - offset;
    if (lx > -20 && lx < 340) {
      tft.fillCircle(lx, y, PLAYER_SIZE, ILI9341_BLACK);
    }
  };

  // 畫新位置
  auto draw = [&](int x, int y, uint16_t color) {
    int nx = x - offset;
    if (nx > -10 && nx < 330) {
      tft.fillCircle(nx, y, PLAYER_SIZE, color);
    }
  };

  erase(lp1X, lp1Y);
  erase(lp2X, lp2Y);
  
  draw(p1X, p1Y, ILI9341_YELLOW);
  draw(p2X, p2Y, ILI9341_RED);

  lp1X = p1X; lp1Y = p1Y; lp2X = p2X; lp2Y = p2Y;
}
