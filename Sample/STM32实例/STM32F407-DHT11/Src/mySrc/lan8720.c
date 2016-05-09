#define DEBUG
#include "debug.h"
#include "drivers.h"
#include "app.h"
#include "api.h"

#include "lan8720.h"

ETH_DMADESCTypeDef *DMARxDscrTab;	//以太网DMA接收描述符数据结构体指针
ETH_DMADESCTypeDef *DMATxDscrTab;	//以太网DMA发送描述符数据结构体指针 
uint8_t *Rx_Buff; 					//以太网底层驱动接收buffers指针 
uint8_t *Tx_Buff; 					//以太网底层驱动发送buffers指针
void eth_int_thread(void *pdata);
#define SYSCFG_OFFSET             (SYSCFG_BASE - PERIPH_BASE)
#define PMC_OFFSET                (SYSCFG_OFFSET + 0x04) 
#define MII_RMII_SEL_BitNumber    ((uint8_t)0x17) 
#define PMC_MII_RMII_SEL_BB       (PERIPH_BB_BASE + (PMC_OFFSET * 32) + (MII_RMII_SEL_BitNumber * 4)) 

//LAN8720初始化
//返回值:0,成功;
//    其他,失败
/** @defgroup SYSCFG_ETHERNET_Media_Interface 
  * @{
  */ 
#define SYSCFG_ETH_MediaInterface_MII    ((uint32_t)0x00000000)
#define SYSCFG_ETH_MediaInterface_RMII   ((uint32_t)0x00000001)

#define IS_SYSCFG_ETH_MEDIA_INTERFACE(INTERFACE) (((INTERFACE) == SYSCFG_ETH_MediaInterface_MII) || \
                                                 ((INTERFACE) == SYSCFG_ETH_MediaInterface_RMII))
extern void eth_int_thread(void *pdata);
u8 LAN8720_Init(void)
{
//	u8 rval=0;
	NVIC_InitTypeDef   NVIC_InitStructure;

	if(ETH_Mem_Malloc())
		return 1;	
	
	SYSCFG_ETH_MediaInterfaceConfig(SYSCFG_ETH_MediaInterface_RMII); //MAC和PHY之间使用RMII接口

	
	gpio_cfg((uint32_t)ETH_MDIO_PORT_GROUP, ETH_MDIO_PIN, GPIO_Mode_AF_PP);
	gpio_cfg((uint32_t)ETH_MDC_PORT_GROUP, ETH_MDC_PIN, GPIO_Mode_AF_PP);
	gpio_cfg((uint32_t)ETH_TXD0_PORT_GROUP, ETH_TXD0_PIN, GPIO_Mode_AF_PP);
	gpio_cfg((uint32_t)ETH_TXD1_PORT_GROUP, ETH_TXD1_PIN, GPIO_Mode_AF_PP);
	gpio_cfg((uint32_t)ETH_TX_EN_PORT_GROUP, ETH_TX_EN_PIN, GPIO_Mode_AF_PP);
	gpio_cfg((uint32_t)ETH_RXD0_PORT_GROUP, ETH_RXD0_PIN, GPIO_Mode_AF_PP);
	gpio_cfg((uint32_t)ETH_RXD1_PORT_GROUP, ETH_RXD1_PIN, GPIO_Mode_AF_PP);
	gpio_cfg((uint32_t)ETH_CRSDV_PORT_GROUP, ETH_CRSDV_PIN, GPIO_Mode_AF_PP);
	gpio_cfg((uint32_t)ETH_RCLK_PORT_GROUP, ETH_RCLK_PIN, GPIO_Mode_AF_PP);
	
	GPIO_PinAFConfig(ETH_MDIO_PORT_GROUP, ETH_MDIO_SOURCE, GPIO_AF_ETH);
	GPIO_PinAFConfig(ETH_MDC_PORT_GROUP, ETH_MDC_SOURCE, GPIO_AF_ETH); 
	GPIO_PinAFConfig(ETH_TXD0_PORT_GROUP, ETH_TXD0_SOURCE, GPIO_AF_ETH); 
	GPIO_PinAFConfig(ETH_TXD1_PORT_GROUP, ETH_TXD1_SOURCE, GPIO_AF_ETH); 
	GPIO_PinAFConfig(ETH_TX_EN_PORT_GROUP, ETH_TX_EN_SOURCE, GPIO_AF_ETH); 
	GPIO_PinAFConfig(ETH_RXD0_PORT_GROUP, ETH_RXD0_SOURCE, GPIO_AF_ETH); 
	GPIO_PinAFConfig(ETH_RXD1_PORT_GROUP, ETH_RXD1_SOURCE, GPIO_AF_ETH); 
	GPIO_PinAFConfig(ETH_CRSDV_PORT_GROUP, ETH_CRSDV_SOURCE, GPIO_AF_ETH); 
	GPIO_PinAFConfig(ETH_RCLK_PORT_GROUP, ETH_RCLK_SOURCE, GPIO_AF_ETH); 


	gpio_cfg((uint32_t)ETH_RESET_PORT_GROUP, ETH_RESET_PIN, GPIO_Mode_Out_PP);
	
	GPIO_CLR(ETH_RESET_PORT_GROUP, ETH_RESET_PIN);
	sleep(50);
	GPIO_SET(ETH_RESET_PORT_GROUP, ETH_RESET_PIN);
	sleep(50);

	NVIC_InitStructure.NVIC_IRQChannel = ETH_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = ETH_IRQn_Priority;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure); 
	
	thread_create(eth_int_thread, 0, TASK_ETH_INT_PRIO, 0, 256, "eth_int_thread");
	return 0;				//ETH的规则为:0,失败;1,成功;所以要取反一下 
}
//得到8720的速度模式
//返回值:
//001:10M半双工
//101:10M全双工
//010:100M半双工
//110:100M全双工
//其他:错误.
u8 LAN8720_Get_Speed(void)
{
	u8 speed;
	speed=((ETH_ReadPHYRegister(0x00,31)&0x1C)>>2); //从LAN8720的31号寄存器中读取网络速度和双工模式
	return speed;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
//以下部分为STM32F407网卡配置/接口函数.

//初始化ETH MAC层及DMA配置
//返回值:ETH_ERROR,发送失败(0)
//		ETH_SUCCESS,发送成功(1)
u8 ETH_MACDMA_Config(void)
{
	u8 rval;
	ETH_InitTypeDef1 ETH_InitStructure; 
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_ETH_MAC | RCC_AHB1Periph_ETH_MAC_Tx |
                        RCC_AHB1Periph_ETH_MAC_Rx, ENABLE);
	
	ETH_DeInit();  								//AHB总线重启以太网
	ETH_SoftwareReset();  						//软件重启网络
	while (ETH_GetSoftwareResetStatus() == SET);//等待软件重启网络完成 
	ETH_StructInit(&ETH_InitStructure); 	 	//初始化网络为默认值  

	///网络MAC参数设置 
	ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;   			//开启网络自适应功能
	ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Disable;					//关闭反馈
	ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Disable; 		//关闭重传功能kp
	ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable; 	//关闭自动去除PDA/CRC功能 
	ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Enable;						//关闭接收所有的帧
	ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable;//允许接收所有广播帧
	ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;			//关闭混合模式的地址过滤  
	ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;//对于组播地址使用完美地址过滤   
	ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect;	//对单播地址使用完美地址过滤 
#ifdef CHECKSUM_BY_HARDWARE
	ETH_InitStructure.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable; 			//开启ipv4和TCP/UDP/ICMP的帧校验和卸载   
#endif
	//当我们使用帧校验和卸载功能的时候，一定要使能存储转发模式,存储转发模式中要保证整个帧存储在FIFO中,
	//这样MAC能插入/识别出帧校验值,当真校验正确的时候DMA就可以处理帧,否则就丢弃掉该帧
	ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable; //开启丢弃TCP/IP错误帧
	ETH_InitStructure.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;     //开启接收数据的存储转发模式    
	ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;   //开启发送数据的存储转发模式  

	ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Disable;     	//禁止转发错误帧  
	ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Disable;	//不转发过小的好帧 
	ETH_InitStructure.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Enable;  		//打开处理第二帧功能
	ETH_InitStructure.ETH_AddressAlignedBeats = ETH_AddressAlignedBeats_Enable;  	//开启DMA传输的地址对齐功能
	ETH_InitStructure.ETH_FixedBurst = ETH_FixedBurst_Enable;            			//开启固定突发功能    
	ETH_InitStructure.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;     		//DMA发送的最大突发长度为32个节拍   
	ETH_InitStructure.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;			//DMA接收的最大突发长度为32个节拍
	ETH_InitStructure.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_2_1;
	ETH_InitStructure.Sys_Clock_Freq= 168; //168;//系统时钟频率为168Mhz
	rval=ETH_Init(&ETH_InitStructure,LAN8720_PHY_ADDRESS);//配置ETH
	if(rval==ETH_SUCCESS)//配置成功
	{
		ETH_DMAITConfig(ETH_DMA_IT_NIS|ETH_DMA_IT_R,ENABLE);//使能以太网接收中断	
	}
	return rval;
}
extern void lwip_packet_handler(void);		//在lwip_comm.c里面定义
//以太网中断服务函数



extern wait_event_t eth_int_event;
extern char eth_int_pending;
void ETH_IRQHandler(void)
{ 
	NVIC_DisableIRQ(ETH_IRQn);
	if(!eth_int_pending && eth_int_event){
		eth_int_pending = 1;
		wake_up(eth_int_event);
	}
}  


//接收一个网卡数据包
//返回值:网络数据包帧结构体
FrameTypeDef ETH_Rx_Packet(void)
{ 
	u32 framelength=0;
	FrameTypeDef frame={0,0};   
	//检查当前描述符,是否属于ETHERNET DMA(设置的时候)/CPU(复位的时候)
	if((DMARxDescToGet->Status&ETH_DMARxDesc_OWN)!=(u32)RESET)
	{	
		frame.length=ETH_ERROR; 
		if ((ETH->DMASR&ETH_DMASR_RBUS)!=(u32)RESET)  
		{ 
			ETH->DMASR = ETH_DMASR_RBUS;//清除ETH DMA的RBUS位 
			ETH->DMARPDR=0;//恢复DMA接收
		}
		return frame;//错误,OWN位被设置了
	}  
	if(((DMARxDescToGet->Status&ETH_DMARxDesc_ES)==(u32)RESET)&& 
	((DMARxDescToGet->Status & ETH_DMARxDesc_LS)!=(u32)RESET)&&  
	((DMARxDescToGet->Status & ETH_DMARxDesc_FS)!=(u32)RESET))  
	{       
		framelength=((DMARxDescToGet->Status&ETH_DMARxDesc_FL)>>ETH_DMARxDesc_FrameLengthShift)-4;//得到接收包帧长度(不包含4字节CRC)
 		frame.buffer = DMARxDescToGet->Buffer1Addr;//得到包数据所在的位置
	}else framelength=ETH_ERROR;//错误  
	frame.length=framelength; 
	frame.descriptor=DMARxDescToGet;  
	//更新ETH DMA全局Rx描述符为下一个Rx描述符
	//为下一次buffer读取设置下一个DMA Rx描述符
	DMARxDescToGet=(ETH_DMADESCTypeDef*)(DMARxDescToGet->Buffer2NextDescAddr);   
	return frame;  
}
//发送一个网卡数据包
//FrameLength:数据包长度
//返回值:ETH_ERROR,发送失败(0)
//		ETH_SUCCESS,发送成功(1)
u8 ETH_Tx_Packet(u16 FrameLength)
{   
	//检查当前描述符,是否属于ETHERNET DMA(设置的时候)/CPU(复位的时候)
	if((DMATxDescToSet->Status&ETH_DMATxDesc_OWN)!=(u32)RESET)return ETH_ERROR;//错误,OWN位被设置了 
 	DMATxDescToSet->ControlBufferSize=(FrameLength&ETH_DMATxDesc_TBS1);//设置帧长度,bits[12:0]
	DMATxDescToSet->Status|=ETH_DMATxDesc_LS|ETH_DMATxDesc_FS;//设置最后一个和第一个位段置位(1个描述符传输一帧)
  	DMATxDescToSet->Status|=ETH_DMATxDesc_OWN;//设置Tx描述符的OWN位,buffer重归ETH DMA
	if((ETH->DMASR&ETH_DMASR_TBUS)!=(u32)RESET)//当Tx Buffer不可用位(TBUS)被设置的时候,重置它.恢复传输
	{ 
		ETH->DMASR=ETH_DMASR_TBUS;//重置ETH DMA TBUS位 
		ETH->DMATPDR=0;//恢复DMA发送
	} 
	//更新ETH DMA全局Tx描述符为下一个Tx描述符
	//为下一次buffer发送设置下一个DMA Tx描述符 
	DMATxDescToSet=(ETH_DMADESCTypeDef*)(DMATxDescToSet->Buffer2NextDescAddr);    
	return ETH_SUCCESS;   
}
//得到当前描述符的Tx buffer地址
//返回值:Tx buffer地址
u32 ETH_GetCurrentTxBuffer(void)
{  
  return DMATxDescToSet->Buffer1Addr;//返回Tx buffer地址  
}
//为ETH底层驱动申请内存
//返回值:0,正常
//    其他,失败
u8 ETH_Mem_Malloc(void)
{ 
	DMARxDscrTab=mem_malloc(ETH_RXBUFNB*sizeof(ETH_DMADESCTypeDef));//申请内存
	DMATxDscrTab=mem_malloc(ETH_TXBUFNB*sizeof(ETH_DMADESCTypeDef));//申请内存  
	Rx_Buff=mem_malloc(ETH_RX_BUF_SIZE*ETH_RXBUFNB);	//申请内存
	Tx_Buff=mem_malloc(ETH_TX_BUF_SIZE*ETH_TXBUFNB);	//申请内存
	if(!DMARxDscrTab||!DMATxDscrTab||!Rx_Buff||!Tx_Buff)
	{
		ETH_Mem_Free();
		return 1;	//申请失败
	}	
	return 0;		//申请成功
}
//释放ETH 底层驱动申请的内存
void ETH_Mem_Free(void)
{ 
	if(DMARxDscrTab)
	mem_free(DMARxDscrTab);//释放内存
	if(DMATxDscrTab)
	mem_free(DMATxDscrTab);//释放内存
	if(Rx_Buff)
	mem_free(Rx_Buff);		//释放内存
	if(Tx_Buff)
	mem_free(Tx_Buff);		//释放内存  
}























