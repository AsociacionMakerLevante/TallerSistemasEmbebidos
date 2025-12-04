/*
LCD screen control
*/
#include <stdio.h>
#include "driver/gpio.h"
#include "lcd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "pinOut.h"
#include "fuentes.h"
#include "imagenes.h"
#include "lcd_fbuffer.h"

#define STACK_SIZE 8048 // task stack

enum pin_state
{
    OFF,
    ON
};

enum lcd_command
{
    VCOM = 0x00,
    CLEAR = 0x04,
    WRITE = 0x01
};

// Create stack RAM and task control block (TCB) for static tasks
static StackType_t lcd_task_stack[STACK_SIZE];
static StaticTask_t lcd_task_tcb;

// Declare handlers for static tasks in case we want to suspend them from other tasks.
TaskHandle_t lcdTaskHandle = NULL;

// SPI device handle for lcd
spi_device_handle_t lcd_spi_handle;

// Function assigned to the LCD task.
static void lcd_main_task(void *pvParameters);

// Function to toggle LCD EXTCON
static void lcd_extcon(enum pin_state estado);

// Functions for SPI transmission.
static void lcd_spi_init(void);
static void lcd_write_line(uint8_t linea, uint8_t *data);
static void lcd_clear_screen();

/************************************/

// Creates the task to control the LCD.
void lcd_main_task_create(void)
{
    lcdTaskHandle = xTaskCreateStatic(
        &lcd_main_task,       // Function assigned to the task.
        "LCD Main task",      // Name assigned to the task (debug).
        STACK_SIZE,           // Size of the task stack (statically reserved previously).
        NULL,                 // Parameters passed to the task at creation.
        tskIDLE_PRIORITY + 2, // Task priority.
        lcd_task_stack,       // Array for the task stack reserved previously.
        &lcd_task_tcb);       // Memory reserved previously for the task control block (TCB) of the task.

    printf("Task to control the LCD created.\n");
}

// Function assigned to the LCD task.
static void lcd_main_task(void *pvParameters)
{
    TickType_t xLastWakeTime;
    enum pin_state state = OFF;

    // Initialize xLastWakeTime with the current time.
    xLastWakeTime = xTaskGetTickCount();

    // Initialize the hardware connected to the LCD.
    lcd_spi_init();
    lcd_clear_screen();
    lcd_clear(DISPLAY_BUFFER1, WHITE);

    while (1)
    {
        // Toggle EXTCON
        state = !state;
        lcd_extcon(state);

        // Write buffer to LCD line by line. 2Hz refresh rate is OK for now
        for (int i = 0, j = 0; i < 240; i++)
        {
            lcd_write_line(i, &DISPLAY_BUFFER1[j]);
            j = j + 50;
        }

        // Block the task for 500 ms
        vTaskDelayUntil(&xLastWakeTime, (500 / portTICK_PERIOD_MS));
    }
}

// Function to toggle LCD EXTCON
static void lcd_extcon(enum pin_state estado)
{
    gpio_set_level(LCD_EXTCON, estado);
}

// SPI initialization.
static void lcd_spi_init(void)
{
    // Configure the LCD EXTCON pin as output.
    gpio_reset_pin(LCD_EXTCON);
    gpio_set_direction(LCD_EXTCON, GPIO_MODE_OUTPUT);
    gpio_pullup_dis(LCD_EXTCON);
    gpio_set_level(LCD_EXTCON, 0);

    // Initialize the SPI bus by calling spi_bus_initialize().
    // SPI bus configuration struct.
    spi_bus_config_t spiBusConfig = {
        .mosi_io_num = SPI_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        //.data_io_default_level = 0,
        .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE,
        .flags = SPI_DEVICE_POSITIVE_CS,
    };
    spi_bus_initialize(SPI2_HOST, &spiBusConfig, SPI_DMA_DISABLED);

    // Register the LCD device on the SPI bus by calling spi_bus_add_device();
    // SPI device interface configuration struct.
    spi_device_interface_config_t spiDeviceConfig =
        {
            .command_bits = 8,
            .address_bits = 8,
            .dummy_bits = 0,
            .mode = 0,
            .clock_source = SPI_CLK_SRC_DEFAULT,
            .duty_cycle_pos = 0,
            .cs_ena_pretrans = 0,
            .cs_ena_posttrans = 0,
            .clock_speed_hz = 2000000UL,
            .input_delay_ns = 0,
            .spics_io_num = SPI_CS,
            .flags = (SPI_DEVICE_POSITIVE_CS | SPI_DEVICE_TXBIT_LSBFIRST),
            .queue_size = 1,
            //.pre_cb = NULL,
            //.post_cb = NULL,
        };
    spi_bus_add_device(SPI2_HOST, &spiDeviceConfig, &lcd_spi_handle);

    // Set EXTCOM to 1
    gpio_set_level(LCD_EXTCON, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

// First line of the LCD 0x01, range from 0 to 239 //TODO Check this
static void lcd_write_line(uint8_t linea, uint8_t *data)
{
    enum lcd_command comando = WRITE;

    // To interact with the device, fill the spi_transaction_t structure.
    // Use functions spi_device_queue_trans() or spi_device_polling_transmit()
    // Write a line. The LCD receives the LSB first
    spi_transaction_t spi_envio_LCD =
        {
            //.flags = NULL,
            .cmd = comando,
            .addr = linea + 1, // First line of the LCD 0x01
            .length = 416,
            .rxlength = 0,
            .user = NULL,
            .tx_buffer = data, // do not use &data, but the content, the address to which data points.
            .rx_buffer = NULL,
        };
    spi_device_polling_transmit(lcd_spi_handle, &spi_envio_LCD);
}
static void lcd_clear_screen()
{
    // Clear screen.

    enum lcd_command comando = CLEAR;

    spi_transaction_t spi_envio_LCD_CS =
        {
            //.flags = NULL,
            .cmd = comando,
            .addr = 0b00000000,
            .length = 0,
            .rxlength = 0,
            .user = NULL,
            .tx_buffer = NULL,
            .rx_buffer = NULL,
        };
    spi_device_polling_transmit(lcd_spi_handle, &spi_envio_LCD_CS);
}
