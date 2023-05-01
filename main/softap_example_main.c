/*#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_eth.h"
#include <esp_https_server.h>
#include "esp_tls.h"
#include <string.h>
#include "driver/gpio.h"
#include <stdio.h>*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include <esp_https_server.h>
#include "esp_tls.h"
#include "sdkconfig.h"

#include "driver/gpio.h"






#define led_L 25

int8_t led_L_state = 0;

static const char *TAG = "main";


/***Timestamp***/
void get_time(char *str){
    sprintf(str, "%d", xTaskGetTickCount() / configTICK_RATE_HZ);
}
void toogle_led_state(){
    led_L_state == 1 ? (led_L_state = 0) : (led_L_state = 1);
    gpio_set_level(led_L, led_L_state);
}

/* An HTTP GET handler */
static esp_err_t root_get_handler(httpd_req_t *req)
{
    char timeStamp[20]={0};
    extern unsigned char view_start[] asm("_binary_view_html_start");
    extern unsigned char view_end[] asm("_binary_view_html_end");
    size_t view_len = view_end - view_start;
    char viewHtml[view_len];
    memcpy(viewHtml, view_start, view_len);
    ESP_LOGI(TAG, "URI: %s", req->uri);

    if (strcmp(req->uri, "/?comm1") == 0)
    {
        get_time(timeStamp);
    }
    if (strcmp(req->uri, "/?comm2") == 0)
    {
        //Estado del led
    }
    if (strcmp(req->uri, "/?comm3") == 0)
    {
        //Temperatura
    }
    if (strcmp(req->uri, "/?comm4") == 0)
    {
        toogle_led_state();
    }

    char *viewHtmlUpdated;
    int formattedStrResult = asprintf(&viewHtmlUpdated, viewHtml, timeStamp, led_L_state ? "ON" : "OFF", "45C", "toggle");

    httpd_resp_set_type(req, "text/html");

    if (formattedStrResult > 0)
    {
        httpd_resp_send(req, viewHtmlUpdated, view_len);
        free(viewHtmlUpdated);
    }
    else
    {
        ESP_LOGE(TAG, "Error updating variables");
        httpd_resp_send(req, viewHtml, view_len);
    }

    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server");
    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
    conf.transport_mode = HTTPD_SSL_TRANSPORT_INSECURE;
    esp_err_t ret = httpd_ssl_start(&server, &conf);
    if (ESP_OK != ret)
    {
        ESP_LOGI(TAG, "Error starting server!");
        return NULL;
    }

    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &root);
    return server;
}
/*
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
        return server;
    }
    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}*/

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    //httpd_stop(server);
    httpd_ssl_stop(server);
}

static void disconnect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server)
    {
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server == NULL)
    {
        *server = start_webserver();
    }
}


void app_main(void)
{
    gpio_reset_pin(led_L);
    gpio_set_direction(led_L, GPIO_MODE_OUTPUT);
    gpio_set_level(led_L, led_L_state);
 //   ESP_ERROR_CHECK(init_led());
    static httpd_handle_t server = NULL;
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,&connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,&disconnect_handler, &server));
    ESP_ERROR_CHECK(example_connect());
}
