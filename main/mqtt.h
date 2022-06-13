#ifndef RIIF_MQTT
#define RIIF_MQTT

#include <stdbool.h>

void mqtt_init();

bool mqtt_pub(char const *topic, char const *data);

bool is_mqtt_connected(void);

#endif
