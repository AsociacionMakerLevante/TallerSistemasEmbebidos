/**
 * @file sht4x.h
 * Driver mínimo para el sensor de temperatura y humedad Sensirion SHT4x.
 *
 * Conexión (ver pinOut.h):
 *   I2C_SCL → GPIO 0
 *   I2C_SDA → GPIO 1
 *
 * Dirección I2C por defecto: 0x44
 * Protocolo:
 *   1. Enviar comando 0xFD (medición de alta precisión)
 *   2. Esperar ≥ 9 ms
 *   3. Leer 6 bytes: [T_MSB, T_LSB, T_CRC, RH_MSB, RH_LSB, RH_CRC]
 *   4. Verificar CRC-8 (polinomio 0x31, semilla 0xFF)
 *   5. Convertir:
 *        T  = -45 + 175 * T_raw  / 65535
 *        RH = -6  + 125 * RH_raw / 65535  (limitado 0-100 %)
 */

#ifndef SHT4X_H
#define SHT4X_H

#include "esp_err.h"

/**
 * @brief Inicializa el bus I2C y añade el dispositivo SHT4x.
 * @return ESP_OK si el sensor responde correctamente.
 */
esp_err_t sht4x_init(void);

/**
 * @brief Lee temperatura y humedad relativa.
 * @param[out] temperature  Temperatura en °C.
 * @param[out] humidity     Humedad relativa en %.
 * @return ESP_OK si la lectura y los CRC son correctos.
 */
esp_err_t sht4x_read(float *temperature, float *humidity);

#endif /* SHT4X_H */
