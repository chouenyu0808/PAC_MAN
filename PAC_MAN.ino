/**
 * PAC-MAN Multi-Screen - Complex Maze Edition (Fixed Rendering & AI)
 */

#define ROLE 2// 1=中, 2=左, 3=右, 4=計分板

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Wire.h>

#define TFT_CS   8
#define TFT_DC   10
#define TFT_RST  9
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

#if ROLE == 1
  #define BUZZER_PIN 5 
#endif

#define ADDR_LEFT  8
#define ADDR_RIGHT 9
#define ADDR_SCORE 10

#define TOTAL_W  720
#define PLAYER_SIZE 8
#define NUM_GHOSTS 4

// --- 初始座標：經由地圖設計器自訂配置 ---
int p1X = 280, p1Y = 160;
int p2X = 440, p2Y = 160;
int p1Dir = 0, p2Dir = 0, p1NextDir = 0, p2NextDir = 0, score1 = 0, score2 = 0;
int lp1X, lp1Y, lp2X, lp2Y;
int gX[NUM_GHOSTS], gY[NUM_GHOSTS], lgX[NUM_GHOSTS], lgY[NUM_GHOSTS];
volatile bool dataReceived = false;
int levelResetCounter = 0;
unsigned long frightenedModeEndTime = 0;
unsigned long ghostEatenTime[NUM_GHOSTS] = {0, 0, 0, 0};
volatile bool frightenedModeActive = false;
volatile int frightenedModeDeciseconds = 0;
volatile int lives1 = 3;
volatile int lives2 = 3;
volatile bool gameOver = false;
volatile int winner = 0;

// --- 智慧型牢籠延遲釋放機制 (Ghost Cage Jail System) ---
bool gameStarted = false;
unsigned long gameStartTime = 0;
const unsigned long ghostReleaseDelays[NUM_GHOSTS] = {0, 3000, 6000, 9000};
int ghostBaseY[NUM_GHOSTS]; // 儲存預設關籠高程

// 音符定義
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

// --- 自訂設計地圖牆壁結構 (共有 274 個牆壁區塊) ---
struct Wall {
  int x, y, w, h;
};
const Wall walls[] PROGMEM = {
  {10, 10, 10, 10},
  {200, 10, 10, 10},
  {350, 10, 20, 10},
  {510, 10, 10, 10},
  {700, 10, 10, 10},
  {10, 20, 10, 10},
  {200, 20, 10, 10},
  {350, 20, 20, 10},
  {510, 20, 10, 10},
  {700, 20, 10, 10},
  {10, 30, 10, 10},
  {200, 30, 10, 10},
  {350, 30, 20, 10},
  {510, 30, 10, 10},
  {700, 30, 10, 10},
  {10, 40, 10, 10},
  {40, 40, 20, 10},
  {240, 40, 10, 10},
  {350, 40, 20, 10},
  {390, 40, 20, 10},
  {470, 40, 10, 10},
  {550, 40, 10, 10},
  {660, 40, 20, 10},
  {700, 40, 10, 10},
  {240, 50, 10, 10},
  {320, 50, 10, 10},
  {350, 50, 20, 10},
  {390, 50, 20, 10},
  {470, 50, 10, 10},
  {550, 50, 10, 10},
  {10, 60, 10, 10},
  {40, 60, 20, 10},
  {200, 60, 50, 10},
  {320, 60, 10, 10},
  {350, 60, 20, 10},
  {390, 60, 20, 10},
  {470, 60, 50, 10},
  {550, 60, 10, 10},
  {590, 60, 50, 10},
  {660, 60, 20, 10},
  {700, 60, 10, 10},
  {10, 70, 10, 10},
  {430, 70, 20, 10},
  {510, 70, 10, 10},
  {550, 70, 10, 10},
  {700, 70, 10, 10},
  {10, 80, 10, 10},
  {430, 80, 20, 10},
  {510, 80, 10, 10},
  {550, 80, 10, 10},
  {700, 80, 10, 10},
  {10, 90, 10, 10},
  {240, 90, 50, 10},
  {430, 90, 50, 10},
  {510, 90, 10, 10},
  {550, 90, 10, 10},
  {590, 90, 50, 10},
  {700, 90, 10, 10},
  {10, 100, 10, 10},
  {40, 100, 20, 10},
  {120, 100, 20, 10},
  {590, 100, 10, 10},
  {660, 100, 20, 10},
  {700, 100, 10, 10},
  {10, 110, 10, 10},
  {40, 110, 20, 10},
  {120, 110, 20, 10},
  {590, 110, 10, 10},
  {660, 110, 20, 10},
  {700, 110, 10, 10},
  {10, 120, 10, 10},
  {40, 120, 20, 10},
  {120, 120, 20, 10},
  {550, 120, 10, 10},
  {590, 120, 10, 10},
  {660, 120, 20, 10},
  {700, 120, 10, 10},
  {10, 130, 10, 10},
  {80, 130, 20, 10},
  {160, 130, 20, 10},
  {550, 130, 10, 10},
  {620, 130, 20, 10},
  {700, 130, 10, 10},
  {10, 140, 10, 10},
  {80, 140, 20, 10},
  {160, 140, 20, 10},
  {550, 140, 10, 10},
  {620, 140, 20, 10},
  {700, 140, 10, 10},
  {10, 150, 10, 10},
  {160, 150, 20, 10},
  {550, 150, 10, 10},
  {590, 150, 80, 10},
  {700, 150, 10, 10},
  {10, 160, 10, 10},
  {160, 160, 20, 10},
  {550, 160, 10, 10},
  {590, 160, 80, 10},
  {700, 160, 10, 10},
  {10, 170, 10, 10},
  {160, 170, 20, 10},
  {700, 170, 10, 10},
  {10, 180, 10, 10},
  {160, 180, 20, 10},
  {700, 180, 10, 10},
  {10, 190, 10, 10},
  {40, 190, 20, 10},
  {90, 190, 50, 10},
  {160, 190, 20, 10},
  {510, 190, 10, 10},
  {550, 190, 10, 10},
  {590, 190, 50, 10},
  {660, 190, 20, 10},
  {700, 190, 10, 10},
  {10, 200, 10, 10},
  {40, 200, 20, 10},
  {120, 200, 20, 10},
  {160, 200, 20, 10},
  {510, 200, 10, 10},
  {550, 200, 10, 10},
  {590, 200, 10, 10},
  {660, 200, 20, 10},
  {700, 200, 10, 10},
  {10, 210, 10, 10},
  {40, 210, 20, 10},
  {120, 210, 20, 10},
  {160, 210, 20, 10},
  {510, 210, 10, 10},
  {550, 210, 10, 10},
  {590, 210, 10, 10},
  {660, 210, 20, 10},
  {700, 210, 10, 10},
  {10, 220, 10, 10},
  {280, 220, 50, 10},
  {470, 220, 50, 10},
  {550, 220, 10, 10},
  {590, 220, 10, 10},
  {630, 220, 40, 10},
  {700, 220, 10, 10},
  {10, 230, 10, 10},
  {700, 230, 10, 10},
  {10, 240, 10, 10},
  {700, 240, 10, 10},
  {10, 250, 10, 10},
  {280, 250, 10, 10},
  {320, 250, 80, 10},
  {430, 250, 20, 10},
  {470, 250, 10, 10},
  {510, 250, 50, 10},
  {590, 250, 50, 10},
  {660, 250, 20, 10},
  {700, 250, 10, 10},
  {80, 260, 20, 10},
  {280, 260, 10, 10},
  {350, 260, 20, 10},
  {430, 260, 20, 10},
  {470, 260, 10, 10},
  {620, 260, 20, 10},
  {10, 270, 10, 10},
  {80, 270, 20, 10},
  {280, 270, 10, 10},
  {350, 270, 20, 10},
  {430, 270, 20, 10},
  {470, 270, 10, 10},
  {620, 270, 20, 10},
  {660, 270, 20, 10},
  {700, 270, 10, 10},
  {10, 280, 10, 10},
  {200, 280, 10, 10},
  {320, 280, 10, 10},
  {390, 280, 20, 10},
  {510, 280, 10, 10},
  {700, 280, 10, 10},
  {10, 290, 10, 10},
  {200, 290, 10, 10},
  {320, 290, 10, 10},
  {390, 290, 20, 10},
  {510, 290, 10, 10},
  {700, 290, 10, 10},
  {10, 300, 10, 10},
  {200, 300, 10, 10},
  {310, 300, 20, 10},
  {390, 300, 20, 10},
  {510, 300, 10, 10},
  {700, 300, 10, 10},
  {390, 120, 40, 10},
  {420, 130, 10, 60},
  {290, 190, 140, 10},
  {290, 130, 10, 60},
  {290, 120, 40, 10},
  {10, 50, 10, 10},
  {40, 50, 20, 10},
  {20, 10, 180, 10},
  {210, 10, 140, 10},
  {370, 10, 140, 10},
  {520, 10, 180, 10},
  {520, 300, 180, 10},
  {410, 300, 100, 10},
  {330, 300, 60, 10},
  {210, 300, 100, 10},
  {20, 300, 180, 10},
  {10, 260, 10, 10},
  {40, 90, 20, 10},
  {120, 90, 20, 10},
  {80, 70, 20, 20},
  {80, 40, 20, 10},
  {80, 110, 20, 20},
  {160, 100, 20, 10},
  {40, 150, 100, 10},
  {90, 180, 50, 10},
  {40, 180, 20, 10},
  {60, 180, 10, 10},
  {60, 190, 10, 10},
  {60, 200, 10, 10},
  {60, 210, 10, 10},
  {40, 220, 60, 10},
  {120, 220, 20, 10},
  {40, 250, 20, 10},
  {40, 260, 20, 10},
  {40, 270, 20, 10},
  {120, 280, 60, 20},
  {180, 280, 20, 20},
  {160, 220, 20, 40},
  {80, 250, 60, 10},
  {200, 90, 20, 10},
  {230, 40, 10, 20},
  {160, 40, 20, 60},
  {120, 40, 20, 30},
  {200, 120, 20, 40},
  {200, 180, 20, 70},
  {220, 120, 50, 10},
  {240, 150, 30, 10},
  {240, 160, 30, 40},
  {240, 220, 40, 10},
  {240, 250, 20, 20},
  {210, 280, 10, 20},
  {240, 270, 20, 10},
  {290, 250, 10, 30},
  {350, 220, 30, 10},
  {400, 220, 50, 10},
  {450, 190, 40, 10},
  {450, 120, 40, 50},
  {510, 120, 10, 70},
  {540, 120, 10, 50},
  {540, 190, 10, 40},
  {540, 40, 10, 60},
  {310, 90, 60, 10},
  {370, 90, 40, 10},
  {410, 40, 40, 10},
  {500, 20, 10, 20},
  {250, 40, 10, 30},
  {320, 40, 10, 10},
  {280, 50, 20, 20},
  {280, 40, 20, 10},
  {480, 90, 10, 10},
  {580, 90, 10, 40},
  {580, 60, 10, 10},
  {580, 20, 60, 20},
  {660, 50, 20, 10},
  {700, 50, 10, 10},
  {620, 120, 20, 10},
  {660, 90, 20, 10},
  {690, 150, 10, 20},
  {660, 260, 20, 10},
  {700, 260, 10, 10},
  {670, 220, 10, 10},
  {560, 150, 10, 20},
  {580, 190, 10, 40},
  {620, 220, 10, 10},
  {550, 280, 50, 20},
  {520, 280, 30, 20},
  {580, 250, 10, 10},
  {480, 250, 10, 30},
  {400, 250, 10, 10},
  {200, 250, 20, 10}
};
const int wallCount = sizeof(walls) / sizeof(Wall);

// --- 自訂設計地圖點點結構 (共有 253 個點點) ---
struct Pellet {
  int x, y;
  bool isPower;
};

const Pellet pellets[] PROGMEM = {
  {20, 20, true},
  {40, 20, false},
  {60, 20, false},
  {80, 20, false},
  {100, 20, false},
  {120, 20, false},
  {140, 20, false},
  {160, 20, false},
  {180, 20, false},
  {210, 20, false},
  {230, 20, false},
  {250, 20, false},
  {270, 20, false},
  {290, 20, false},
  {310, 20, false},
  {330, 20, false},
  {370, 20, false},
  {390, 20, false},
  {410, 20, false},
  {430, 20, false},
  {450, 20, false},
  {470, 20, false},
  {520, 20, false},
  {540, 20, false},
  {560, 20, false},
  {640, 20, false},
  {660, 20, false},
  {680, 20, true},
  {260, 30, false},
  {300, 30, false},
  {480, 30, false},
  {20, 40, false},
  {60, 40, false},
  {100, 40, false},
  {140, 40, false},
  {180, 40, false},
  {200, 40, false},
  {330, 40, false},
  {370, 40, false},
  {450, 40, false},
  {490, 40, false},
  {510, 40, false},
  {560, 40, false},
  {580, 40, false},
  {600, 40, false},
  {620, 40, false},
  {640, 40, false},
  {680, 40, false},
  {70, 50, false},
  {90, 50, false},
  {260, 50, false},
  {300, 50, false},
  {410, 50, false},
  {430, 50, false},
  {520, 50, false},
  {20, 60, false},
  {60, 60, false},
  {100, 60, false},
  {140, 60, false},
  {180, 60, false},
  {330, 60, false},
  {370, 60, false},
  {450, 60, false},
  {560, 60, false},
  {640, 60, false},
  {680, 60, false},
  {30, 70, false},
  {50, 70, false},
  {110, 70, false},
  {130, 70, false},
  {190, 70, false},
  {210, 70, false},
  {230, 70, false},
  {250, 70, false},
  {270, 70, false},
  {290, 70, false},
  {310, 70, false},
  {340, 70, false},
  {360, 70, false},
  {380, 70, false},
  {400, 70, false},
  {460, 70, false},
  {480, 70, false},
  {520, 70, false},
  {570, 70, false},
  {590, 70, false},
  {610, 70, false},
  {630, 70, false},
  {650, 70, false},
  {670, 70, false},
  {20, 80, false},
  {60, 80, false},
  {100, 80, false},
  {140, 80, false},
  {180, 80, false},
  {220, 80, false},
  {410, 80, false},
  {490, 80, false},
  {560, 80, false},
  {640, 80, false},
  {680, 80, false},
  {70, 90, false},
  {90, 90, false},
  {290, 90, false},
  {520, 90, false},
  {20, 100, false},
  {60, 100, false},
  {100, 100, false},
  {140, 100, false},
  {180, 100, false},
  {200, 100, false},
  {220, 100, true},
  {240, 100, false},
  {260, 100, false},
  {280, 100, false},
  {300, 100, false},
  {320, 100, false},
  {340, 100, false},
  {360, 100, false},
  {380, 100, false},
  {400, 100, false},
  {420, 100, false},
  {440, 100, false},
  {460, 100, false},
  {480, 100, true},
  {500, 100, false},
  {530, 100, false},
  {550, 100, false},
  {600, 100, false},
  {620, 100, false},
  {640, 100, false},
  {680, 100, false},
  {150, 110, false},
  {170, 110, false},
  {270, 110, false},
  {430, 110, false},
  {490, 110, false},
  {520, 110, false},
  {560, 110, false},
  {20, 120, false},
  {60, 120, false},
  {100, 120, false},
  {140, 120, false},
  {180, 120, false},
  {600, 120, false},
  {640, 120, false},
  {680, 120, false},
  {30, 130, false},
  {50, 130, false},
  {110, 130, false},
  {130, 130, false},
  {220, 130, false},
  {240, 130, false},
  {260, 130, false},
  {430, 130, false},
  {490, 130, false},
  {520, 130, false},
  {560, 130, false},
  {580, 130, false},
  {650, 130, false},
  {670, 130, false},
  {20, 140, false},
  {140, 140, false},
  {180, 140, false},
  {270, 140, false},
  {570, 140, false},
  {220, 150, false},
  {430, 150, false},
  {490, 150, false},
  {520, 150, false},
  {670, 150, false},
  {20, 160, false},
  {40, 160, false},
  {60, 160, false},
  {80, 160, false},
  {100, 160, false},
  {120, 160, false},
  {140, 160, false},
  {180, 160, false},
  {200, 160, false},
  {270, 160, false},
  {570, 160, false},
  {70, 170, false},
  {220, 170, false},
  {430, 170, false},
  {450, 170, false},
  {470, 170, false},
  {490, 170, false},
  {520, 170, false},
  {540, 170, false},
  {560, 170, false},
  {580, 170, false},
  {600, 170, false},
  {620, 170, false},
  {640, 170, false},
  {660, 170, false},
  {680, 170, false},
  {20, 180, false},
  {140, 180, false},
  {180, 180, false},
  {270, 180, false},
  {70, 190, false},
  {220, 190, false},
  {430, 190, false},
  {490, 190, false},
  {520, 190, false},
  {560, 190, false},
  {640, 190, false},
  {680, 190, false},
  {20, 200, false},
  {80, 200, false},
  {100, 200, false},
  {140, 200, false},
  {180, 200, false},
  {230, 200, false},
  {250, 200, false},
  {270, 200, false},
  {290, 200, false},
  {310, 200, false},
  {330, 200, false},
  {350, 200, false},
  {370, 200, false},
  {390, 200, false},
  {410, 200, false},
  {440, 200, false},
  {460, 200, false},
  {480, 200, false},
  {600, 200, false},
  {620, 200, false},
  {220, 210, false},
  {380, 210, false},
  {450, 210, false},
  {520, 210, false},
  {560, 210, false},
  {680, 210, false},
  {20, 220, false},
  {100, 220, false},
  {140, 220, false},
  {180, 220, false},
  {330, 220, false},
  {600, 220, false},
  {30, 230, false},
  {50, 230, false},
  {70, 230, false},
  {90, 230, false},
  {110, 230, false},
  {130, 230, false},
  {220, 230, false},
  {240, 230, false},
  {260, 230, false},
  {280, 230, false},
  {300, 230, false},
  {320, 230, false},
  {340, 230, false},
  {360, 230, false},
  {380, 230, false},
  {400, 230, false},
  {420, 230, false},
  {440, 230, false},
  {460, 230, false},
  {480, 230, false},
  {500, 230, false},
  {520, 230, false},
  {540, 230, false},
  {560, 230, false},
  {580, 230, false},
  {610, 230, false},
  {630, 230, false},
  {650, 230, false},
  {670, 230, false},
  {20, 240, false},
  {60, 240, false},
  {140, 240, false},
  {180, 240, false},
  {410, 240, false},
  {450, 240, false},
  {490, 240, false},
  {640, 240, false},
  {680, 240, false},
  {220, 250, false},
  {260, 250, false},
  {300, 250, false},
  {560, 250, false},
  {20, 260, false},
  {60, 260, false},
  {100, 260, false},
  {120, 260, false},
  {140, 260, false},
  {160, 260, false},
  {180, 260, false},
  {200, 260, false},
  {310, 260, false},
  {330, 260, false},
  {370, 260, false},
  {390, 260, false},
  {410, 260, false},
  {450, 260, false},
  {490, 260, false},
  {510, 260, false},
  {530, 260, false},
  {550, 260, false},
  {570, 260, false},
  {590, 260, false},
  {640, 260, false},
  {680, 260, false},
  {220, 270, false},
  {260, 270, false},
  {300, 270, false},
  {600, 270, false},
  {20, 280, true},
  {40, 280, false},
  {60, 280, false},
  {80, 280, false},
  {100, 280, false},
  {230, 280, false},
  {250, 280, false},
  {270, 280, false},
  {290, 280, false},
  {330, 280, false},
  {350, 280, false},
  {370, 280, false},
  {410, 280, false},
  {430, 280, false},
  {450, 280, false},
  {470, 280, false},
  {490, 280, false},
  {610, 280, false},
  {630, 280, false},
  {650, 280, false},
  {670, 280, true}
};
const int pelletCount = sizeof(pellets) / sizeof(Pellet);
bool pelletEaten[pelletCount > 0 ? pelletCount : 1]; // 追蹤各點點是否被吃掉的狀態

bool checkCollision(int x, int y) {
  for (int i = 0; i < wallCount; i++) {
    int wx = pgm_read_word(&walls[i].x), wy = pgm_read_word(&walls[i].y), ww = pgm_read_word(&walls[i].w), wh = pgm_read_word(&walls[i].h);
    if (x + PLAYER_SIZE > wx && x - PLAYER_SIZE < wx + ww && y + PLAYER_SIZE > wy && y - PLAYER_SIZE < wy + wh) return true;
  }
  return false;
}

void drawMaze(int offset) {
  tft.fillScreen(ILI9341_BLACK);
  for (int i = 0; i < wallCount; i++) {
    int x = pgm_read_word(&walls[i].x);
    int y = pgm_read_word(&walls[i].y);
    int w = pgm_read_word(&walls[i].w);
    int h = pgm_read_word(&walls[i].h);
    int lx = x - offset;
    
    // 如果牆壁有任何一部分落在這個螢幕的 0~240 範圍內
    if (lx + w > 0 && lx < 240) {
      int drawX = max(0, lx);
      // 計算實際要在這個螢幕上畫出的寬度
      int drawW = min(lx + w, 240) - drawX;
      tft.fillRect(drawX, y, drawW, h, ILI9341_BLUE);
    }
  }
  
  // 繪製點點 (寬 240 px 顯示限制)
  for (int i = 0; i < pelletCount; i++) {
    if (!pelletEaten[i]) {
      int px = pgm_read_word(&pellets[i].x) + 10;
      int py = pgm_read_word(&pellets[i].y) + 10;
      bool isPower = pgm_read_byte(&pellets[i].isPower);
      int lx = px - offset;
      if (lx > 0 && lx < 240) {
        if (isPower) {
          tft.fillCircle(lx, py, 4, ILI9341_YELLOW); // 吃豆人大點點 (能量丸)
        } else {
          tft.fillCircle(lx, py, 1, ILI9341_WHITE); // 吃豆人小點點
        }
      }
    }
  }
}

#if ROLE == 1
  const int introMelody[] PROGMEM = { NOTE_B4, NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_C5, NOTE_C6, NOTE_G5, NOTE_E5, NOTE_C6, NOTE_G5, NOTE_E5, NOTE_B4, NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_B5 };
  const int introDurations[] PROGMEM = { 150, 150, 150, 150, 75, 75, 150, 150, 150, 150, 150, 75, 75, 150, 150, 150, 150, 150, 75, 75, 150, 75, 75, 75, 75, 75, 75, 75, 75, 75, 300 };
  const int deathMelody[] PROGMEM = { NOTE_B5, 700, 600, 500 };
  const int deathDurations[] PROGMEM = { 150, 150, 150, 300 };
  unsigned long noteStartTime = 0; int currentNote = 0; bool isPlayingMusic = false; int melodyLength = 0; const int *currentMelody, *currentDurations;
  void playMelody(const int* m, const int* d, int l) { currentMelody=m; currentDurations=d; melodyLength=l; currentNote=0; isPlayingMusic=true; noteStartTime=millis(); tone(BUZZER_PIN, pgm_read_word(&currentMelody[0])); }
  void updateMusic() {
    if (!isPlayingMusic) return;
    int dur = pgm_read_word(&currentDurations[currentNote]);
    if (millis() - noteStartTime >= (unsigned long)dur) {
      currentNote++;
      if (currentNote >= melodyLength) { noTone(BUZZER_PIN); isPlayingMusic = false; }
      else { tone(BUZZER_PIN, pgm_read_word(&currentMelody[currentNote])); noteStartTime = millis(); }
    } else if (millis() - noteStartTime >= (unsigned long)dur * 0.8) noTone(BUZZER_PIN);
  }
#endif

#if ROLE == 4
void drawScoreUI() {
  tft.fillScreen(ILI9341_BLACK); tft.drawRect(5, 5, 230, 310, ILI9341_WHITE);
  tft.fillCircle(120, 60, 20, ILI9341_YELLOW); tft.setCursor(60, 100); tft.setTextColor(ILI9341_YELLOW); tft.setTextSize(2); tft.print("PLAYER 1");
  tft.fillCircle(120, 200, 20, ILI9341_GREEN); tft.setCursor(60, 240); tft.setTextColor(ILI9341_GREEN); tft.setTextSize(2); tft.print("PLAYER 2");
}
void updateScoreText() {
  if (gameOver) {
    return; // 遊戲結束時由事件觸發繪製，此處直接返回避免 I2C 刷新閃爍
  }
  
  // 正常遊戲時的分數與生命值顯示
  tft.fillRect(10, 120, 220, 40, ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setCursor(20, 120); tft.setTextColor(ILI9341_YELLOW); tft.print(score1);
  tft.setTextSize(2);
  tft.setCursor(140, 125); tft.print("L: "); tft.print(lives1);
  
  tft.fillRect(10, 260, 220, 40, ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setCursor(20, 260); tft.setTextColor(ILI9341_GREEN); tft.print(score2);
  tft.setTextSize(2);
  tft.setCursor(140, 265); tft.print("L: "); tft.print(lives2);
}
#endif

void receiveEvent(int howMany) {
  if (howMany >= 32) {
    p1X = Wire.read() | (Wire.read() << 8); p1Y = Wire.read() | (Wire.read() << 8);
    p2X = Wire.read() | (Wire.read() << 8); p2Y = Wire.read() | (Wire.read() << 8);
    score1 = Wire.read() | (Wire.read() << 8); score2 = Wire.read() | (Wire.read() << 8);
    
    // 讀取方向與生命值包裝
    int dirs = Wire.read() | (Wire.read() << 8);
    p1Dir = dirs & 0x0F;
    lives1 = (dirs >> 4) & 0x0F;
    p2Dir = (dirs >> 8) & 0x0F;
    lives2 = (dirs >> 12) & 0x0F;
    
    // 讀取重置計數器與無敵時間狀態、遊戲結束狀態與贏家
    int d7 = Wire.read() | (Wire.read() << 8);
    int remoteResetCounter = d7 & 0x0F;
    
    bool oldGameOver = gameOver;
    gameOver = (d7 >> 4) & 1;
    winner = (d7 >> 5) & 0x03;
    frightenedModeDeciseconds = d7 >> 8;
    frightenedModeActive = (frightenedModeDeciseconds > 0);
    
    // 檢測從遊戲結束重置回遊玩狀態，重新繪製迷宮/UI
    if (oldGameOver && !gameOver) {
      #if ROLE == 4
      drawScoreUI();
      #else
      drawMaze((ROLE==1?240:(ROLE==2?0:480)));
      #endif
    }
    
    if (remoteResetCounter != levelResetCounter) {
      levelResetCounter = remoteResetCounter;
      for (int i = 0; i < pelletCount; i++) {
        pelletEaten[i] = false;
      }
    }
    
    for(int i=0; i<NUM_GHOSTS; i++) { gX[i] = Wire.read() | (Wire.read() << 8); gY[i] = Wire.read() | (Wire.read() << 8); }
    dataReceived = true;
  }
}

void requestEvent() {
  int dir = 0;
  if (digitalRead(A0) == LOW) dir = 1; else if (digitalRead(A1) == LOW) dir = 2; else if (digitalRead(A2) == LOW) dir = 3; else if (digitalRead(A3) == LOW) dir = 4;
  Wire.write(dir);
}

void setup() {
  tft.begin();
  tft.setRotation(0);
  
  // 幽靈初始位置：經由網頁地圖設計器自訂座標
  gX[0] = 320;
  gY[0] = 160;
  gX[1] = 345;
  gY[1] = 160;
  gX[2] = 375;
  gY[2] = 160;
  gX[3] = 395;
  gY[3] = 160;
  
  lp1X = p1X; lp1Y = p1Y;
  lp2X = p2X; lp2Y = p2Y;
  for(int i=0; i<NUM_GHOSTS; i++) {
    lgX[i] = gX[i]; lgY[i] = gY[i];
  }

#if ROLE == 1
  Wire.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  playMelody(introMelody, introDurations, 31);
#elif ROLE == 2
  Wire.begin(ADDR_LEFT);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
#elif ROLE == 3
  Wire.begin(ADDR_RIGHT);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
#elif ROLE == 4
  Wire.begin(ADDR_SCORE);
  Wire.onReceive(receiveEvent);
#endif

#if ROLE == 4
  drawScoreUI();
#else
  drawMaze((ROLE==1?240:(ROLE==2?0:480)));
#endif

#if ROLE == 2 || ROLE == 3
  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);
#endif

  // 紀錄幽靈高程預設，用於籠中抖動與漂浮動畫
  for(int i=0; i<NUM_GHOSTS; i++) {
    ghostBaseY[i] = gY[i];
  }
  
}

void loop() {
  #if ROLE == 1
    handleI2CInputs(); updateLogic(); syncI2CSlaves(); render(240); updateMusic();
  #elif ROLE == 4
    if (dataReceived) { updateScoreText(); dataReceived = false; }
  #else
    if (dataReceived) { render((ROLE == 2) ? 0 : 480); dataReceived = false; }
  #endif
  delay(15); // 稍微增加延遲減少閃爍
}

#if ROLE == 1
void handleI2CInputs() {
  Wire.requestFrom(ADDR_LEFT, 1); 
  if (Wire.available()) {
    int dir = Wire.read();
    if (dir != 0) p1NextDir = dir; // 緩衝玩家1的方向輸入
  }
  
  Wire.requestFrom(ADDR_RIGHT, 1); 
  if (Wire.available()) {
    int dir = Wire.read();
    if (dir != 0) p2NextDir = dir; // 緩衝玩家2的方向輸入
  }
}
void updateLogic() {
  if (gameOver) {
    if (p1NextDir != 0 || p2NextDir != 0) {
      // 遊戲重置以重新開始
      lives1 = 3;
      lives2 = 3;
      score1 = 0;
      score2 = 0;
      gameOver = false;
      winner = 0;
      p1X = 280; p1Y = 160;
      p2X = 440; p2Y = 160;
      p1Dir = 0; p2Dir = 0;
      p1NextDir = 0; p2NextDir = 0;
      gameStarted = false;
      frightenedModeEndTime = 0;
      for (int i = 0; i < pelletCount; i++) {
        pelletEaten[i] = false;
      }
      int baseInsideX[] = {320, 345, 375, 395};
      for (int i = 0; i < NUM_GHOSTS; i++) {
        gX[i] = baseInsideX[i]; gY[i] = 160;
        ghostEatenTime[i] = 0;
      }
      levelResetCounter++; // 通知從屬螢幕也重置狀態
      drawMaze(240);
    }
    return;
  }
  // 當玩家按任何方向按鈕 (開始移動/開始遊戲)
  if (!gameStarted && (p1NextDir != 0 || p2NextDir != 0)) {
    gameStarted = true;
    gameStartTime = millis();
  }
  
  // 檢查預轉向是否合法，合法則更新當前方向
  auto checkTurn = [](int x, int y, int &nextDir, int &currentDir) {
    if (nextDir == 0 || nextDir == currentDir) return;
    int nx = x, ny = y;
    if (nextDir == 1) ny -= 5;
    else if (nextDir == 2) ny += 5;
    else if (nextDir == 3) nx -= 5;
    else if (nextDir == 4) nx += 5;
    
    if (!checkCollision(nx, ny)) {
      currentDir = nextDir; // 如果轉向後不會撞牆，就正式轉向
      nextDir = 0; // 清除緩衝
    }
  };

  checkTurn(p1X, p1Y, p1NextDir, p1Dir);
  checkTurn(p2X, p2Y, p2NextDir, p2Dir);

  auto move = [](int &x, int &y, int dir) {
    int nx = x, ny = y;
    if (dir == 1) ny -= 5;
    else if (dir == 2) ny += 5;
    else if (dir == 3) nx -= 5;
    else if (dir == 4) nx += 5;
    if (!checkCollision(nx, ny)) {
      x = nx;
      y = ny;
    }
    x = constrain(x, 15, TOTAL_W-15);
    y = constrain(y, 15, 305);
  };
  
  move(p1X, p1Y, p1Dir);
  move(p2X, p2Y, p2Dir);
  
  // 檢查是否吃到點點
  for (int i = 0; i < pelletCount; i++) {
    if (!pelletEaten[i]) {
      int px = pgm_read_word(&pellets[i].x) + 10;
      int py = pgm_read_word(&pellets[i].y) + 10;
      bool isPower = pgm_read_byte(&pellets[i].isPower);
      
if (abs(p1X - px) < 12 && abs(p1Y - py) < 12) {
        pelletEaten[i] = true;
        if (isPower) {
          score1 += 50;
          frightenedModeEndTime = millis() + 8000; // 觸發 8 秒無敵時間
        } else {
          score1 += 10;
        }
        #if ROLE == 1
        tone(BUZZER_PIN, (isPower ? NOTE_E5 : NOTE_C5), 45);
        #endif
      }
      else if (abs(p2X - px) < 12 && abs(p2Y - py) < 12) {
        pelletEaten[i] = true;
        if (isPower) {
          score2 += 50;
          frightenedModeEndTime = millis() + 8000; // 觸發 8 秒無敵時間
        } else {
          score2 += 10;
        }
        #if ROLE == 1
        tone(BUZZER_PIN, (isPower ? NOTE_G5 : NOTE_C5), 45);
        #endif
      }
    }
  }

  // 當所有點點被吃完時，將其全體重生，並提示進入下一關
  bool allEaten = true;
  for (int i = 0; i < pelletCount; i++) {
    if (!pelletEaten[i]) {
      allEaten = false;
      break;
    }
  }
  if (allEaten && pelletCount > 0) {
    gameOver = true;
    if (score1 > score2) winner = 1;
    else if (score2 > score1) winner = 2;
    else winner = 3;
    drawGameOverUI();
    gameStarted = false;
    #if ROLE == 1
    playMelody(introMelody, introDurations, 15); // 播放通關音樂
    #endif
  }

  static int gDirs[NUM_GHOSTS] = {1, 2, 3, 4};
  for(int i=0; i<NUM_GHOSTS; i++) {
    // 如果遊戲尚未正式啟動，或者幽靈沒到釋放時間，保持在中央牢籠內探頭跳躍等待！
    unsigned long releaseTime = gameStartTime + ghostReleaseDelays[i];
    if (ghostEatenTime[i] > 0) {
      releaseTime = max(releaseTime, ghostEatenTime[i] + 4000); // 被吃掉後關在籠子裡 4 秒
    }
    
    // 如果遊戲尚未正式啟動，或者幽靈沒到釋放時間，保持在中央牢籠內探頭跳躍等待！
    if (!gameStarted || (millis() < releaseTime)) {
      int baseInsideX[] = {320, 345, 375, 395};
      int baseInsideY = 160;
      gX[i] = baseInsideX[i];
      if (((millis() + i * 100) / 250) % 2 == 0) {
        gY[i] = baseInsideY + 4;
      } else {
        gY[i] = baseInsideY - 4;
      }
      continue;
    } else {
      // 剛好釋放！如果仍在籠內，引導其主動走出牢籠
      if (gX[i] >= 290 && gX[i] <= 430 && gY[i] >= 120 && gY[i] <= 190) {
        if (gX[i] < 360) {
          gX[i] += min(2, 360 - gX[i]);
          gDirs[i] = 4; // 向右
        } else if (gX[i] > 360) {
          gX[i] -= min(2, gX[i] - 360);
          gDirs[i] = 3; // 向左
        } else {
          gY[i] -= 2;
          gDirs[i] = 1; // 向上
        }
        continue;
      }
    }
    

    int gnx = gX[i], gny = gY[i];
    if (gDirs[i] == 1) gny -= 2;
    else if (gDirs[i] == 2) gny += 2;
    else if (gDirs[i] == 3) gnx -= 2;
    else if (gDirs[i] == 4) gnx += 2;
    
    if (checkCollision(gnx, gny)) {
      gDirs[i] = random(1, 5);
    } else {
      gX[i] = gnx;
      gY[i] = gny;
    }
    
    if (abs(p1X - gX[i]) < 12 && abs(p1Y - gY[i]) < 12) {
      if (millis() < frightenedModeEndTime) {
        // 玩家 1 吃到鬼
        ghostEatenTime[i] = millis();
        int baseInsideX[] = {320, 345, 375, 395};
        gX[i] = baseInsideX[i];
        gY[i] = 160;
        score1 += 200; // 吃到鬼加 200 分
        #if ROLE == 1
        tone(BUZZER_PIN, NOTE_E5, 100);
        #endif
      } else {
        // 玩家 1 碰到鬼扣一命
        lives1--;
        if (lives1 <= 0) {
          gameOver = true;
          if (score1 > score2) winner = 1;
          else if (score2 > score1) winner = 2;
          else winner = 3;
          drawGameOverUI();
        }
        p1X = 280; p1Y = 160; p2X = 440; p2Y = 160;
        gameStarted = false;
        frightenedModeEndTime = 0;
        int baseInsideX[] = {320, 345, 375, 395};
        for (int k = 0; k < NUM_GHOSTS; k++) {
          gX[k] = baseInsideX[k]; gY[k] = 160;
          ghostEatenTime[k] = 0;
        }
        playMelody(deathMelody, deathDurations, 4);
      }
    }
    if (abs(p2X - gX[i]) < 12 && abs(p2Y - gY[i]) < 12) {
      if (millis() < frightenedModeEndTime) {
        // 玩家 2 吃到鬼
        ghostEatenTime[i] = millis();
        int baseInsideX[] = {320, 345, 375, 395};
        gX[i] = baseInsideX[i];
        gY[i] = 160;
        score2 += 200; // 吃到鬼加 200 分
        #if ROLE == 1
        tone(BUZZER_PIN, NOTE_G5, 100);
        #endif
      } else {
        // 玩家 2 碰到鬼扣一命
        lives2--;
        if (lives2 <= 0) {
          gameOver = true;
          if (score1 > score2) winner = 1;
          else if (score2 > score1) winner = 2;
          else winner = 3;
          drawGameOverUI();
        }
        p1X = 280; p1Y = 160; p2X = 440; p2Y = 160;
        gameStarted = false;
        frightenedModeEndTime = 0;
        int baseInsideX[] = {320, 345, 375, 395};
        for (int k = 0; k < NUM_GHOSTS; k++) {
          gX[k] = baseInsideX[k]; gY[k] = 160;
          ghostEatenTime[k] = 0;
        }
        playMelody(deathMelody, deathDurations, 4);
      }
    }
  }
}
void syncI2CSlaves() {
  unsigned long remaining = (millis() < frightenedModeEndTime) ? (frightenedModeEndTime - millis()) : 0;
  int remainingDeciseconds = min(255, remaining / 100);
  auto sendData = [remainingDeciseconds](int addr) {
    Wire.beginTransmission(addr);
    // 包裝生命值與方向到 d[6]
    int d6 = (p1Dir | (lives1 << 4)) | ((p2Dir | (lives2 << 4)) << 8);
    // 包裝關卡重置、無敵剩餘時間、遊戲結束與贏家到 d[7]
    int d7 = (levelResetCounter & 0x0F) | (gameOver ? 0x10 : 0) | ((winner & 0x03) << 5) | (remainingDeciseconds << 8);
    int d[] = {p1X, p1Y, p2X, p2Y, score1, score2, d6, d7};
    for(int i=0; i<8; i++) { Wire.write(d[i] & 0xFF); Wire.write(d[i] >> 8); }
    for(int i=0; i<NUM_GHOSTS; i++) { Wire.write(gX[i] & 0xFF); Wire.write(gX[i] >> 8); Wire.write(gY[i] & 0xFF); Wire.write(gY[i] >> 8); }
    Wire.endTransmission();
  };
  sendData(ADDR_LEFT); sendData(ADDR_RIGHT); sendData(ADDR_SCORE);
}
#endif

void render(int offset) {
  if (gameOver) {
    return; // 遊戲結束時由事件觸發繪製，此處直接返回避免 I2C 刷新閃爍
  }
  // 從屬螢幕局部計算點點被吃掉的狀態
  #if ROLE != 1
  for (int i = 0; i < pelletCount; i++) {
    if (!pelletEaten[i]) {
      int px = pgm_read_word(&pellets[i].x) + 10;
      int py = pgm_read_word(&pellets[i].y) + 10;
      if ((abs(p1X - px) < 12 && abs(p1Y - py) < 12) || (abs(p2X - px) < 12 && abs(p2Y - py) < 12)) {
        pelletEaten[i] = true;
      }
    }
  }
  #endif
  auto redrawWallNear = [&](int x, int y) {
    for (int i = 0; i < wallCount; i++) {
      int wx = pgm_read_word(&walls[i].x);
      int wy = pgm_read_word(&walls[i].y);
      int ww = pgm_read_word(&walls[i].w);
      int wh = pgm_read_word(&walls[i].h);
      if (x + 16 >= wx && x - 16 <= wx + ww && y + 16 >= wy && y - 16 <= wy + wh) {
        int lx = wx - offset;
        if (lx + ww > 0 && lx < 240) {
          int drawX = max(0, lx);
          int drawW = min(lx + ww, 240) - drawX;
          tft.fillRect(drawX, wy, drawW, wh, ILI9341_BLUE);
        }
      }
    }
    
    // 重建被踩踏覆蓋區域內的未吃點點
    for (int i = 0; i < pelletCount; i++) {
      if (!pelletEaten[i]) {
        int px = pgm_read_word(&pellets[i].x) + 10;
        int py = pgm_read_word(&pellets[i].y) + 10;
        bool isPower = pgm_read_byte(&pellets[i].isPower);
        if (abs(px - x) < 15 && abs(py - y) < 15) {
          int lx = px - offset;
          if (lx > 0 && lx < 240) {
            if (isPower) {
              tft.fillCircle(lx, py, 4, ILI9341_YELLOW);
            } else {
              tft.fillCircle(lx, py, 1, ILI9341_WHITE);
            }
          }
        }
      }
    }
  };
  
  // 最佳化抹除邏輯：只抹除移動過的區域
  auto erase = [&](int x, int y, int lx_old, int ly_old) {
    if (x == lx_old && y == ly_old) return; // 沒動就不抹除
    int l_offset_x = lx_old - offset;
    if (l_offset_x > -20 && l_offset_x < 260) {
      // 使用方形抹除，確保能完全覆蓋幽靈的方形裙襬部分 (原本的圓形抹除會漏掉角落)
      tft.fillRect(l_offset_x - PLAYER_SIZE - 1, ly_old - PLAYER_SIZE - 1, (PLAYER_SIZE + 1) * 2, (PLAYER_SIZE + 1) * 2, ILI9341_BLACK);
      redrawWallNear(lx_old, ly_old);
    }
  };
  auto drawPac = [&](int x, int y, uint16_t color, int dir) {
    int lx = x - offset; if (lx < -15 || lx > 255) return;
    tft.fillCircle(lx, y, PLAYER_SIZE, color);
    if (dir == 1) tft.fillTriangle(lx, y, lx-5, y-10, lx+5, y-10, ILI9341_BLACK);
    else if (dir == 2) tft.fillTriangle(lx, y, lx-5, y+10, lx+5, y+10, ILI9341_BLACK);
    else if (dir == 3) tft.fillTriangle(lx, y, lx-10, y-5, lx-10, y+5, ILI9341_BLACK);
    else if (dir == 4) tft.fillTriangle(lx, y, lx+10, y-5, lx+10, y+5, ILI9341_BLACK);
    tft.fillCircle(lx + (dir==3?-2:2), y - 4, 1, ILI9341_BLACK);
  };
  auto drawGhost = [&](int x, int y, uint16_t color) {
    int lx = x - offset; if (lx < -15 || lx > 255) return;
    tft.fillCircle(lx, y, PLAYER_SIZE, color);
    tft.fillRect(lx - PLAYER_SIZE, y, PLAYER_SIZE * 2, PLAYER_SIZE, color);
    tft.fillCircle(lx - 3, y - 2, 2, ILI9341_WHITE); tft.fillCircle(lx + 3, y - 2, 2, ILI9341_WHITE);
  };

  erase(p1X, p1Y, lp1X, lp1Y); erase(p2X, p2Y, lp2X, lp2Y);
  for(int i=0; i<NUM_GHOSTS; i++) erase(gX[i], gY[i], lgX[i], lgY[i]);
  
  drawPac(p1X, p1Y, ILI9341_YELLOW, p1Dir);
  drawPac(p2X, p2Y, ILI9341_GREEN, p2Dir);
  
  uint16_t ghostColors[NUM_GHOSTS] = {ILI9341_RED, 0xF81F, ILI9341_CYAN, 0xFD20};
  #if ROLE == 1
  frightenedModeActive = (millis() < frightenedModeEndTime);
  frightenedModeDeciseconds = frightenedModeActive ? ((frightenedModeEndTime - millis()) / 100) : 0;
  #endif
  
  bool flashWhite = false;
  if (frightenedModeActive && frightenedModeDeciseconds < 20) { // 倒數 2 秒開始閃爍
    flashWhite = ((millis() / 150) % 2 == 0);
  }
  
  for(int i=0; i<NUM_GHOSTS; i++) {
    uint16_t color = frightenedModeActive ? (flashWhite ? ILI9341_WHITE : 0x001F) : ghostColors[i];
    drawGhost(gX[i], gY[i], color);
    lgX[i] = gX[i]; lgY[i] = gY[i];
  }
  lp1X = p1X; lp1Y = p1Y; lp2X = p2X; lp2Y = p2Y;
}