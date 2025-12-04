/*
Functions to control arrays in RAM that represent the content of the LCD screen.

You can create as many arrays as screen representations you want to have
stored in RAM (12 kbytes each), or maintain only one representation of
the screen in RAM and modify it each time you want to change its content.
*/
#ifndef LCD_FBUFFER_H
#define LCD_FBUFFER_H

// Pixels on the screen. LCD of 400x200 in our case.
#define LCD_WIDTH 400
#define LCD_HEIGHT 240
#define FB_SIZE (LCD_HEIGHT * LCD_WIDTH) / 8

// Color of the pixels.
enum color
{
    WHITE,
    BLACK
};

// Fill types for geometric figures.
enum fill_type
{
    HOLLOW,
    FILLED
};

// Array in RAM that contains the pixel values of the LCD screen.
extern uint8_t DISPLAY_BUFFER1[FB_SIZE];

// Buffer functions.
void lcd_clear(uint8_t *buffer, enum color colorpixel);
void lcd_set_pixel(uint8_t *buffer, uint16_t posicion_eje_x, uint16_t linea, enum color colorpixel);
void lcd_draw_rectangle(uint8_t *buffer, uint16_t x, uint16_t y, uint16_t ancho, uint16_t alto, enum color colorpixel, enum fill_type relleno);
void lcd_render_image_buffer(uint8_t *buffer, uint16_t posicion_eje_x, uint16_t linea, uint16_t ancho_img, uint16_t alto_img, const uint8_t *img);
void lcd_draw_text(uint8_t *buffer, uint16_t x, uint16_t y, uint16_t width, uint16_t height, char *text);
void lcd_draw_horizontal_line(uint8_t *buffer, uint16_t posicion_eje_x, uint16_t linea, uint16_t longitud, enum color colorpixel);
void lcd_draw_vertical_line(uint8_t *buffer, uint16_t posicion_eje_x, uint16_t linea, uint16_t longitud, enum color colorpixel);
#endif