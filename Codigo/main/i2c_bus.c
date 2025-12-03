/*
Funciones para controlar el bus I2C
https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32c3/api-reference/peripherals/i2c.html
https://github.com/jefflongo/max17048/tree/master
*/
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "i2c_bus.h"
#include "pinOut.h"

static const char *TAG = "i2c_bus.c";

#define STACK_SIZE 8048 // Stack size for the tasks.
// SHT40
#define I2C_TX_BYTES 1
#define I2C_RX_BYTES 6
#define CMD_SHT40_READ 0xFD
// MAX17048
#define MAX17040_VCELL 0x02
#define MAX17040_SOC 0x04

// Reserve RAM for the stack and task control block (TCB) for the statically created tasks
static StackType_t stack_tarea_i2c[STACK_SIZE];
static StaticTask_t TCB_tarea_i2c;
// Declare handlers for static tasks in case we want to suspend them from other tasks.
TaskHandle_t xHandle_i2c_crear_tarea_i2c = NULL;

// Function assigned to the i2c task
static void i2c_task(void *pvParameters);

static sht40_data_t *sht40Data;
static max17048_data_t *max17048Data;
static mcp7940_data_t *mcp7940Data;

i2c_master_dev_handle_t mcp7940n_handle;

/************** Definición de las funciones ****************************/

// Function to create the task that tests the I2C bus
void i2c_main_task_create(sht40_data_t *sht40_data, max17048_data_t *max17048_data, mcp7940_data_t *mcp7940_data)
{
    // Assign the passed pointers to the global variables
    sht40Data = sht40_data;
    max17048Data = max17048_data;
    mcp7940Data = mcp7940_data;
    xHandle_i2c_crear_tarea_i2c = xTaskCreateStatic(
        &i2c_task,            // Fuction assigned to the task.
        "i2c_task",           // Task name for debugging.
        STACK_SIZE,           // stack size.
        NULL,                 // Parameters passed to the task during its creation.
        tskIDLE_PRIORITY + 1, // Task priority.
        stack_tarea_i2c,      // Array for the task stack previously reserved.
        &TCB_tarea_i2c);      // Memory previously reserved for the task control block (TCB) of the task.
    printf("Task to test the I2C bus created.\n");
}

static void i2c_task(void *pvParameters)
{
    // Configure ESP as master.
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = 0,
        .scl_io_num = I2C_SCL,
        .sda_io_num = I2C_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    // Add SHT40 to the I2C bus
    i2c_device_config_t sht40_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x45,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t sht40_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &sht40_cfg, &sht40_handle));

    // Add MAX17048 to the I2C bus
    i2c_device_config_t max17048_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x36,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t max17048_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &max17048_cfg, &max17048_handle));

    // Add MCP7940N to the I2C bus
    i2c_device_config_t mcp7940n_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x6F,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &mcp7940n_cfg, &mcp7940n_handle));

    // SHT40
    uint8_t data_wr[1];
    uint8_t data_rd[6];
    int sht40_temperature = 0;
    int sht40_humidity = 0;

    // MAX17048
    uint8_t max_data_wr[1];
    uint8_t max_data_rd[2];

    // MCP7940N
    uint8_t mcp_data_wr[2] = {0, 0};
    uint8_t mcp_data_rd[3] = {0, 0, 0};
    uint8_t bcd_seconds = 0, bcd_minutes = 0, bcd_hours = 0;
    uint8_t bcd_year = 0, bcd_month = 0, bcd_dayOfMonth = 0, bcd_dayOfWeek = 0;

    while (1)
    {
        // SHT40
        data_wr[0] = CMD_SHT40_READ;
        ESP_ERROR_CHECK(i2c_master_transmit(sht40_handle, data_wr, I2C_TX_BYTES, -1));
        vTaskDelay(20 / portTICK_PERIOD_MS);
        i2c_master_receive(sht40_handle, data_rd, I2C_RX_BYTES, -1);
        // Temperature and humidity value calculation.
        sht40_temperature = (data_rd[0] * 256L) + data_rd[1];
        sht40Data->temp = -45 + 175 * (sht40_temperature / 65535.0f);
        sht40_humidity = (data_rd[3] * 256L) + data_rd[4];
        sht40Data->humidity = -6 + 125 * (sht40_humidity / 65535.0f);
        // Print temperature and humidity values
        ESP_LOGI(TAG, "SHT40 Reading: Temperature %.1f.\n", sht40Data->temp);
        ESP_LOGI(TAG, "SHT40 Reading: Humidity %.1f.\n", sht40Data->humidity);

        // MAX17048
        max_data_wr[0] = MAX17040_SOC; // SOC value register
        ESP_ERROR_CHECK(i2c_master_transmit_receive(max17048_handle, max_data_wr, sizeof(max_data_wr), max_data_rd, 2, -1));
        // SOC value calculation.
        max17048Data->soc = max_data_rd[0];
        ESP_LOGI(TAG, "MAX17040 Reading: Battery percentage %u %%.\n", max17048Data->soc);
        max_data_wr[0] = MAX17040_VCELL; // VCELL value register
        ESP_ERROR_CHECK(i2c_master_transmit_receive(max17048_handle, max_data_wr, sizeof(max_data_wr), max_data_rd, 2, -1));
        // VCELL value calculation.
        max17048Data->voltage = ((max_data_rd[0] * 256L + max_data_rd[1]) * 78.125f) / 1000000;
        ESP_LOGI(TAG, "MAX17040 Reading: Battery voltage %.3f V.\n", max17048Data->voltage);

        // MCP7940N
        // Write register 0x00 to subsequently read from that address.
        mcp_data_wr[0] = 0x00;
        ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 1, -1));
        i2c_master_receive(mcp7940n_handle, mcp_data_rd, 7, -1);

        bcd_seconds = mcp_data_rd[0];
        bcd_minutes = mcp_data_rd[1];
        bcd_hours = mcp_data_rd[2];
        bcd_dayOfWeek = mcp_data_rd[3];
        bcd_dayOfMonth = mcp_data_rd[4];
        bcd_month = mcp_data_rd[5];
        bcd_year = mcp_data_rd[6];

        // BCD to Decimal conversion
        mcp7940Data->seconds = (((bcd_seconds >> 4) * 10) + (bcd_seconds & 0x0F)) - 80;
        mcp7940Data->minutes = ((bcd_minutes >> 4) * 10) + (bcd_minutes & 0x0F);
        mcp7940Data->hours = ((bcd_hours >> 4) * 10) + (bcd_hours & 0x0F);
        mcp7940Data->dayOfWeek = bcd_dayOfWeek & 0x07;
        mcp7940Data->dayOfMonth = ((bcd_dayOfMonth >> 4) * 10) + (bcd_dayOfMonth & 0x0F);
        mcp7940Data->month = ((bcd_month >> 4) * 10) + (bcd_month & 0x0F);
        mcp7940Data->year = ((bcd_year >> 4) * 10) + (bcd_year & 0x0F);

        printf("READ bcd_year: 0x%02X, bcd_month: 0x%02X, bcd_dayOfMonth: 0x%02X, bcd_dayOfWeek: 0x%02X, bcd_hours: 0x%02X, bcd_minutes: 0x%02X\n, bcd_seconds: 0x%02X",
               bcd_year, bcd_month, bcd_dayOfMonth, bcd_dayOfWeek, bcd_hours, bcd_minutes, bcd_seconds);
        ESP_LOGI(TAG, " MCP7940N reading: Time %02u:%02u:%02u.\n\n", mcp7940Data->hours, mcp7940Data->minutes, mcp7940Data->seconds);
        ESP_LOGI(TAG, "MCP7940N Reading: Date %02u/%02u/%02u.\n", mcp7940Data->dayOfMonth, mcp7940Data->month, mcp7940Data->year);

        // Blocking delay unitil next minute start
        vTaskDelay((60000 - (mcp7940Data->seconds * 1000)) / portTICK_PERIOD_MS);
    }
}

void set_date_time(mcp7940_data_t *new_datetime)
{
    // This function can be implemented to set the date and time on the MCP7940N RTC.
    // It would involve writing the new date and time values to the appropriate registers
    // on the RTC via I2C.
    uint8_t mcp_data_wr[2] = {0, 0};
    uint8_t bcd_minutes = 0, bcd_hours = 0;
    uint8_t bcd_year = 0, bcd_month = 0, bcd_dayOfMonth = 0, bcd_dayOfWeek = 0;

    bcd_year = ((new_datetime->year / 10) << 4) | (new_datetime->year % 10);
    bcd_month = ((new_datetime->month / 10) << 4) | (new_datetime->month % 10);
    bcd_dayOfMonth = ((new_datetime->dayOfMonth / 10) << 4) | (new_datetime->dayOfMonth % 10);
    bcd_dayOfWeek = (new_datetime->dayOfWeek & 0x07) | 0x08; // Ensure VBATEN is set for enabling battery backup
    bcd_hours = ((new_datetime->hours / 10) << 4) | (new_datetime->hours % 10);
    bcd_minutes = ((new_datetime->minutes / 10) << 4) | (new_datetime->minutes % 10);

    // printf("WRITTEN bcd_year: 0x%02X, bcd_month: 0x%02X, bcd_dayOfMonth: 0x%02X, bcd_dayOfWeek: 0x%02X, bcd_hours: 0x%02X, bcd_minutes: 0x%02X\n",
    //        bcd_year, bcd_month, bcd_dayOfMonth, bcd_dayOfWeek, bcd_hours, bcd_minutes);

    // Disable the oscillator to set time
    mcp_data_wr[0] = 0x00; // Write on register 0x00
    mcp_data_wr[1] = 0b00000000;
    ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 2, -1));

    mcp_data_wr[0] = 0x01; // Write on register 0x01
    mcp_data_wr[1] = bcd_minutes;
    ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 2, -1));
    mcp_data_wr[0] = 0x02; // Write on register 0x02
    mcp_data_wr[1] = bcd_hours;
    ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 2, -1));
    mcp_data_wr[0] = 0x03; // Write on register 0x03
    mcp_data_wr[1] = bcd_dayOfWeek;
    ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 2, -1));
    mcp_data_wr[0] = 0x04; // Write on register 0x04
    mcp_data_wr[1] = bcd_dayOfMonth;
    ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 2, -1));
    mcp_data_wr[0] = 0x05; // Write on register 0x05
    mcp_data_wr[1] = bcd_month;
    ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 2, -1));
    mcp_data_wr[0] = 0x06; // Write on register 0x06
    mcp_data_wr[1] = bcd_year;
    ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 2, -1));
    // Reset seconds and enable the oscillator
    mcp_data_wr[0] = 0x00; // Write on register 0x00
    mcp_data_wr[1] = 0b10000000;
    ESP_ERROR_CHECK(i2c_master_transmit(mcp7940n_handle, mcp_data_wr, 2, -1));
}
