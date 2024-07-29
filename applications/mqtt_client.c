#include <stdio.h>
#include <rtthread.h>
#include "mqttclient.h"
#include <drv_gpio.h>
#include "mqtt_client.h"

extern rt_mailbox_t mailbox;
extern rt_mq_t mq;


static void sub_topic_handle1(void *client, message_data_t *msg) {
    (void) client;
    if (strcmp((char *) msg->message->payload, "on") == 0) {
        rt_pin_write(GET_PIN(F, 12), 0);
    } else if (strcmp((char *) msg->message->payload, "off") == 0) {
        rt_pin_write(GET_PIN(F, 12), 1);
    }
//    rt_kprintf("\r\n");
//    rt_kprintf("topic: %s\nmessage:%s\r\n", msg->topic_name,
//               (char *) msg->message->payload);
//    rt_kprintf("\r\n");
}

void mqtt_task_entry(void *parameter) {
    mqtt_client_t *mqtt_client = NULL;
    mqtt_log_init();
    mqtt_client = mqtt_lease();
    mqtt_set_host(mqtt_client, KAWAII_MQTT_HOST);
    mqtt_set_port(mqtt_client, KAWAII_MQTT_PORT);
    mqtt_set_user_name(mqtt_client, KAWAII_MQTT_USERNAME);
    mqtt_set_password(mqtt_client, KAWAII_MQTT_PASSWORD);
    mqtt_set_client_id(mqtt_client, KAWAII_MQTT_CLIENTID);
    mqtt_set_clean_session(mqtt_client, 1);
    mqtt_connect(mqtt_client);
    mqtt_subscribe(mqtt_client, KAWAII_MQTT_SUBTOPIC, QOS1, sub_topic_handle1);

    uint32_t receive;
    char payload[37]; // Buffer to hold the string representation of the receive value
    while (1) {
//        邮箱
//        if (rt_mb_recv(mailbox, &receive, RT_WAITING_FOREVER) == RT_EOK)
//        {
//            KAWAII_MQTT_LOG_I("received message %u", receive); // Changed %f to %u to correctly print uint32_t
//
//            mqtt_message_t msg;
//            memset(&msg, 0, sizeof(msg));
//            msg.qos = QOS0;
//
//            // Convert receive value to string and assign to msg.payload
//            sprintf(payload, "%lu", receive);
//            msg.payload = (void *)payload;
//
//            mqtt_publish(mqtt_client, KAWAII_MQTT_PUBTOPIC, &msg);
//        }
        if (rt_mq_recv(mq, &payload, sizeof(payload), 1000) > 0) {
            mqtt_message_t msg;
            memset(&msg, 0, sizeof(msg));
            msg.qos = QOS1;

            // msg.payload 直接指向接收到的 payload
            msg.payload = (void *) payload;

            mqtt_publish(mqtt_client, KAWAII_MQTT_PUBTOPIC, &msg);
        }
        rt_thread_delay(1000);

    }

}

