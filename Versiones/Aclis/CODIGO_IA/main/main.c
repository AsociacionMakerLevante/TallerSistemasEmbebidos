/**
 * @file main.c
 * Muestra temperatura y humedad del SHT4x en un cuadrado centrado en la
 * pantalla Sharp LS027B7DH01 (400×240). Los valores se actualizan cada 60 s.
 *
 * Arquitectura:
 *  - Una tarea FreeRTOS gestiona todo LVGL.
 *  - lv_timer_t interno de LVGL dispara la lectura del sensor cada 60 s.
 *  - El tick de LVGL viene de lv_tick_set_cb() + esp_timer.
 *  - El flush callback convierte RGB565 → 1 bit para el driver Sharp.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "lvgl.h"
#include "sharp_lcd.h"
#include "sht4x.h"

static const char *TAG = "main";

/* -----------------------------------------------------------------------
   Tick de LVGL 9.5+
   ----------------------------------------------------------------------- */
static uint32_t lvgl_tick_cb(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000LL);
}

/* -----------------------------------------------------------------------
   Buffers de renderizado: 20 filas × 400 px × 2 bytes (RGB565)
   ----------------------------------------------------------------------- */
#define LVGL_BUFFER_ROWS 20
#define LVGL_BUF_BYTES   (LCD_WIDTH * LVGL_BUFFER_ROWS * 2)

static uint8_t lvgl_buf1[LVGL_BUF_BYTES];
static uint8_t lvgl_buf2[LVGL_BUF_BYTES];

/* -----------------------------------------------------------------------
   Label de sensor (acceso desde el timer callback)
   ----------------------------------------------------------------------- */
static lv_obj_t *sensor_label = NULL;

/* -----------------------------------------------------------------------
   Callback de flush LVGL → Sharp LCD
   ----------------------------------------------------------------------- */
static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    sharp_lcd_flush(area->x1, area->y1, area->x2, area->y2, px_map);
    lv_display_flush_ready(disp);
}

/* -----------------------------------------------------------------------
   Lectura del sensor y actualización del label
   Llamado una vez al arrancar y luego por el lv_timer cada 60 s.
   ----------------------------------------------------------------------- */
static void sensor_update_cb(lv_timer_t *timer)
{
    float temp = 0.0f, hum = 0.0f;
    char  buf[48];

    if (sht4x_read(&temp, &hum) == ESP_OK) {
        /* Formato: dos líneas dentro del cuadrado */
        snprintf(buf, sizeof(buf), "Temp:\n%.1f \xc2\xb0""C\n\nHum:\n%.1f%%",
                 temp, hum);
        ESP_LOGI(TAG, "Sensor → T=%.1f°C  H=%.1f%%", temp, hum);
    } else {
        snprintf(buf, sizeof(buf), "Temp:\n-- \xc2\xb0""C\n\nHum:\n--.-%%" );
        ESP_LOGW(TAG, "Sin lectura del sensor");
    }

    if (sensor_label) {
        lv_label_set_text(sensor_label, buf);
    }
}

/* -----------------------------------------------------------------------
   Tarea LVGL
   ----------------------------------------------------------------------- */
static void lvgl_task(void *arg)
{
    ESP_LOGI(TAG, "Iniciando LVGL 9.5");
    lv_init();
    lv_tick_set_cb(lvgl_tick_cb);

    /* Display 400×240 */
    lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp, lvgl_buf1, lvgl_buf2,
                           LVGL_BUF_BYTES, LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Pantalla blanca */
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    /* ---- Cuadrado centrado 200×200 px ---- */
    lv_obj_t *box = lv_obj_create(scr);
    lv_obj_set_size(box, 200, 200);
    lv_obj_align(box, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(box, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(box, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(box, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(box, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(box, 10, LV_PART_MAIN);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    /* ---- Label de temperatura y humedad ---- */
    sensor_label = lv_label_create(box);
    lv_label_set_text(sensor_label, "Temp:\n-- \xc2\xb0""C\n\nHum:\n--.--%%");
    lv_label_set_long_mode(sensor_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(sensor_label, lv_pct(100));
    lv_obj_set_style_text_color(sensor_label, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_text_font(sensor_label, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_align(sensor_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(sensor_label, LV_ALIGN_CENTER, 0, 0);

    /* ---- Primera lectura inmediata ---- */
    sensor_update_cb(NULL);

    /* ---- Timer LVGL: actualizar cada 60 s ---- */
    lv_timer_create(sensor_update_cb, 60000, NULL);

    ESP_LOGI(TAG, "UI lista. Actualizando cada 60 s");

    /* Bucle LVGL */
    while (1) {
        uint32_t delay_ms = lv_timer_handler();
        if (delay_ms == 0 || delay_ms > 100) delay_ms = 10;
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

/* -----------------------------------------------------------------------
   Punto de entrada
   ----------------------------------------------------------------------- */
void app_main(void)
{
    /* Pausa para que USB-CDC enumere antes de loggear */
    vTaskDelay(pdMS_TO_TICKS(4000));

    ESP_LOGI(TAG, "=== PLACAMAKER – Temp/Hum con SHT4x ===");

    /* Periféricos */
    sharp_lcd_init();
    sht4x_init();

    /* Tarea LVGL */
    xTaskCreate(lvgl_task, "lvgl", 8192, NULL, 5, NULL);

    /* Heartbeat */
    uint32_t cnt = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000));
        ESP_LOGI(TAG, "Heartbeat #%lu", (unsigned long)++cnt);
    }
}
