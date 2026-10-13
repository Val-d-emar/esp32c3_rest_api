#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#define _STR(a) #a
#define STR(a) _STR(a)

#ifndef WIFI_SSID
#define WIFI_SSID UNKNOWN
#endif
#ifndef WIFI_PASS
#define WIFI_PASS UNKNOWN
#endif
#ifndef PORT
#define PORT 80
#endif
#ifndef ENDPOINT
#define ENDPOINT /api
#endif

static const char* ssid = STR(WIFI_SSID);
static const char* pass = STR(WIFI_PASS);
static const char* endpoint = STR(ENDPOINT);
static const char* port = STR(PORT);

constexpr uint8_t LED_PIN = 8;

WebServer server(std::stoi(port));
bool server_started = false;

// GET / — проверка жизни
void handleRoot() {
    String resp = "ESP32-C3 Super Mini REST API\n";
    resp += "Uptime: " + String(millis() / 1000) + " s\n";
    resp += "Free Heap: " + String(ESP.getFreeHeap()) + " bytes\n";
    server.send(200, "text/plain; charset=utf-8", resp);
}

// POST /api/message — прием строки и вывод в TTY
void handlePostMessage() {
    String body = "";

    // 1. Проверяем сырое тело (text/plain, application/json и т.д.)
    if (server.hasArg("plain")) {
        body = server.arg("plain");
    } 
    // 2. Если curl прислал x-www-form-urlencoded со знаком '='
    else if (server.args() > 0) {
        body = (server.arg(0).length() > 0) ? server.arg(0) : server.argName(0);
    }

    if (body.length() == 0) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Empty body\"}\n");
        return;
    }

    // Выводим принятую через сеть строку прямо в TTY терминал
    Serial.println("\n==================================================");
    Serial.println(">>> [REST API] ПОЛУЧЕНО СООБЩЕНИЕ ИЗ СЕТИ:");
    Serial.printf(">>> Длина: %u байт\n", body.length());
    Serial.printf(">>> Текст: %s\n", body.c_str());
    Serial.println("==================================================");

    // Подтверждаем приём физическим светодиодом на плате (моргаем)
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);

    // Возвращаем клиенту успешный JSON ответ
    String json_reply = "{\"status\":\"ok\",\"received_bytes\":" + String(body.length()) + "}\n";
    server.send(200, "application/json", json_reply);
}

void handleNotFound() {
    server.send(404, "text/plain", "404 Not Found\n");
}

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // гасим LED

    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
}

void loop() {
    // 1. Если Wi-Fi подключен
    if (WiFi.status() == WL_CONNECTED) {
        // Стартуем сервер один раз при появлении сети
        if (!server_started) {
            digitalWrite(LED_PIN, LOW); // Светодиод горит постоянно

            Serial.println("\n------------------------------------------");
            Serial.printf("[Wi-Fi] Подключено к [%s]!\n", ssid);
            Serial.printf("[Wi-Fi] IP платы: %s\n", WiFi.localIP().toString().c_str());

            server.on("/", HTTP_GET, handleRoot);
            server.on(endpoint, HTTP_POST, handlePostMessage);
            server.onNotFound(handleNotFound);

            server.begin();
            server_started = true;

            Serial.printf("[HTTP] REST сервер слушает порт %s!\n", port);
            Serial.printf("[HTTP] Готов принимать POST %s через curl\n", endpoint);
            Serial.println("------------------------------------------");
        }

        // Обслуживаем HTTP клиентов
        server.handleClient();
        delay(2); // Критично: отдаем 2 мс планировщику задач RTOS
        return;
    }

    // 2. Если Wi-Fi еще в процессе подключения
    server_started = false;
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    Serial.print(".");
    delay(1900);
}
