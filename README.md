# ZE_FreeRTOS_SDK `[freeRTOS V8.2.1+LwIp]`
采用Eclipse Paho MQTT C/C++ Client，兼容[V3.1 MQTT协议](http://mqtt.org/documentation)和[V3.1.1 MQTT协议](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/csprd02/mqtt-v3.1.1-csprd02.html)。

集成cJson包。

[ZE_FreeRTOS_SDK](https://github.com/zjykymt/ZE_FreeRTOS_SDK.git)使用说明：

- 开发板上已成功移植了LwIP的RTOS实现，实现了相关以太网（或Wi-Fi）芯片的LwIP驱动程序，可以使用LwIP协议的POSIX接口进行TCP/IP通信，有此基础的开发板才可以使用到[ZE_FreeRTOS_SDK](https://github.com/zjykymt/ZE_FreeRTOS_SDK.git)。

- 基于以C语言开发，实现了RTOS MQTT v3.1.1协议，适用于M2M和物联网应用。

- 本MQTT同步客户端基于paho项目的C/C++ MQTT Embedded clients开发，在EPL 1.0协议下发布。

- MQTT服务器平台采用[j1st.io](http://j1st.io/)或[j1st.io测试](http://139.198.0.174:3000/signin)，注册属于你的账号并很好的使用它。

- 本MQTT同步客户端示例程序环境：

   >- STM32Cube_FW_F4_V1.11.0（包含FreeRToS v8.2.1, LwIP）等
   >- MDK V5.14
   >- 硬件环境：STM32F407，RMII口连接DP83846PHY

## 目录
- Installation
- Usage and API
    - Getting Started
    - Constructor
    - NewNetwork / ConnectNetwork / MQTTConnect / MQTTDisconnect
    - Network loop
    - Publishing
    - Subscribe / Unsubscribe
- cJson
- Reporting bugs
- More information

### Installation
下载库函数，Inc文件夹下包含.h文件，Src文件夹下包含.c文件，而mywork.c是此MQTT库的一个实用示例；

将这个包解压，并将`Inc`和`Src`文件夹添加入您的工程；

其中文件包括：
- cJSON.c, cJSON.h，这两个文件是JSON编解码包，最初期，你可以不管它们；
- 其它文件是MQTT库，MQTTClient.c是MQTT协议的接口库，MQTTLwIP是MQTT的LwIP/FreeRToS适配；
- mywork.c是ZE_FreeRTOS_SDK的一个实用示例，供参考。

### Usage and API
#### Getting Started
使用ZE_FreeRTOS_SDK只需在你的代码中导入\"cJSON.h\"和\"MQTTClient.h\"即可调用所有的API;
```c
#include "cJSON.h"
#include "MQTTClient.h"
```
你可以使用SDK封装的API来实现相关的MQTT操作，如下（详细说明在后面）：

-  使用`` NewNetwork() ``完成初始化网络适配
-  使用`` ConnectNetwork() ``检查TCP/IP栈是否就需并实施TCPIP连接
-  使用`` MQTTClient() `` 为MQTT连接准备空间（此处使用静态变量），创建一个``MQTTClient``
-  使用`` MQTTConnect() ``完成MQTT服务器登录
-  使用`` MQTTSubscribe() `` 去订阅一个topic然后接收``MQTTMessages``及消息处理
-  使用`` MQTTPublish() `` 去发送数据包
-  经常性调用`` MQTTYield() `` 以完成MQTT事务操作（例如检查是否有数据包等待处理，数据包的回应，KeepAlive的处理等）
-  使用`` MQTTDisconnect() `` 释放MQTT连接

##### Constructor
###### Network
```c
struct Network
{
	int my_socket;
	int (*mqttread) (Network*, unsigned char*, int, int);
	int (*mqttwrite) (Network*, unsigned char*, int, int);
	void (*disconnect) (Network*);
};
```

###### Client
```c
struct Client {
    unsigned int next_packetid;
    unsigned int command_timeout_ms;
    size_t buf_size, readbuf_size;
    unsigned char *buf;  
    unsigned char *readbuf; 
    unsigned int keepAliveInterval;
    char ping_outstanding;
    int isconnected;

    struct MessageHandlers
    {
        const char* topicFilter;
        void (*fp) (MessageData*);
    } messageHandlers[MAX_MESSAGE_HANDLERS];      // Message handlers are indexed by subscription topic
    
    void (*defaultMessageHandler) (MessageData*);
    
    Network* ipstack;
    Timer ping_timer;
};
```

###### MQTTPacket_connectData
```c
typedef struct
{
	/** The eyecatcher for this structure.  must be MQTC. */
	char struct_id[4];
	/** The version number of this structure.  Must be 0 */
	int struct_version;
	/** Version of MQTT to be used.  3 = 3.1 4 = 3.1.1 */
	unsigned char MQTTVersion;
	MQTTString clientID;
	unsigned short keepAliveInterval;
	unsigned char cleansession;
	unsigned char willFlag;
	MQTTPacket_willOptions will;
	MQTTString username;
	MQTTString password;
} MQTTPacket_connectData;
```

###### MessageData
```c
struct MessageData
{
    MQTTMessage* message;
    MQTTString* topicName;
};
```

###### MQTTMessage
```c
struct MQTTMessage
{
    enum QoS qos;
    char retained;
    char dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
};
```

##### NewNetwork / ConnectNetwork / MQTTConnect / MQTTDisconnect
###### NewNetwork()
准备paho库所需要调用的操作系统API接口。
```c
void NewNetwork(Network* n)
```

###### ConnectNetwork()
创建TCP端口，并连接到指定的服务器。
```c
int ConnectNetwork(Network* n, char* addr, int port)
```

###### MQTTConnect()
执行MQTT连接操作。
```c
int MQTTConnect(Client* c, MQTTPacket_connectData* options)
```

###### MQTTDisconnect
执行MQTT的断开操作。注意：在连接断开后，需另外执行close(handle)操作。
```c
int MQTTDisconnect(Client* c)
```

##### Network loop
###### MQTTYield
循环指定时间，进行MQTT勤务操作。主循环应定期频繁调用该函数。
```c
int MQTTYield(Client* c, int timeout_ms)
```

##### Publishing
###### MQTTPublish
向指定topic发送数据。MQTTMessage是C++语言风格的字符串，可以包含任意ASCII字符（包括字符0），但是必须显式给出字符串的长度。库函数已经妥善封装，以保证数据包ID的自增。
```c
int MQTTPublish(Client* c, const char* topicName, MQTTMessage* message)
```

##### Subscribe
###### MQTTSubscribe
执行MQTT订阅操作。当收到相应的订阅内容的时候，messageHandle将被调用。注意：在本实现中，每个MQTTAgent最多只能订阅MAX_MESSAGE_HANDLERS个主题。
```c
int MQTTSubscribe(Client* c, const char* topicFilter, enum QoS qos, messageHandler messageHandler)
```

### cJson
cJSON库，具体使用请参见[cJSON](https://github.com/DaveGamble/cJSON)库主页。
