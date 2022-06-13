#include "mqtt.h"
#include "ethernet.h"
#include "io.h"
#include "portal300.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "application logic"

void app_main(void)
{
    ethernet_init();
    mqtt_init();
    io_init();

    while(1) {

        if(io_was_doorbell_triggered())
        {
            if(is_mqtt_connected()) {
                ESP_LOGI(TAG, "The doorbell was rang, sending MQTT message.");
                mqtt_pub("shackspace/portal/event/doorbell", "");
            }
            else {
                ESP_LOGI(TAG, "The doorbell was rang, but there's nobody here to listen.");
            }
        }

        if(was_door_signalled())
        {
            ESP_LOGI(TAG, "MQTT message to unlock front door received. Opening door...");
            io_trigger_door_unlock();
        }

        vTaskDelay(100);
    }
}
