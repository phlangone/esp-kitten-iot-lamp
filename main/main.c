#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_spp_api.h"

#include "wifi_app.h"
#include "blue.h"
#include "led_strip.h"

void app_main(void)
{
    // Inicializa NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicializa Fita de LED
    led_strip_init();

    // Inicializa Bluetooth
    bluetooth_init();

    // Inicializa Wi-Fi
    wifi_app_start();
}
