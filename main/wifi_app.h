/******************************************************************************
 * @file    wifi_app.h
 * @brief   Cabeçalho para a aplicação WiFi.
 * 
 * Este arquivo define as constantes e funções para configurar e gerenciar a
 * conexão WiFi. Inclui definições para o SSID e senha da rede WiFi, configurações
 * de IP estático, e funções para enviar mensagens para a task de WiFi.
 * 
 * @version 1.0
 * @date    2024-09-03
 * 
 * @note    Certifique-se de ajustar as configurações de IP, SSID e senha conforme
 *          necessário para o seu ambiente de rede.
 * 
 * @copyright Copyright (c) 2024
 *           Todos os direitos reservados.
 ******************************************************************************
 */

#ifndef MAIN_WIFI_APP_H_
#define MAIN_WIFI_APP_H_

#include "esp_netif.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"

// Task para a aplicação WiFi
#define WIFI_APP_TASK_STACK_SIZE        4096        ///< Tamanho da stack para a task de aplicação WiFi
#define WIFI_APP_TASK_PRIORITY          5           ///< Prioridade da task de aplicação WiFi
#define WIFI_APP_TASK_CORE_ID           0           ///< Core onde a task de aplicação WiFi será executada

#define WIFI_AP_SSID                    ""   ///< SSID da rede WiFi
#define WIFI_AP_PASS                    ""  ///< Senha da rede WiFi
#define WIFI_AP_MAXIMUM_RETRY           5           ///< Número máximo de tentativas de conexão

#define WIFI_STA_IP            "192.168.0.1"          ///< IP estático para a interface WiFi
#define WIFI_STA_GATEWAY       "192.168.0.1"          ///< Gateway para a interface WiFi
#define WIFI_STA_NETMASK       "255.255.255.0"        ///< Máscara de rede para a interface WiFi

//#define DHCP_OFF                                    // Comente para usar DHCP
#define SET_MAC_ADDRESS_OF_TARGET_AP              ///< Comente para não setar o MAC address do AP 

// Objeto netif para a estação
extern esp_netif_t* esp_netif_sta;

/**
 * Mensagens para a task de aplicação WiFi
 * @note Expanda conforme as necessidades do seu projeto
*/
typedef enum wifi_app_messages
{
    WIFI_APP_MSG_STA_CONNECTED_GOT_IP = 0,  ///< Mensagem quando a estação WiFi obtém um IP
    WIFI_APP_MSG_START_SC,                  ///< Mensagem solicitando o início do Smart Config
    WIFI_APP_MSG_ESPTOUCH_DONE              ///< Mensagem quando a configuração Smart Config é concluída
} wifi_app_message_e;

/**
 * Estrutura para a fila de mensagens
 * @note Expanda se necessário, por exemplo, adicione outro tipo e parâmetros necessários
*/
typedef struct wifi_app_queue_message
{
    wifi_app_message_e msgID;  ///< ID da mensagem
} wifi_app_queue_message_t;

/**
 * @brief Envia uma mensagem para a fila da aplicação Wi-Fi.
 *
 * @param msgID ID da mensagem a ser enviada.
 * @return BaseType_t Retorna o resultado da operação de envio.
 */
BaseType_t wifi_app_send_message(wifi_app_message_e msgID);

/**
 * @brief Inicia a aplicação Wi-Fi.
 * 
 * Esta função é responsável por configurar e iniciar a tarefa principal da aplicação Wi-Fi.
 */
void wifi_app_start(void);

#endif /* MAIN_WIFI_APP_H_ */

