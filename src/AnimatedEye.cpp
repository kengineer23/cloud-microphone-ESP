#include "AnimatedEye.h"

/**
 * @brief Constructs an AnimatedEye object.
 */
AnimatedEye::AnimatedEye() {}

/**
 * @brief Initializes the TFT display and sets the rotation.
 * 
 * This method initializes the TFT display, sets its rotation to a specific value, 
 * and clears the screen with a black background.
 */
void AnimatedEye::init() {
  tft.init();
  tft.setRotation(1);  ///< Set the rotation (0-3)
  tft.fillScreen(TFT_BLACK);  ///< Clear the screen with black
}

/**
 * @brief Simulates a blinking animation of the eyes.
 * 
 * This method clears the screen, draws a blinking line over the eyes, and then
 * clears the screen again to simulate a blink. The blink effect includes a brief 
 * delay between the clear and draw actions.
 */
void AnimatedEye::blink() {
  // Clear the screen
  tft.fillScreen(TFT_BLACK);

  vTaskDelay(50 / portTICK_PERIOD_MS); ///< Delay for the blink effect

  // Blinking line
  tft.drawLine(eyeX1, blinkLineY, eyeX1 + eyeWidth, blinkLineY, TFT_WHITE);
  tft.drawLine(eyeX2, blinkLineY, eyeX2 + eyeWidth, blinkLineY, TFT_WHITE);
  vTaskDelay(100 / portTICK_PERIOD_MS); ///< Delay after drawing the blink line
  
  // Clear screen for a moment
  tft.fillScreen(TFT_BLACK);
}

/**
 * @brief Draws the eyes on the TFT display.
 * 
 * This method draws two capsule-shaped eyes on the display with a specified 
 * boundary thickness. Each eye is drawn with a rounded rectangle to represent 
 * the eye shape.
 */
/*
void AnimatedEye::moveEyes(){
  eyeX1 += random(-2,2);
  eyeX2 += random(-2,2);
}
*/
void AnimatedEye::drawEyes() {
  int eyeRadius = eyeWidth / 2; ///< Radius for the rounded corners

  //moveEyes();

  // Draw first eye (capsule shape with thick boundary)
  for (int i = 0; i < thickness; i++) {
    tft.drawRoundRect(eyeX1 - i, eyeY - i, eyeWidth + 2 * i, eyeHeight + 2 * i, eyeRadius + i, TFT_WHITE);
  }

  // Draw second eye (capsule shape with thick boundary)
  for (int i = 0; i < thickness; i++) {
    tft.drawRoundRect(eyeX2 - i, eyeY - i, eyeWidth + 2 * i, eyeHeight + 2 * i, eyeRadius + i, TFT_WHITE);
  }
}
