#include <Arduino.h>
#include "driver/twai.h"
#include "pin_config.h"
#include <FastLED.h>

// Интервалы
#define POLLING_RATE_MS 100

// LED
#define NUM_LEDS 1
#define DATA_PIN WS2812B_DATA
CRGB leds[NUM_LEDS];

// Буфер для Serial
#define RXBUF_LEN 100
const char SEPARATOR = ',';
const char TERMINATOR = '\n';

size_t CycleTime = 0;
bool firstMessage = true;
unsigned long startTime = 0;

// Инициализация CAN
void CAN_Drive_Initialization() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        Serial.println("Driver install failed");
        return;
    }
    if (twai_start() != ESP_OK) {
        Serial.println("Driver start failed");
        return;
    }

    uint32_t alerts_to_enable = TWAI_ALERT_RX_DATA | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_QUEUE_FULL;
    twai_reconfigure_alerts(alerts_to_enable, NULL);

    // Cycle for Blinking Green Light
    for (int i = 0; i < 5; i++) {
        leds[0] = CRGB::Green;
        FastLED.show();
        delay(20);
        leds[0] = CRGB::Black;
        FastLED.show();
        delay(20);
    }
}

// Утилита: вывод пакета в формате GUI
void printPacket(twai_message_t *msg) {
    Serial.printf("%03X,%02X,%02X,", msg->identifier, msg->rtr, msg->extd);
    for (int i = 0; i < msg->data_length_code; i++) {
        Serial.printf("%02X", msg->data[i]);
    }
    Serial.println();
}

// Утилита: конвертация ASCII HEX в байт
uint8_t hexPairToByte(char high, char low) {
    auto hex = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return 0;
    };
    return (hex(high) << 4) | hex(low);
}

// Логирование результата отправки CAN сообщения
void logTransmitResult(esp_err_t res) {
    if (res == ESP_OK) {
        //Serial.println("CAN TX OK");
        leds[0] = CRGB::Blue;
        FastLED.show();
        delay(100);
        leds[0] = CRGB::Black;
        FastLED.show();
    } else {
        Serial.printf("CAN TX FAIL, err = %d\n", res);
        leds[0] = CRGB::Red;
        FastLED.show();
        delay(100);
        leds[0] = CRGB::Black;
        FastLED.show();
    }
}

// Обновлённый parseAndSend с логом
void parseAndSend(char *line) {
    twai_message_t msg = {0};

    //Serial.print("Received line: ");
    // RECEIVED LINE WORKING TO SEND
    Serial.println(line);

    // ID
    char *token = strtok(line, ",");
    if (!token) return;
    msg.identifier = strtol(token, NULL, 16);

    // RTR
    token = strtok(NULL, ",");
    if (!token) return;
    msg.rtr = strtol(token, NULL, 16);

    // IDE
    token = strtok(NULL, ",");
    if (!token) return;
    msg.extd = strtol(token, NULL, 16);

    // DATA
    token = strtok(NULL, "\n");
    if (!token) return;
    size_t len = strlen(token);
    msg.data_length_code = len / 2;
    for (int i = 0; i < msg.data_length_code && i < 8; i++) {
        msg.data[i] = hexPairToByte(token[i * 2], token[i * 2 + 1]);
    }

    esp_err_t res = twai_transmit(&msg, pdMS_TO_TICKS(100));
    logTransmitResult(res);
}

// Сниффинг
void sniffCAN() {
    twai_message_t rx_msg;
    while (twai_receive(&rx_msg, 0) == ESP_OK) {
        printPacket(&rx_msg);
        //leds[0] = CRGB::White;
        //FastLED.show();
        //delay(20);
        //leds[0] = CRGB::Black;
        //FastLED.show();
    }
}

// Обновлённый serialHandler без лишних println
void serialHandler() {
    static char rxBuf[RXBUF_LEN];
    static int rxPtr = 0;

    while (Serial.available()) {
        char c = Serial.read();
        if (rxPtr < RXBUF_LEN - 1) {
            rxBuf[rxPtr++] = c;
        }

        if (c == TERMINATOR) {
            rxBuf[rxPtr - 1] = '\0'; // заменить \n на \0
            parseAndSend(rxBuf);
            rxPtr = 0;
        }
    }
}

// SETUP
void setup() {
    Serial.begin(115200);
    delay(100);

    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(20);

    pinMode(ME2107_EN, OUTPUT);
    digitalWrite(ME2107_EN, HIGH);

    pinMode(CAN_SPEED_MODE, OUTPUT);
    digitalWrite(CAN_SPEED_MODE, LOW);

    CAN_Drive_Initialization();
    Serial.println("CAN TWAI Ready.");
}

// Можно добавить функцию для печати статуса CAN драйвера, вызывать её например раз в секунду
void printCanStatus() {
    twai_status_info_t status;
    twai_get_status_info(&status);
    //Serial.printf("CAN Status: RX missed = %d, Bus errors = %d\n",
      //            status.rx_missed_count, status.bus_error_count);
}

// LOOP
void loop() {
    serialHandler();
    sniffCAN();

    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 1000) {
        printCanStatus();
        lastStatusPrint = millis();
    }
}
