# Day5 组件

在本章节我将会介绍如何使用软件包，其中包括如何使用第三方的MQTT组件（Kawayi MQTT）连接远程的MQTT服务器。以及如何使用文件系统。同时将数据写入到
片外的flash上。 通过MQTT的回调函数控制小灯的闪烁等。

由于本章节我们需要使用MQTT服务，来进行消息的收发，但是在此之前我们需要Enable对RW007的支持

![rw007.jpg](ass%2Frw007.jpg)

并且修改PIN的定义为下图所示。

![speed.jpg](ass%2Fspeed.jpg)

此时执行 pkgs --update 将会把RT007的相关依赖下载到本地。

![shell.jpg](ass%2Fshell.jpg)

我们可以在这里执行wifi的连接，但是这里每次的连接都需要手动执行shell,不是很方便。经过我对代码的追踪，发现命令行中执行wifi连接的命令实际上底层
调用的是 ``wlan_mgnt.c``的代码。

所以我们可以自己写一个初始化函数，放到main function最前面执行。 这样的话我们就可以在代码中连接WiFi了。
```c

    char *ssid = "ImmortalWrt";
    char *password = "mazha1997";
    rt_wlan_scan();
    rt_wlan_connect(ssid, password);
```

之后我们需要在RT-Thread的组件库中找一个MQTT client的组件。

![kawayi.jpg](ass%2Fkawayi.jpg)

经过我的尝试我最终选择了上文中的Kawayi MQTT，执行 ``pkgs --update`` 把新增的包下载到本地。由于我使用的是Clion, 为了不手动添加依赖所以再执
行一下 `scons --target=cmake` 来更新Cmakelist.

我们将这个kawali MQTT 下的test.c 文件拷贝到Application中去，我们在上面做一下修改使其可以连接到我们本地的MQTT服务器上。

![demo.jpg](ass%2Fdemo.jpg)

修改后的test.c, 分别为mqtt_client.c 和 mqtt_clent.h

我们在mqtt_clent.h 上定义一些MQTT的连接信息

**mqtt_clent.h**
```c
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
#define KAWAII_MQTT_PASSWORD           "yourpassword"
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
```

**mqtt_clent.c**

```c

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
   rt_kprintf("\r\n");
   rt_kprintf("topic: %s\nmessage:%s\r\n", msg->topic_name,
              (char *) msg->message->payload);
    rt_kprintf("\r\n");
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

    char payload[37];
    while (1) {
        if (rt_mq_recv(mq, &payload, sizeof(payload), 1000) > 0) {
            mqtt_message_t msg;
            memset(&msg, 0, sizeof(msg));
            msg.qos = QOS1;
            msg.payload = (void *) payload;
            mqtt_publish(mqtt_client, KAWAII_MQTT_PUBTOPIC, &msg);
        }
        rt_thread_delay(1000);

    }

}


```
此时便可以实现MQTT的消息发送和接受。可能你注意到了在消息的接收回调函数上，我这里根据接收的消息进行判断来点亮和关闭LED灯。同时在mqtt_task_entry
里面我写了一个MQTT发布的任务，用于将消息队列内的数据发送到MQTT服务器上。

现象如下，可以接收来自AHT10发送到消息队列的消息，并且推送到MQTT服务器上。
![MQTT.jpg](ass%2FMQTT.jpg)

同时，如果我们在MQTT的这个topic里发送off的话，那么板载的这个led灯将会熄灭。 

![off1.jpg](ass%2Foff1.jpg)

**现象关闭**

![off2.jpg](ass%2Foff2.jpg)

on的话将会开启。

![on1.jpg](ass%2Fon1.jpg)

**现象开启**
![on2.jpg](ass%2Fon2.jpg)



## 文件系统

星火一号板载了一个8Mb的flash，我们可以使用文件系统来快速的读取或者将数据写入到flash里。

1- 在组件中开启对旧版本库的支持
```c
                                                                                                                             RT-Thread Configuration
(2048) Set main thread stack size
(10) Set main thread priority
[*] Support legacy version for compatibility
[*] MSH: command shell  --->
    DFS: device virtual file system  --->
-*- FAL: flash abstraction layer  --->
    Device Drivers  --->
    C/C++ and POSIX layer  --->
    Network  --->
    Memory protection  --->
    Utilities  --->
[ ] VBus: virtual software bus  ----
    Using USB legacy version  --->
[ ] Using fdt legacy version


```

2- 在硬件驱动中开启Flash的支持，开启fal以及file system
```c
[ ] Compatible with Arduino Ecosystem (RTduino)
[*] Enable USB TO USART (uart1)
[ ] Enable COM2 (uart2 pin conflict with Ethernet and PWM)
[ ] Enable COM3 (uart3)
[ ] Enable RS485 (uart6)  ----
[ ] Enable SRAM
[ ] Enable LCD(ST7787)
[ ] Enable Led MATRIX
[ ] Enable LVGL for LCD
-*- Enable SPI FLASH (W25Q64 spi2)
[ ] Enable I2C EEPROM (i2c1)
[ ] Enable Ethernet 28j60
[*] Enable File System  --->
-*- Enable FAL (enable on-chip flash and spi2 flash)
[ ]     Enable bootloader partition table  ----
[ ] Enable Easy Flash base on FAL
[*] Enable Rw007 Wlan Base on SPI2  --->
[*] Enable AHT21(i2c3)
[ ] Enable AP3216C(i2c2)
[ ] Enable ICM20608(i2c2)
[ ] Enable Usb Mouse(usb hid device)
[ ] Enable On Board CAN  ----
[ ] Enable Audio Device  ----
```

3 - 在flash的子目录中开启分区表和自动挂载（实际上自动挂载总是失败）,后面我将会分享解决办法
```c
[ ] Enable SDCARD (FATFS)
[*] Enable FAL filesystem partition base on W25Q64
[*]     Enable filesystem auto mount  ----
```

4- 在RT-Thread components 下的DFS开启 Enable elm-chan fatfs
```c
-*- DFS: device virtual file system
-*-     Using posix-like functions, open/read/write/close
[*]     Using working directory
[ ]     Using mount table for file system
(16)    The maximal number of opened files
        The version of DFS (DFS v1.0)  --->
(4)     The maximal number of mounted file system
(4)     The maximal number of file system type
[*]     Enable elm-chan fatfs
            elm-chan's FatFs, Generic FAT Filesystem Module  --->
-*-     Using devfs for device objects
-*-     Enable ReadOnly file system on flash
[ ]         Use user's romfs root
[ ]     Enable ReadOnly compressed file system on flash
[ ]     Enable RAM file system
[ ]     Enable TMP file system
[ ]     Enable MQUEUE file system
[ ]     Using NFS v3 client file system
```

5- 修改elm-chan's fatfs 下的最大扇区大小为4096bytes
```c
(437) OEM code page
[*] Using RT_DFS_ELM_WORD_ACCESS
    Support long file name (3: LFN with dynamic LFN working buffer on the heap)  --->
    Support unicode for long file name (0: ANSI/OEM in current CP (TCHAR = char))  --->
(255) Maximal size of file name length
(2) Number of volumes (logical drives) to be used.
(4096) Maximum sector size to be handled.
[ ] Enable sector erase feature
[*] Enable the reentrancy (thread safe) of the FatFs module
(3000)  Timeout of thread-safe protection mutex
[ ] Enable RT_DFS_ELM_USE_EXFAT
```

6- 保存，执行 `pkgs --update` 和 `scons --target=cmake` 来更新Cmakelist.

7- 在程序中由于优先级关系我们先disable wifi的spi 片选信号
```c
#define WIFI_CS GET_PIN(F,10)
int disable_wifi()
{
    rt_pin_mode(WIFI_CS, PIN_MODE_OUTPUT);
    rt_pin_write(WIFI_CS, PIN_LOW);
    return 0;
}
```

此时启动系统的话，那么便可以看到文件的挂载信息和分区信息。

```c
 \ | /
- RT -     Thread Operating System
/ | \     5.2.0 build Jul 29 2024 17:53:35
2006 - 2024 Copyright by RT-Thread team
lwIP-2.0.3 initialized!
[I/SFUD] Warning: Read SFDP parameter header information failed. The W25Q64 does not support JEDEC SFDP.
[I/SFUD] Found a Winbond W25Q64CV flash chip. Size is 8388608 bytes.
[I/SFUD] W25Q64 flash device initialized successfully.
[I/SFUD] Probe SPI flash W25Q64 by SPI device spi20 success.
[I/sal.skt] Socket Abstraction Layer initialize success.
[D/FAL] (fal_flash_init:47) Flash device |        onchip_flash_128k | addr: 0x08000000 | len: 0x00100000 | blk_size: 0x00020000 |initialized finish.
[D/FAL] (fal_flash_init:47) Flash device |                   W25Q64 | addr: 0x00000000 | len: 0x00800000 | blk_size: 0x00001000 |initialized finish.
[I/FAL] ==================== FAL partition table ====================
[I/FAL] | name       | flash_dev         |   offset   |    length  |
[I/FAL] -------------------------------------------------------------
[I/FAL] | app        | onchip_flash_128k | 0x00000000 | 0x00060000 |
[I/FAL] | param      | onchip_flash_128k | 0x00060000 | 0x000a0000 |
[I/FAL] | easyflash  | W25Q64            | 0x00000000 | 0x00080000 |
[I/FAL] | download   | W25Q64            | 0x00080000 | 0x00100000 |
[I/FAL] | wifi_image | W25Q64            | 0x00180000 | 0x00080000 |
[I/FAL] | font       | W25Q64            | 0x00200000 | 0x00300000 |
[I/FAL] | filesystem | W25Q64            | 0x00500000 | 0x00300000 |
[I/FAL] =============================================================
[I/FAL] RT-Thread Flash Abstraction Layer initialize success.
[I/FAL] The FAL block device (filesystem) created successfully
[E/app.filesystem] Failed to initialize filesystem!
msh />[E/[RW007]] The wifi Stage 1 status 0 0 0 1
[I/WLAN.dev] wlan init success
[I/WLAN.lwip] eth device init ok name:w0
[I/WLAN.dev] wlan init success
[I/WLAN.lwip] eth device init ok name:w1

rw007  sn: [rw007c745bb22fc584aa6d25a]
rw007 ver: [RW007_2.1.0-a7a0d089-57]

```

但是也许你会注意到，上面的日志提示我们没有正常的挂载文件系统。也就是说自动化挂载失效了。 我们只能手动挂载。那么这样每次使用finsh挂载的话效率太低了
于是我通过追踪finsh 命令的方式找到了MSH的导出指令的代码。从而实现了在主函数上来手动挂载他。

```c
#include "dfs_file.h"
#include "sys/unistd.h"

dfs_mount("filesystem", "/fal", "elm", 0,0);
    chdir("/fal");
```

同理，我也找到了echo写入文件的操作 我们便可以把湿温度信息写入到flash里
```c
 int fd;
    fd = open("1.txt", O_RDWR | O_APPEND | O_CREAT, 0);
    if (fd >= 0)
    {
        write(fd, "hello", strlen("hello"));
        close(fd);
    }
```

效果如下，我这里演示了每次写入hello

```c
Connected MQTT server !

msh /fal>
msh /fal>ls /fa
msh /fal>ls /fal/
Directory /fal/:
1.txt               38                       
msh /fal>ls
Directory /fal:
1.txt               38                       
msh /fal>cat 1
msh /fal>cat 1.txt
1222222\r\n222\r\n222\nhellohellohello
msh /fal>
```