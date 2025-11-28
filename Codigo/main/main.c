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

static sht_40_data_t sht40_data = {.temp = 0.0f, .humidity = 0.0f};

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

        char sht40_text[40];
        snprintf(sht40_text, sizeof(sht40_text), "T: %.1f C, HR: %.1f%%", sht40_data.temp, sht40_data.humidity);
        lcd_draw_text(DISPLAY_BUFFER1, 10, 50, 16, 32, sht40_text);

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
    i2c_main_task_create(&sht40_data);

    /* Start Test counter task */
    xTaskCreate(&counter_task, "counter_task", 1024 * 2, NULL, 5, NULL);
}