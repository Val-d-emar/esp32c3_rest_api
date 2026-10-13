#!/usr/bin/env bash
# Читаем переменные из .env и отдаем их в stdout для GCC
if [ -f .env ]; then
    # Экспортируем только не закомментированные строки
    export $(grep -v '^#' .env | xargs)
    echo "-DWIFI_SSID=\"$WIFI_SSID\" -DWIFI_PASS=\"$WIFI_PASS\""
fi
