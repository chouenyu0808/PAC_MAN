/**
 * PAC-MAN Multi-Screen Edition (4-Screen Version)
 * ----------------------------
 * 1 = Master (中間螢幕 / 遊戲邏輯 / 音效中心)
 * 2 = Slave Left (左邊螢幕 / 玩家 1 輸入)
 * 3 = Slave Right (右邊螢幕 / 玩家 2 輸入)
 * 4 = Scoreboard (計分板螢幕 / 顯示大分數與頭像)
 */

#define ROLE 1  // <--- 修改這裡：1:中, 2:左, 3:右, 4:計分板

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SoftwareSerial.h>

// --- 硬體腳位定義 ---
#define TFT_CS   8
#define TFT_DC   10
#define TFT_RST  9

// --- 通訊與喇叭腳位 ---
#if ROLE == 1
  SoftwareSerial SlaveL(2, 3);
  SoftwareSerial SlaveR(4, A5);
  #define BUZZER_PIN A4
#else
  SoftwareSerial Comm(2, 3); // Slave 與 Scoreboard 統一用 2,3
#endif

// --- 遊戲常數 ---
#define SCREEN_W 320
#define TOTAL_W  960
#define PLAYER_SIZE 8

// ==========================================
// --- 音樂資料 (僅 Master 使用) ---
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

  const int introMelody[] PROGMEM = { NOTE_B4, NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_C5, NOTE_C6, NOTE_G5, NOTE_E5, NOTE_C6, NOTE_G5, NOTE_E5, NOTE_B4, NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_B5 };
  const int introDurations[] PROGMEM = { 150, 150, 150, 150, 75, 75, 150, 150, 150, 150, 150, 75, 75, 150, 150, 150, 150, 150, 75, 75, 150, 75, 75, 75, 75, 75, 75, 75, 75, 75, 300 };
  const int deathMelody[] PROGMEM = { NOTE_B5, NOTE_F5, NOTE_D5, NOTE_B4 };
  const int deathDurations[] PROGMEM = { 150, 150, 150, 300 };

  unsigned long noteStartTime = 0;
  int currentNote = 0;
  bool isPlayingMusic = false;
  int melodyLength = 0;
  const int* currentMelody;
  const int* currentDurations;

  void playMelody(const int* melody, const int* durations, int length) {
    currentMelody = melody; currentDurations = durations; melodyLength = length;
    currentNote = 0; isPlayingMusic = true; noteStartTime = millis();
    int freq = pgm_read_word(&currentMelody[0]);
    if (freq > 0) tone(BUZZER_PIN, freq);
  }

  void updateMusic() {
    if (!isPlayingMusic) return;
    unsigned long currentMillis = millis();
    int duration = pgm_read_word(&currentDurations[currentNote]);
    if (currentMillis - noteStartTime >= duration) {
      currentNote++;
      if (currentNote >= melodyLength) { noTone(BUZZER_PIN); isPlayingMusic = false; } 
      else {
        int freq = pgm_read_word(&currentMelody[currentNote]);
        if (freq > 0) tone(BUZZER_PIN, freq);
        noteStartTime = currentMillis;
      }
    } else if (currentMillis - noteStartTime >= duration * 0.8) { noTone(BUZZER_PIN); }
  }
#endif

// --- 顯示器初始化 ---
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// --- 全域遊戲變數 ---
int p1X = 100, p1Y = 120, p2X = 860, p2Y = 120;
int p1Dir = 0, p2Dir = 0, score1 = 0, score2 = 0;
int lp1X, lp1Y, lp2X, lp2Y;
int lastS1 = -1, lastS2 = -1; // 用於計分板重繪判定

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  #if ROLE == 1
    SlaveL.begin(38400); SlaveR.begin(38400);
    pinMode(BUZZER_PIN, OUTPUT);
    tft.setTextColor(ILI9341_YELLOW); tft.setCursor(100, 110); tft.print("MASTER UNIT");
    playMelody(introMelody, introDurations, 31);
  #elif ROLE == 4
    Comm.begin(38400);
    drawScoreUI(); // 初始化計分板介面
  #else
    Comm.begin(38400);
    pinMode(A0, INPUT_PULLUP); pinMode(A1, INPUT_PULLUP);
    pinMode(A2, INPUT_PULLUP); pinMode(A3, INPUT_PULLUP);
  #endif
}

void loop() {
  #if ROLE == 1
    handleInputs(); updateLogic(); syncSlaves(); render(320); updateMusic();
  #elif ROLE == 4
    receiveAndRenderScoreboard();
  #else
    sendInput(); receiveAndRender();
  #endif
  delay(10); 
}

// --- Master 專用 ---
#if ROLE == 1
void handleInputs() {
  if (SlaveL.available()) p1Dir = SlaveL.read();
  if (SlaveR.available()) p2Dir = SlaveR.read();
}
void updateLogic() {
  auto move = [](int &x, int &y, int dir) {
    if (dir == 1) y -= 4; if (dir == 2) y += 4;
    if (dir == 3) x -= 4; if (dir == 4) x += 4;
    x = constrain(x, 10, TOTAL_W - 10); y = constrain(y, 10, 230);
  };
  move(p1X, p1Y, p1Dir); move(p2X, p2Y, p2Dir);
  if (abs(p1X - p2X) < 15 && abs(p1Y - p2Y) < 15) {
    score2++; p1X = 100; p2X = 860;
    playMelody(deathMelody, deathDurations, 4);
  }
}
void syncSlaves() {
  String packet = String(p1X)+","+String(p1Y)+","+String(p2X)+","+String(p2Y)+","+String(score1)+","+String(score2)+"\n";
  SlaveL.print(packet); SlaveR.print(packet);
}
#endif

// --- Slave & Scoreboard 共用解析 ---
#if ROLE != 1
void parseData(String data) {
  int vals[6], count = 0, lastPos = 0;
  for (int i = 0; i < data.length(); i++) {
    if (data[i] == ',' || data[i] == '\n') {
      vals[count++] = data.substring(lastPos, i).toInt();
      lastPos = i + 1;
      if (count >= 6) break;
    }
  }
  if (count >= 6) {
    p1X = vals[0]; p1Y = vals[1]; p2X = vals[2]; p2Y = vals[3];
    score1 = vals[4]; score2 = vals[5];
  }
}
#endif

// --- Scoreboard (ROLE 4) 專用函式 ---
#if ROLE == 4
void drawScoreUI() {
  tft.fillScreen(ILI9341_BLACK);
  // 裝飾框
  tft.drawRect(5, 5, 310, 230, ILI9341_WHITE);
  tft.drawLine(160, 5, 160, 235, ILI9341_WHITE);
  
  // 畫 P1 頭像 (小精靈)
  tft.fillCircle(80, 60, 30, ILI9341_YELLOW);
  tft.fillTriangle(80, 60, 115, 40, 115, 80, ILI9341_BLACK);
  tft.setCursor(45, 100); tft.setTextColor(ILI9341_YELLOW); tft.setTextSize(2);
  tft.print("PAC-MAN");

  // 畫 P2 頭像 (幽靈)
  tft.fillCircle(240, 60, 30, ILI9341_RED);
  tft.fillRect(210, 60, 60, 30, ILI9341_RED);
  tft.setCursor(215, 100); tft.setTextColor(ILI9341_RED); tft.setTextSize(2);
  tft.print("GHOST");

  // VS 字樣
  tft.setCursor(145, 120); tft.setTextColor(ILI9341_WHITE); tft.setTextSize(3);
  tft.print("VS");
}

void receiveAndRenderScoreboard() {
  if (Comm.available()) {
    String data = Comm.readStringUntil('\n');
    parseData(data);
    
    // 只有分數改變時才更新，避免閃爍
    if (score1 != lastS1 || score2 != lastS2) {
      updateScoreText();
      lastS1 = score1; lastS2 = score2;
    }
  }
}

void updateScoreText() {
  tft.setTextSize(6);
  // 更新 P1 分數
  tft.fillRect(30, 140, 100, 60, ILI9341_BLACK);
  tft.setCursor(55, 150); tft.setTextColor(ILI9341_YELLOW);
  tft.print(score1);
  
  // 更新 P2 分數
  tft.fillRect(190, 140, 100, 60, ILI9341_BLACK);
  tft.setCursor(215, 150); tft.setTextColor(ILI9341_RED);
  tft.print(score2);
}
#endif

// --- Slave 專用 ---
#if ROLE == 2 || ROLE == 3
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
    render((ROLE == 2) ? 0 : 640);
  }
}
#endif

// --- 共用繪圖 (渲染遊戲畫面) ---
void render(int offset) {
  auto erase = [&](int x, int y) {
    int lx = x - offset;
    if (lx > -20 && lx < 340) tft.fillCircle(lx, y, PLAYER_SIZE, ILI9341_BLACK);
  };
  auto draw = [&](int x, int y, uint16_t color) {
    int nx = x - offset;
    if (nx > -10 && nx < 330) tft.fillCircle(nx, y, PLAYER_SIZE, color);
  };
  erase(lp1X, lp1Y); erase(lp2X, lp2Y);
  draw(p1X, p1Y, ILI9341_YELLOW); draw(p2X, p2Y, ILI9341_RED);
  lp1X = p1X; lp1Y = p1Y; lp2X = p2X; lp2Y = p2Y;
}
