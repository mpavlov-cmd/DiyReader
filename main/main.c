#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "lwip/sockets.h"

#include "blink.h"
#include "wifistation.h"
#include "tcpserver.h"

#define LED GPIO_NUM_2

void blinky(void *pvParameter) {

     configure(GPIO_NUM_2);

     while(1)
     {
          blink(GPIO_NUM_2, 1);
          vTaskDelay(1000 / portTICK_PERIOD_MS);

          blink(GPIO_NUM_2, 0);
          vTaskDelay(1000 / portTICK_PERIOD_MS);
     }
}

void app_main(void)
{

     printf("Hello world!\n");

     esp_err_t ret = nvs_flash_init();
     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
     {
          ESP_ERROR_CHECK(nvs_flash_erase());
          ret = nvs_flash_init();
     }
     ESP_ERROR_CHECK(ret);

     // Init Wifi Station
     wifi_init_sta();

     // Blink 
     xTaskCreate(&blinky, "blinky", 2048, NULL, 5, NULL);
     xTaskCreate(&tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, NULL);
     
     //Test
}