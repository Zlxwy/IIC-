#include "my_IIC.h"
#include "stm32f10x.h"                  // Device header
#include "delay.h"
#include "sys.h"

//如果要更改IIC外设、对应的引脚，在这里改就行了
#define my_IIC_Port_RCC			RCC_APB2Periph_GPIOB
#define my_IIC_PORT					GPIOB

#define my_IIC_SCL					GPIO_Pin_6
#define Write_my_SCL				PBout(6)

#define my_IIC_SDA					GPIO_Pin_7
#define Write_my_SDA				PBout(7)
#define Read_my_SDA					PBin(7)

/**指定地址写一个字节
	*@process	产生起始信号→发送要呼叫的从机地址+写操作→发送所呼叫从机的寄存器地址→发送要写的数据→产生停止信号
	*@param		从机的7位地址（靠右，读写位还没留空）
	*@param		从机的寄存器地址
	*@param		要写入的数据
**/
void my_IIC_WriteByte(u8 EquiAddr, u8 RegAddr, u8 Data)
{
	EquiAddr = (EquiAddr<<1)&0xFE;	//加上读写位，这里都是写操作
	
	my_IIC_START();									//产生起始信号
	my_IIC_SendByte(EquiAddr);				//发送要呼叫的从机地址+写操作
	my_IIC_WaitAck();							//接收应答
	
	my_IIC_SendByte(RegAddr);				//发送所呼叫从机的寄存器地址
	my_IIC_WaitAck();							//接收应答
	
	my_IIC_SendByte(Data);						//发送要写的数据
	my_IIC_WaitAck();							//接收应答
	my_IIC_STOP();										//产生停止信号
}

/**指定地址写多个字节
	*@param		从机的7位地址（靠右，读写位还没留空）
	*@param		从机的寄存器地址
	*@param		存放写入数据的数组
	*@param		写入数据的长度
	**/
void my_IIC_ScanWrite(u8 EquiAddr,u8 RegAddr,u8 *SendArray,u8 len)
{
	EquiAddr = (EquiAddr<<1)&0xFE;	//加上读写位，这里都是写操作
	
	my_IIC_START();									//产生起始信号
	my_IIC_SendByte(EquiAddr);				//发送要呼叫的从机地址+写操作
	my_IIC_WaitAck();							//接收应答
	
	my_IIC_SendByte(RegAddr);				//发送所呼叫从机的寄存器地址
	my_IIC_WaitAck();							//接收应答
	
	while(len--)
	{
		my_IIC_SendByte(*(SendArray++));	//发送要写的数据
		my_IIC_WaitAck();							//接收应答
	}
	my_IIC_STOP();										//产生停止信号
}

/**指定地址读一个字节
	*@process	产生起始信号→发送要呼叫的从机地址+写操作→发送所呼叫从机的寄存器地址↓
	*@process	重新产生起始信号→发送要呼叫的从机地址+读操作→读取一个字节并给非应答→产生停止信号→获取数据寄存器内容并将其返回
	*@param		从机的7位地址（靠右，读写位还没留空）
	*@param		从机的寄存器地址
	*@return	读取的数据
**/
u8 my_IIC_ReadByte(u8 EquiAddr, u8 RegAddr)
{
	u8
		EquiAddr_write = (EquiAddr<<1)&0xFE,
		//含有读写位（写）的从机地址（把最后一位置0）
		EquiAddr_read = (EquiAddr<<1)|0x01,
		//含有读写位（读）的从机地址（把最后一位置1）
		Data;
		//接收字节存放
	
	my_IIC_START();									//产生起始信号
	my_IIC_SendByte(EquiAddr_write);	//发送要呼叫的从机地址+写操作
	my_IIC_WaitAck();							//接收应答
	
	my_IIC_SendByte(RegAddr);				//发送所呼叫从机的寄存器地址
	my_IIC_WaitAck();							//接收应答
	
	my_IIC_START();									//重新产生起始条件
	my_IIC_SendByte(EquiAddr_read);	//发送要呼叫的从机地址+读操作
	my_IIC_WaitAck();							//接收应答
	
	Data = my_IIC_ReceiveByte();			//读取数据
	my_IIC_NACK();										//给从机非应答
	my_IIC_STOP();										//产生停止信号
	return Data;
}

/**指定地址读多个字节
	*@param		从机的7位地址（靠右，读写位还没留空）
	*@param		从机的寄存器地址
	*@param		存放读取数据的数组
	*@param		读取数据的长度
	**/
void my_IIC_ScanRead(u8 EquiAddr,u8 RegAddr,u8 *GetArray,u8 len)
{
	u8
		EquiAddr_write = (EquiAddr<<1)&0xFE,
		//含有读写位（写）的从机地址（把最后一位置0）
		EquiAddr_read = (EquiAddr<<1)|0x01;
		//含有读写位（读）的从机地址（把最后一位置1）
	
	my_IIC_START();									//产生起始信号
	my_IIC_SendByte(EquiAddr_write);	//发送要呼叫的从机地址+写操作
	my_IIC_WaitAck();							//接收应答
	
	my_IIC_SendByte(RegAddr);				//发送所呼叫从机的寄存器地址
	my_IIC_WaitAck();							//接收应答
	
	my_IIC_START();									//重新产生起始条件
	my_IIC_SendByte(EquiAddr_read);	//发送要呼叫的从机地址+读操作
	my_IIC_WaitAck();							//接收应答
	while(len--)
	{
		*(GetArray++)=my_IIC_ReceiveByte();	//数组不断位置自增
		if(len!=0)	my_IIC_ACK();						//还没读完，给应答继续读
		else				my_IIC_NACK();						//最后一个了，给非应答不继续读了
	}
	my_IIC_STOP();										//产生停止信号
}

/**除了停止信号，每一个时序都会在最后拉低SCL，以便下一个时序直接就能开始**/
void my_IIC_delay(void)
{
	//delay_us(2);
	//DDDDDRBJBSHCJZZBSM
}

//产生起始信号
void my_IIC_START(void)
{
	Write_my_SDA=1;
	Write_my_SCL=1;
	
	Write_my_SDA=0;
	my_IIC_delay();	//等待，让从机分清是哪个先拉低的
	Write_my_SCL=0;
	my_IIC_delay();	//等待，让从机响应起始信号
}

//产生停止信号
void my_IIC_STOP(void)
{
	Write_my_SCL=0;
	Write_my_SDA=0;
	
	Write_my_SCL=1;
	my_IIC_delay();	//等待，让从机分清是哪个先松开的
  Write_my_SDA=1;
	my_IIC_delay();	//等待下让从机响应停止信号
}

//产生应答
void my_IIC_ACK(void)
{
	Write_my_SDA=0;
	my_IIC_delay();	//等待让STM32的SDA引脚能完全改变
	Write_my_SCL=1;
	my_IIC_delay();	//等待让从机读取
	Write_my_SCL=0;
}

//产生非应答
void my_IIC_NACK(void)
{
	Write_my_SDA=1;
	my_IIC_delay();	//等待，让STM32的SDA引脚能完全松开
	Write_my_SCL=1;
	my_IIC_delay();	//等待让从机读取
	Write_my_SCL=0;
}

//接收应答位
u8 my_IIC_WaitAck(void)
{
	u8 waittime;
	Write_my_SDA=1;					//主机释放SDA，避免干扰从机的数据发送
	my_IIC_delay();					//等待，让从机能及时改变SDA电平
	Write_my_SCL=1;					//主机释放SCL，主机在SCL高电平期间读取SDA
	while(Read_my_SDA)
	{
		waittime++;
		if(waittime>250)
		{
			Write_my_SCL=0;
			return 1;		//循环了250次检查还是没有应答，则直接返回1
		}
	}
	Write_my_SCL=0;
	return 0;				//循环中接收到了应答，则会跳出循环，返回0
}

//单纯发送一个字节，不是指定地址写
void my_IIC_SendByte(u8 Byte)
{
	for (u8 i=0;i<8;i++)				//循环8次，主机依次发送数据的每一位
	{
		Write_my_SDA=(Byte>>(7-i))&0x01;
		//由最高位到最低位依次发送
		my_IIC_delay();							//等待，让SDA线能完全改变电平
		Write_my_SCL=1;							//释放SCL，从机将在SCL高电平期间读取SDA
		my_IIC_delay();							//等待让SCL能完全松开，并且从机读取到数据
		Write_my_SCL=0;							//拉低SCL，主机开始发送下一位数据
	}
}

//单纯接收一个字节，不是指定地址读
u8 my_IIC_ReceiveByte(void)
{
	u8 Byte = 0x00;					//定义接收的数据
	Write_my_SDA=1;						//接收前，主机先确保释放SDA，避免干扰从机的数据发送
	for (u8 i=0;i<8;i++)		//循环8次，主机依次接收数据的每一位
	{
		my_IIC_delay();						//等待让从机能完全改变电平
		Write_my_SCL=1;						//释放SCL，主机机在SCL高电平期间读取SDA
		
		Byte |= Read_my_SDA<<(7-i);//读取SDA数据，并存储到Byte变量
		
		Write_my_SCL=0;						//拉低SCL，从机在SCL低电平期间写入SDA
	}
	return Byte;						//返回接收到的一个字节数据
}

//软件IIC使用引脚初始化
void my_IIC_Init(void)
{
	RCC_APB2PeriphClockCmd(my_IIC_Port_RCC, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Pin = my_IIC_SCL | my_IIC_SDA;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(my_IIC_PORT, &GPIO_InitStructure);
	
	GPIO_SetBits(my_IIC_PORT, my_IIC_SCL | my_IIC_SDA);
}
