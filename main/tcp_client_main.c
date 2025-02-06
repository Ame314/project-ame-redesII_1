#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "lwip/sockets.h"

#define WIFI_SSID "Casa_GROB_ROSERO"  // Cambia por tu red WiFi
#define WIFI_PASS "laClave;-)"        // Cambia por la contraseña de tu WiFi
#define SERVER_IP "192.168.1.23"      // Dirección IP del servidor
#define SERVER_PORT 12345             // Puerto del servidor
#define LED_GPIO GPIO_NUM_2           // GPIO donde está conectado el LED

static const char *TAG = "ESP32_TCP_CLIENT";  // Etiqueta para logs

void wifi_init() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

void toggle_led_and_send_status(int sock) {
    static int led_state = 0;
    led_state = !led_state;
    gpio_set_level(LED_GPIO, led_state);

    char message[32];
    snprintf(message, sizeof(message), "LED %s", led_state ? "ENCENDIDO" : "APAGADO");

    if (send(sock, message, strlen(message), 0) < 0) {
        ESP_LOGE(TAG, "Error enviando datos");
    } else {
        ESP_LOGI(TAG, "Mensaje enviado: %s", message);
    }
}

void tcp_client_task(void *pvParameters) {
    char rx_buffer[128];
    struct sockaddr_in server_addr;

    while (1) {  
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            ESP_LOGE(TAG, "Error creando socket");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);

        ESP_LOGI(TAG, "intentando conectar a %s:%d...", SERVER_IP, SERVER_PORT);
        
        while (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            ESP_LOGE(TAG, " Error conectando al servidor. Reintentando en 3s...");
            vTaskDelay(pdMS_TO_TICKS(3000));
        }

        ESP_LOGI(TAG, "Conexión establecida con el servidor");

        while (1) {
            toggle_led_and_send_status(sock);
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len > 0) {
                rx_buffer[len] = '\0';
                ESP_LOGI(TAG, "Respuesta recibida: %s", rx_buffer);
            } else {
                ESP_LOGE(TAG, " Servidor desconectado, cerrando socket");
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(5000));
        }

        close(sock);
        ESP_LOGI(TAG, " Reintentando conexión en 5 segundos...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main() {
    ESP_LOGI(TAG, "Iniciando ESP32 TCP Cliente...");
    nvs_flash_init();
    wifi_init();
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    xTaskCreate(&tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
}
