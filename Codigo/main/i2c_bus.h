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

typedef struct
{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t dayOfWeek;
    uint8_t dayOfMonth;
    uint8_t month;
    uint8_t year;
} mcp7940_data_t;

void i2c_main_task_create(sht40_data_t *sht40_data, max17048_data_t *max17048_data, mcp7940_data_t *mcp7940_data);
void set_date_time(mcp7940_data_t *new_datetime);

#endif