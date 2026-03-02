/******************************************************************************
 * @file    wifi_app.c
 * @brief   Aplicativo Wi-Fi para ESP32
 * 
 * Este arquivo contém a implementação das funcionalidades do aplicativo Wi-Fi,
 * incluindo o gerenciamento de eventos, configuração e tarefas para conectar-se 
 * a uma rede Wi-Fi e lidar com eventos de Wi-Fi.
 * 
 * @version 1.0
 * @date    2024-09-03
 * 
 * @note    Certifique-se de incluir os cabeçalhos apropriados e definir as macros
 *          necessárias, como WIFI_AP_SSID, WIFI_AP_PASS, etc., em seu projeto.
 * 
 * @copyright Copyright (c) 2024
 *           Todos os direitos reservados.
 ******************************************************************************
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"

#include "esp_eap_client.h"
#include "esp_smartconfig.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "lwip/netdb.h"

#include "wifi_app.h"
#include "mqtt.h"

// Tag usada para mensagens no console serial do ESP
static const char TAG[] = "wifi_app";

// Handle da fila usada para manipular a fila principal de eventos
static QueueHandle_t wifi_app_queue_handle;

// Objeto netif para a estação Wi-Fi
esp_netif_t* esp_netif_sta = NULL;

/**
 * @brief Manipulador de eventos da aplicação Wi-Fi.
 * 
 * Este manipulador lida com eventos relacionados ao Wi-Fi e IP.
 *
 * @param arg Parâmetros adicionais passados ao manipulador.
 * @param event_base O tipo de evento (WIFI_EVENT ou IP_EVENT).
 * @param event_id O ID específico do evento.
 * @param event_data Dados associados ao evento.
 */
static void wifi_app_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_START"); 
                esp_wifi_connect();               
                break;

            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
            #ifdef DHCP_OFF
                // Configurações manuais de IP quando o DHCP está desativado
                esp_netif_ip_info_t sta_ip_info;
                inet_pton(AF_INET, WIFI_STA_IP, &sta_ip_info.ip);
                inet_pton(AF_INET, WIFI_STA_GATEWAY, &sta_ip_info.gw);
                inet_pton(AF_INET, WIFI_STA_NETMASK, &sta_ip_info.netmask);

                esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
                esp_err_t err = esp_netif_dhcpc_stop(netif);
                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "Erro ao parar o DHCP: %s", esp_err_to_name(err));
                    break;
                }
                else
                {
                    ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &sta_ip_info));
                }
            #endif
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
                // Tenta reconectar ao AP se a conexão for perdida
                for(int retry = 0; retry < WIFI_AP_MAXIMUM_RETRY; retry++)
                {
                    esp_wifi_connect();
                    ESP_LOGI(TAG, "Tentando novamente a conexão...");
                }

                ESP_LOGI(TAG, "Conexão ao AP falhou");

                // Inicia Smart Config para configurar as credenciais
                wifi_app_send_message(WIFI_APP_MSG_START_SC);
                break;
            
            default:
                break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        switch(event_id)
        {
            case IP_EVENT_STA_GOT_IP:
                // Evento de obtenção de IP, indica conexão bem-sucedida
                ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
                wifi_app_send_message(WIFI_APP_MSG_STA_CONNECTED_GOT_IP);
                break;
        }
    }

    else if (event_base == SC_EVENT)
    {
        switch(event_id)
        {
            case SC_EVENT_SCAN_DONE:
                ESP_LOGI(TAG, "Smart Config Scan Done");
                break;
            
            case SC_EVENT_FOUND_CHANNEL:
                ESP_LOGI(TAG, "Smart Config Found Channel");
                break;

            case SC_EVENT_GOT_SSID_PSWD:
                ESP_LOGI(TAG, "Smart Config Got SSID and Password");

                smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
                wifi_config_t wifi_config;
                uint8_t ssid[33] = { 0 };
                uint8_t password[65] = { 0 };
                uint8_t rvd_data[33] = { 0 };

                bzero(&wifi_config, sizeof(wifi_config_t));
                memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
                memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));

        #ifdef SET_MAC_ADDRESS_OF_TARGET_AP
                wifi_config.sta.bssid_set = evt->bssid_set;
                if (wifi_config.sta.bssid_set == true) 
                {
                    ESP_LOGI(TAG, "Set MAC address of target AP: "MACSTR" ", MAC2STR(evt->bssid));
                    memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
                }
        #endif

                memcpy(ssid, evt->ssid, sizeof(evt->ssid));
                memcpy(password, evt->password, sizeof(evt->password));
                ESP_LOGI(TAG, "SSID:%s", ssid);
                ESP_LOGI(TAG, "PASSWORD:%s", password);

                // Caso esteja utilizando o app Esp Touch V2
                if (evt->type == SC_TYPE_ESPTOUCH_V2) 
                {
                    ESP_ERROR_CHECK( esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)) );
                    ESP_LOGI(TAG, "RVD_DATA:");
                    for (int i=0; i<33; i++) 
                    {
                        printf("%02x ", rvd_data[i]);
                    }
                    printf("\n");
                }

                ESP_ERROR_CHECK( esp_wifi_disconnect() );
                ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
                esp_wifi_connect();
                break;

            case SC_EVENT_SEND_ACK_DONE:
                ESP_LOGI(TAG, "Smart Config Acknolegment Done");

                wifi_app_send_message(WIFI_APP_MSG_ESPTOUCH_DONE);
                break;                
        }
    }
}

/**
 * @brief Inicializa o manipulador de eventos da aplicação Wi-Fi para eventos de Wi-Fi e IP.
 */
static void wifi_app_event_handler_init(void)
{
    // Cria o loop de eventos para o driver Wi-Fi
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Instâncias para os manipuladores de eventos
    esp_event_handler_instance_t instance_wifi_event;
    esp_event_handler_instance_t instance_ip_event;

    // Registra o manipulador de eventos para os eventos Wi-Fi
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,           // Eventos Wi-Fi
                                                        ESP_EVENT_ANY_ID,     // ID universal - handler é chamado para qualquer evento WIFI
                                                        &wifi_app_event_handler, // Ponteiro para a função de callback
                                                        NULL,                 // Parâmetro para a função de callback
                                                        &instance_wifi_event)); // Instância para os eventos registrados

    // Registra o manipulador de eventos para os eventos de IP
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,             // Eventos IP
                                                        ESP_EVENT_ANY_ID,     // ID para qualquer evento IP
                                                        &wifi_app_event_handler, // Ponteiro para a função de callback
                                                        NULL,                 // Parâmetro para a função de callback
                                                        &instance_ip_event)); // Instância para os eventos registrados

    // Registra o manipulador de eventos para os eventos Smart Config
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, 
                                                ESP_EVENT_ANY_ID, 
                                                &wifi_app_event_handler, 
                                                NULL) );                                                       
}

/**
 * @brief Inicializa a pilha TCP/IP e a configuração padrão do Wi-Fi.
 */
static void wifi_app_default_wifi_init(void)
{
    // Inicializa a pilha TCP/IP
    ESP_ERROR_CHECK(esp_netif_init());

    // Configuração padrão do Wi-Fi - deve ser feito nesta ordem
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    esp_netif_sta = esp_netif_create_default_wifi_sta();
}

/**
 * @brief Configura o Wi-Fi em modo Station (STA).
 */
static void wifi_app_sta_config(void)
{
    // Configuração do Station
    wifi_config_t wifi_config =
    {
        .sta = 
        {
            .ssid = WIFI_AP_SSID,
            .password = WIFI_AP_PASS, // OBS: Autenticação WPA2 por padrão
        },
    };

    // Configura o modo Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    /**
     * Aplica a configuração definida na interface (STA ou AP).
     * Essa configuração é gravada na NVS.
     */
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
}

/**
 * @brief Tarefa principal para a aplicação Wi-Fi.
 * 
 * Esta tarefa lida com a inicialização e manutenção da conexão Wi-Fi.
 *
 * @param pvParameters Parâmetro que pode ser passado para a tarefa.
 */
static void wifi_app_task(void *pvParameter)
{
    wifi_app_queue_message_t msg;

    // Inicializa o manipulador de eventos
    wifi_app_event_handler_init();

    // Inicializa a pilha TCP/IP e a configuração Wi-Fi
    wifi_app_default_wifi_init();

    // Configura o STA
    wifi_app_sta_config();

    // Inicia o Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_start());

    for(;;)
    {
        if (xQueueReceive(wifi_app_queue_handle, &msg, portMAX_DELAY))
        {
            switch (msg.msgID)
            {
                case WIFI_APP_MSG_STA_CONNECTED_GOT_IP:
                    // Mensagem de sucesso na conexão ao AP
                    ESP_LOGI(TAG, "Conectado ao AP SSID:%s password:%s", WIFI_AP_SSID, WIFI_AP_PASS);
                    mqtt_start();         // Inicia conexão com broker MQTT 
                    break;

                case WIFI_APP_MSG_START_SC:
                    // Mensagem de início na tentativa de conexão WiFi
                    ESP_LOGI(TAG, "Iniciado Smart Config para configuração das credenciais WiFi");
                    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
                    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
                    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) ); 
                    break; 

                case WIFI_APP_MSG_ESPTOUCH_DONE:
                    // Mensagem de encerramento do Smart Config
                    ESP_LOGI(TAG, "Smart Config Encerrado");
                    esp_smartconfig_stop();
                    break;

                default:
                    break;      
            }
        }
    }
}

/**
 * @brief Envia uma mensagem para a fila da aplicação Wi-Fi.
 *
 * @param msgID ID da mensagem a ser enviada.
 * @return BaseType_t Retorna o resultado da operação de envio.
 */
BaseType_t wifi_app_send_message(wifi_app_message_e msgID)
{
    wifi_app_queue_message_t msg;
    msg.msgID = msgID;
    return xQueueSend(wifi_app_queue_handle, &msg, portMAX_DELAY);
}

/**
 * @brief Inicia a aplicação Wi-Fi.
 * 
 * Esta função é responsável por configurar e iniciar a tarefa principal da aplicação Wi-Fi.
 */
void wifi_app_start(void)
{
    ESP_LOGI(TAG, "STARTING WIFI APPLICATION");

    // Desabilita mensagens de log padrão do Wi-Fi
    esp_log_level_set("wifi", ESP_LOG_NONE);

    // Cria a fila de mensagens
    wifi_app_queue_handle = xQueueCreate(3, sizeof(wifi_app_queue_message_t));

    // Inicia a tarefa da aplicação Wi-Fi
    xTaskCreatePinnedToCore(&wifi_app_task, "wifi_app_task", WIFI_APP_TASK_STACK_SIZE,
                                                 NULL,
                                                 WIFI_APP_TASK_PRIORITY, 
                                                 NULL,
                                                 WIFI_APP_TASK_CORE_ID);    
}
