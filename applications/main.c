#include <rtthread.h>
#include <drv_gpio.h>
#include <wlan_mgnt.h>
#include "aht_10.h"
#include "mqtt_client.h"
#include "drv_common.h"
#include "dfs_file.h"
#include "sys/unistd.h"



rt_sem_t dynamic_sem = RT_NULL;
rt_mailbox_t mailbox = RT_NULL;
rt_mq_t mq = RT_NULL;


void wifi_connection(void) {
    //上锁
    rt_sem_trytake(dynamic_sem);
    char *ssid = "ImmortalWrt";
    char *password = "mazha1997";
    rt_wlan_scan();
    rt_wlan_connect(ssid, password);
    dfs_mount("filesystem", "/fal", "elm", 0,0);
    chdir("/fal");
    int fd;
    fd = open("1.txt", O_RDWR | O_APPEND | O_CREAT, 0);
    if (fd >= 0)
    {
        write(fd, "hello", strlen("hello"));
        close(fd);
    }
}


void wifi_disconnection(void) {
    rt_wlan_disconnect();
}


int main(void) {

    mq = rt_mq_create("mq", 37,
                      5, RT_IPC_FLAG_FIFO);

    //信号量确保WIFI先连接,在wlan_lwip里release
    dynamic_sem = rt_sem_create("mqtt_sem", 1, RT_IPC_FLAG_PRIO);
    mailbox = rt_mb_create("aht10", 10, RT_IPC_FLAG_FIFO);
    rt_pin_mode(GET_PIN(F, 12), PIN_MODE_OUTPUT);
    rt_pin_write(GET_PIN(F, 12), 1);
    wifi_connection();
    //等待网路连接完成然后启动MQTT, 获取不到的话一直等待
    rt_sem_take(dynamic_sem, RT_WAITING_FOREVER);
    //这里栈内存不要给小了
    rt_thread_t tid_mqtt = rt_thread_create("mqtt", mqtt_task_entry, RT_NULL, 4096, 24, 10);
    if (tid_mqtt == RT_NULL) {
        return -RT_ERROR;
    }
    rt_thread_startup(tid_mqtt);
    if(rt_sem_take(dynamic_sem, 10000)==RT_EOK)
    {
        rt_pin_write(GET_PIN(F, 12), 0);
        rt_thread_t thread = rt_thread_create("aht_10", AHT_test_entry, RT_NULL, 2048, 24, 10);
        rt_thread_startup(thread);
    }
    return 0;
}

#define WIFI_CS GET_PIN(F,10)
int disable_wifi()
{
    rt_pin_mode(WIFI_CS, PIN_MODE_OUTPUT);
    rt_pin_write(WIFI_CS, PIN_LOW);
    return 0;
}



INIT_BOARD_EXPORT(disable_wifi);


