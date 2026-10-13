#include <Arduino.h>
#include <WiFi.h>

// Макрос стрингификации: превращает любые токены GCC в C-строку "..."
#define _STR(a) #a
#define STR(a) _STR(a)

#ifndef WIFI_SSID
#define WIFI_SSID UNKNOWN
#endif
#ifndef WIFI_PASS
#define WIFI_PASS UNKNOWN
#endif

// Превращаем макросы из флагов сборки в настоящие C-строки
static const char* ssid = STR(WIFI_SSID);
static const char* pass = STR(WIFI_PASS);

constexpr uint8_t LED_PIN = 8;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
}

void loop() {
    // Если подключились — светодиод горит, выводим IP
    if (WiFi.status() == WL_CONNECTED) {
        digitalWrite(LED_PIN, LOW); // Горит
        Serial.printf("[УСПЕХ] Подключено к [%s]! IP: %s | RSSI: %d dBm\n",
                      ssid,
                      WiFi.localIP().toString().c_str(),
                      WiFi.RSSI());
        delay(5000);
        return;
    }

    // Пока подключаемся — коротко мигаем и выводим статус
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);

    Serial.printf("[ПОДКЛЮЧЕНИЕ...] SSID: [%s] (символов: %d) | Статус кода: %d\n",
                  ssid,
                  strlen(ssid),
                  WiFi.status());

    delay(2000);
}
