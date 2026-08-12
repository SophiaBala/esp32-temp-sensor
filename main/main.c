#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

#define ONEWIRE_GPIO 4
static const char *TAG = "onewire";

static bool on_presence(void){
    gpio_set_direction(ONEWIRE_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(ONEWIRE_GPIO, 0);
    esp_rom_delay_us(480);

    gpio_set_level(ONEWIRE_GPIO, GPIO_MODE_INPUT);
    esp_rom_delay_us(70);

    bool presence = (gpio_get_level(ONEWIRE_GPIO) == 0);
    esp_rom_delay_us(410);

    return presence;
}

void app_main(void)
{
    gpio_reset_pin(ONEWIRE_GPIO);
    gpio_set_pull_mode(ONEWIRE_GPIO, GPIO_PULLUP_ONLY);

    while (1) {
        bool found = on_presence();
        ESP_LOGI(TAG, "Presence: %s", found ? "YES" : "NO");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
