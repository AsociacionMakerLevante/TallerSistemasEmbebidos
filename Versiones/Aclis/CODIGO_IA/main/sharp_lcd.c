/**
 * @file sharp_lcd.c
 * Driver para Sharp Memory LCD LS027B7DH01 (400×240, 1 bpp)
 *
 * Protocolo SPI de la pantalla:
 *  - CPOL=0, CPHA=0 (Modo SPI 0), reloj máx. 2 MHz
 *  - Bits enviados LSB primero (M0 es el primer bit en el bus)
 *  - SCS activo en ALTO (gestionado manualmente como GPIO)
 *  - Formato de trama "Write Lines":
 *      [CMD=0x01][dir_linea][50 bytes de píxeles][dummy 0x00]  × N líneas
 *      [0x00][0x00]  (fin de trama)
 *  - Las líneas se numeran de 1 a 240
 *  - Bit 0 de cada byte de píxel = píxel más a la izquierda
 *    1 = blanco, 0 = negro
 *
 * EXTCOMIN:
 *  - Debe oscilar entre 1 y 20 Hz (EXTMODE conectado a VDD en el HW)
 *  - Se usa un timer de esp_timer para alternar el pin cada 250 ms → 2 Hz
 */

#include "sharp_lcd.h"
#include "pinOut.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include <string.h>

/* -----------------------------------------------------------------------
   Constantes del protocolo
   ----------------------------------------------------------------------- */

static const char *TAG = "sharp_lcd";

#define LCD_ROW_BYTES    (LCD_WIDTH / 8)   /* 50 bytes por fila */

/* Comandos (bits enviados LSB primero) */
#define CMD_WRITE_LINE   0x01U   /* M0=1 → actualizar líneas */
#define CMD_ALL_CLEAR    0x04U   /* M2=1 → borrar toda la pantalla */

/* Tamaño máximo del buffer de transmisión DMA:
   1 byte cmd + 240 × (1 addr + 50 datos + 1 dummy) + 2 bytes fin */
#define TX_BUF_SIZE  (1 + LCD_HEIGHT * (1 + LCD_ROW_BYTES + 1) + 2)

/* -----------------------------------------------------------------------
   Variables del módulo
   ----------------------------------------------------------------------- */

/** Framebuffer interno: 240 × 50 bytes = 12 000 bytes */
static uint8_t framebuffer[LCD_HEIGHT][LCD_ROW_BYTES];

/** Buffer de transmisión SPI en memoria DMA */
static DMA_ATTR uint8_t tx_buf[TX_BUF_SIZE];

static spi_device_handle_t spi_dev;

/* -----------------------------------------------------------------------
   Callbacks y funciones internas
   ----------------------------------------------------------------------- */

/** Alterna el pin EXTCOMIN cada llamada → señal cuadrada de 2 Hz */
static void extcomin_timer_cb(void *arg)
{
    static int level = 0;
    level ^= 1;
    gpio_set_level(LCD_EXTCON, level);
}

/** Envía un bloque de datos por SPI (bloqueante) */
static void spi_write(const uint8_t *data, size_t len)
{
    spi_transaction_t t = {
        .length    = len * 8,   /* longitud en bits */
        .tx_buffer = data,
        .flags     = 0,
    };
    ESP_ERROR_CHECK(spi_device_transmit(spi_dev, &t));
}

/**
 * Construye la trama SPI para las filas [y1, y2] y la envía de una sola vez.
 * Mantiene SCS en ALTO durante toda la transacción.
 */
static void lcd_send_lines(int32_t y1, int32_t y2)
{
    int32_t n_lines = y2 - y1 + 1;
    size_t  buf_len = 1u + (size_t)n_lines * (1u + LCD_ROW_BYTES + 1u) + 2u;

    /* Construir trama en tx_buf */
    size_t idx = 0;
    tx_buf[idx++] = CMD_WRITE_LINE;

    for (int32_t y = y1; y <= y2; y++) {
        tx_buf[idx++] = (uint8_t)(y + 1);              /* dirección 1-indexed */
        memcpy(&tx_buf[idx], framebuffer[y], LCD_ROW_BYTES);
        idx += LCD_ROW_BYTES;
        tx_buf[idx++] = 0x00;                           /* dummy de fin de línea */
    }
    tx_buf[idx++] = 0x00;   /* 16 bits dummy de fin de trama */
    tx_buf[idx++] = 0x00;

    /* SCS activo en ALTO durante toda la transacción */
    gpio_set_level(SPI_CS, 1);
    spi_write(tx_buf, buf_len);
    gpio_set_level(SPI_CS, 0);
}

/* -----------------------------------------------------------------------
   API pública
   ----------------------------------------------------------------------- */

void sharp_lcd_init(void)
{
    /* --- GPIO: SCS (activo HIGH) --- */
    gpio_reset_pin(SPI_CS);
    gpio_set_direction(SPI_CS, GPIO_MODE_OUTPUT);
    gpio_set_level(SPI_CS, 0);

    /* --- GPIO: EXTCOMIN --- */
    gpio_reset_pin(LCD_EXTCON);
    gpio_set_direction(LCD_EXTCON, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_EXTCON, 0);

    /* --- Bus SPI --- */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = SPI_MOSI,
        .miso_io_num     = -1,
        .sclk_io_num     = SPI_CLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = TX_BUF_SIZE,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    /* --- Dispositivo SPI:
           - 2 MHz (máximo según datasheet)
           - Modo 0 (CPOL=0, CPHA=0)
           - CS gestionado manualmente → spics_io_num = -1
           - LSB primero (M0 es el primer bit transmitido)
       --- */
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 2 * 1000 * 1000,
        .mode           = 0,
        .spics_io_num   = -1,
        .queue_size     = 1,
        .flags          = SPI_DEVICE_BIT_LSBFIRST,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_dev));

    /* --- Framebuffer: todo blanco (0xFF) --- */
    memset(framebuffer, 0xFF, sizeof(framebuffer));

    /* --- Limpiar la pantalla con el comando All Clear --- */
    sharp_lcd_clear();

    /* --- Timer EXTCOMIN: alterna cada 250 ms → señal cuadrada de 2 Hz --- */
    esp_timer_handle_t extcomin_timer;
    const esp_timer_create_args_t timer_args = {
        .callback = extcomin_timer_cb,
        .name     = "extcomin",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &extcomin_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(extcomin_timer, 250000)); /* 250 ms */

    ESP_LOGI(TAG, "Pantalla Sharp LS027B7DH01 inicializada (%dx%d)",
             LCD_WIDTH, LCD_HEIGHT);
}

void sharp_lcd_clear(void)
{
    /* Comando All Clear: SCS alto, enviar 0x04 + dummy, SCS bajo */
    uint8_t clear_cmd[2] = {CMD_ALL_CLEAR, 0x00};
    gpio_set_level(SPI_CS, 1);
    spi_write(clear_cmd, sizeof(clear_cmd));
    gpio_set_level(SPI_CS, 0);

    /* Sincronizar framebuffer */
    memset(framebuffer, 0xFF, sizeof(framebuffer));
}

void sharp_lcd_flush(int32_t x1, int32_t y1,
                     int32_t x2, int32_t y2,
                     const uint8_t *px_map)
{
    const int32_t   area_w = x2 - x1 + 1;
    const uint16_t *buf    = (const uint16_t *)px_map; /* RGB565 */

    /* Convertir cada píxel RGB565 → 1 bit y guardar en la fila física invertida.
     * La pantalla está montada con la línea 240 arriba, por eso la fila LVGL y=0
     * va a la fila física LCD_HEIGHT-1 (dirección 240) y viceversa. */
    for (int32_t y = y1; y <= y2; y++) {
        int32_t phy_y = (LCD_HEIGHT - 1) - y;   /* fila física correspondiente */

        for (int32_t x = x1; x <= x2; x++) {

            uint16_t color = buf[(y - y1) * area_w + (x - x1)];

            /* Extraer componentes RGB565 y expandir a 8 bits */
            uint8_t r = (color >> 11) & 0x1Fu;
            uint8_t g = (color >>  5) & 0x3Fu;
            uint8_t b =  color        & 0x1Fu;

            uint8_t r8 = (uint8_t)((r << 3) | (r >> 2));
            uint8_t g8 = (uint8_t)((g << 2) | (g >> 4));
            uint8_t b8 = (uint8_t)((b << 3) | (b >> 2));

            /* Luminancia BT.601 */
            uint8_t lum = (uint8_t)((77u * r8 + 150u * g8 + 29u * b8) >> 8);

            /* Invertir X: la pantalla está montada con el eje X invertido */
            int32_t phy_x = (LCD_WIDTH - 1) - x;
            int byte_idx = phy_x / 8;
            int bit_idx  = phy_x % 8;

            if (lum >= 128u) {
                framebuffer[phy_y][byte_idx] |=  (1u << bit_idx); /* blanco */
            } else {
                framebuffer[phy_y][byte_idx] &= ~(1u << bit_idx); /* negro  */
            }
        }
    }

    /* Enviar las filas físicas afectadas (rango invertido, direcciones crecientes) */
    int32_t phy_y1 = (LCD_HEIGHT - 1) - y2;
    int32_t phy_y2 = (LCD_HEIGHT - 1) - y1;
    lcd_send_lines(phy_y1, phy_y2);
}
