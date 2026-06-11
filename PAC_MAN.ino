/**
 * PAC-MAN Multi-Screen I2C Edition (Portrait Mode Layout)
 * ----------------------------
 * 1 = Master (中間螢幕 240x320 / 偏移 240)
 * 2 = Slave Left (左邊螢幕 240x320 / 偏移 0)
 * 3 = Slave Right (右邊螢幕 240x320 / 偏移 480)
 * 4 = Scoreboard (計分板)
 */

#define ROLE 1

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Wire.h>

// --- 硬體腳位定義 ---
#define TFT_CS   8
#define TFT_DC   10
#define TFT_RST  9

#if ROLE == 1
  #define BUZZER_PIN 5 
#endif

// I2C 位址
#define ADDR_LEFT  8
#define ADDR_RIGHT 9
#define ADDR_SCORE 10

// --- 遊戲常數 (直向拼貼模式) ---
#define SCREEN_W 240
#define TOTAL_W  720
#define SCREEN_H 320
#define PLAYER_SIZE 8

// --- 全域遊戲變數 ---
int p1X = 50, p1Y = 160, p2X = 670, p2Y = 160;
int p1Dir = 0, p2Dir = 0, score1 = 0, score2 = 0;
int lp1X, lp1Y, lp2X, lp2Y;
int lastS1 = -1, lastS2 = -1;
volatile bool dataReceived = false;

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

// --- I2C 接收回呼 ---
void receiveEvent(int howMany) {
  if (howMany >= 12) {
    p1X = Wire.read() | (Wire.read() << 8);
    p1Y = Wire.read() | (Wire.read() << 8);
    p2X = Wire.read() | (Wire.read() << 8);
    p2Y = Wire.read() | (Wire.read() << 8);
    score1 = Wire.read() | (Wire.read() << 8);
    score2 = Wire.read() | (Wire.read() << 8);
    dataReceived = true;
  }
}

void requestEvent() {
  int dir = 0;
  if (digitalRead(A0) == LOW) dir = 1;
  else if (digitalRead(A1) == LOW) dir = 2;
  else if (digitalRead(A2) == LOW) dir = 3;
  else if (digitalRead(A3) == LOW) dir = 4;
  Wire.write(dir);
}

// --- 適合 720x320 的迷宮 ---
struct Wall {
  int x, y, w, h;
};

const Wall walls[] PROGMEM = {
  {0, 0, 720, 5}, {0, 315, 720, 5}, // 外框上下
  {0, 0, 5, 320}, {715, 0, 5, 320}, // 外框左右
  {60, 60, 120, 20}, {240, 60, 240, 20}, {540, 60, 120, 20}, // 上排牆
  {110, 120, 20, 80}, {350, 120, 20, 80}, {590, 120, 20, 80}, // 中排柱子
  {60, 240, 240, 20}, {420, 240, 240, 20}, // 下排牆
  {240, 150, 240, 20} // 中間橫條
};
const int wallCount = sizeof(walls) / sizeof(Wall);

bool checkCollision(int x, int y) {
  for (int i = 0; i < wallCount; i++) {
    int wx = pgm_read_word(&walls[i].x);
    int wy = pgm_read_word(&walls[i].y);
    int ww = pgm_read_word(&walls[i].w);
    int wh = pgm_read_word(&walls[i].h);
    // 檢查矩形碰撞 (考慮 Player 大小)
    if (x + PLAYER_SIZE > wx && x - PLAYER_SIZE < wx + ww &&
        y + PLAYER_SIZE > wy && y - PLAYER_SIZE < wy + wh) {
      return true;
    }
  }
  return false;
}

void drawMaze(int offset) {
  uint16_t wallColor = ILI9341_BLUE;
  for (int i = 0; i < wallCount; i++) {
    int x = pgm_read_word(&walls[i].x);
    int y = pgm_read_word(&walls[i].y);
    int w = pgm_read_word(&walls[i].w);
    int h = pgm_read_word(&walls[i].h);
    
    int lx = x - offset;
    if (lx + w > 0 && lx < 240) {
      int dx = max(0, lx);
      int dw = min(w, 240 - lx);
      if (lx < 0) dw = w + lx;
      tft.fillRect(dx, y, dw, h, wallColor);
    }
  }
}

void setup() {
  tft.begin();
  tft.setRotation(0); // 全部設為直向 (Portrait)
  
  #if ROLE == 1
    Wire.begin(); 
  #elif ROLE == 2
    Wire.begin(ADDR_LEFT); Wire.onReceive(receiveEvent); Wire.onRequest(requestEvent);
  #elif ROLE == 3
    Wire.begin(ADDR_RIGHT); Wire.onReceive(receiveEvent); Wire.onRequest(requestEvent);
  #elif ROLE == 4
    Wire.begin(ADDR_SCORE); Wire.onReceive(receiveEvent);
  #endif

  tft.fillScreen(ILI9341_BLACK);

  #if ROLE == 1
    drawMaze(240); pinMode(BUZZER_PIN, OUTPUT); playMelody(introMelody, introDurations, 31);
  #elif ROLE == 2
    drawMaze(0);
  #elif ROLE == 3
    drawMaze(480);
  #elif ROLE == 4
    drawScoreUI();
  #endif

  #if ROLE == 2 || ROLE == 3
    pinMode(A0, INPUT_PULLUP); pinMode(A1, INPUT_PULLUP);
    pinMode(A2, INPUT_PULLUP); pinMode(A3, INPUT_PULLUP);
  #endif
}

void loop() {
  #if ROLE == 1
    handleI2CInputs(); updateLogic(); syncI2CSlaves(); render(240); updateMusic();
  #elif ROLE == 4
    if (dataReceived) { 
      if (score1 != lastS1 || score2 != lastS2) { updateScoreText(); lastS1 = score1; lastS2 = score2; }
      dataReceived = false;
    }
  #else
    if (dataReceived) { render((ROLE == 2) ? 0 : 480); dataReceived = false; }
  #endif
  delay(10); 
}

#if ROLE == 1
void handleI2CInputs() {
  Wire.requestFrom(ADDR_LEFT, 1); if (Wire.available()) p1Dir = Wire.read();
  Wire.requestFrom(ADDR_RIGHT, 1); if (Wire.available()) p2Dir = Wire.read();
}
void updateLogic() {
  auto move = [](int &x, int &y, int dir) {
    int nextX = x, nextY = y;
    if (dir == 1) nextY -= 4; else if (dir == 2) nextY += 4;
    else if (dir == 3) nextX -= 4; else if (dir == 4) nextX += 4;
    
    if (!checkCollision(nextX, nextY)) {
      x = nextX; y = nextY;
    }
    x = constrain(x, 10, TOTAL_W - 10); y = constrain(y, 10, 310);
  };
  move(p1X, p1Y, p1Dir); move(p2X, p2Y, p2Dir);
  if (abs(p1X - p2X) < 15 && abs(p1Y - p2Y) < 15) {
    score2++; p1X = 50; p1Y = 160; p2X = 670; p2Y = 160; playMelody(deathMelody, deathDurations, 4);
  }
}
void syncI2CSlaves() {
  auto send = [](int addr) {
    Wire.beginTransmission(addr);
    int d[] = {p1X, p1Y, p2X, p2Y, score1, score2};
    for(int i=0; i<6; i++) { Wire.write(d[i] & 0xFF); Wire.write(d[i] >> 8); }
    Wire.endTransmission();
  };
  send(ADDR_LEFT); send(ADDR_RIGHT); send(ADDR_SCORE);
}
#endif

#if ROLE == 4
void drawScoreUI() {
  tft.fillScreen(ILI9341_BLACK);
  tft.drawRect(5, 5, 230, 310, ILI9341_WHITE);
  tft.fillCircle(120, 60, 30, ILI9341_YELLOW);
  tft.setCursor(80, 100); tft.setTextColor(ILI9341_YELLOW); tft.setTextSize(2); tft.print("PAC-MAN");
  tft.fillCircle(120, 200, 30, ILI9341_RED);
  tft.setCursor(90, 240); tft.setTextColor(ILI9341_RED); tft.setTextSize(2); tft.print("GHOST");
}
void updateScoreText() {
  tft.setTextSize(4);
  tft.fillRect(80, 120, 100, 40, ILI9341_BLACK);
  tft.setCursor(105, 120); tft.setTextColor(ILI9341_YELLOW); tft.print(score1);
  tft.fillRect(80, 260, 100, 40, ILI9341_BLACK);
  tft.setCursor(105, 260); tft.setTextColor(ILI9341_RED); tft.print(score2);
}
#endif

void render(int offset) {
  auto erase = [&](int x, int y) {
    int lx = x - offset;
    if (lx > -20 && lx < 260) tft.fillCircle(lx, y, PLAYER_SIZE + 1, ILI9341_BLACK);
  };
  if (millis() % 500 < 20) drawMaze(offset);
  erase(lp1X, lp1Y); erase(lp2X, lp2Y);
  int n1x = p1X - offset;
  if (n1x > -10 && n1x < 250) {
    tft.fillCircle(n1x, p1Y, PLAYER_SIZE, ILI9341_YELLOW);
    tft.fillTriangle(n1x, p1Y, n1x+10, p1Y-5, n1x+10, p1Y+5, ILI9341_BLACK);
  }
  int n2x = p2X - offset;
  if (n2x > -10 && n2x < 250) {
    tft.fillCircle(n2x, p2Y, PLAYER_SIZE, ILI9341_RED);
    tft.fillRect(n2x - PLAYER_SIZE, p2Y, PLAYER_SIZE * 2, PLAYER_SIZE, ILI9341_RED);
  }
  lp1X = p1X; lp1Y = p1Y; lp2X = p2X; lp2Y = p2Y;
}