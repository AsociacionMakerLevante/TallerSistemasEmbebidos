/*
Funciones para controlar la pantalla LCD
*/
#ifndef LS027B7DH01_H
#define LS027B7DH01_H

#define FB_ANCHO 400
#define FB_ALTO 240
#define FB_SIZE (FB_ALTO * FB_ANCHO)/8

extern uint8_t FRAME_BUFFER[FB_SIZE];

void escribir_texto(uint16_t x, uint16_t y, uint16_t size, char *texto);
void escribir_imagen(uint16_t x, uint16_t y, uint16_t ancho, uint16_t alto, const uint8_t *imagen);

#endif