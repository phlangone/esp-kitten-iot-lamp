/******************************************************************************
 * @file    mqtt.c
 * @brief   Gerencia conexão com o broker e eventos MQTT
 * 
 * Este arquivo 
 * 
 * @version 1.0
 * @date    2024-09-13
 * 
 * @note    Certifique-se de configurar corretamente a url do broker MQTT
 * 
 * @copyright Copyright (c) 2024
 *           Todos os direitos reservados.
 ******************************************************************************
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_ota_ops.h"
#include "esp_tls.h"
#include "esp_netif.h"
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"

#include "mqtt.h"
#include "led_strip.h"

#define DEBUG 1

static const char *TAG = "mqtt";

/*
* Certificado para conexão TLS com o broker
*/
extern const uint8_t mqtt_broker_tls_io_pem_start[]   asm("_binary_mqtt_broker_tls_io_pem_start");
extern const uint8_t mqtt_broker_tls_io_pem_end[]     asm("_binary_mqtt_broker_tls_io_pem_end");

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);

    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) 
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            // Assinatura dos tópicos
            msg_id = esp_mqtt_client_subscribe(client, MQTT_TOPIC_SUBSCRIBE, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");

            if(DEBUG)
            {
                printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
                printf("DATA=%.*s\r\n", event->data_len, event->data);
            }

            cJSON *json = cJSON_Parse((const char *)event->data);
            if (json == NULL) 
            {
                ESP_LOGE(TAG, "Erro ao analisar JSON");
                return;
            }

            // Acessar os dados do JSON
            cJSON *cmd = cJSON_GetObjectItem(json, "cmd");
            cJSON *color = cJSON_GetObjectItem(json, "color");

            if (cJSON_IsString(cmd) && cJSON_IsNumber(color)) 
            {
                uint32_t color_value = (uint32_t)cJSON_GetNumberValue(color);
                if(strcmp(cmd->valuestring, "turn_on") == 0)
                {
                    led_strip_fill(color_value);
                }
                else
                {
                    led_strip_fill(0);
                }
            }
            
            cJSON_Delete(json);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) 
            {
                log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;

        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

void mqtt_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = 
    {
        .broker =
        {
            .address.uri = "mqtts://fae84c8242f8470890d733685a23abad.s1.eu.hivemq.cloud:8883",
            .verification.certificate = (const char *)mqtt_broker_tls_io_pem_start
        },

        .credentials =
        {
                .username = MQTT_CLIENT_USERNAME,
                .authentication.password = MQTT_CLIENT_PASSWORD
        }
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));
}
