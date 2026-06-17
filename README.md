# [Bug Fix & Root Cause Analysis] 多螢幕小精靈「穿牆 (Wall-Clipping)」漏洞修復

## 📝 總結 (Overview)
目前在多螢幕 I2C 叢集測試中，發現玩家與幽靈會出現無視物理碰撞、直接穿透牆壁的嚴重 Bug。經過逐步 Debug 與記憶體狀態追蹤，確認此現象並非單一錯誤，而是由三個不同維度的邏輯缺陷交互作用（Race Condition / State Machine Flaw / Hitbox Overflow）所導致。

本 PR 針對以下三個核心層面進行修復：
1. **幽靈狀態機越界**：修復全域範圍的異常觸發傳送。
2. **座標對齊運算順序錯誤**：修復轉角預判機制導致的座標強制覆寫。
3. **碰撞體積與網格衝突**：修正實體半徑與地圖巷道寬度的數學矛盾。

---

## 🔍 核心問題 1：幽靈的全域瞬移 (Global Teleportation Bug)

### 📌 發生原因
在 `updateLogic()` 處理幽靈釋放機制的邏輯中，觸發條件寫得過於寬鬆：`if (gY[i] >= 130)`。
這個判斷式原本的意圖是「檢查幽靈是否還在牢籠區」，但因為缺乏 X 軸的約束，導致整個地圖下半部（Y ≧ 130 的所有區域）變成了巨大的觸發區。一旦遊戲開始，只要幽靈走到地圖下方，就會不斷觸發條件，導致其座標被強制覆寫回中央 `(360, 110)`。在 Slave 端（左右螢幕）的視覺表現上，就是幽靈突然穿過半張地圖。

### 🛠 修復細節
嚴格限縮 `AABB` (Axis-Aligned Bounding Box) 觸發範圍，確保只有真正處於牢籠區內的幽靈才會執行釋放邏輯。

```cpp
// ❌ [原始邏輯] 缺乏 X 軸邊界，導致全域觸發
/*
if (gY[i] >= 130) {
  gX[i] = 360;
  gY[i] = 110;
  ...
}
*/

// ✅ [修復邏輯] 嚴格定義牢籠的 Bounding Box (X: 290~430, Y: 130~190)
if (gX[i] >= 290 && gX[i] <= 430 && gY[i] >= 130 && gY[i] <= 190) {
  gX[i] = 360;
  gY[i] = 110;
  gDirs[i] = random(1, 5); // 釋放時給予隨機初始方向
}


核心問題 2：玩家轉角對齊導致的強制擠壓 (Coordinate Snap Corruption)
📌 發生原因
為了讓玩家能順利進入 90 度的巷道，原本的 checkTurn 實作了將座標「對齊到 10 的倍數網格」的功能 (snap-to-grid)。
然而，原本的演算法在執行順序上有致命的瑕疵：它是「先檢查下一步 (nx, ny) 是否撞牆」，確認沒撞牆後，才把目前的 x 或 y 加上 5 個像素進行對齊。
這意味著，當玩家貼著牆壁並輸入轉向時，這個「加 5 像素的對齊偏移量」完全繞過了 checkCollision 的檢查。這會直接把玩家的座標覆寫到牆壁內，一旦座標落入牆內，後續的碰撞運算就會完全失效，讓玩家得以在牆體內自由移動。
🛠 修復細節
導入「預先計算 (Look-ahead)」機制。先算出對齊後的新座標 (snapX, snapY)，接著對這個虛擬的座標做「原地」與「下一步」的雙重碰撞檢查，確認絕對安全後才提交狀態。

// ✅ [修復邏輯] 重構 checkTurn 演算法
auto checkTurn = [](int &x, int &y, int &nextDir, int &currentDir) {
  if (nextDir == 0 || nextDir == currentDir) return;

  int snapX = x, snapY = y;
  
  // 加上括號確保 C++ 邏輯運算子優先權無誤
  bool isPerpendicular = ((currentDir == 1 || currentDir == 2) && (nextDir == 3 || nextDir == 4)) ||
                         ((currentDir == 3 || currentDir == 4) && (nextDir == 1 || nextDir == 2));

  // 1. 產生虛擬對齊座標 (Look-ahead)
  if (isPerpendicular) {
     if (nextDir == 1 || nextDir == 2) snapX = ((x + 5) / 10) * 10;
     else snapY = ((y + 5) / 10) * 10;
  }

  // 2. 基於虛擬座標，計算跨出下一步的座標
  int nx = snapX, ny = snapY;
  if (nextDir == 1) ny -= 4;
  else if (nextDir == 2) ny += 4;
  else if (nextDir == 3) nx -= 4;
  else if (nextDir == 4) nx += 4;

  // 3. 雙重 AABB 碰撞驗證：確保對齊後不會卡進牆壁，且下一步合法
  if (!checkCollision(nx, ny) && !checkCollision(snapX, snapY)) {
    x = snapX;             // 提交座標
    y = snapY;             // 提交座標
    currentDir = nextDir;  // 更新實際方向
    nextDir = 0;           // 清空輸入緩衝
  }
};



核心問題 3：物理碰撞體積與地圖網格的數學矛盾 (Hitbox Size Mismatch)
📌 發生原因
檢查 walls 陣列的資料結構，許多牆壁與牆壁之間的縫隙寬度僅有 10px（例如 y=10 與 y=30 的區間，扣掉高度本身，實體通道寬度為 10）。
但是，遊戲宣告的 #define PLAYER_SIZE 8 指的是半徑，也就是角色的碰撞直徑高達 16px。
從數學與物理學角度來看，16px 的剛體無法塞入 10px 的通道。當玩家在死胡同被卡住並連續輸入方向鍵時，會不斷觸發上述的「對齊 Bug」將自己強行擠破牆壁。
🛠 修復細節
重新定義實體的半徑參數，確保角色直徑小於地圖中最窄的通道寬度（10px），讓碰撞演算法得以正常運作。


// ❌ [原始設定] 直徑為 16px
// #define PLAYER_SIZE 8

// ✅ [修復設定] 將半徑改為 4 (直徑 8px)，確保能合法通過 10px 巷道
#define PLAYER_SIZE 4
