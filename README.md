# Miau Lamp Project

## Overview
The **Miau Lamp** project is an embedded system developed to control a decorative 3D-printed lamp shaped like a cat. The system uses an **ESP32 microcontroller** to drive an **addressable LED strip (WS2812B)** using the **RMT peripheral**.

The device provides two control interfaces:

* **Local:** Via **Bluetooth Classic (SPP)** using a custom Android application developed with **MIT App Inventor**.
* **Remote:** Via **Wi-Fi** using the **secure MQTT protocol (TLS)**, integrated with **Amazon Alexa** through a custom skill ("Miau Lamp") enabling voice commands.

## Software and Firmware Architecture

The firmware was developed in **C using the ESP-IDF framework**, applying concepts such as **unions, structures, and bit fields** to optimize in-memory manipulation of the communication protocol.

The firmware is organized into the following modules:

* `main.c`  
  Entry point of the application. Initializes **NVS, LED strip driver, Bluetooth stack, and Wi-Fi application**.

* `wifi_app.c / .h`  
  Handles **Wi-Fi Station (STA) connection**, including automatic reconnection and **SmartConfig (ESP-Touch)** provisioning when credentials are not available.

* `mqtt.c / .h`  
  Manages the **secure TLS connection** with the **HiveMQ broker** and processes **JSON payloads received from Alexa**.

* `blue.c / .h`  
  Configures the **Bluetooth controller and SPP profile**, parsing control frames sent by the Android application.

* `led_strip.c / .h`  
  LED strip driver (**8 LEDs on GPIO 4**) using the **RMT peripheral** to guarantee accurate **WS2812 timing at 10 MHz**.

---

## Communication Protocols and Interfaces

### 1. Android Application (Bluetooth SPP)

The application sends a **4-byte packet** for each interaction with the graphical interface.  
The packet structure uses **bit fields** to define the lamp state and RGB control.

| Byte | Field | Size | Description |
|-----|------|------|-------------|
| 0 | Control Byte | 8 bits | Bit 0: Color Mode (0 = White, 1 = RGB). Bit 1: On/Off. Bits 2–7: Reserved. |
| 1 | Red | 8 bits | Red intensity (0–255). |
| 2 | Green | 8 bits | Green intensity (0–255). |
| 3 | Blue | 8 bits | Blue intensity (0–255). |

**Application Interface:**

![Miau Light App Interface]

<img width="526" height="364" alt="image" src="https://github.com/user-attachments/assets/751f0eb3-6a7a-469d-8bd9-fae9a4202896" />

---

### 2. Amazon Alexa Integration (MQTT)

Communication with Alexa occurs indirectly through an **MQTT broker (HiveMQ Cloud)**.

The lamp subscribes to the topic: Langone/Luminaria/Cmd

When the user issues a voice command, the **Alexa Skill processes the intent** and publishes a JSON payload such as:

```json
{
  "cmd": "turn_on",
  "color": 16777215
}
```

---
