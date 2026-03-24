/**
 * @file lv_conf.h
 * Configuración de LVGL 9.x para ESP32-C3 con pantalla Sharp LS027B7DH01
 * 400x240 píxeles, monocromo
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   AJUSTES DE COLOR
 *====================*/

/* Profundidad de color: 16 bits (RGB565) – conversión a 1 bit en el driver */
#define LV_COLOR_DEPTH 16

/*====================
   MEMORIA
 *====================*/

/* Heap interno de LVGL (32 KB es suficiente para UI simple) */
#define LV_MEM_SIZE (32U * 1024U)

/*====================
   HAL / TICK
 *====================*/

/* En LVGL 9.5+ el tick se registra en código con lv_tick_set_cb()
   o llamando lv_tick_inc() periódicamente. Ver main.c. */

/* Período de refresco por defecto (ms) */
#define LV_DEF_REFR_PERIOD  33

/*====================
   SISTEMA OPERATIVO
 *====================*/

/* Sin OS integrado en LVGL – gestionamos thread-safety con una única tarea */
#define LV_USE_OS   LV_OS_NONE

/*====================
   DIBUJO
 *====================*/

#define LV_DRAW_BUF_STRIDE_ALIGN   1
#define LV_DRAW_BUF_ALIGN          4

/*====================
   FUENTES
 *====================*/

/* Fuentes Montserrat incluidas en LVGL */
#define LV_FONT_MONTSERRAT_8   0
#define LV_FONT_MONTSERRAT_10  0
#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  0
#define LV_FONT_MONTSERRAT_18  0
#define LV_FONT_MONTSERRAT_20  0
#define LV_FONT_MONTSERRAT_22  0
#define LV_FONT_MONTSERRAT_24  0
#define LV_FONT_MONTSERRAT_28  1
#define LV_FONT_MONTSERRAT_32  0
#define LV_FONT_MONTSERRAT_36  0
#define LV_FONT_MONTSERRAT_48  0

/* Fuente por defecto */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*====================
   WIDGETS
 *====================*/

#define LV_USE_OBJ      1
#define LV_USE_LABEL    1
#define LV_USE_IMAGE    0
#define LV_USE_LINE     0
#define LV_USE_ARC      0
#define LV_USE_BUTTON   0
#define LV_USE_BTNMATRIX 0
#define LV_USE_CANVAS   0
#define LV_USE_CHECKBOX 0
#define LV_USE_DROPDOWN 0
#define LV_USE_IMGBTN   0
#define LV_USE_KEYBOARD 0
#define LV_USE_LED      0
#define LV_USE_LIST     0
#define LV_USE_MENU     0
#define LV_USE_MSGBOX   0
#define LV_USE_ROLLER   0
#define LV_USE_SCALE    0
#define LV_USE_SLIDER   0
#define LV_USE_SPAN     0
#define LV_USE_SPINBOX  0
#define LV_USE_SPINNER  0
#define LV_USE_SWITCH   0
#define LV_USE_TABLE    0
#define LV_USE_TABVIEW  0
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN      0
#define LV_USE_BAR      0

/*====================
   LOG
 *====================*/

#define LV_USE_LOG       0
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/*====================
   EXTRAS
 *====================*/

#define LV_USE_THEME_DEFAULT        0
#define LV_USE_THEME_SIMPLE         1
#define LV_USE_THEME_MONO           1

#define LV_USE_FONT_PLACEHOLDER     1

#endif /* LV_CONF_H */
