#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "pinOut.h"
#include <LS027B7DH01.h>
#include <imagenes.h>
#include <fuentes.h>

uint8_t FRAME_BUFFER[FB_SIZE];
uint8_t FRAME_BUFFER2[FB_SIZE];

void escribir_pixel(uint16_t x, uint16_t y, bool estado);
bool leer_pixel(uint16_t x, uint16_t y);
void dibujar_rectangulo(uint16_t x, uint16_t y, uint16_t ancho, uint16_t alto);
void dibujar_linea_diagonal(uint16_t x, uint16_t y, uint16_t ancho, uint16_t alto);


/*********************************************************************/
/*
//Escribimos el valor de un pixel.
void escribir_pixel(uint16_t x, uint16_t y, bool estado)
{
    uint16_t posicion = (FB_ANCHO * y + x) >> 3; //Dividimos entre 8
    //uint16_t posicion = (FB_ALTO * x + y) >> 3; //Dividimos entre 8

    if(estado)
    {
        FRAME_BUFFER[posicion] &= ~(0x80 >> (x & 0x07)); //Invertimos el color
        //FRAME_BUFFER[posicion] |= (0x80 >> (x & 0x07)); //Si es mayor de 8 saltamos de byte en el array
    }
    else
    {
        FRAME_BUFFER[posicion] |= (0x80 >> (x & 0x07));
        //FRAME_BUFFER[posicion] &= ~(0x80 >> (x & 0x07));
    }
}
/*/
//*
//Escribimos el valor de un pixel.
void escribir_pixel(uint16_t x, uint16_t y, bool estado)
{
    int16_t posicion = ((FB_ANCHO * (FB_ALTO - y - 1)) + (FB_ANCHO - x - 1)) >> 3; //Dividimos entre 8
    //uint16_t posicion = (FB_ALTO * x + y) >> 3; //Dividimos entre 8

    if(estado)
    {
        FRAME_BUFFER[posicion] &= ~(0x80 >> (x & 0x07)); //Invertimos el color
        //FRAME_BUFFER[posicion] |= (0x80 >> (x & 0x07)); //Si es mayor de 8 saltamos de byte en el array
    }
    else
    {
        FRAME_BUFFER[posicion] |= (0x80 >> (x & 0x07));
        //FRAME_BUFFER[posicion] &= ~(0x80 >> (x & 0x07));
    }
}
//*/
//Leemos el valor de un pixel
bool leer_pixel(uint16_t x, uint16_t y)
{
    uint16_t posicion = (FB_ANCHO * y + x) >> 3; //Dividimos entre 8
    return FRAME_BUFFER[posicion] & (0x80 >> (x & 0x07));
}

//Escribimos una imagen en el frame buffer.
void escribir_imagen(uint16_t x, uint16_t y, uint16_t ancho, uint16_t alto, const uint8_t *imagen)
{
    uint8_t chkMsk = 0;
    uint16_t size = (ancho * alto) >> 3;

    uint16_t x_inicial = x;
    uint16_t y_inicial = y;

    for(int i = 0; i < size; i++)
    {
        for(int j =0; j < 8; j++)
        {
            chkMsk = 0x80 >> j;
            escribir_pixel(x, y, imagen[i] & chkMsk);
            x++;

            if((x - x_inicial) == ancho)
            {
                y++;
                x = x_inicial;
            }
            if((y - y_inicial) == alto)
            {
                return;
            }

            if( y > FB_ALTO)
            {
                return;
            }
        }
    }
}

//Dibujar un rectangulo
void dibujar_rectangulo(uint16_t x, uint16_t y, uint16_t ancho, uint16_t alto)
{
    uint16_t offset = 0;
    for(int j = y; j < y; j++)
    {
        for(int i = x; i < x + ancho; i++)
        {
            escribir_pixel(i - offset, j, true);
        }
        offset += 1;
    }
}

//Dibujar línea diagonal
void dibujar_linea_diagonal(uint16_t x, uint16_t y, uint16_t ancho, uint16_t alto)
{
    uint16_t offset = 0;
    for(int j = y; j < y + alto; j++)
    {
        for(int i = x; i < x + ancho; i++)
        {
            escribir_pixel(i - offset, j, true);
        }
        offset += 1;
    }
}

//Escribe texto al frame buffer
void escribir_texto(uint16_t x, uint16_t y, uint16_t size, char *texto)
{
    const uint8_t *fuente; //Fuente que queremos usar
    const uint8_t *escribirChar; //Carácter que queremos escribir

    uint16_t x_actual = x;
    uint16_t y_actual = y;

    uint16_t target;

    //Seleccionar la fuente a usar en función del tamaño de fuente deseado.
    switch(size)
    {
        case 24:
            fuente = FUENTE_8_8_1;
            break;
        case 20:
            fuente = FUENTE_8_8_1;
            break;
        default:
            fuente = FUENTE_8_8_1;
    }

    for(uint16_t i = 0; i < strlen(texto); i++)
    {
        if(x_actual >= FB_ANCHO)
        {
            x_actual = x;
            y_actual += size;
        }

        if(y_actual >= FB_ALTO)
        {
            return;
        }

        target = (size*size >> 3) * (texto[i] - ' ');
        escribirChar = fuente + target;
        escribir_imagen(x_actual, y_actual, size, size, escribirChar);

        x_actual += size;
    }
}