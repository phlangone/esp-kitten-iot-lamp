/******************************************************************************
 * @file    mqtt.h
 * @brief   
 * 
 * Este arquivo define
 * 
 * @version 1.0
 * @date    2024-09-13
 * 
 * @note    Verifique as configurações de pinos GPIO e parâmetros de áudio para
 *          garantir que correspondam ao seu hardware e requisitos do projeto.
 * 
 * @copyright Copyright (c) 2024
 *           Todos os direitos reservados.
 ******************************************************************************
 */

#ifndef MAIN_MQTT_H_
#define MAIN_MQTT_H_

#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_err.h"

#define MQTT_BROKER_IP_ADDRESS      "fae84c8242f8470890d733685a23abad.s1.eu.hivemq.cloud"
#define MQTT_BROKER_PORT            "8883"
#define MQTT_TOPIC_SUBSCRIBE        "Langone/Luminaria/Cmd"
#define MQTT_CLIENT_USERNAME        "luminaria_miau_esp"
#define MQTT_CLIENT_PASSWORD        "br@ker_h1veMQ"

/**
 * @brief Inicializa a conexão com o broker MQTT.
 * 
 * Esta função é chamada para configurar e conectar o ESP32 ao broker MQTT.
 */
void mqtt_start(void);

#endif /* MAIN_MQTT_H_ */
