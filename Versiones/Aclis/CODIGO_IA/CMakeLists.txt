cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(placamaker)

# Exponer el directorio raíz del proyecto para que LVGL encuentre lv_conf.h
idf_build_set_property(COMPILE_OPTIONS "-I${CMAKE_SOURCE_DIR}" APPEND)
idf_build_set_property(COMPILE_DEFINITIONS "LV_CONF_INCLUDE_SIMPLE" APPEND)
