#include "ws2812.h"
#include "SysTick.h"

// Define the character patterns for G, H, Y, X, D, B and heart
// Each character is represented as a 5x5 matrix
const u8 custom_char_patterns[][5] = {
    // G
    {0x70, 0x88, 0x80, 0x98, 0x70},
    // H
    {0xF8, 0x20, 0x20, 0x20, 0xF8},
    // Y
    {0x80, 0x40, 0x20, 0x40, 0x80},
    // X
    {0x88, 0x50, 0x20, 0x50, 0x88},
    // Y (repeat)
    {0x80, 0x40, 0x20, 0x40, 0x80},
    // D
    {0xF8, 0x88, 0x88, 0x88, 0x70},
    // B
    {0xF8, 0xA8, 0xA8, 0xA8, 0x50},
    // H (repeat)
    {0xF8, 0x20, 0x20, 0x20, 0xF8},
    // Y (repeat)
    {0x80, 0x40, 0x20, 0x40, 0x80},
    // Heart
    {0x50, 0xA8, 0xA8, 0x50, 0x20}};

// Array of different colors to use for each character
const u32 display_colors[] = {
    RGB_COLOR_RED,    // Red
    RGB_COLOR_GREEN,  // Green
    RGB_COLOR_BLUE,   // Blue
    RGB_COLOR_YELLOW, // Yellow
    0xFF00FF,         // Purple
    0x00FFFF,         // Cyan
    0xFFA500,         // Orange
    0x800080,         // Pink
    0x008000,         // Dark Green
    0xFF0080          // Pink-Red for heart
};

// Draw a custom character based on pattern
void RGB_ShowCustomChar(const u8 *pattern, u32 color)
{
    u8 i, j;
    u8 x = 0, y = 0;
    u8 temp = 0;

    RGB_LED_Clear(); // Clear previous display

    for (j = 0; j < 5; j++)
    {
        temp = pattern[j];
        for (i = 0; i < 8; i++) // Process all 8 bits of each byte
        {
            if (i < 5) // Only use the first 5 bits for 5x5 display
            {
                if (temp & 0x80)
                    RGB_DrawDotColor(x, y, 1, color); // Set LED on
                else
                    RGB_DrawDotColor(x, y, 0, 0); // Set LED off
                x++;
                if (x == RGB_LED_XWIDTH)
                {
                    x = 0;
                    y++;
                    if (y == RGB_LED_YHIGH)
                        return;
                }
            }
            temp <<= 1; // Shift to process next bit
        }
    }
}

// Heart shape function removed to avoid duplicate definition
// (RGB_ShowHeart is now defined in ws2812.c)

// Main function to cycle through all characters with different colors
void RGB_DisplaySequence(u16 delay_duration_ms)
{
    u8 i;

    for (i = 0; i < 10; i++) // 9 characters + 1 heart
    {
        RGB_ShowCustomChar(custom_char_patterns[i], display_colors[i]);
        delay_ms(delay_duration_ms); // Delay between characters
    }
}

// Initialize and start continuous display of the sequence
void RGB_StartDisplay(void)
{
    RGB_LED_Init(); // Initialize the RGB LED module

    while (1)
    {
        RGB_DisplaySequence(1000); // Display each character for 1 second
    }
}