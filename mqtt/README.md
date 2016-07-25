# J1ST C-core for the FreeRTOS+TCP
J1ST FreeRTOS SDK ([Source](https://github.com/zenin-tech/HW-c-core/tree/master/freertos)) 是[及时云](http://j1st.io)提供的适用于 FreeRTOS+TCP 特定平台的同步 MQTT 客户端库。及时云MQTT Server 支持所有以 "MQTT V3.1/3.1.1标准协议" 通讯的客户端连接，只需客户端符合  [J1ST.IO Communication Specification(MQTT)](https://github.com/zenin-tech/J1ST.IO/wiki/J1ST.IO-Communication-Specification(MQTT)) 会话通讯规范。若您未了解我们的产品，请先阅读[及时云入门指导](http://j1st.io/views/quickstart.html)。

## 说明
- `freertos/` 目录结构
	- `inc/`: *J1ST FreeRTOS SDK* Head files
	- `src/`: *J1ST FreeRTOS SDK* Source files
	- `sample/`: *ClientEcho.c* Client sample files
- MQTT同步客户端库
	- 集成cJson，即将支持 SSL/TLS 连接
	- 适用FreeRTOS+TCP特定平台
	- 基于 [Eclipse Paho](http://www.eclipse.org/paho/) 项目的C/C++ MQTT Embedded clients开发，在EPL 1.0协议下发布
- 示例
	- 运行于STM32429ZI-Nucleo开发板，已移植LwIP的RTOS实现，实现相关以太网（或Wi-Fi）芯片的LwIP驱动程序，可使用LwIP协议的POSIX接口进行TCP/IP通信
	- 基于 [MQTT V3.1.1](http://mqtt.org/) 传输协议，会话通讯规范请查看  [J1ST.IO Communication Specification(MQTT)](https://github.com/zenin-tech/J1ST.IO/wiki/J1ST.IO-Communication-Specification(MQTT))
	- 实现功能
		- 接入J1ST.IO平台及相关协议的勤务操作
		- 生成符合J1ST.IO会话通讯规范的报文，并定时（或按需）上传数据到平台
		- 订阅有关话题名并解析平台下发指令
	- MQTT Server相关参数
		- 硬件连接许可参数（[Developer Console](http://developer.j1st.io/)获取）
	
		| Section | Value |
		|--------|--------|
		| DEFAULTPORT   | Use `1883`, which is provided by J1ST.IO |
		| DEFAULTHOST   | Use `developer.j1st.io`, which is provided by J1ST.IO |
		| DEFAULTAGENT  | Use `AGENT_ID`, which is provided by J1ST.IO (Need replaced) |
		| DEFAULTTOKEN  | Use `AGENT_TOKEN`, which is provided by J1ST.IO (Need replaced)|
	
		- 符合及时云会话通讯规范的 "PUBLISH" 和 "SUBSCRIBE" 的主题名格式
		```
		PUBLISH:  agents/{AGENT_ID}/upstream
		```
		```
		SUBSCRIBE:  agents/{AGENT_ID}/downstream
		```

## 更多信息
- API Reference，详细文档参考[*J1ST FreeRTOS SDK v1.0.0 api*(TODO)](https://github.com/zenin-tech/HW-c-core/wiki/J1ST-FreeRTOS-SDK-api-(en))

- 更多完整工程可查看 [HW-freertos-nucleo](https://github.com/zenin-tech/HW-freertos-nucleo) 或 [HW-freertos-esp8266](https://github.com/zenin-tech/HW-freertos-esp8266) 

- J1ST.IO 控制台，请到 [Developer Console](http://developer.j1st.io/) 获取更多帮助 

--------------------------------------------
# User Manual
*此文档专用于FreeRTOS Client*

# J1ST FreeRTOS SDK 1.0.0
## Hello J1st!
J1ST FreeRTOS SDK ([Source](https://github.com/zenin-tech/HW-c-core/tree/master/freertos)) 是[及时云](http://j1st.io)提供的适用于 FreeRTOS+TCP 特定平台的同步 MQTT 客户端库。及时云MQTT Server 支持所有以 "MQTT V3.1/3.1.1标准协议" 通讯的客户端连接，只需客户端符合  [J1ST.IO Communication Specification(MQTT)](https://github.com/zenin-tech/J1ST.IO/wiki/J1ST.IO-Communication-Specification(MQTT)) 会话通讯规范。若您未了解我们的产品，请先阅读[及时云入门指导](http://j1st.io/views/quickstart.html)。

*FreeRTOS+TCP 备注*
- SDK 使用 POSIX 接口进行TCP/IP通信，其内没初始化 TCP/IP 协议栈，所以预计在使用前您须自行初始化 TCP/IP 协议栈

- 测试环境为已移植LwIP的RTOS实现并实现以太网芯片的LwIP驱动程序，可使用LwIP协议的POSIX接口进行TCP/IP通信的STM32429ZI-Nucleo开发板

- 相关API详细文档可参考 [*J1ST FreeRTOS SDK v1.0.0 api*](https://github.com/zenin-tech/HW-c-core/wiki/J1ST-FreeRTOS-SDK-api-(en))

- 提供的不同硬件平台下的完整工程项目，可查看 [HW-freertos-nucleo](https://github.com/zenin-tech/HW-freertos-nucleo) 或 [HW-freertos-esp8266](https://github.com/zenin-tech/HW-freertos-esp8266)

## 下载源码
您可选择使用 `git clone` 命令下载源码 [https://github.com/zenin-tech/HW-c-core](https://github.com/zenin-tech/HW-c-core) 到本地目录，或从 [https://github.com/zenin-tech/HW-c-core/releases](https://github.com/zenin-tech/HW-c-core/releases) 下载源码压缩文件，如放置于`~/HW-c-core`。

```
$git clone https://github.com/zenin-tech/HW-c-core.git
```
-----

***以下代码样例来自 [/freertos/sample/ClientEcho.c](https://github.com/zenin-tech/HW-c-core/blob/master/freertos/sample/ClientEcho.c)。***

## 连接许可授权
通讯控制器（Agent）即客户端基于 [MQTT V3.1.1](http://mqtt.org/) 传输通讯协议，与J1ST.IO平台进行基于 [J1ST.IO Communication Specification(MQTT)](https://github.com/zenin-tech/J1ST.IO/wiki/J1ST.IO-Communication-Specification(MQTT)) 会话通讯规范进行数据传输。

首先为运行的客户端添加硬件连接许可才能使用及时云MQTT Server 的服务。从 [Developer Console](http://developer.j1st.io/) 控制台页面免费申请并创建一个`Product`获取已授权的 { AGENT_ID } 以及 { AGENT_TOKEN } 信息，如`{id:xxx,token:xxx}`的内容，按照以下提示替换样例代码内容。

修改项：

```c
#define DEFAULTAGENT "577362d56097e92f1f677ffb"      //Replace here of id
#define DEFAULTTOKEN "RGhgdpJZCuaThFUUbvodthAnnrLzbsCi"   //Replace here of token
```
*注：此处id即 { AGENT_ID } ，token即 { AGENT_TOKEN }，仅能同时提供给一台硬件设备使用并运行。*

## 样例解析
#### 入口函数
入口函数 vStartMQTTTasks()，新建主任务 prvMQTTEchoTask() 做相关 MQTT 客户端操作。

```c
void vStartMQTTTasks(uint16_t usTaskStackSize, UBaseType_t uxTaskPriority)
{
    BaseType_t x = 0L;

    xTaskCreate(prvMQTTEchoTask,    /* The function that implements the task. */
        "MQTTEcho0",    /* Just a text name for the task to aid debugging. */
        usTaskStackSize,    /* The stack size suggestion is 512. */
        (void *)x,    /* The task parameter, not used in this case. */
        uxTaskPriority,    /* The priority assigned to the task suggestion is 3. */
        NULL);    /* The task handle is not used. */
	
    xPubQueue = xQueueCreate(6, 2);
}
```

#### Topic Name主题名生成
按照规范生成 Topic Name，用于上传 Publish 报文以及订阅相关主题名并接收 Publish 报文。

```c
sprintf(gTopicDown, "agents/%s/downstream", gAgent);
sprintf(gTopicUp, "agents/%s/upstream", gAgent);
```

#### Client初始化
客户端与及时云MQTT Server进行MQTT通信，调用jNetInit()初始化socket通道。

```c
jNet * pJnet = jNetInit();
if (NULL == pJnet)
{
    printf("Cannot allocate jnet resources.");
    return;
}
```

#### Agent连接平台
调用jNetConnect()接口并设置相关重连机制（设备断线之后也将回到此处）。其中服务器（Server）参数为`host="developer.j1st.io", port=1883`，客户端参数为 { AGENT_ID } 以及 { AGENT_TOKEN } 。

```c
rc = jNetConnect(pJnet, gHost, gPort, gAgent, gToken);
if (rc != 0 ) 
{
    /*rc: No IP address or stack not ready = -1, OKDONE = 0 , AGENT_ID & AGENT_TOKEN is authorized = 5*/
    printf("Cannot connect to :%s, rc: %d. \n", gHost, rc);
    vTaskDelay(1000);
    continue;
}
printf("Connect to J1ST.IO server %s:%d succeeded.\n", gHost, gPort);
```

#### Agent订阅Topic
调用jNetSubscribeT()函数进行`gTopicDown`的订阅操作，采用回调机制将从平台发送到通讯控制器（Agent）的Publish 报文中的有效载荷（Pub Payload）数据传输到处理函数`messageHandler`里。
- 由于采用回调处理，所以需要先声明jNetSubscribeT()接口
    
    ```c
    extern int jNetSubscribeT(jNet *, const char *, enum QoS, messageHandler);
    ```
- 注意尽量避免处理函数`messageHandler`中调用 J1ST.IO Network Functions，包括jNetConnect()、jNetDisconnect()、jNetYield()、jNetPublishT()、jNetSubscribeT()等操作
    
    ```c
    rc = jNetSubscribeT(pJnet, gTopicDown, QOS2, messageArrived);
    if (rc != 0) goto clean;
    printf("Subscribe the topic of \"%s\" result %d.\n", gTopicDown, rc);
    ```

#### Agent接收消息
- 客户端订阅指定的 Topic 之后，即可接收在用户或平台发出该 Topic 的下行控制消息，订阅`gTopicDown`时添加了 `messageArrived`的处理函数句柄
    
    ```c
    void messageArrived(MessageData* data)
    {
        printf("Message arrived on topic %.*s: %.*s\n", data->topicName->lenstring.len,
            data->topicName->lenstring.data,data->message->payloadlen, data->message->payload);
    }
    ```
- 下行控制消息数据格式在 [Developer Console](developer.j1st.io) 配置 FnButton 功能时生成，以 { Fn_Code } 作为封装的起始字段，下行控制消息（如控制上传间隔）有效载荷（Pub Payload）的数据格式样例如下，可由`cJson`模块解析
   
    ```json
    {
        "SetInterval":
        [{
            "hwid":"577362d56097e92f1f677ffb",
            "sec":300
        }]
    }
    ```

#### Agent提交数据
- 调用jNetPublishT()发送 PUBLISH 报文至指定的 `gTopicUp`到及时云MQTT Server
    
    ```c
    int publishData(jNet *pJnet, int upstreamId)
    {
        int rc;
    
        cJSON *root, *son1, *son2;
        char *out;
    
        root = cJSON_CreateArray();
        switch(upstreamId)
        {
            /*查看代码*/
        }
    
        out=cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
    
        rc = jNetPublishT(pJnet, gTopicUp, out);
        if(rc == 0)
            printf("Published on topic %s: %s, result %d.\n", gTopicUp, out, rc);
        free(out);
        return rc;
    }
    ```
- 提交数据的有效载荷（Pub Payload）的数据格式如下，可由`cJson`模块生成
    
    ```json
    [
        {
            "hwid": "577362d56097e92f1f677ffb",
            "type": "AGENT",
            "values": {
                "testMessage": "Hello J1ST!"
            }
        }
    ]
    ```
- 定时（或按需）提交数据到平台
    
    ```c
    if(publishData(pJnet, 1) != 0) goto clean;
    xQueueSendToBack(xPubQueue, &rc, 0);
    
    do
    {
        /*Demand sending data*/
        short sock;
        if (xQueueReceive(xPubQueue, &sock, 1) == pdPASS)
        {
            printf("Rcvd: xPubQueue %d.\n", sock);
            if (publishData(pJnet, sock)) goto clean;
        }
        /* Make jNet library do background tasks, send and receive messages(PING/PONG) regularly every 1 sec */
        rc = jNetYield(pJnet);
        if (rc < 0) break;
    }  while (!gFinish);
    printf("Stopping...\n");
    ```

#### Agent断开连接
调用jNetDisconnect()进行断线操作，断线操作后调用jNetFree()释放初始化socket通道时申请的内存。

```c
jNetDisconnect(pJnet);
```
```c
jNetFree(pJnet);
```

## 更多信息
- API Reference，详细文档参考[*J1ST FreeRTOS SDK v1.0.0 api*(TODO)](https://github.com/zenin-tech/HW-c-core/wiki/J1ST-FreeRTOS-SDK-api-(en))

- 更多完整工程可查看 [HW-freertos-nucleo](https://github.com/zenin-tech/HW-freertos-nucleo) 或 [HW-freertos-esp8266](https://github.com/zenin-tech/HW-freertos-esp8266) 

- J1ST.IO 控制台，请到 [Developer Console](http://developer.j1st.io/) 获取更多帮助
