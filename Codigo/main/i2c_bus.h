/*
I2C bus handling for SHT40 temperature and humidity sensor, MAX17048 battery gauge, and MCP7940N RTC.
*/
#ifndef I2C_BUS_H
#define I2C_BUS_H

typedef struct
{
    float temp;
    float humidity;
} sht40_data_t;

typedef struct
{
    float voltage;
    uint8_t soc;
} max17048_data_t;

void i2c_main_task_create(sht40_data_t *sht40_data, max17048_data_t *max17048_data);

#endif