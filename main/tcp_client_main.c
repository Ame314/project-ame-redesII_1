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

// Configuración de red y hardware
#define WIFI_SSID "Casa_GROB_ROSERO"  // Nombre de la red WiFi
#define WIFI_PASS "laClave;-)"        // Contraseña de la red WiFi
#define SERVER_IP "192.168.1.23"      // IP del servidor al que se conectará
#define SERVER_PORT 12345             // Puerto del servidor
#define LED_GPIO GPIO_NUM_2           // Pin GPIO donde está conectado el LED

// Etiqueta para los logs
static const char *TAG = "TCP_CLIENT_APP";

// Inicializa la conexión WiFi en modo estación
void initialize_wifi() {
    esp_netif_init();  // Inicializa la red
    esp_event_loop_create_default();  // Crea el bucle de eventos predeterminado
    esp_netif_create_default_wifi_sta();  // Configura el modo estación
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Configuración de la red WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);  // Configura el modo estación
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);  // Aplica la configuración
    esp_wifi_start();  // Inicia el WiFi
    esp_wifi_connect();  // Conecta a la red
}

// Alterna el estado del LED y envía el estado al servidor
void handle_led_and_send(int sock) {
    static int led_status = 0;  // Estado actual del LED (0 = apagado, 1 = encendido)
    led_status = !led_status;  // Cambia el estado del LED
    gpio_set_level(LED_GPIO, led_status);  // Actualiza el GPIO del LED

    // Prepara el mensaje para enviar al servidor
    char message[32];
    snprintf(message, sizeof(message), "Estado del LED: %s", led_status ? "ON" : "OFF");

    // Envía el mensaje al servidor
    if (send(sock, message, strlen(message), 0) < 0) {
        ESP_LOGE(TAG, "No se pudo enviar el mensaje al servidor");
    } else {
        ESP_LOGI(TAG, "Mensaje enviado correctamente: %s", message);
    }
}

// Tarea principal del cliente TCP
void tcp_client_task(void *pvParameters) {
    char rx_buffer[128];  // Buffer para recibir datos
    struct sockaddr_in server_addr;  // Dirección del servidor

    while (1) {  
        // Crea un socket
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            ESP_LOGE(TAG, "Error al crear el socket");
            vTaskDelay(pdMS_TO_TICKS(5000));  // Espera antes de reintentar
            continue;
        }

        // Configura la dirección del servidor
        server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);

        ESP_LOGI(TAG, "Intentando conectar al servidor %s:%d...", SERVER_IP, SERVER_PORT);
        
        // Intenta conectar al servidor
        while (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            ESP_LOGE(TAG, "No se pudo conectar al servidor. Reintentando en 3 segundos...");
            vTaskDelay(pdMS_TO_TICKS(3000));
        }

        ESP_LOGI(TAG, "Conexión establecida con el servidor");

        // Bucle principal de comunicación
        while (1) {
            handle_led_and_send(sock);  // Alterna el LED y envía el estado
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);  // Recibe datos
            if (len > 0) {
                rx_buffer[len] = '\0';  // Termina la cadena recibida
                ESP_LOGI(TAG, "Mensaje recibido del servidor: %s", rx_buffer);
            } else {
                ESP_LOGE(TAG, "El servidor cerró la conexión. Cerrando socket...");
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(5000));  // Espera antes de enviar el siguiente mensaje
        }

        close(sock);  // Cierra el socket
        ESP_LOGI(TAG, "Reintentando conexión en 5 segundos...");
        vTaskDelay(pdMS_TO_TICKS(5000));  // Espera antes de reintentar
    }
}

// Punto de entrada principal de la aplicación
void app_main() {
    ESP_LOGI(TAG, "Iniciando cliente TCP en ESP32...");
    nvs_flash_init();  // Inicializa el almacenamiento no volátil
    initialize_wifi();  // Configura la conexión WiFi
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);  // Configura el GPIO del LED como salida
    xTaskCreate(&tcp_client_task, "tcp_client_task", 4096, NULL, 5, NULL);  // Crea la tarea del cliente TCP
}