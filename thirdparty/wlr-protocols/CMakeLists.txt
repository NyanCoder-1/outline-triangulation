cmake_minimum_required (VERSION 3.8)

add_library(wlr-protocols STATIC src/xdg-shell.c)

target_include_directories(wlr-protocols PUBLIC include)
