# [Bug Fix & 優化建議] 多螢幕 I2C 通訊與遊戲邏輯隱患修正

## 📝 概述
這次的更新大幅改善了先前的陣列語法問題，並加入了 90 度轉角緩衝機制，體驗提升很多！但在實際硬體運行上，目前的架構仍有幾個會導致「畫面撕裂/閃爍」、「通訊當機」以及「效能異常」的底層隱患。

建議進行以下修正，以確保多微控制器環境下的系統穩定性。

---

## 🛑 1. I2C 緩衝區滿載邊緣警告 (極度危險)

**問題描述：**
在 `syncI2CSlaves()` 函式中，目前的封包大小精準踩在邊界上：
- 玩家資料與分數：8 個 `int` = 16 Bytes
- 幽靈資料：8 個 `int` = 16 Bytes
- **總計：32 Bytes**

標準 Arduino `<Wire.h>` 的底層硬體緩衝區極限剛好就是 32 Bytes。目前的寫法雖然勉強能過關，但這是一顆定時炸彈。未來只要多傳遞任何一個變數（例如遊戲狀態旗標），封包就會直接被硬體截斷，導致 Slave 座標錯亂或當機。

**建議動作：**
目前若不想大改，請務必在該段程式碼旁加上醒目的警告註解。若未來有擴充計畫，必須將 `sendData` 拆分成兩次傳輸（例如：封包A傳玩家、封包B傳幽靈）。

---

## 🐛 2. Slave 端中斷資料競爭 (Data Race) 與畫面撕裂

**問題描述：**
在 Slave (ROLE 2, 3, 4) 的 `loop()` 中，一旦 `dataReceived` 為 `true` 就直接呼叫 `render()` 或 `updateScoreText()`。
由於 I2C 的 `receiveEvent` 是中斷驅動 (Interrupt-driven)，若主迴圈畫畫面畫到一半時收到 Master 傳來的新資料，中斷會強制觸發並更新底層變數。這會導致在同一幀內讀取到「一半舊、一半新」的座標，螢幕上會出現物件閃現或畫面撕裂 (Tearing)。

**修正方式：**
在提取資料進行渲染前，必須先關閉中斷鎖定變數，重置旗標後再重新開啟中斷：

```cpp
void loop() {
#if ROLE == 1
  handleI2CInputs(); updateLogic(); syncI2CSlaves(); render(240); updateMusic();
#elif ROLE == 4
  if (dataReceived) { 
    noInterrupts(); // 關閉中斷
    dataReceived = false; 
    interrupts();   // 重啟中斷
    updateScoreText(); 
  }
#else
  if (dataReceived) { 
    noInterrupts(); // 關閉中斷
    dataReceived = false; 
    interrupts();   // 重啟中斷
    render((ROLE == 2) ? 0 : 480); 
  }
#endif
  delay(15);
}
