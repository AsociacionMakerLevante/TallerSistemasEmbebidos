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

static const char *TAG = "main.c";

static uint16_t s_refresh_period = 1000;

static sht40_data_t sht40_data = {.temp = 0.0f, .humidity = 0.0f};
static max17048_data_t max17048_data = {.voltage = 0.0f, .soc = 0};
static mcp7940_data_t mcp7940_data = {.seconds = 0, .minutes = 0, .hours = 0, .dayOfWeek = 0, .dayOfMonth = 0, .month = 0, .year = 0};

volatile uint8_t gvui8_positive = 0;
volatile uint8_t gvui8_negative = 0;

// Function that updates the data on display
void display_task(void *arg)
{
    while (1)
    {
        if (gvui8_positive)
        {
            ESP_LOGI(TAG, "Positive edge ISR callback called for button %d\n", gvui8_positive);
            gvui8_positive = 0;
        }
        if (gvui8_negative)
        {
            ESP_LOGI(TAG, "Negative edge ISR callback called for button %d\n", gvui8_negative);
            gvui8_negative = 0;
        }

        lcd_draw_text(DISPLAY_BUFFER1, 10, 10, 16, 32, "Makers Levante");

        char sht40_text[40];
        snprintf(sht40_text, sizeof(sht40_text), "T: %.1f C, HR: %.1f%%   ", sht40_data.temp, sht40_data.humidity);
        lcd_draw_text(DISPLAY_BUFFER1, 10, 50, 16, 32, sht40_text);

        char max17048_text[40];
        snprintf(max17048_text, sizeof(max17048_text), "V: %.3f V, %%: %d%%", max17048_data.voltage, max17048_data.soc);
        lcd_draw_text(DISPLAY_BUFFER1, 10, 90, 16, 32, max17048_text);

        char mcp7940_text[40];
        snprintf(mcp7940_text, sizeof(mcp7940_text), "RTC: %02u:%02u:%02u   ", mcp7940_data.hours, mcp7940_data.minutes, mcp7940_data.seconds);
        lcd_draw_text(DISPLAY_BUFFER1, 10, 130, 16, 32, mcp7940_text);
        char mcp7940_date_text[40];
        snprintf(mcp7940_date_text, sizeof(mcp7940_date_text), "Date: %02u/%02u/20%02u", mcp7940_data.dayOfMonth, mcp7940_data.month, mcp7940_data.year);
        lcd_draw_text(DISPLAY_BUFFER1, 10, 170, 16, 32, mcp7940_date_text);
        mcp7940_data.seconds++;

        vTaskDelay(s_refresh_period / portTICK_PERIOD_MS);
    }
}

void pulsador_isr_handler_neg_callback(void *arg)
{
    gvui8_negative = (int)arg;
}

void pulsador_isr_handler_pos_callback(void *arg)
{
    gvui8_positive = (int)arg;
}

void app_main(void)
{
    /* GP IO devices (buttons, buzzer) init */
    // gpios_crear_tarea();
    init_hardware();

    /* Display init */
    lcd_main_task_create();

    /* I2C devices (Thermometer, RTC, Battery gauge) init */
    i2c_main_task_create(&sht40_data, &max17048_data, &mcp7940_data);

    // Example of setting date and time on the RTC
    // mcp7940_data_t new_datetime = {.seconds = 0, .minutes = 40, .hours = 16, .dayOfWeek = 2, .dayOfMonth = 2, .month = 12, .year = 25};
    // set_date_time(&new_datetime);

    // Wait one second to let I2C tasks to read initial data
    vTaskDelay(s_refresh_period / portTICK_PERIOD_MS);
    /* Start display task */
    xTaskCreate(&display_task, "display_task", 1024 * 2, NULL, 5, NULL);
}