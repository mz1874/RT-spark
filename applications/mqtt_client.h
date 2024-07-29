#include <stdio.h>
#include <rtthread.h>
#include "mqttclient.h"
#include <drv_gpio.h>

#ifndef KAWAII_MQTT_HOST
#define KAWAII_MQTT_HOST               "192.168.1.113"
#endif
#ifndef KAWAII_MQTT_PORT
#define KAWAII_MQTT_PORT               "1883"
#endif
#ifndef KAWAII_MQTT_CLIENTID
#define KAWAII_MQTT_CLIENTID           "rtthread001"
#endif
#ifndef KAWAII_MQTT_USERNAME
#define KAWAII_MQTT_USERNAME           "root"
#endif
#ifndef KAWAII_MQTT_PASSWORD
#define KAWAII_MQTT_PASSWORD           "mazha1997"
#endif
#ifndef KAWAII_MQTT_SUBTOPIC
#define KAWAII_MQTT_SUBTOPIC           "/mqtt/test"
#endif
#ifndef KAWAII_MQTT_PUBTOPIC
#define KAWAII_MQTT_PUBTOPIC           "/mqtt/test"
#endif



static void sub_topic_handle1(void *client, message_data_t *msg);

static int mqtt_publish_handle1(mqtt_client_t *client);

void mqtt_task_entry(void *parameter);