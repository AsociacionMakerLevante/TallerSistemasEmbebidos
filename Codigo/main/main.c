/*
 Makers levante
*/
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "esp_log.h"

#include "pinOut.h"
#include "gpios.h"
#include "lcd.h"
#include "lcd_fbuffer.h"
#include "i2c_bus.h"

static uint16_t s_refresh_period = 1000;
static uint16_t s_counter = 0;

// Function that updates a counter and displays it on the LCD every refresh period.
void counter_task(void *arg)
{
    while (1)
    {
        s_counter++;
        ESP_LOGI("TEST", "Counter value: %d", s_counter);
        char text[20];
        snprintf(text, sizeof(text), "Counter: %d", s_counter);
        lcd_draw_text(DISPLAY_BUFFER1, 10, 10, 16, 32, text);
        vTaskDelay(s_refresh_period / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    /* GP IO devices (buttons, buzzer) init */
    gpios_crear_tarea();

    /* Display init */
    lcd_main_task_create();

    /* I2C devices (Thermometer, RTC, Battery gauge) init */
    i2c_crear_tarea_i2c();

    /* Start Test counter task */
    xTaskCreate(&counter_task, "counter_task", 1024 * 2, NULL, 5, NULL);
}