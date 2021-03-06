/*********************************************************************
 *
 *             J1939 Include File
 *
 *********************************************************************
 * 文件名:        J1939.c
 *
 *	本程序是由XieTongXueFlyMe对现有的J1939协议文档分析，和对前辈的贡献总结和封装，写出
 *的一套开源的J1939驱动。
 *	本协议特点：
 *		1.易移植（不针对特定的CAN硬件，只要满足CAN2.0B即可）
 *		2.轻量级（可适应低端的MCU）
 *		3.支持多任务调用接口（可用于嵌入式系统）
 *		4.双模式（轮询或者中断，逻辑更加简单明了）
 *		5.不掉帧（数据采用收发列队缓存）
 *	协议参考文献：
 *		1.SAE J1939 J1939概述
 *		2.SAE J1939-01 卡车，大客车控制通信文档（大概的浏览J1939协议的用法）
 *		3.SAE J1939-11 物理层文档
 *		4.SAE J1939-13 物理层文档
 *		5.SAE J1939-15 物理层文档
 *		6.SAE J1939-21 数据链路层文档（定义信息帧的数据结构，编码规则）
 *		7.SAE J1939-31 网络层文档（定义网络层的链接协议）
 *		8.SAE J1939-71 应用层文档（定义常用物理参数格式）
 *		9.SAE J1939-73 应用层文档（用于故障诊断）
 *		10.SAE J1939-74 应用层文档（可配置信息）
 *		11.SAE J1939-75 应用层文档（发电机组和工业设备）
 *		12.SAE J1939-81 网络管理协议
 *
 *  源代码分析网址：
 *		http://blog.csdn.net/xietongxueflyme/article/details/74908563
 *
 *
 * Version     Date        Description
 * ----------------------------------------------------------------------
 * v1.00     2017/06/04    首个版本
 *
 *
 * Author               Date         changes
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *XieTongXueFlyMe       7/06/04      首个版本
 **********************************************************************/
#ifndef __J1939_H 
#define __J1939_H 
 
typedef enum _BOOL { FALSE = 0, TRUE } BOOL; 
 
//函数返回代码
#define RC_SUCCESS			    0  //成功
#define RC_QUEUEEMPTY			1  //列队为空
#define RC_QUEUEFULL			1  //列队满
#define RC_CANNOTRECEIVE		2  //不能接收
#define RC_CANNOTTRANSMIT		2  //不能传输
#define RC_PARAMERROR			3  //参数错误
 
//内部常量
#define J1939_FALSE				0 
#define J1939_TRUE				1 
 
// J1939 默认的优先级（参考J1939文档）
#define J1939_CONTROL_PRIORITY			0x03 
#define J1939_INFO_PRIORITY			    0x06 
#define J1939_PROPRIETARY_PRIORITY		0x06 
#define J1939_REQUEST_PRIORITY			0x06 
#define J1939_ACK_PRIORITY			    0x06 
#define J1939_TP_CM_PRIORITY			0x07 
#define J1939_TP_DT_PRIORITY			0x07 
 
// J1939 定义的地址
#define J1939_GLOBAL_ADDRESS			255 
#define J1939_NULL_ADDRESS			    254 
 
// Some J1939 PDU Formats, Control Bytes, and PGN's 
#define J1939_PF_REQUEST2			201      //请求
#define J1939_PF_TRANSFER			202      //转移

#define J1939_PF_REQUEST			234      //请求，用提供握手机制
#define J1939_PF_ACKNOWLEDGMENT			232  //确认请求，用提供握手机制

#define J1939_ACK_CONTROL_BYTE			0    //代表确认
#define J1939_NACK_CONTROL_BYTE			1    //PNG不被支持。否定消息
#define J1939_ACCESS_DENIED_CONTROL_BYTE	2//拒绝访问，但是信息是被支持
#define J1939_CANNOT_RESPOND_CONTROL_BYTE	3//不能做出反应，有空能是接受的缓存不够 
 
#define J1939_PF_DT				        235	    // 协议传输---数据传输
#define J1939_PF_TP_CM				    236		// 协议传输---链接管理

#define J1939_RTS_CONTROL_BYTE			16		// CM消息的请求发送控制字节
#define J1939_CTS_CONTROL_BYTE			17		// 清除发送控制字节的CM消息
#define J1939_EOMACK_CONTROL_BYTE		19
#define J1939_BAM_CONTROL_BYTE			32
#define J1939_CONNABORT_CONTROL_BYTE		255	// 连接中断控制字节 of CM message 
 
//与J1939网络层有关的定义 
#define J1939_PGN2_REQ_ADDRESS_CLAIM		0x00 
#define J1939_PGN1_REQ_ADDRESS_CLAIM		0xEA   
#define J1939_PGN0_REQ_ADDRESS_CLAIM		0x00 
 
#define J1939_PGN2_COMMANDED_ADDRESS		0x00 
#define J1939_PGN1_COMMANDED_ADDRESS		0xFE	//命令地址消息 
#define J1939_PGN0_COMMANDED_ADDRESS		0xD8    //参考J1939-81 地址命令配置
 
#define J1939_PF_ADDRESS_CLAIMED		238
#define J1939_PF_CANNOT_CLAIM_ADDRESS	238
#define J1939_PF_PROPRIETARY_A			239         //专用A
#define J1939_PF_PROPRIETARY_B			255         //专用B
 

// J1939 Data Structures 
// J1939_MESSAGE_STRUCT旨在J1939消息块映射到设备的地址映射。 只有字段PDU格式不映射到设备寄存器。
// 结构应该简单地使用PDUFormat和忽视PDUFormat_Top。调整将立即接收和传输之前。
// 注:编译器创建结构从低一点的位置高一些位置，所以可能出现不匹配的设备寄存器。
#define J1939_MSG_LENGTH	5  //消息长度
#define J1939_DATA_LENGTH	8  //数据长度
 
union J1939_MESSAGE_UNION 
{ 
	struct   me 
	{ 
		unsigned int	DataPage			: 1; 
		unsigned int	Res					: 1; 
		unsigned int	Priority			: 3; 
		unsigned int	PDUFormat_Top		: 3;	// 这就需要前后处理。
		unsigned char	PDUFormat;					// CA should use only PDUFormat. 
		unsigned char	PDUSpecific; 
		unsigned char	SourceAddress; 
		unsigned int	DataLength 			: 4; 
		unsigned int	RTR					: 4;	// RTR bit, value always 0x00 
		unsigned char	Data[J1939_DATA_LENGTH]; 
	};
	struct me Mxe;
	unsigned char		Array[J1939_MSG_LENGTH + J1939_DATA_LENGTH]; 
}; 

#define GroupExtension 		PDUSpecific 
#define DestinationAddress 	PDUSpecific 
typedef union J1939_MESSAGE_UNION J1939_MESSAGE; 

union J1939_FLAGS_UNION
{ 
	
	struct 
	{ 
		unsigned int	CannotClaimAddress				: 1; 
		unsigned int	WaitingForAddressClaimContention: 1; 
		unsigned int	GettingCommandedAddress			: 1; 
		unsigned int	GotFirstDataPacket				: 1; 
		unsigned int	ReceivedMessagesDropped			: 1; 
   }; 
	  unsigned char		FlagVal; 
}; 
typedef union J1939_FLAGS_UNION J1939_FLAG; 
 
// 向外部声明的全局变量
extern unsigned char	CA_Name[J1939_DATA_LENGTH]; 
extern unsigned char 	J1939_Address; 
extern J1939_FLAG    	J1939_Flags; 
extern J1939_MESSAGE    OneMessage;  
extern unsigned char	RXQueueCount; //RX 列队计数器
 

#ifdef J1939_ACCEPT_CMDADD 
extern void  J1939_CommandedAddressHandling( void );
#endif 
//出列 用于接受 *MsgPtr代表要存入的缓存
extern unsigned char	J1939_DequeueMessage( J1939_MESSAGE *MsgPtr );
//入列 用于发送 *MsgPtr代表要发送的缓存
extern unsigned char  	J1939_EnqueueMessage( J1939_MESSAGE *MsgPtr );
//初始化
extern void 			J1939_Initialization( BOOL );
//中断入口
extern void			    J1939_ISR( void );
//扫描入口
extern void 			J1939_Poll( unsigned long ElapsedTime );
 
#ifdef  __J1939_SOURCE 
static void 		    J1939_ReceiveMessages( void );
static void 		    J1939_RequestForAddressClaimHandling( void );
static unsigned char 	J1939_TransmitMessages( void );
static void 		    J1939_AddressClaimHandling( unsigned char Mode );
#endif 
 
#endif


 
