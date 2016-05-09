#define DEBUG
#include "debug.h"
#include "drivers.h"
#include "app.h"
#include "api.h"

#include "lan8720.h"

ETH_DMADESCTypeDef *DMARxDscrTab;	//��̫��DMA�������������ݽṹ��ָ��
ETH_DMADESCTypeDef *DMATxDscrTab;	//��̫��DMA�������������ݽṹ��ָ�� 
uint8_t *Rx_Buff; 					//��̫���ײ���������buffersָ�� 
uint8_t *Tx_Buff; 					//��̫���ײ���������buffersָ��
void eth_int_thread(void *pdata);
#define SYSCFG_OFFSET             (SYSCFG_BASE - PERIPH_BASE)
#define PMC_OFFSET                (SYSCFG_OFFSET + 0x04) 
#define MII_RMII_SEL_BitNumber    ((uint8_t)0x17) 
#define PMC_MII_RMII_SEL_BB       (PERIPH_BB_BASE + (PMC_OFFSET * 32) + (MII_RMII_SEL_BitNumber * 4)) 

//LAN8720��ʼ��
//����ֵ:0,�ɹ�;
//    ����,ʧ��
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
	
	SYSCFG_ETH_MediaInterfaceConfig(SYSCFG_ETH_MediaInterface_RMII); //MAC��PHY֮��ʹ��RMII�ӿ�

	
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
	return 0;				//ETH�Ĺ���Ϊ:0,ʧ��;1,�ɹ�;����Ҫȡ��һ�� 
}
//�õ�8720���ٶ�ģʽ
//����ֵ:
//001:10M��˫��
//101:10Mȫ˫��
//010:100M��˫��
//110:100Mȫ˫��
//����:����.
u8 LAN8720_Get_Speed(void)
{
	u8 speed;
	speed=((ETH_ReadPHYRegister(0x00,31)&0x1C)>>2); //��LAN8720��31�żĴ����ж�ȡ�����ٶȺ�˫��ģʽ
	return speed;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
//���²���ΪSTM32F407��������/�ӿں���.

//��ʼ��ETH MAC�㼰DMA����
//����ֵ:ETH_ERROR,����ʧ��(0)
//		ETH_SUCCESS,���ͳɹ�(1)
u8 ETH_MACDMA_Config(void)
{
	u8 rval;
	ETH_InitTypeDef1 ETH_InitStructure; 
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_ETH_MAC | RCC_AHB1Periph_ETH_MAC_Tx |
                        RCC_AHB1Periph_ETH_MAC_Rx, ENABLE);
	
	ETH_DeInit();  								//AHB����������̫��
	ETH_SoftwareReset();  						//�����������
	while (ETH_GetSoftwareResetStatus() == SET);//�ȴ��������������� 
	ETH_StructInit(&ETH_InitStructure); 	 	//��ʼ������ΪĬ��ֵ  

	///����MAC�������� 
	ETH_InitStructure.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;   			//������������Ӧ����
	ETH_InitStructure.ETH_LoopbackMode = ETH_LoopbackMode_Disable;					//�رշ���
	ETH_InitStructure.ETH_RetryTransmission = ETH_RetryTransmission_Disable; 		//�ر��ش�����kp
	ETH_InitStructure.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable; 	//�ر��Զ�ȥ��PDA/CRC���� 
	ETH_InitStructure.ETH_ReceiveAll = ETH_ReceiveAll_Enable;						//�رս������е�֡
	ETH_InitStructure.ETH_BroadcastFramesReception = ETH_BroadcastFramesReception_Enable;//����������й㲥֡
	ETH_InitStructure.ETH_PromiscuousMode = ETH_PromiscuousMode_Disable;			//�رջ��ģʽ�ĵ�ַ����  
	ETH_InitStructure.ETH_MulticastFramesFilter = ETH_MulticastFramesFilter_Perfect;//�����鲥��ַʹ��������ַ����   
	ETH_InitStructure.ETH_UnicastFramesFilter = ETH_UnicastFramesFilter_Perfect;	//�Ե�����ַʹ��������ַ���� 
#ifdef CHECKSUM_BY_HARDWARE
	ETH_InitStructure.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable; 			//����ipv4��TCP/UDP/ICMP��֡У���ж��   
#endif
	//������ʹ��֡У���ж�ع��ܵ�ʱ��һ��Ҫʹ�ܴ洢ת��ģʽ,�洢ת��ģʽ��Ҫ��֤����֡�洢��FIFO��,
	//����MAC�ܲ���/ʶ���֡У��ֵ,����У����ȷ��ʱ��DMA�Ϳ��Դ���֡,����Ͷ�������֡
	ETH_InitStructure.ETH_DropTCPIPChecksumErrorFrame = ETH_DropTCPIPChecksumErrorFrame_Enable; //��������TCP/IP����֡
	ETH_InitStructure.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;     //�����������ݵĴ洢ת��ģʽ    
	ETH_InitStructure.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;   //�����������ݵĴ洢ת��ģʽ  

	ETH_InitStructure.ETH_ForwardErrorFrames = ETH_ForwardErrorFrames_Disable;     	//��ֹת������֡  
	ETH_InitStructure.ETH_ForwardUndersizedGoodFrames = ETH_ForwardUndersizedGoodFrames_Disable;	//��ת����С�ĺ�֡ 
	ETH_InitStructure.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Enable;  		//�򿪴���ڶ�֡����
	ETH_InitStructure.ETH_AddressAlignedBeats = ETH_AddressAlignedBeats_Enable;  	//����DMA����ĵ�ַ���빦��
	ETH_InitStructure.ETH_FixedBurst = ETH_FixedBurst_Enable;            			//�����̶�ͻ������    
	ETH_InitStructure.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;     		//DMA���͵����ͻ������Ϊ32������   
	ETH_InitStructure.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;			//DMA���յ����ͻ������Ϊ32������
	ETH_InitStructure.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_2_1;
	ETH_InitStructure.Sys_Clock_Freq= 168; //168;//ϵͳʱ��Ƶ��Ϊ168Mhz
	rval=ETH_Init(&ETH_InitStructure,LAN8720_PHY_ADDRESS);//����ETH
	if(rval==ETH_SUCCESS)//���óɹ�
	{
		ETH_DMAITConfig(ETH_DMA_IT_NIS|ETH_DMA_IT_R,ENABLE);//ʹ����̫�������ж�	
	}
	return rval;
}
extern void lwip_packet_handler(void);		//��lwip_comm.c���涨��
//��̫���жϷ�����



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


//����һ���������ݰ�
//����ֵ:�������ݰ�֡�ṹ��
FrameTypeDef ETH_Rx_Packet(void)
{ 
	u32 framelength=0;
	FrameTypeDef frame={0,0};   
	//��鵱ǰ������,�Ƿ�����ETHERNET DMA(���õ�ʱ��)/CPU(��λ��ʱ��)
	if((DMARxDescToGet->Status&ETH_DMARxDesc_OWN)!=(u32)RESET)
	{	
		frame.length=ETH_ERROR; 
		if ((ETH->DMASR&ETH_DMASR_RBUS)!=(u32)RESET)  
		{ 
			ETH->DMASR = ETH_DMASR_RBUS;//���ETH DMA��RBUSλ 
			ETH->DMARPDR=0;//�ָ�DMA����
		}
		return frame;//����,OWNλ��������
	}  
	if(((DMARxDescToGet->Status&ETH_DMARxDesc_ES)==(u32)RESET)&& 
	((DMARxDescToGet->Status & ETH_DMARxDesc_LS)!=(u32)RESET)&&  
	((DMARxDescToGet->Status & ETH_DMARxDesc_FS)!=(u32)RESET))  
	{       
		framelength=((DMARxDescToGet->Status&ETH_DMARxDesc_FL)>>ETH_DMARxDesc_FrameLengthShift)-4;//�õ����հ�֡����(������4�ֽ�CRC)
 		frame.buffer = DMARxDescToGet->Buffer1Addr;//�õ����������ڵ�λ��
	}else framelength=ETH_ERROR;//����  
	frame.length=framelength; 
	frame.descriptor=DMARxDescToGet;  
	//����ETH DMAȫ��Rx������Ϊ��һ��Rx������
	//Ϊ��һ��buffer��ȡ������һ��DMA Rx������
	DMARxDescToGet=(ETH_DMADESCTypeDef*)(DMARxDescToGet->Buffer2NextDescAddr);   
	return frame;  
}
//����һ���������ݰ�
//FrameLength:���ݰ�����
//����ֵ:ETH_ERROR,����ʧ��(0)
//		ETH_SUCCESS,���ͳɹ�(1)
u8 ETH_Tx_Packet(u16 FrameLength)
{   
	//��鵱ǰ������,�Ƿ�����ETHERNET DMA(���õ�ʱ��)/CPU(��λ��ʱ��)
	if((DMATxDescToSet->Status&ETH_DMATxDesc_OWN)!=(u32)RESET)return ETH_ERROR;//����,OWNλ�������� 
 	DMATxDescToSet->ControlBufferSize=(FrameLength&ETH_DMATxDesc_TBS1);//����֡����,bits[12:0]
	DMATxDescToSet->Status|=ETH_DMATxDesc_LS|ETH_DMATxDesc_FS;//�������һ���͵�һ��λ����λ(1������������һ֡)
  	DMATxDescToSet->Status|=ETH_DMATxDesc_OWN;//����Tx��������OWNλ,buffer�ع�ETH DMA
	if((ETH->DMASR&ETH_DMASR_TBUS)!=(u32)RESET)//��Tx Buffer������λ(TBUS)�����õ�ʱ��,������.�ָ�����
	{ 
		ETH->DMASR=ETH_DMASR_TBUS;//����ETH DMA TBUSλ 
		ETH->DMATPDR=0;//�ָ�DMA����
	} 
	//����ETH DMAȫ��Tx������Ϊ��һ��Tx������
	//Ϊ��һ��buffer����������һ��DMA Tx������ 
	DMATxDescToSet=(ETH_DMADESCTypeDef*)(DMATxDescToSet->Buffer2NextDescAddr);    
	return ETH_SUCCESS;   
}
//�õ���ǰ��������Tx buffer��ַ
//����ֵ:Tx buffer��ַ
u32 ETH_GetCurrentTxBuffer(void)
{  
  return DMATxDescToSet->Buffer1Addr;//����Tx buffer��ַ  
}
//ΪETH�ײ����������ڴ�
//����ֵ:0,����
//    ����,ʧ��
u8 ETH_Mem_Malloc(void)
{ 
	DMARxDscrTab=mem_malloc(ETH_RXBUFNB*sizeof(ETH_DMADESCTypeDef));//�����ڴ�
	DMATxDscrTab=mem_malloc(ETH_TXBUFNB*sizeof(ETH_DMADESCTypeDef));//�����ڴ�  
	Rx_Buff=mem_malloc(ETH_RX_BUF_SIZE*ETH_RXBUFNB);	//�����ڴ�
	Tx_Buff=mem_malloc(ETH_TX_BUF_SIZE*ETH_TXBUFNB);	//�����ڴ�
	if(!DMARxDscrTab||!DMATxDscrTab||!Rx_Buff||!Tx_Buff)
	{
		ETH_Mem_Free();
		return 1;	//����ʧ��
	}	
	return 0;		//����ɹ�
}
//�ͷ�ETH �ײ�����������ڴ�
void ETH_Mem_Free(void)
{ 
	if(DMARxDscrTab)
	mem_free(DMARxDscrTab);//�ͷ��ڴ�
	if(DMATxDscrTab)
	mem_free(DMATxDscrTab);//�ͷ��ڴ�
	if(Rx_Buff)
	mem_free(Rx_Buff);		//�ͷ��ڴ�
	if(Tx_Buff)
	mem_free(Tx_Buff);		//�ͷ��ڴ�  
}























