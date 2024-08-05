#ifndef ANIMATED_EYE_H
#define ANIMATED_EYE_H

#include "TFT_eSPI.h"

/**
 * @class AnimatedEye
 * @brief A class to handle the display and animation of eyes on a TFT display.
 * 
 * This class provides methods to initialize the TFT display, draw eyes, and simulate 
 * blinking animation. The eyes are drawn as capsule shapes with adjustable boundaries 
 * and are displayed on a TFT screen using the TFT_eSPI library.
 */
class AnimatedEye {
    public:
        /**
         * @brief Constructs an AnimatedEye object.
         */
        AnimatedEye();

        /**
         * @brief Initializes the TFT display and sets up the eye display.
         * 
         * This method initializes the TFT display, sets its rotation, and clears the 
         * screen to prepare for drawing the eyes.
         */
        void init();

        /**
         * @brief Draws the eyes on the TFT display.
         * 
         * This method draws two capsule-shaped eyes on the display with a specified 
         * boundary thickness. The eyes are represented as rounded rectangles.
         */
        void drawEyes();

        /**
         * @brief Simulates the blinking animation of the eyes.
         * 
         * This method clears the screen, draws a blinking line over the eyes, and then 
         * clears the screen again to simulate a blink effect. Includes delays for visual 
         * effect.
         */
        void blink();

        /**
         * @brief Decides the movement of eyes.
         * 
         * For now a random function is being used to move the eyes right or left
         * after each blink. Further, we need to move based on the direction of movement
         * of the device.
         */
        void moveEyes();
       
    private:
        TFT_eSPI tft = TFT_eSPI();   ///< TFT display object used for drawing

        int eyeX1 = 50;             ///< X-coordinate for the first eye
        int eyeX2 = 230;            ///< X-coordinate for the second eye
        int eyeY = 12;              ///< Y-coordinate for both eyes (top of the screen)
        int eyeWidth = 40;          ///< Width of the eyes
        int eyeHeight = 200;        ///< Height of the eyes (90% of 240)
        int thickness = 5;         ///< Thickness of the boundary around the eyes
        int blinkLineY = eyeY + eyeHeight / 2; ///< Y-coordinate for the blinking line (middle of the eye height)
        
};

#endif // !ANIMATED_EYE_H
