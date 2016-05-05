# ZE_FreeRTOS_SDK `[freeRTOS V8.2.1+LwIp]`
采用Eclipse Paho MQTT C/C++ Client，兼容[V3.1 MQTT协议](http://mqtt.org/documentation)和[V3.1.1 MQTT协议](http://mqtt.org/documentation)。

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
``Network``构造函数包含以下几个参数：

`my_socket`

`*mqttread`

`*mqttwrite`

`*disconnect`

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
``Client``构造函数包含以下几个参数：
###### MQTTPacket_connectData
```c
typedef struct
{
	/** The eyecatcher for this structure.  must be MQTC. */
	char struct_id[4];
	/** The version number of this structure.  Must be 0 */
	int struct_version;
	/** Version of MQTT to be used.  3 = 3.1 4 = 3.1.1
	  */
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
``MessageData``构造函数包含以下几个参数：

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
``MQTTMessage``构造函数包含以下几个参数：

##### NewNetwork / ConnectNetwork / MQTTConnect / MQTTDisconnect
###### NewNetwork()
```c
void NewNetwork(Network* n)
```
###### ConnectNetwork()
```c
int ConnectNetwork(Network* n, char* addr, int port)
```
###### MQTTConnect()
```c
int MQTTConnect(Client* c, MQTTPacket_connectData* options)
```
###### MQTTDisconnect
```c
int MQTTDisconnect(Client* c)
```
##### Network loop
###### MQTTYield
```c
int MQTTYield(Client* c, int timeout_ms)
```

##### Publishing
###### MQTTPublish
```c
int MQTTPublish(Client* c, const char* topicName, MQTTMessage* message)
```

##### Subscribe
###### MQTTSubscribe
```c
int MQTTSubscribe(Client* c, const char* topicFilter, enum QoS qos, messageHandler messageHandler)
```

- TCPClt：#174 - #218是主循环，实际是一个线程的执行函数，NewNetwork完成适配，ConnectNetwork检查TCP/IP栈是否就需并实施TCPIP连接，MQTTClient为MQTT连接准备空间（此处使用静态变量），MQTTConnect完成MQTT服务器登录，MQTTSubscribe注册感兴趣的话题（下行命令接收）,PublishLED实际通过MQTTPublish发送数据包，MQTTYield需要经常性调用以完成MQTT事务操作（例如检查是否有数据包等待处理，数据包的回应，KeepAlive的处理等）。MQTTDisconnect释放MQTT连接
- PublishLED是一个使用JSON格式封装数据并发送的函数;
- messageArrived供回调。当收到数据包以后，本函数被调用。这儿示范了一个对JSON解包的过程（并没有根据最新的格式调整），仅供参考;
- #10-#28 包括了常量定义。实际使用中，需要把相关ID和Token改为从网站上申请的数据。缓冲区也在此定义，请根据实际使用大小调整，以免溢出;
- JSON相关函数从堆中申请内存，请保证堆中有充足空间。

