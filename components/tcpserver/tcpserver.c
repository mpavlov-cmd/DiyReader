#include "tcpserver.h" 

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>


#define PORT 3330
#define KEEPALIVE_IDLE 10
#define KEEPALIVE_INTERVAL 10
#define KEEPALIVE_COUNT 5

static const char *TCP_TAG = "tcp-server";

static void do_retransmit(QueueHandle_t globalQueueHandle, const int sock)
{
    int len;
    char rx_buffer[128];

    do
    {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0)
        {
            ESP_LOGE(TCP_TAG, "Error occurred during receiving: errno %d", errno);
            break;
        }

        if (len == 0)
        {
            ESP_LOGW(TCP_TAG, "Connection closed");
            break;
        }

        rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
        ESP_LOGI(TCP_TAG, "Received %d bytes: %s", len, rx_buffer);

        // --------- USER Code before retransmit --------- //
        
        char firstChar  = rx_buffer[0];
        int valueToSend = firstChar == 'Y' ? 1 : 0;

        ESP_LOGI(TCP_TAG, "Sending data to queue: %d", valueToSend);
        if (!xQueueSend(globalQueueHandle, &valueToSend, 1000)) {
            ESP_LOGI(TCP_TAG, "Failed to send data to queue");
        }

        // --------- END USER Code after retransmi------- //

        // send() can return less bytes than supplied length.
        // Walk-around for robust implementation.
        int to_write = len;
        while (to_write > 0)
        {
            int written = send(sock, rx_buffer + (len - to_write), to_write, 0);
            if (written < 0)
            {
                ESP_LOGE(TCP_TAG, "Error occurred during sending: errno %d", errno);
            }
            to_write -= written;
        }

    } while (len > 0);
}

void tcp_server_task(void *pvParameters)
{
    // Extract parameter
    QueueHandle_t globalQueueHandle = *(QueueHandle_t *) pvParameters;

    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;

    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(PORT);
    ip_protocol = IPPROTO_IP;

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0)
    {
        ESP_LOGE(TCP_TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ESP_LOGI(TCP_TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE(TCP_TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TCP_TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }

    ESP_LOGI(TCP_TAG, "Socket bound, port %d", PORT);
    err = listen(listen_sock, 1);
    if (err != 0)
    {
        ESP_LOGE(TCP_TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1)
    {
        ESP_LOGI(TCP_TAG, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0)
        {
            ESP_LOGE(TCP_TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));

        // Convert ip address to string
        if (source_addr.ss_family == PF_INET)
        {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }

        ESP_LOGI(TCP_TAG, "Socket accepted ip address: %s", addr_str);

        do_retransmit(globalQueueHandle, sock);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}