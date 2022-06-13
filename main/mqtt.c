// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_netif.html

#include "mqtt.h"
#include <esp_log.h>
#include <mqtt_client.h>
#include <freertos/event_groups.h>

static const char *TAG = "MQTT";

static const char STATUS_TOPIC[] = "esp32/ethernet/status";
static const char STATUS_ONLINE[] = "online";
static const char STATUS_OFFLINE[] = "offline";

static const EventBits_t EVENT_CONNECTED_BIT = 1;

static esp_mqtt_client_handle_t client = NULL;
static EventGroupHandle_t event_group = NULL;

bool is_mqtt_connected(void)
{
    EventBits_t bits = xEventGroupGetBits(event_group);
    return (bits & EVENT_CONNECTED_BIT) != 0;
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

bool mqtt_pub(char const * topic, char const * data)
{
  int msg_id = esp_mqtt_client_publish(client, topic, data, 0, 0, 0);
  ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
  return (msg_id != -1);
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
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        xEventGroupSetBits(event_group, EVENT_CONNECTED_BIT);
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, STATUS_TOPIC, STATUS_ONLINE, 0, 2, 0); // exactly once
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "#", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        // ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;

    case MQTT_EVENT_DISCONNECTED:
        xEventGroupClearBits(event_group, EVENT_CONNECTED_BIT);
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        // ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        // ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        // ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
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

static const char client_crt[] =
#include "certs/buzzer.crt.h"
;
static const char client_key[] =
#include "certs/buzzer.key.h"
;
static const char ca_crt[] =
#include "certs/ca.crt.h"
;

void mqtt_init()
{
  if(client != NULL) {
    return;
  }
  event_group = xEventGroupCreate();

  esp_mqtt_client_config_t mqtt_cfg = {
    
    .host = "10.0.0.11",
    .port = 8883,
    .transport = MQTT_TRANSPORT_OVER_SSL,

    .client_id = "buzzer.portal",

    .client_cert_pem = client_crt,
    .client_key_pem = client_key,
    
    .cert_pem = ca_crt,
    .use_global_ca_store = false,
    .skip_cert_common_name_check = true,

    .lwt_topic = STATUS_TOPIC,
    .lwt_msg = STATUS_OFFLINE,
    .lwt_qos = 2, // exactly once
    .lwt_retain = 0,
    .lwt_msg_len = 0, // NUL terminated
  };

  client = esp_mqtt_client_init(&mqtt_cfg);
  /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
  esp_mqtt_client_start(client);
}
