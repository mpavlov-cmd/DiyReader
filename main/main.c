#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "lwip/sockets.h"

#include "blink.h"
#include "wifistation.h"
#include "tcpserver.h"

#define LED GPIO_NUM_2


void blinky(void *pvParameters) {

     // Extract parameter
     QueueHandle_t globalQueueHandle = *(QueueHandle_t *) pvParameters;

     configure(GPIO_NUM_2);

     int receivedData = 0;
     while(1)
     {
          if (xQueueReceive(globalQueueHandle, &receivedData, 1000)) {
               ESP_LOGI("blinky", "Received data from queue %d", receivedData);
               blink(GPIO_NUM_2, receivedData);
          }
     }
}

void app_main(void)
{
     printf("Hello world!\n");

     ESP_ERROR_CHECK(nvs_flash_init());
     ESP_ERROR_CHECK(esp_netif_init());

     QueueHandle_t globalQueueHandle = xQueueCreate(3, sizeof(int));

     // Init Wifi Station
     wifi_init_sta();

     xTaskCreate(&blinky, "blinky", 2048, (void *) &globalQueueHandle, 5, NULL);
     xTaskCreate(&tcp_server_task, "tcp-server", 4096, (void *) &globalQueueHandle, 5, NULL);
}