#!/bin/bash

# prepare folders
mkdir -p include src

# xdg-shell
wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml ./include/xdg-shell.h
wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml ./src/xdg-shell.c
