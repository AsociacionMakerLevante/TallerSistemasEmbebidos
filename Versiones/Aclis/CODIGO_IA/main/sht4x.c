/**
 * @file sht4x.c
 * Driver para el sensor Sensirion SHT4x mediante la API I2C Master de ESP-IDF 5.x.
 */

#include "sht4x.h"
#include "pinOut.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "sht4x";

/* Posibles direcciones I2C del SHT4x */
static const uint8_t SHT4X_ADDRS[] = { 0x44, 0x45, 0x46 };

/* Comando: medición de alta precisión, sin calentador */
#define SHT4X_CMD_MEASURE_HPM  0xFD

/* Comando: soft reset */
#define SHT4X_CMD_SOFT_RESET   0x94

/* Tiempo de medición de alta precisión: 8.3 ms (max), 15 ms margen */
#define SHT4X_MEASURE_DELAY_MS 15

/* Handles I2C (módulo-estáticos) */
static i2c_master_bus_handle_t  i2c_bus  = NULL;
static i2c_master_dev_handle_t  i2c_dev  = NULL;

/* -----------------------------------------------------------------------
   CRC-8: polinomio 0x31, semilla 0xFF (estándar Sensirion)
   ----------------------------------------------------------------------- */
static uint8_t sht4x_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31u : (crc << 1);
        }
    }
    return crc;
}

/* -----------------------------------------------------------------------
   API pública
   ----------------------------------------------------------------------- */

esp_err_t sht4x_init(void)
{
    /* Bus I2C a 100 kHz (pull-ups internos ~45 kΩ; 400 kHz causa errores de CRC) */
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port            = I2C_NUM_0,
        .sda_io_num          = I2C_SDA,
        .scl_io_num          = I2C_SCL,
        .clk_source          = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt   = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &i2c_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error iniciando bus I2C: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Escanear las posibles direcciones del SHT4x: 0x44, 0x45, 0x46 */
    uint8_t found_addr = 0;
    for (int i = 0; i < (int)(sizeof(SHT4X_ADDRS)); i++) {
        esp_err_t probe = i2c_master_probe(i2c_bus, SHT4X_ADDRS[i], 50);
        ESP_LOGI(TAG, "Probe 0x%02X → %s", SHT4X_ADDRS[i],
                 (probe == ESP_OK) ? "ACK" : "NACK");
        if (probe == ESP_OK && found_addr == 0) {
            found_addr = SHT4X_ADDRS[i];
        }
    }

    if (found_addr == 0) {
        ESP_LOGE(TAG, "SHT4x no encontrado en ninguna dirección");
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "SHT4x encontrado en 0x%02X", found_addr);

    /* Añadir dispositivo a 100 kHz */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = found_addr,
        .scl_speed_hz    = 100000,
    };
    ret = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &i2c_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error añadiendo dispositivo I2C: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Soft reset para partir de estado conocido */
    uint8_t rst_cmd = SHT4X_CMD_SOFT_RESET;
    i2c_master_transmit(i2c_dev, &rst_cmd, 1, 50);
    vTaskDelay(pdMS_TO_TICKS(2)); /* t_SR max = 1 ms */

    /* Comprobación rápida: lanzar una medición y leer */
    float t, h;
    ret = sht4x_read(&t, &h);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SHT4x OK → T=%.1f°C  H=%.1f%%", t, h);
    } else {
        ESP_LOGW(TAG, "SHT4x primera lectura fallida en 0x%02X", found_addr);
    }
    return ret;
}

esp_err_t sht4x_read(float *temperature, float *humidity)
{
    if (!i2c_dev) {
        return ESP_ERR_INVALID_STATE;
    }

    /* 1. Enviar comando de medición */
    uint8_t cmd = SHT4X_CMD_MEASURE_HPM;
    esp_err_t ret = i2c_master_transmit(i2c_dev, &cmd, 1, 100 /*ms*/);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error enviando comando: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 2. Esperar a que el sensor complete la medición */
    vTaskDelay(pdMS_TO_TICKS(SHT4X_MEASURE_DELAY_MS));

    /* 3. Leer 6 bytes de resultado */
    uint8_t data[6];
    ret = i2c_master_receive(i2c_dev, data, sizeof(data), 100 /*ms*/);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error leyendo datos: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 4. Log raw bytes y verificar CRC */
    ESP_LOGI(TAG, "Raw: %02X %02X %02X  %02X %02X %02X",
             data[0], data[1], data[2], data[3], data[4], data[5]);

    if (sht4x_crc8(&data[0], 2) != data[2]) {
        ESP_LOGE(TAG, "CRC temperatura inválido (calc=0x%02X, recv=0x%02X)",
                 sht4x_crc8(&data[0], 2), data[2]);
        return ESP_ERR_INVALID_CRC;
    }
    if (sht4x_crc8(&data[3], 2) != data[5]) {
        ESP_LOGE(TAG, "CRC humedad inválido (calc=0x%02X, recv=0x%02X)",
                 sht4x_crc8(&data[3], 2), data[5]);
        return ESP_ERR_INVALID_CRC;
    }

    /* 5. Convertir a magnitudes físicas */
    uint16_t t_raw  = (uint16_t)(data[0] << 8) | data[1];
    uint16_t rh_raw = (uint16_t)(data[3] << 8) | data[4];

    float t  = -45.0f + 175.0f * (float)t_raw  / 65535.0f;
    float rh = -6.0f  + 125.0f * (float)rh_raw / 65535.0f;

    /* Limitar humedad al rango físico */
    if (rh < 0.0f)   rh = 0.0f;
    if (rh > 100.0f) rh = 100.0f;

    *temperature = t;
    *humidity    = rh;
    return ESP_OK;
}
