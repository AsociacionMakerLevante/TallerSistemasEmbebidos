/*Funciones para escribir arrays en memoria RAM que representan el contenido del contenido
de los pixeles de la pantalla LCD. 

Se pueden crear tantos arrays como reprensentaciones de la pantalla se quieran tener 
guardadas en memoria RAM (12 kbytes cada una), o mantener solo una representación de 
la pantalla en RAM y modificarla cada vez que se quiera cambiar el contenido de esta.

Código comentado para el taller de Sistema Embebidos de la Asociación Maker Levante.

Enlaces relacionados:
- https://www.youtube.com/watch?v=5cp2iPGWmUY
*/
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "lcd_fbuffer.h"
#include "imagenes.h"
#include "fuentes.h"

//Orientación del LCD.
#define ORIENTACION_LCD_INVERSA

//Definimos arrays que representan las pantallas del LCD en RAM.
uint8_t BUFFER_PANTALLA1[FB_SIZE];

/*
Función para dibujar en pixel en el buffer.
Parámetros.
buffer: buffer en RAM que vamos a modificar.
posicion_eje_x: posición del pixel en el eje x de la pantalla. Rango 0 a (FB_ANCHO - 1)
línea: posición del pixel en el eje y de la pantalla. Rango 0 a (FB_ALTO  - 1)
color = 1 pixel encendido en el lcd, color = 0 pixel apagado.
*/
void lcd_fbuffer_dibujar_pixel(uint8_t *buffer, uint16_t posicion_eje_x, uint16_t linea, enum color colorpixel)
{
    //Comprobamos no escribir un pixel fuera de los límites del array.
    if(linea >= LCD_LINEAS)
    {
        linea = LCD_LINEAS - 1;
    }
    //La primera posición del eje x es el bit 0.
    if(posicion_eje_x >= LCD_ANCHO )
    {
        posicion_eje_x = LCD_ANCHO - 1; 
    }

    //Calculamos el byte donde está el bit (pixel) a modificar en el array que representa la pantalla.
    #ifdef ORIENTACION_LCD_INVERSA
    int16_t posicion = (((LCD_ANCHO * (LCD_LINEAS - 1 - linea)) + (LCD_ANCHO - 1 - posicion_eje_x )) >> 3);
    //Restamos 1 para no salirnos de los límites del array, el array empieza en 0 
    //y dividimos entre 8 para localizar el bit dentro del byte correspondiente.
    //if(posicion < 0){posicion = 0;} //no se daría este caso, el bit de signo no se desplaza.

    //Dibujamos el pixel en el LCD. Un 0 en el LCD -> pixel en negro.
    if(posicion < FB_SIZE)
    {
        if(colorpixel)
        {
            buffer[posicion] &= ~(0x80 >> (posicion_eje_x & 0x07)); //Si es mayor de 8 (7 en bin) saltamos de byte en el array
        }
        else
        {
            buffer[posicion] |= (0x80 >> (posicion_eje_x & 0x07));
        }
    }

    #else
    uint16_t posicion = (LCD_ANCHO * linea + posicion_eje_x) >> 3; //Dividimos entre 8
    //Dibujamos el pixel en el LCD. Un 0 en el LCD -> pixel en negro.
    if(posicion < FB_SIZE)
    {
        if(colorpixel)
        {
            buffer[posicion] &= ~(0x01 << (posicion_eje_x & 0x07)); //Si es mayor de 8 saltamos de byte en el array
        }
        else
        {
            buffer[posicion] |= (0x01 << (posicion_eje_x & 0x07));
        }
    }
    #endif
}

/*
Función para dibujar una imagen almacenada en RAM o en FLASH en el buffer de la pantalla.
Parámetros.
buffer: buffer en RAM que vamos a modificar.
posicion_eje_x: posición de x desde donde empezar a dibujar la imagen.
línea: línea desde donde empezar a dibujar la imagen.
ancho_img: dimesiones en pixeles del ancho de la imagen.
alto_img: dimesiones en pixeles del alto de la imagen. 
*img = puntero al array que contiene la imagen.

por hacer: añadir la opción de color haciendo una XOR ^ al último parámetro de lcd_fbuffer_dibujar_pixel
*/
void lcd_fbuffer_imagen(uint8_t *buffer, uint16_t posicion_eje_x, uint16_t linea, uint16_t ancho_img, uint16_t alto_img, const uint8_t *img)
{
    uint8_t mascara = 0;
    //Número de bytes que contiene la imagen.
    uint16_t bytes_imagen = (ancho_img * alto_img) >> 3;
    uint16_t x_inicial = posicion_eje_x;
    uint16_t linea_inicial = linea;

    //Dibujamos los pixeles 1 a 1 de la imagen en el buffer que contiene la pantalla.
    for(uint16_t byte_del_pixel = 0; byte_del_pixel < bytes_imagen; byte_del_pixel++)
    {
        //Los pixeles están contenidos en bytes img[i]
        for(uint16_t bit =0; bit < 8; bit++)
        {
            //Seleccionamos y dibujamos los bits de los bytes de la imagen de 1 en 1 mediante la máscara.
            mascara = 0x80 >> bit;
            //Si el pixel en la imagen está a 1 escribe un 1, si está a 0 un 0.
            lcd_fbuffer_dibujar_pixel(buffer, posicion_eje_x, linea, img[byte_del_pixel] & mascara);
            posicion_eje_x++;
            //Cuando hemos escrito un número de pixeles iguales al ancho de la imagen saltamos de línea.
            if((posicion_eje_x - x_inicial) == ancho_img)
            {
                linea++;
                posicion_eje_x = x_inicial;
            }
            //Cuando escribimos todas las líneas que equivalen al alto de la imagen, hemos terminado.
            if((linea - linea_inicial) == alto_img)
            {
                return;
            }
            //Si nos salimos del tamaño del LCD paramos de escribir.
            if(linea >= LCD_LINEAS)
            {
                return;
            }
        }
    }
}
/*
Función para dibujar una carácteres almacenado en RAM o en FLASH en el buffer de la pantalla.
Parámetros.
buffer: buffer en RAM que vamos a modificar.
posicion_eje_x: posición de x desde donde empezar a dibujar la imagen.
línea: línea desde donde empezar a dibujar la imagen.
ancho_caracter: dimesiones en pixeles del ancho de la imagen.
alto_caracter: dimesiones en pixeles del alto de la imagen. 
*texto = puntero al array que contiene la cadena de caracteres a escribir.

por hacer: añadir el parámetro para seleccionar el color de las letras.
*/
void lcd_fbuffer_texto(uint8_t *buffer, uint16_t posicion_eje_x, uint16_t linea, uint16_t ancho_caracter, uint16_t alto_caracter, char *texto)
{
    const uint8_t *fuente;       //Fuente que queremos usar, puntero a arrays de imagenes de caracteres almacenados en RAM o FLASH.
    const uint8_t *escribirChar; //Puntero a la posición del carácter que queremos escribir contenido en el array anterior.
    uint16_t x_actual = posicion_eje_x;
    uint16_t y_actual = linea;
    //Valor quer usamos para calcular la posición del carácter a escribir dentro de un array anterior de fuentes.
    uint16_t caracter;

    //Seleccionar la fuente a usar en función del tamaño de fuente deseado. 
    switch(alto_caracter)
    {   
        case 8:
            fuente = FUENTE_8_8_1;
            break;
        case 16:
            if(ancho_caracter == 8)
            {
                fuente = FUENTE_16_8;
                break;
            }
            else
            {
                fuente = FUENTE_16_16_3;
                break;
            }
        case 32:
            fuente = FUENTE_32_16_8;
            break;
        default:
            fuente = FUENTE_8_8_1;
    }

    //Dibujamos cada carácter de la cadena en la pantalla.
    for(uint16_t numero_caracter = 0; numero_caracter < strlen(texto); numero_caracter++)
    {
        /*
        //Saltar de línea si las letras llegan al final del ancho de la pantalla.
        if(x_actual >= LCD_ANCHO)
        {
            x_actual = posicion_eje_x;
            y_actual += alto_caracter;
        }

        if(y_actual >= LCD_LINEAS)
        {
            return;
        }
        */
        //Determinamos la posición del byte donde empieza el carácter a escribir dentro del array.
        //Bytes que reperensetan un caracter * (su valor en ASCII - 32 (carácter en la posición 0 del array)).
        caracter = ((alto_caracter * ancho_caracter) >> 3) * (texto[numero_caracter] - ' ');
        //Puntero al inicio del carácter a escribir dentro del array.
        escribirChar = fuente + caracter;
        //Dibujamos el carácter.
        lcd_fbuffer_imagen(buffer, x_actual, y_actual, ancho_caracter, alto_caracter, escribirChar);
        x_actual += ancho_caracter;
    }
}
/*
Función para dibujar un rectángulo en la pantalla.
Parámetros.
buffer: buffer en RAM que vamos a modificar.
posicion_eje_x: posición de x desde donde empezar a dibujar el ractángulo.
línea: línea desde donde empezar a dibujar el rectángulo.
ancho: ancho del rectángulo.
alto: alto del rectángulo.
color: color del pixel (blanco o negro).
relleno: RELLENO, HUECO.
*/
void lcd_fbuffer_rectangulo(uint8_t *buffer, uint16_t posicion_eje_x, uint16_t linea, uint16_t ancho, uint16_t alto, enum color colorpixel, enum rellenar relleno)
{
    if(relleno)
    {
        for(uint16_t linea_a_dibujar = linea; linea_a_dibujar < linea + alto; linea_a_dibujar++)
        {
            for(uint16_t pixel = posicion_eje_x; pixel < posicion_eje_x + ancho; pixel++)
            {
                lcd_fbuffer_dibujar_pixel( buffer ,pixel, linea_a_dibujar, colorpixel);
            }
        }
    }
    else
    {
        //Dibujamos las dos líneas horizontales.
        for(uint16_t pixel = posicion_eje_x; pixel <= ancho + posicion_eje_x;  pixel++)
        {
            lcd_fbuffer_dibujar_pixel(buffer ,pixel, linea, colorpixel);
        }
        for(uint16_t pixel = posicion_eje_x; pixel <= ancho + posicion_eje_x;  pixel++)
        {
            lcd_fbuffer_dibujar_pixel(buffer ,pixel, linea + alto, colorpixel);
        }
        //Dibujamos las dos líneas verticales. 
        for(uint16_t pixel = linea; pixel <= linea + alto; pixel++)
        {
            lcd_fbuffer_dibujar_pixel(buffer ,posicion_eje_x, pixel, colorpixel);
        }   
        for(uint16_t pixel = linea; pixel <= linea + alto; pixel++)
        {
            lcd_fbuffer_dibujar_pixel(buffer ,posicion_eje_x + ancho, pixel, colorpixel);
        }     

    }
}
/*
Función para dibujar una línea vertical en la pantalla.
Parámetros.
buffer: buffer en RAM que vamos a modificar.
posicion_eje_x: posición de x desde donde empezar a dibujar el ractángulo.
línea: línea desde donde empezar a dibujar el rectángulo.
longitud: longitud de la linea
color: color del pixel (blanco o negro)
*/
void lcd_fbuffer_linea_h(uint8_t *buffer, uint16_t posicion_eje_x, uint16_t linea, uint16_t longitud, enum color colorpixel)
{
    for(uint16_t pixel = posicion_eje_x; pixel <= longitud + posicion_eje_x;  pixel++)
    {
        lcd_fbuffer_dibujar_pixel(buffer ,pixel, linea, colorpixel);
    }
}
/*
Función para dibujar una línea vertical en la pantalla.
Parámetros.
buffer: buffer en RAM que vamos a modificar.
posicion_eje_x: posición de x desde donde empezar a dibujar el ractángulo.
línea: línea desde donde empezar a dibujar el rectángulo.
longitud: longitud de la linea
color: color del pixel (blanco o negro)
*/
void lcd_fbuffer_linea_v(uint8_t *buffer, uint16_t posicion_eje_x, uint16_t linea, uint16_t longitud, enum color colorpixel)
{
    for(uint16_t pixel = linea; pixel <= linea + longitud; pixel++)
    {
        lcd_fbuffer_dibujar_pixel(buffer ,posicion_eje_x, pixel, colorpixel);
    }
}
/*
Función para escribir a 1 o a 0 todo el array que contiene la imagen.
Parámetros.
buffer: buffer en RAM que vamos a modificar.
color: color del pixel (0: blanco, 1: negro)
*/
void lcd_fbuffer_limpiar(uint8_t *buffer, enum color colorpixel)
{
    if(colorpixel)
    {
        for(int i = 0; i < FB_SIZE; i++)
        {
         buffer[i] = 0x00;
        }
    }
    else
    {
        for(int i = 0; i < FB_SIZE; i++)
        {
         buffer[i] = 0xFF;
        }       
    }
}
