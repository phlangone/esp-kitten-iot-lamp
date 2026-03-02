# Projeto Luminária Miau

## Descrição Geral
O projeto Luminária Miau é um sistema embarcado desenvolvido para o controle de uma luminária decorativa impressa em 3D no formato de um gato. O sistema utiliza um microcontrolador ESP32 para acionar uma fita de LEDs endereçáveis (WS2812B) por meio do periférico RMT. 

O dispositivo oferece dupla interface de controle:
* **Local:** Via Bluetooth Clássico (SPP) através de um aplicativo Android personalizado desenvolvido no MIT App Inventor.
* **Remoto:** Via rede Wi-Fi utilizando o protocolo MQTT seguro (TLS), integrado com a Amazon Alexa por meio de uma Skill ("Luminária Miau") para acionamento por comandos de voz.

## Especificações de Hardware e Modelagem 3D
O projeto mecânico é dividido em duas partes principais, ambas modeladas e impressas em 3D:

### Corpo da Luminária (Gato)
* **Dimensões Finais:** Aproximadamente 150mm (A) x 70mm (L) x 75mm (C).
* **Manufatura:** Impressora Creality Ender 5.
* **Parâmetros de Impressão:** Bico de 0.8mm, altura de camada de 0.16mm, preenchimento em grade de 20%, temperatura do bico 210°C, mesa 65°C. Suporte do tipo árvore habilitado.
* **Acabamento:** O modelo foi preparado, pintado com tinta acrílica e adaptado para servir como vaso para plantas artificiais decorativas.

### Case da Eletrônica (ESP32)
* **Modelagem:** Autodesk Fusion.
* **Manufatura:** Impressora Creality Ender 3.
* **Parâmetros de Impressão:** Bico de 0.6mm, altura de camada de 0.2mm, preenchimento em grade de 20%, temperatura do bico 195°C, mesa 60°C.

## Arquitetura de Software e Firmware
O firmware foi desenvolvido em linguagem C utilizando o framework ESP-IDF, aplicando conceitos de programação como uniões (`unions`), estruturas (`structs`) e campos de bits (`bit fields`) para a otimização da manipulação do protocolo de dados em memória.

A estrutura de módulos do firmware consiste em:
* `main.c`: Ponto de entrada. Inicializa a NVS, a fita de LED, o Bluetooth e a aplicação Wi-Fi.
* `wifi_app.c / .h`: Gerencia a conexão Station (STA) com rotina de reconexão automática e provisionamento via SmartConfig (ESP-Touch) em caso de falha.
* `mqtt.c / .h`: Gerencia a conexão segura via TLS com o broker HiveMQ e o processamento dos payloads JSON recebidos da Alexa.
* `blue.c / .h`: Configura o controlador Bluetooth e o perfil SPP, realizando o parseamento dos frames de controle enviados pelo aplicativo.
* `led_strip.c / .h`: Driver de controle da fita de LED (8 LEDs no GPIO 4) utilizando o periférico RMT para garantir a temporização exata do protocolo WS2812 em 10MHz.

## Protocolos de Comunicação e Interfaces

### 1. Aplicativo Android (Bluetooth SPP)
O aplicativo envia um pacote de 4 bytes a cada interação com a interface gráfica. A estrutura do pacote utiliza campos de bits para definição do estado e controle RGB:

| Byte | Campo | Tamanho | Descrição |
| :--- | :--- | :--- | :--- |
| 0 | Control Byte | 8 bits | Bit 0: Color Mode (0=Branco, 1=RGB). Bit 1: On/Off. Bits 2 a 7: Não utilizados. |
| 1 | Red | 8 bits | Intensidade de vermelho (0 a 255). |
| 2 | Green | 8 bits | Intensidade de verde (0 a 255). |
| 3 | Blue | 8 bits | Intensidade de azul (0 a 255). |

**Interface do Aplicativo:**

![Interface do Aplicativo Miau Light]

<img width="526" height="364" alt="image" src="https://github.com/user-attachments/assets/751f0eb3-6a7a-469d-8bd9-fae9a4202896" />

### 2. Integração Amazon Alexa (MQTT)
A comunicação com a Alexa ocorre de forma indireta via Broker MQTT (HiveMQ Cloud). A luminária assina o tópico configurado (`Langone/Luminaria/Cmd`). Quando o usuário emite um comando de voz, a Skill processa a intenção e publica um payload JSON no formato:

```json
{
  "cmd": "turn_on",
  "color": 16777215
}
