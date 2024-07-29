#include "aht_10.h"
#include "mqttclient.h"

//I2C_BUS�豸ָ�룬���ڵȻ�Ѱ�����¼AHT���ص�I2C����
struct rt_i2c_bus_device *i2c_bus = RT_NULL;
extern rt_mailbox_t mailbox;

extern rt_mq_t mq;
//AHT����Ŀղ���
rt_uint8_t Parm_Null[2] = {0, 0};



//д���������ӻ��������ݣ�
rt_err_t write_reg(struct rt_i2c_bus_device *Device, rt_uint8_t reg, rt_uint8_t *data) {
    //��д�������
    //�����СΪ3��ԭ��buf[0]--����������AHT_CALIBRATION_CMD��AHT_NORMAL_CMD��AHT_GET_DATA_CMD
    //                  buf[1]/buf[2]Ϊ�������Ĳ�����AHT��Щ���������Ҫ���ϲ���������ɲ鿴�����ֲ�
    rt_uint8_t buf[3];

    //��¼�����С
    rt_uint8_t buf_size;

    //I2C��������ݽṹ��
    struct rt_i2c_msg msgs;

    buf[0] = reg;
    if (data != RT_NULL) {
        buf[1] = data[0];
        buf[2] = data[1];
        buf_size = 3;
    } else {
        buf_size = 1;
    }

    msgs.addr = AHT_ADDR;   //��ϢҪ���͵ĵ�ַ����AHT��ַ
    msgs.flags = RT_I2C_WR; //��Ϣ�ı�־λ��������д���Ƿ���Ҫ����ACK��Ӧ���Ƿ���Ҫ����ֹͣλ���Ƿ���Ҫ���Ϳ�ʼλ(����ƴ������ʹ��)...
    msgs.buf = buf;         //��Ϣ�Ļ�������������/���յ�����
    msgs.len = buf_size;    //��Ϣ�Ļ�������С��������/���յ�����Ĵ�С

    //������Ϣ
    //����i2c.core���ṩ����������APIȥ����I2C�����ݴ��ݣ�
    /*
     * 1.����API
     * rt_size_t rt_i2c_master_send(struct rt_i2c_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             const rt_uint8_t         *buf,
                             rt_uint32_t               count)

       2.����API
       rt_size_t rt_i2c_master_recv(struct rt_i2c_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             rt_uint8_t               *buf,
                             rt_uint32_t               count)
       3.����API
       rt_size_t rt_i2c_transfer(struct rt_i2c_bus_device *bus,
                          struct rt_i2c_msg         msgs[],
                          rt_uint32_t               num)
      * ʵ����1��2��󶼻���û�3����ҿ��԰����Լ�������е���
    */
    if (rt_i2c_transfer(Device, &msgs, 1) == 1) {
        return RT_EOK;
    } else {
        return RT_ERROR;
    }
}

//�����ݣ��ӻ��������������ݣ�
rt_err_t read_reg(struct rt_i2c_bus_device *Device, rt_uint8_t len, rt_uint8_t *buf) {
    struct rt_i2c_msg msgs;

    msgs.addr = AHT_ADDR;       //��ϢҪ���͵ĵ�ַ����AHT��ַ
    msgs.flags = RT_I2C_RD;     //��Ϣ�ı�־λ��������д���Ƿ���Ҫ����ACK��Ӧ���Ƿ���Ҫ����ֹͣλ���Ƿ���Ҫ���Ϳ�ʼλ(����ƴ������ʹ��)...
    msgs.buf = buf;             //��Ϣ�Ļ�������������/���յ�����
    msgs.len = len;             //��Ϣ�Ļ�������С��������/���յ�����Ĵ�С

    //���亯���������н���
    if (rt_i2c_transfer(Device, &msgs, 1) == 1) {
        return RT_EOK;
    } else {
        return RT_ERROR;
    }
}

//��ȡAHT����ʪ������
void read_temp_humi(float *Temp_Data, float *Humi_Data) {
    //���������ֲ����ǿ��Կ���Ҫ��ȡһ��������Ҫʹ�õ��������СΪ6
    rt_uint8_t Data[6];


    write_reg(i2c_bus, AHT_GET_DATA_CMD, Parm_Null);      //����һ����ȡ�����AHT����һ�����ݲɼ�
    rt_thread_mdelay(500);                          //�ȴ��ɼ�
    read_reg(i2c_bus, 6, Data);                     //��ȡ����

    //���������ֲ�������ݴ���
    *Humi_Data = (Data[1] << 12 | Data[2] << 4 | (Data[3] & 0xf0) >> 4) * 100.0 / (1 << 20);
    *Temp_Data = ((Data[3] & 0x0f) << 16 | Data[4] << 8 | Data[5]) * 200.0 / (1 << 20) - 50;
}

//AHT���г�ʼ��
void AHT_Init(const char *name) {
    //Ѱ��AHT�������豸
    i2c_bus = rt_i2c_bus_device_find(name);
    if (i2c_bus == RT_NULL) {
        rt_kprintf("Can't Find I2C_BUS Device");    //�Ҳ��������豸
    } else {
        write_reg(i2c_bus, AHT_NORMAL_CMD, Parm_Null);    //����Ϊ��������ģʽ
        rt_thread_mdelay(400);

        rt_uint8_t Temp[2];     //AHT_CALIBRATION_CMD��Ҫ�Ĳ���
        Temp[0] = 0x08;
        Temp[1] = 0x00;
        write_reg(i2c_bus, AHT_CALIBRATION_CMD, Temp);
        rt_thread_mdelay(400);
    }

}

//AHT�豸�����߳�
void AHT_test_entry(void *args) {
    float humidity, temperature;
    char payload[37];
    AHT_Init(AHT_I2C_BUS_NAME);     //�����豸��ʼ��
    while (1) {
        read_temp_humi(&temperature, &humidity);    //��ȡ����
        // ����
//        rt_mb_send(mailbox, humidity);
        snprintf(payload, sizeof(payload), "{\"humidity\":%.1f,\"temperature\":%.1f}", humidity, temperature);
        rt_mq_send(mq, payload, sizeof(payload));

//        rt_kprintf("humidity   : %d.%d %%\r\n", (int) humidity, (int) (humidity * 10) % 10);
//        rt_kprintf("temperature: %d.%d\r\n", (int) temperature, (int) (temperature * 10) % 10);
        rt_thread_mdelay(1000);
    }

}


