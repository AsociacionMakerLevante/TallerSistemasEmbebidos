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
#include "i2c_bus.h"

void app_main(void)
{
    gpios_crear_tarea();
    lcd_main_task_create();
    i2c_crear_tarea_i2c();
}