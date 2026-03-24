idf_component_register(
    SRCS
        "main.c"
        "sharp_lcd.c"
        "sht4x.c"
    INCLUDE_DIRS
        "."       # cabeceras locales (sharp_lcd.h)
        ".."      # pinOut.h en la raíz del proyecto
    REQUIRES
        driver
        esp_timer
)
