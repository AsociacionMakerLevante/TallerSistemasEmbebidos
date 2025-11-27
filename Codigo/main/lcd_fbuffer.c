/*
Functions for writting and drawing to screen

Enlaces relacionados:
- https://www.youtube.com/watch?v=5cp2iPGWmUY
*/
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "lcd_fbuffer.h"
#include "imagenes.h"
#include "fuentes.h"

// Orientation of the LCD.
#define ORIENTACION_LCD_INVERSA

// We define arrays that represent the LCD screens in RAM.
uint8_t DISPLAY_BUFFER1[FB_SIZE];

/*
Function to draw a pixel in the buffer.
Parameters.
buffer: buffer in RAM that we are going to modify.
x: position of the pixel on the x-axis of the screen. Range 0 to (FB_ANCHO - 1)
y: position of the pixel on the y-axis of the screen. Range 0 to (FB_ALTO  - 1)
pixelColor = 1 pixel on the lcd, color = 0 pixel off.
*/
void lcd_set_pixel(uint8_t *buffer, uint16_t x, uint16_t y, enum color pixelColor)
{
    if (y >= LCD_HEIGHT)
    {
        y = LCD_HEIGHT - 1;
    }
    if (x >= LCD_WIDTH)
    {
        x = LCD_WIDTH - 1;
    }

// Calculate byte where the bit (pixel) to be modified is located in the array that represents the screen.
#ifdef ORIENTACION_LCD_INVERSA
    int16_t posicion = (((LCD_WIDTH * (LCD_HEIGHT - 1 - y)) + (LCD_WIDTH - 1 - x)) >> 3);
    // Subtract 1 to avoid going out of the array bounds, the array starts at 0
    // and divide by 8 to locate the bit within the corresponding byte.
    // if(posicion < 0){posicion = 0;} //this case would not occur, the sign bit does not shift.

    // Draw the pixel on the LCD. A 0 on the LCD -> black pixel.
    if (posicion < FB_SIZE)
    {
        if (pixelColor)
        {
            buffer[posicion] &= ~(0x80 >> (x & 0x07)); // Si es mayor de 8 saltamos de byte en el array
        }
        else
        {
            buffer[posicion] |= (0x80 >> (x & 0x07));
        }
    }

#else
    uint16_t posicion = (LCD_WIDTH * linea + posicion_eje_x) >> 3; // Divide by 8
    // Draw the pixel on the LCD. A 0 on the LCD -> black pixel.
    if (posicion < FB_SIZE)
    {
        if (colorpixel)
        {
            buffer[posicion] &= ~(0x01 << (posicion_eje_x & 0x07)); // Si es mayor de 8 saltamos de byte en el array
        }
        else
        {
            buffer[posicion] |= (0x01 << (posicion_eje_x & 0x07));
        }
    }
#endif
}

/*
Function to draw an image stored in RAM or FLASH in the screen buffer.
Parameters.
buffer: buffer in RAM that we are going to modify.
posicion_eje_x: x position from where to start drawing the image.
línea: line from where to start drawing the image.
ancho_img: dimensions in pixels of the width of the image.
alto_img: dimensions in pixels of the height of the image.
*img = pointer to the array that contains the image.
*/
void lcd_render_image_buffer(uint8_t *buffer, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *img)
{
    uint8_t mascara = 0;
    // Número de bytes que contiene la imagen.
    uint16_t bytes_imagen = (width * height) >> 3;
    uint16_t x_inicial = x;
    uint16_t linea_inicial = y;

    // Dibujamos los pixeles 1 a 1 de la imagen en el buffer que contiene la pantalla.
    for (uint16_t byte_del_pixel = 0; byte_del_pixel < bytes_imagen; byte_del_pixel++)
    {
        // Los pixeles están contenidos en bytes img[i]
        for (uint16_t bit = 0; bit < 8; bit++)
        {
            // Seleccionamos y dibujamos los bits de los bytes de la imagen de 1 en 1 mediante la máscara.
            mascara = 0x80 >> bit;
            // Si el pixel en la imagen está a 1 escribe un 1, si está a 0 un 0.
            lcd_set_pixel(buffer, x, y, img[byte_del_pixel] & mascara);
            x++;
            // Cuando hemos escrito un número de pixeles iguales al ancho de la imagen saltamos de línea.
            if ((x - x_inicial) == width)
            {
                y++;
                x = x_inicial;
            }
            // Cuando escribimos todas las líneas que equivalen al alto de la imagen, hemos terminado.
            if ((y - linea_inicial) == height)
            {
                return;
            }
            // Si nos salimos del tamaño del LCD paramos de escribir.
            if (y >= LCD_HEIGHT)
            {
                return;
            }
        }
    }
}
/*
Function to draw characters stored in RAM or FLASH in the screen buffer.
Parameters.
buffer: buffer in RAM that we are going to modify.
x: x position from where to start drawing the image.
y: line from where to start drawing the image.
width: dimensions in pixels of the width of the image.
height: dimensions in pixels of the height of the image.
*text = pointer to the array that contains the string of characters to write.
*/
void lcd_draw_text(uint8_t *buffer, uint16_t x, uint16_t y, uint16_t width, uint16_t height, char *text)
{
    const uint8_t *font;      // Font we want to use, pointer to arrays of character images stored in RAM or FLASH.
    const uint8_t *writeChar; // Pointer to the position of the character we want to write contained in the previous array.
    uint16_t x_actual = x;
    uint16_t y_actual = y;
    // Value we use to calculate the position of the character to write within a previous font array.
    uint16_t caracter;

    // Select the font to use based on the desired font size.
    switch (height)
    {
    case 8:
        font = FUENTE_8_8_1;
        break;
    case 16:
        if (width == 8)
        {
            font = FUENTE_16_8;
            break;
        }
        else
        {
            font = FUENTE_16_16_3;
            break;
        }
    case 32:
        font = FUENTE_32_16_8;
        break;
    default:
        font = FUENTE_8_8_1;
    }

    // Draw each character of the string on the screen.
    for (uint16_t numero_caracter = 0; numero_caracter < strlen(text); numero_caracter++)
    {
        // Determine the position of the byte where the character to write starts within the array.
        // Bytes that represent a character * (its ASCII value - 32 (character at position 0 of the array)).
        caracter = ((height * width) >> 3) * (text[numero_caracter] - ' ');
        // Pointer to the start of the character to write within the array.
        writeChar = font + caracter;
        // Draw the character.
        lcd_render_image_buffer(buffer, x_actual, y_actual, width, height, writeChar);
        x_actual += width;
    }
}

/*
Function to draw a rectangle on the screen.
Parameters.
buffer: buffer in RAM that we are going to modify.
x: x position from where to start drawing the rectangle.
y: line from where to start drawing the rectangle.
width: width of the rectangle.
height: height of the rectangle.
pixelcolor: color of the pixel (white or black).
fillType: FILL, HOLLOW.

A filled rectangle can be used to erase areas of the screen by giving it a white or black color.
*/
void lcd_draw_rectangle(uint8_t *buffer, uint16_t x, uint16_t y, uint16_t width, uint16_t height, enum color pixelColor, enum fill_type fillType)
{
    if (fillType)
    {
        for (uint16_t linea_a_dibujar = y; linea_a_dibujar < y + height; linea_a_dibujar++)
        {
            for (uint16_t pixel = x; pixel < x + width; pixel++)
            {
                lcd_set_pixel(buffer, pixel, linea_a_dibujar, pixelColor);
            }
        }
    }
    else
    {
        // Draw the two horizontal lines.
        for (uint16_t pixel = x; pixel <= width + x; pixel++)
        {
            lcd_set_pixel(buffer, pixel, y, pixelColor);
        }
        for (uint16_t pixel = x; pixel <= width + x; pixel++)
        {
            lcd_set_pixel(buffer, pixel, y + height, pixelColor);
        }
        // Draw the two vertical lines.
        for (uint16_t pixel = y; pixel <= y + height; pixel++)
        {
            lcd_set_pixel(buffer, x, pixel, pixelColor);
        }
        for (uint16_t pixel = y; pixel <= y + height; pixel++)
        {
            lcd_set_pixel(buffer, x + width, pixel, pixelColor);
        }
    }
}

/*
Function to draw a horizontal line on the screen.
Parameters.
buffer: buffer in RAM that we are going to modify.
x: x position from where to start drawing the rectangle.
y: line from where to start drawing the rectangle.
length: length of the line
pixelColor: color of the pixel (white or black)
*/
void lcd_draw_horizontal_line(uint8_t *buffer, uint16_t x, uint16_t y, uint16_t length, enum color pixelColor)
{
    for (uint16_t pixel = x; pixel <= length + x; pixel++)
    {
        lcd_set_pixel(buffer, pixel, y, pixelColor);
    }
}

/*
Function to draw a vertical line on the screen.
Parameters.
buffer: buffer in RAM that we are going to modify.
x: x position from where to start drawing the rectangle.
y: line from where to start drawing the rectangle.
length: length of the line
pixelColor: color of the pixel (white or black)
*/
void lcd_draw_vertical_line(uint8_t *buffer, uint16_t x, uint16_t y, uint16_t length, enum color pixelColor)
{
    for (uint16_t pixel = y; pixel <= y + length; pixel++)
    {
        lcd_set_pixel(buffer, x, pixel, pixelColor);
    }
}

/*
Function to draw a vertical line on the screen.
Parameters.
buffer: buffer in RAM that we are going to modify.
color: color of the pixel (0: white, 1: black)
*/
void lcd_clear(uint8_t *buffer, enum color pixelColor)
{
    if (pixelColor)
    {
        for (int i = 0; i < FB_SIZE; i++)
        {
            buffer[i] = 0x00;
        }
    }
    else
    {
        for (int i = 0; i < FB_SIZE; i++)
        {
            buffer[i] = 0xFF;
        }
    }
}