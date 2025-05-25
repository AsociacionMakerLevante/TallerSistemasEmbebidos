/*Funciones para controlar arrays en memoria RAM que representan el contenido de la 
pantalla LCD. 

Se pueden crear tantos arrays como reprensentaciones de la pantalla se quieran tener 
guardadas en memoria RAM (12 kbytes cada una), o mantener solo una representación de 
la pantalla en RAM y modificarla cada vez que se quiera cambiar el contenido de esta.
*/
#ifndef LCD_FBUFFER_H
#define LCD_FBUFFER_H

//Pixeles en la pantalla. LCD de 400x200 en nuestro caso.
#define LCD_ANCHO 400
#define LCD_LINEAS 240
#define FB_SIZE (LCD_LINEAS * LCD_ANCHO)/8

//Color de los pixeles.
enum color {BLANCO, NEGRO};

//Relleno figuras geométricas.
enum rellenar{HUECO, RELLENO};

//Array en RAM que contiene el valor de los pixeles de la pantalla del LCD.
extern uint8_t BUFFER_PANTALLA1[FB_SIZE];

//Funciones para modificar el array anterior.
void lcd_fbuffer_limpiar(uint8_t *buffer, enum color colorpixel);
void lcd_fbuffer_dibujar_pixel(uint8_t *buffer, uint16_t posicion_eje_x, uint16_t linea, enum color colorpixel);
void lcd_fbuffer_rectangulo(uint8_t *buffer, uint16_t x, uint16_t y, uint16_t ancho, uint16_t alto, enum color colorpixel, enum rellenar relleno);
void lcd_fbuffer_imagen(uint8_t *buffer, uint16_t posicion_eje_x, uint16_t linea, uint16_t ancho_img, uint16_t alto_img, const uint8_t *img);
void lcd_fbuffer_texto(uint8_t *buffer, uint16_t posicion_eje_x, uint16_t linea, uint16_t ancho_caracter, uint16_t alto_caracter, char *texto);
void lcd_fbuffer_linea_h(uint8_t *buffer, uint16_t posicion_eje_x, uint16_t linea, uint16_t longitud, enum color colorpixel);
void lcd_fbuffer_linea_v(uint8_t *buffer, uint16_t posicion_eje_x, uint16_t linea, uint16_t longitud, enum color colorpixel);
#endif