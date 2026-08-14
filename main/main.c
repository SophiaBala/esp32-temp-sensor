#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#define ONEWIRE_GPIO 4
static const char *TAG = "onewire";

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

static void wifi_init(void)
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "TP-Link_4D66",
            .password = "19897462",
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

static bool on_presence(void){
    gpio_set_direction(ONEWIRE_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(ONEWIRE_GPIO, 0);
    esp_rom_delay_us(480);

    gpio_set_direction(ONEWIRE_GPIO, GPIO_MODE_INPUT);
    esp_rom_delay_us(70);

    bool presence = (gpio_get_level(ONEWIRE_GPIO) == 0);
    esp_rom_delay_us(410);

    return presence;
}

static void write_bit(int bit){
    gpio_set_direction(ONEWIRE_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(ONEWIRE_GPIO, 0);
    esp_rom_delay_us(bit ? 6 : 60);
    gpio_set_direction(ONEWIRE_GPIO, GPIO_MODE_INPUT);
    esp_rom_delay_us(bit ? 64 : 10);
}


static int read_bit(void){
    gpio_set_direction(ONEWIRE_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(ONEWIRE_GPIO, 0);
    esp_rom_delay_us(2);
    gpio_set_direction(ONEWIRE_GPIO, GPIO_MODE_INPUT);
    esp_rom_delay_us(10);

    int bit = gpio_get_level(ONEWIRE_GPIO);
    esp_rom_delay_us(50);
    return bit;
}

static void write_byte(uint8_t byte){
    for (int i=0; i < 8; i++){
        write_bit(byte&1);
        byte>>=1;
    }
}

static uint8_t read_byte(void){
    uint8_t byte = 0;
    for (int i=0; i < 8; i++){
        byte |= (read_bit()<<i);
    }
    return byte;
}

static bool read_ds18b20(float *final_temperature){
    uint8_t data[9];

    if(on_presence() == false){
        return false;
    }

    write_byte(0xCC);
    write_byte(0x44);

    vTaskDelay(pdMS_TO_TICKS(750));

    if(on_presence() == false){
        return false;
    }

    write_byte(0xCC);
    write_byte(0xBE);

    for(int i = 0; i<9; i++){
        data[i] = read_byte();
    }

    int16_t temperature;
    temperature = (data[1] << 8) | data[0];

    *final_temperature = temperature / 16.0f;

    return true;
}

void app_main(void)
{
    wifi_init();
    vTaskDelay(pdMS_TO_TICKS(3000));

    gpio_reset_pin(ONEWIRE_GPIO);
    gpio_set_pull_mode(ONEWIRE_GPIO, GPIO_PULLUP_ONLY);

    while (1) {
        float t;
        if (read_ds18b20(&t)) {
            ESP_LOGI(TAG, "Temperature: %.2f C", t);
        } else {
            ESP_LOGW(TAG, "failed");
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}