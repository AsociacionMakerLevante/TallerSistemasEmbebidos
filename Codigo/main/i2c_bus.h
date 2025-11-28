/*
I2C bus handling for SHT40 temperature and humidity sensor, MAX17048 battery gauge, and MCP7940N RTC.
*/
#ifndef I2C_BUS_H
#define I2C_BUS_H

typedef struct
{
    float temp;
    float humidity;
} sht_40_data_t;

void i2c_main_task_create(sht_40_data_t *sht40_data);

#endif