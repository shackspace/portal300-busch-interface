#ifndef RIIF_MQTT
#define RIIF_MQTT

#include <stdbool.h>
#include <stddef.h>

struct MqttEvent {
  void const *topic;
  void const *data;

  size_t topic_len;
  size_t data_len;
};

typedef void (*MqttEventHandler)(struct MqttEvent *event, void *user_data);

void mqtt_init(void);

bool mqtt_pub(char const *topic, char const *data);

bool is_mqtt_connected(void);

bool mqtt_subscribe(char const *topic, MqttEventHandler handler,
                    void *user_data);

#endif
