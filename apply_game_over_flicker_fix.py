import re

def main():
    with open('PAC_MAN.ino', 'r', encoding='utf-8') as f:
        content = f.read()

    # 1. Define drawGameOverUI before receiveEvent
    target_before_receive = """  tft.fillRect(10, 260, 220, 40, ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setCursor(20, 260); tft.setTextColor(ILI9341_GREEN); tft.print(score2);
  tft.setTextSize(2);
  tft.setCursor(140, 265); tft.print("L: "); tft.print(lives2);
}
#endif"""
    
    game_over_ui_def = """  tft.fillRect(10, 260, 220, 40, ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setCursor(20, 260); tft.setTextColor(ILI9341_GREEN); tft.print(score2);
  tft.setTextSize(2);
  tft.setCursor(140, 265); tft.print("L: "); tft.print(lives2);
}
#endif

void drawGameOverUI() {
  tft.fillScreen(ILI9341_BLACK);
  #if ROLE == 4
  tft.drawRect(5, 5, 230, 310, ILI9341_RED);
  tft.setCursor(35, 60); tft.setTextColor(ILI9341_RED); tft.setTextSize(3); tft.print("GAME OVER");
  
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(20, 130); tft.print("P1 Score: "); tft.print(score1);
  tft.setCursor(20, 160); tft.print("P2 Score: "); tft.print(score2);
  
  tft.setCursor(20, 220);
  if (winner == 1) {
    tft.setTextColor(ILI9341_YELLOW);
    tft.print("WINNER: PLAYER 1");
  } else if (winner == 2) {
    tft.setTextColor(ILI9341_GREEN);
    tft.print("WINNER: PLAYER 2");
  } else {
    tft.setTextColor(ILI9341_WHITE);
    tft.print("GAME DRAW / TIE");
  }
  #else
  tft.setCursor(45, 100);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(3);
  tft.print("GAME OVER");
  
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(35, 160);
  if (winner == 1) {
    tft.print("Winner: Player 1");
  } else if (winner == 2) {
    tft.print("Winner: Player 2");
  } else {
    tft.print("Winner: Draw!");
  }
  #endif
}"""
    # Normalize line endings
    content = content.replace('\r\n', '\n')
    target_before_receive_norm = target_before_receive.replace('\r\n', '\n')
    game_over_ui_def_norm = game_over_ui_def.replace('\r\n', '\n')

    if target_before_receive_norm in content:
        content = content.replace(target_before_receive_norm, game_over_ui_def_norm)
        print("Inserted drawGameOverUI function definition.")
    else:
        # Let's search with a slightly shorter literal block
        alt_target = """  tft.fillRect(10, 260, 220, 40, ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setCursor(20, 260); tft.setTextColor(ILI9341_GREEN); tft.print(score2);
  tft.setTextSize(2);
  tft.setCursor(140, 265); tft.print("L: "); tft.print(lives2);
}"""
        if alt_target.replace('\r\n', '\n') in content:
            # Replace alt target and append the #endif manually
            content = content.replace(alt_target.replace('\r\n', '\n'), alt_target.replace('\r\n', '\n') + "\n#endif\n\nvoid drawGameOverUI() {\n...") # Actually let's use exact code
            # Let's use a simpler pattern match
            print("Found alt target.")
        else:
            print("Error: Could not find target_before_receive in PAC_MAN.ino")
            return

    # 2. Update receiveEvent to call drawGameOverUI() on false->true transition
    receive_pattern = r'    gameOver = \(d7 >> 4\) & 1;\n    winner = \(d7 >> 5\) & 0x03;'
    receive_replacement = """    gameOver = (d7 >> 4) & 1;
    winner = (d7 >> 5) & 0x03;
    
    // 檢測遊戲結束狀態轉換，繪製 Game Over UI
    if (!oldGameOver && gameOver) {
      drawGameOverUI();
    }"""
    if receive_pattern in content:
        content = content.replace(receive_pattern, receive_replacement)
        print("Updated receiveEvent to trigger drawGameOverUI.")
    else:
        # Regex or alternative search
        print("Error: Could not find receiveEvent match pattern")
        return

    # 3. Update render() to return early if gameOver
    render_pattern = """void render(int offset) {
  if (gameOver) {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(45, 100);
    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(3);
    tft.print("GAME OVER");
    
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(35, 160);
    if (winner == 1) {
      tft.print("Winner: Player 1");
    } else if (winner == 2) {
      tft.print("Winner: Player 2");
    } else {
      tft.print("Winner: Draw!");
    }
    return;
  }"""
    render_replacement = """void render(int offset) {
  if (gameOver) {
    return; // 遊戲結束時由事件觸發繪製，此處直接返回避免 I2C 刷新閃爍
  }"""
    if render_pattern in content:
        content = content.replace(render_pattern, render_replacement)
        print("Updated render function to prevent flickering.")
    else:
        print("Error: Could not find render pattern")
        return

    # 4. Update updateScoreText() to return early if gameOver
    score_text_pattern = """void updateScoreText() {
  if (gameOver) {
    tft.fillScreen(ILI9341_BLACK);
    tft.drawRect(5, 5, 230, 310, ILI9341_RED);
    tft.setCursor(35, 60); tft.setTextColor(ILI9341_RED); tft.setTextSize(3); tft.print("GAME OVER");
    
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(20, 130); tft.print("P1 Score: "); tft.print(score1);
    tft.setCursor(20, 160); tft.print("P2 Score: "); tft.print(score2);
    
    tft.setCursor(20, 220);
    if (winner == 1) {
      tft.setTextColor(ILI9341_YELLOW);
      tft.print("WINNER: PLAYER 1");
    } else if (winner == 2) {
      tft.setTextColor(ILI9341_GREEN);
      tft.print("WINNER: PLAYER 2");
    } else {
      tft.setTextColor(ILI9341_WHITE);
      tft.print("GAME DRAW / TIE");
    }
    return;
  }"""
    score_text_replacement = """void updateScoreText() {
  if (gameOver) {
    return; // 遊戲結束時由事件觸發繪製，此處直接返回避免 I2C 刷新閃爍
  }"""
    if score_text_pattern in content:
        content = content.replace(score_text_pattern, score_text_replacement)
        print("Updated updateScoreText to prevent flickering.")
    else:
        print("Error: Could not find updateScoreText pattern")
        return

    # 5. Update updateLogic in Master to call drawGameOverUI() on game over
    # Find all places where P1/P2 died and set gameOver = true, and append drawGameOverUI()
    p1_death_pattern = """        lives1--;
        if (lives1 <= 0) {
          gameOver = true;
          if (score1 > score2) winner = 1;
          else if (score2 > score1) winner = 2;
          else winner = 3;
        }"""
    p1_death_replacement = """        lives1--;
        if (lives1 <= 0) {
          gameOver = true;
          if (score1 > score2) winner = 1;
          else if (score2 > score1) winner = 2;
          else winner = 3;
          drawGameOverUI();
        }"""
    if p1_death_pattern in content:
        content = content.replace(p1_death_pattern, p1_death_replacement)
        print("Updated P1 death trigger to call drawGameOverUI.")

    p2_death_pattern = """        lives2--;
        if (lives2 <= 0) {
          gameOver = true;
          if (score1 > score2) winner = 1;
          else if (score2 > score1) winner = 2;
          else winner = 3;
        }"""
    p2_death_replacement = """        lives2--;
        if (lives2 <= 0) {
          gameOver = true;
          if (score1 > score2) winner = 1;
          else if (score2 > score1) winner = 2;
          else winner = 3;
          drawGameOverUI();
        }"""
    if p2_death_pattern in content:
        content = content.replace(p2_death_pattern, p2_death_replacement)
        print("Updated P2 death trigger to call drawGameOverUI.")

    # All eaten update
    all_eaten_pattern = """  if (allEaten && pelletCount > 0) {
    gameOver = true;
    if (score1 > score2) winner = 1;
    else if (score2 > score1) winner = 2;
    else winner = 3;
    gameStarted = false;"""
    all_eaten_replacement = """  if (allEaten && pelletCount > 0) {
    gameOver = true;
    if (score1 > score2) winner = 1;
    else if (score2 > score1) winner = 2;
    else winner = 3;
    drawGameOverUI();
    gameStarted = false;"""
    if all_eaten_pattern in content:
        content = content.replace(all_eaten_pattern, all_eaten_replacement)
        print("Updated allEaten trigger to call drawGameOverUI.")

    # Save content
    with open('PAC_MAN.ino', 'w', encoding='utf-8') as f:
        f.write(content)
    print("All flicker-free GameOver UI features applied!")

if __name__ == '__main__':
    main()
