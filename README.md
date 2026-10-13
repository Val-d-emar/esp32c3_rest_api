<div align="center">

# ESP32-C3 Super Mini: REST API to TTY Bridge

[🇷🇺 Русский](#-русский) &nbsp;|&nbsp; [🇬🇧 English](#-english)

</div>

---

# 🇷🇺 Русский

Сетевой аппаратно-программный мост на базе микроконтроллера **ESP32-C3 Super Mini**.
Устройство подключается к Wi-Fi сети, поднимает конфигурируемый HTTP REST-сервер под управлением FreeRTOS/Arduino Core и транслирует входящие POST-пакеты из сети напрямую в консоль хоста через **Native USB CDC**, сопровождая приём физической индикацией на светодиоде.

---

## 1. Аппаратная спецификация и особенности платы

- **SoC:** Espressif ESP32-C3 (Архитектура RISC-V RV32IMC, 32-bit @ 160 МГц, кремний rev 4).
- **Память:** 4 МБ встроенной SPI Flash, ~306 КБ свободного ОЗУ (Heap).
- **Сетевой интерфейс:** Wi-Fi 2.4 ГГц (802.11 b/g/n, чувствительность до -90 dBm).
- **Интерфейс USB:** Встроенный аппаратный контроллер USB Serial/JTAG (GPIO18 `D-`, GPIO19 `D+`). Внешние мосты UART (CH340/CP2102) отсутствуют — кристалл регистрируется ядром Linux как `/dev/ttyACM*`.
- **Периферия:**
  - **GPIO8:** Синий светодиод (активный уровень — **`LOW`** / 0).
  - **GPIO9:** Кнопка **BOOT** (пользовательский вход с подтяжкой к 3.3V, замыкает на GND).
  - **RST:** Аппаратный сброс (CHIP_EN).

> ⚠️ **Инженерные особенности форм-фактора Super Mini:**
>
> 1. **Отсутствие схемы автосброса:** На плате нет транзисторной пары DTR/RTS. Для прошивки плату необходимо переводить в режим ROM Bootloader вручную комбинацией кнопок.
> 2. **Отсутствие экранирующего кожуха:** Кварцевый резонатор 40 МГц и шина SPI Flash открыты. Касание пальцем работающего чипа вносит емкость и наводку 50 Гц, что срывает генерацию кварца и вызывает зависание ядра со сбросом сторожевым таймером (`rst:0x7 TG0WDT_SYS_RST`). В готовых устройствах плату следует изолировать корпусом или компаундом.

---

## 2. Архитектура и безопасность секретов

Пароли и параметры сети изолированы от Git-репозитория:

```text
├── .env                 # Конфигурация сети и эндпоинта (в .gitignore)
├── .gitignore           # Исключает .env, .pio, .vscode
├── get_env.sh           # Bash-генератор определений препроцессора GCC
├── platformio.ini       # Конфигурация PlatformIO с динамическими флагами
└── src/
    └── main.cpp         # Прошивка с безопасной стрингификацией макросов
```

- Переменные хранятся в `.env` и подтягиваются сборщиком через bash-скрипт `get_env.sh` директивой `!./get_env.sh`.
- В `src/main.cpp` реализован макрос стрингификации (`#define STR(a) #a`), исключающий ошибки вычисления дефисов компилятором GCC (например, в SSID `DIR-815-8445`).
- Неблокирующий автомат состояний в `loop()` исключает панику Task Watchdog Timer (WDT) при долгом получении DHCP-аренды.

---

## 3. Настройка и запуск

### 1. Файл секретов (`.env`)

Создайте в корне проекта файл `.env`:

```env
WIFI_SSID="ИМЯ_ВАШЕЙ_СЕТИ"
WIFI_PASS="ПАРОЛЬ_ОТ_СЕТИ"
PORT=80
ENDPOINT=/api
```

Сделайте генератор исполняемым:

```bash
chmod +x get_env.sh
```

Содержимое `get_env.sh`:

```bash
#!/usr/bin/env bash
if [ -f .env ]; then
    set -a
    source .env
    set +a
    echo "-DWIFI_SSID=\\\"$WIFI_SSID\\\" -DWIFI_PASS=\\\"$WIFI_PASS\\\" -DPORT=\\\"$PORT\\\" -DENDPOINT=\\\"$ENDPOINT\\\""
fi
```

### 2. Конфигурация PlatformIO (`platformio.ini`)

```ini
[env:esp32-c3-devkitm-1]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200

build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
    !./get_env.sh

upload_port = /dev/ttyACM0
monitor_port = /dev/ttyACM0
```

### 3. Прошивка платы

1. Зажмите **BOOT**, кратковременно нажмите **RST**, отпустите **BOOT**.
2. В VSCode нажмите **PlatformIO: Upload** (`Ctrl + Alt + U`).
3. Нажмите кнопку **RST** на плате.
4. Откройте **PlatformIO: Serial Monitor** и дождитесь строки с выданным IP-адресом.

---

## 4. Тестирование через `curl` (без хардкода)

Задайте переменные окружения в консоли Linux (подставив реальный IP из Serial Monitor и значения из `.env`):

```bash
export ESP_IP="192.168.100.193"  # IP адрес, выданный плате роутером
export ESP_PORT="80"             # Значение PORT из .env
export ESP_EP="/api"             # Значение ENDPOINT из .env
```

### Проверка работоспособности (Healthcheck):

```bash
curl -i "http://${ESP_IP}:${ESP_PORT}/"
```

### Отправка строки в TTY терминал (POST):

```bash
curl -X POST -H "Content-Type: text/plain" \
     -d "Привет ESP32-C3 от радиоинженера из Debian 13!" \
     "http://${ESP_IP}:${ESP_PORT}${ESP_EP}"
```

**Вывод в Serial Monitor (`/dev/ttyACM0`):**

```text
==================================================
>>> [REST API] ПОЛУЧЕНО СООБЩЕНИЕ ИЗ СЕТИ:
>>> Длина: 80 байт
>>> Текст: Привет ESP32-C3 от радиоинженера из Debian 13!
==================================================
```

### Отправка JSON-пакета:

```bash
curl -X POST -H "Content-Type: application/json" \
     -d '{"sensor":"temperature","value":24.5}' \
     "http://${ESP_IP}:${ESP_PORT}${ESP_EP}"
```

---

<br>

# 🇬🇧 English

Hardware/software network bridge powered by the **ESP32-C3 Super Mini** MCU.
The device connects to a Wi-Fi access point, runs a configurable HTTP REST server on top of FreeRTOS/Arduino Core, and streams incoming network POST payloads directly into the host console via **Native USB CDC**, accompanied by physical LED feedback.

---

## 1. Hardware Specifications & Board Quirks

- **SoC:** Espressif ESP32-C3 (RISC-V RV32IMC architecture, 32-bit @ 160 MHz, silicon rev 4).
- **Memory:** 4 MB embedded SPI Flash, ~306 KB available Heap.
- **RF Transceiver:** 2.4 GHz Wi-Fi (802.11 b/g/n, sensitivity down to -90 dBm).
- **USB Interface:** Native USB Serial/JTAG hardware block (GPIO18 `D-`, GPIO19 `D+`). No external UART bridges (CH340/CP2102) — recognized directly by Linux kernel as `/dev/ttyACM*`.
- **Peripherals:**
  - **GPIO8:** Blue onboard LED (active **`LOW`** / 0).
  - **GPIO9:** **BOOT** button (weak pull-up to 3.3V, shorts to GND).
  - **RST:** Hardware reset line (CHIP_EN).

> ⚠️ **Super Mini Engineering Caveats:**
>
> 1. **No Auto-Reset Circuit:** The board lacks DTR/RTS transistor toggling. Flashing requires manual invocation of ROM Bootloader mode using tactile buttons.
> 2. **Unshielded PCB Layout:** The 40 MHz crystal oscillator and SPI Flash lines are exposed. Touching the chip injects stray body capacitance and 50 Hz hum, disrupting the oscillator and triggering an immediate watchdog reset (`rst:0x7 TG0WDT_SYS_RST`). Enclosure or conformal coating is mandatory for production use.

---

## 2. Architecture & Secret Management

Credentials and network parameters are kept out of Git tracking:

```text
├── .env                 # Wi-Fi credentials & endpoints (ignored by Git)
├── .gitignore           # Ignores .env, .pio, .vscode
├── get_env.sh           # Bash generator for GCC preprocessor defines
├── platformio.ini       # PlatformIO configuration using dynamic flags
└── src/
    └── main.cpp         # Firmware with C-macro stringification
```

- Variables are read from `.env` and injected into the GCC build step via `get_env.sh` triggered by `!./get_env.sh`.
- In `src/main.cpp`, a C stringification macro (`#define STR(a) #a`) ensures SSIDs containing hyphens (e.g. `DIR-815-8445`) are treated as string literals rather than arithmetic operations.
- A non-blocking state machine in `loop()` prevents FreeRTOS Task Watchdog Timer (WDT) trips during lengthy DHCP association sequences.

---

## 3. Setup and Flashing

### 1. Secret Configuration (`.env`)

Create a `.env` file in the project root:

```env
WIFI_SSID="YOUR_WIFI_SSID"
WIFI_PASS="YOUR_WIFI_PASSWORD"
PORT=80
ENDPOINT=/api
```

Make the script executable:

```bash
chmod +x get_env.sh
```

Content of `get_env.sh`:

```bash
#!/usr/bin/env bash
if [ -f .env ]; then
    set -a
    source .env
    set +a
    echo "-DWIFI_SSID=\\\"$WIFI_SSID\\\" -DWIFI_PASS=\\\"$WIFI_PASS\\\" -DPORT=\\\"$PORT\\\" -DENDPOINT=\\\"$ENDPOINT\\\""
fi
```

### 2. PlatformIO Configuration (`platformio.ini`)

```ini
[env:esp32-c3-devkitm-1]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200

build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
    !./get_env.sh

upload_port = /dev/ttyACM0
monitor_port = /dev/ttyACM0
```

### 3. Flashing Procedure

1. Hold **BOOT**, short-press **RST**, release **BOOT**.
2. Run **PlatformIO: Upload** in VSCode (`Ctrl + Alt + U`).
3. Press **RST** on the board.
4. Launch **PlatformIO: Serial Monitor** and note the assigned IP address.

---

## 4. Testing with `curl` (Parameterized)

Export environment variables in your terminal using the assigned IP and your `.env` values:

```bash
export ESP_IP="192.168.100.193"  # Replace with IP printed in Serial Monitor
export ESP_PORT="80"             # PORT defined in .env
export ESP_EP="/api"             # ENDPOINT defined in .env
```

### Healthcheck:

```bash
curl -i "http://${ESP_IP}:${ESP_PORT}/"
```

### Sending String to TTY Terminal (POST):

```bash
curl -X POST -H "Content-Type: text/plain" \
     -d "Hello ESP32-C3 from Debian 13!" \
     "http://${ESP_IP}:${ESP_PORT}${ESP_EP}"
```

**Serial Monitor Output (`/dev/ttyACM0`):**

```text
==================================================
>>> [REST API] ПОЛУЧЕНО СООБЩЕНИЕ ИЗ СЕТИ:
>>> Длина: 30 байт
>>> Текст: Hello ESP32-C3 from Debian 13!
==================================================
```

### Sending JSON Payload:

```bash
curl -X POST -H "Content-Type: application/json" \
     -d '{"sensor":"temperature","value":24.5}' \
     "http://${ESP_IP}:${ESP_PORT}${ESP_EP}"
```
