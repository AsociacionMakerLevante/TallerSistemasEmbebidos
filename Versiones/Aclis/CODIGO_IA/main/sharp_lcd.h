/**
 * @file sharp_lcd.h
 * Driver para la pantalla Sharp Memory LCD LS027B7DH01
 * 400×240 píxeles, monocromo, interfaz SPI
 *
 * Conexiones (ver pinOut.h):
 *   SPI_CLK   → SCLK  (GPIO 4)
 *   SPI_MOSI  → SI    (GPIO 5)
 *   SPI_CS    → SCS   (GPIO 6) – activo en ALTO (gestionado como GPIO)
 *   LCD_EXTCON→ EXTCOMIN (GPIO 3) – señal de inversión COM periódica
 */

#ifndef SHARP_LCD_H
#define SHARP_LCD_H

#include <stdint.h>

/** Resolución del display */
#define LCD_WIDTH   400
#define LCD_HEIGHT  240

/**
 * @brief Inicializa el bus SPI, los GPIO y el driver de la pantalla.
 *        Envía un comando "All Clear" inicial y arranca el temporizador
 *        de EXTCOMIN.
 */
void sharp_lcd_init(void);

/**
 * @brief Borra toda la pantalla (pone todos los píxeles en blanco).
 */
void sharp_lcd_clear(void);

/**
 * @brief Escribe un área rectangular en la pantalla.
 *
 * Debe llamarse desde el callback de flush de LVGL.
 * El formato de píxel de entrada es RGB565 (2 bytes por píxel, little-endian).
 * El driver convierte internamente a 1 bit por píxel usando umbral de luminancia.
 *
 * @param x1     Columna inicial (0-based)
 * @param y1     Fila inicial    (0-based)
 * @param x2     Columna final   (0-based, inclusive)
 * @param y2     Fila final      (0-based, inclusive)
 * @param px_map Puntero al buffer de píxeles RGB565 de LVGL
 */
void sharp_lcd_flush(int32_t x1, int32_t y1,
                     int32_t x2, int32_t y2,
                     const uint8_t *px_map);

#endif /* SHARP_LCD_H */
