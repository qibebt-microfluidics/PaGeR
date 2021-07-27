//----------------------------------------------
//名称：温度控制系统
//----------------------------------------------
//说明：略
//----------------------------------------------

#include <reg51.h>
#include <intrins.h>
//#include <temp_c.h>
#define uchar unsigned char
#define uint  unsigned int
#define delayNOP() {_nop_();_nop_();_nop_();_nop_();}

sbit DQ        = P1^2;
sbit DQ1       = P1^3;
sbit BUTTON    = P1^4;
sbit STP1_LED  = P1^5;
sbit STP2_LED  = P1^6;
sbit STP3_LED  = P1^7;
sbit LCD_RS    = P2^0;
sbit LCD_RW    = P2^1;
sbit LCD_EN    = P2^2;
sbit HEATER1   = P2^3;
sbit HEATER2   = P2^4;
sbit LASER     = P2^5;

uchar Current_Time_Display_Buffer[]      = {" CD-Time:   M  S"};
uchar Current_Temp_Display_Buffer[]      = {" C-Temp :"}; 
uchar Welcome_Buffer[]                   = {"    Welcome!"};
uchar Next_Step_Buffer[]                 = {" PLS. Press BUT."};
uchar Ask_Buffer[]                       = {"    If Ready"};

uchar code Temperature_Char[8]           = {0x0c,0x12,0x12,0x0C,0x00,0x00,0x00,0x00}; 
uchar code df_Table[]                    = {0,1,1,2,3,3,4,4,5,6,6,7,8,8,9,9 };
uchar CurrentT                           = 0;
uchar GoalT                              = 0;
uchar CDTM                               = 0;
uchar CDTS                               = 0;
uchar Flag                               = 0;
uchar Set_Time                           = 0;
uchar T_Count                            = 0;
uchar PH                                 = 0;
uchar Temp_Value[]                       = {0x00,0x00};
uchar Display_Digit[]                    = {0,0,0,0};
uchar Display_Time[]                     = {0,0,0,0};
bit DS18B20_IS_OK                        = 1;


//-----------------------------------------------
//延时1-us
//-----------------------------------------------
void DelayXus(int x)
{
	uchar i;
	while ( x-- ) for( i=0; i<200; i++);
}
//-----------------------------------------------------------------
// 延时2-ms
//-----------------------------------------------------------------
void delay_ms(uint ms)
{
	uchar i; while(ms--) for(i = 0; i<120; i++);
}

//----------------------------------------------
//LCD忙等待
//----------------------------------------------
bit LCD_Busy_Check()
{
	bit result;
	LCD_RS      = 0;
	LCD_RW      = 1;
	LCD_EN      = 1;
	delayNOP();
	result      = (bit)( P0 & 0x80 );
  LCD_EN      = 0;
  return result;	
}

//----------------------------------------------
//LCD写指令
//----------------------------------------------
void Write_LCD_Command(uchar cmd)
{
	while(LCD_Busy_Check());
	LCD_RS      = 0;
	LCD_RW      = 0;
	LCD_EN      = 0;  _nop_(); _nop_();
	P0          =cmd; delayNOP();
	LCD_EN      = 1;  delayNOP(); LCD_EN      = 0;
}

//----------------------------------------------
//LCD写数据
//----------------------------------------------
void Write_LCD_Data(uchar dat)
{
	while(LCD_Busy_Check());
	LCD_RS      = 1;
	LCD_RW      = 0;
	LCD_EN      = 0; 
	P0          =dat; delayNOP();
	LCD_EN      = 1;  delayNOP(); LCD_EN      = 0;
}

//----------------------------------------------
//LCD初始化显示
//----------------------------------------------
void LCD_Initialise()
{
	Write_LCD_Command(0x01); DelayXus(5);
	Write_LCD_Command(0x38); DelayXus(5);
	Write_LCD_Command(0x01); DelayXus(5);
	Write_LCD_Command(0x0C); DelayXus(5);
	Write_LCD_Command(0x06); DelayXus(5);
}

//----------------------------------------------
//LCD设置显示位置
//----------------------------------------------
void Set_LCD_POS(uchar pos)
{
	Write_LCD_Command(pos | 0x80);
}

//----------------------------------------------
//延时3
//----------------------------------------------
void Delay(uint x)
{
	while(--x);
}

//----------------------------------------------
//初始化18B20-0
//----------------------------------------------
uchar Init_DS18B20()
{
	uchar status;
	DQ = 1; Delay(8); DQ = 0; Delay(90); DQ = 1; Delay(8);
	status = DQ; Delay(100);
	DQ = 1; 
	return status;
}

//----------------------------------------------
//初始化18B20-1
//----------------------------------------------
uchar Init_DS18B20_1()
{
	uchar status;
	DQ1 = 1; Delay(8); DQ1 = 0; Delay(90); DQ1 = 1; Delay(8);
	status = DQ1; Delay(100);
	DQ1 = 1; 
	return status;
}

//----------------------------------------------
//读字节18B20
//----------------------------------------------
uchar ReadOneByte()
{
	uchar i;
	uchar dat;
	i = 0;
	dat = 0;
	DQ = 1; _nop_();
	for (i = 0; i < 8; i++)
	{
		DQ = 0; dat >>= 1; DQ = 1; _nop_();_nop_();
		if(DQ) dat |= 0x80;
		Delay(30);
		DQ = 1;
	}
	return dat;
}

//----------------------------------------------
//读字节18B20-1
//----------------------------------------------
uchar ReadOneByte_1()
{
	uchar i;
	uchar dat;
	i = 0;
	dat = 0;
	DQ1 = 1; _nop_();
	for (i = 0; i < 8; i++)
	{
		DQ1 = 0; dat >>= 1; DQ1 = 1; _nop_();_nop_();
		if(DQ1) dat |= 0x80;
		Delay(30);
		DQ1 = 1;
	}
	return dat;
}

//----------------------------------------------
//写字节18B20
//----------------------------------------------
void WriteOneByte(uchar dat)
{
	uchar i;
	for(i = 0; i < 8; i++)
	{
		DQ = 0; DQ = dat & 0x01; Delay(5); DQ = 1; dat>>=1;
	}
}

//----------------------------------------------
//写字节18B20-1
//----------------------------------------------
void WriteOneByte_1(uchar dat)
{
	uchar i;
	for(i = 0; i < 8; i++)
	{
		DQ1 = 0; DQ1 = dat & 0x01; Delay(5); DQ1 = 1; dat>>=1;
	}
}

//----------------------------------------------
//读温度值18B20
//----------------------------------------------
void Read_Temperature()
{
	if( Init_DS18B20() == 1)
		DS18B20_IS_OK = 0;
	else
	{
		WriteOneByte(0xCC);
		WriteOneByte(0x44);
		Init_DS18B20();
		WriteOneByte(0xCC);
		WriteOneByte(0xBE);
		Temp_Value[0] = ReadOneByte();
		Temp_Value[1] = ReadOneByte();
		DS18B20_IS_OK = 1;
	}
}

//----------------------------------------------
//读温度值18B20-1
//----------------------------------------------
void Read_Temperature_1()
{
	if( Init_DS18B20_1() == 1)
		DS18B20_IS_OK = 0;
	else
	{
		WriteOneByte_1(0xCC);
		WriteOneByte_1(0x44);
		Init_DS18B20_1();
		WriteOneByte_1(0xCC);
		WriteOneByte_1(0xBE);
		Temp_Value[0] = ReadOneByte_1();
		Temp_Value[1] = ReadOneByte_1();
		DS18B20_IS_OK = 1;
	}
}

//----------------------------------------------
//在LCD上显示当前温度
//----------------------------------------------
void Display_Temperature()
{
	uchar i;
	uchar t = 150, ng = 0;
	if((Temp_Value[1] & 0xF8) == 0xF8)
	{
		Temp_Value[1] = ~Temp_Value[1];
		Temp_Value[0] = ~Temp_Value[0] + 1;
		if(Temp_Value[0] == 0x00) Temp_Value[1]++;
		ng = 1;
	}
	Display_Digit[0] = df_Table[Temp_Value[0] & 0x0F];
	CurrentT = ((Temp_Value[0] & 0xF0)>>4)|((Temp_Value[1] & 0x07)<<4);
	Display_Digit[3] = CurrentT / 100;
	Display_Digit[2] = CurrentT % 100 / 10;
	Display_Digit[1] = CurrentT % 10;
	
	Current_Temp_Display_Buffer[13] = Display_Digit[0] + '0';
	Current_Temp_Display_Buffer[12] = '.';
	Current_Temp_Display_Buffer[11] = Display_Digit[1] + '0';
	Current_Temp_Display_Buffer[10] = Display_Digit[2] + '0';
	Current_Temp_Display_Buffer[ 9] = Display_Digit[3] + '0';
	if(Display_Digit[3] == 0) Current_Temp_Display_Buffer[ 9] = ' ';
	if(Display_Digit[2] == 0 && Display_Digit[3] ==0)
		Current_Temp_Display_Buffer[10] = ' ';
	if(ng)
	{
		if(Current_Temp_Display_Buffer[10] == ' ')
			Current_Temp_Display_Buffer[10] = '-';
		else
			if(Current_Temp_Display_Buffer[ 9] == ' ')
			  Current_Temp_Display_Buffer[ 9] = '-';
			else
				Current_Temp_Display_Buffer[ 8] = '-';
				
	}

	Display_Time[3] = CDTS / 10;
	Display_Time[2] = CDTS % 10;
	Display_Time[1] = CDTM / 10;
	Display_Time[0] = CDTM % 10;
	Current_Time_Display_Buffer[10] = Display_Time[1] + '0';
	Current_Time_Display_Buffer[11] = Display_Time[0] + '0';
	Current_Time_Display_Buffer[13] = Display_Time[3] + '0';
	Current_Time_Display_Buffer[14] = Display_Time[2] + '0';
	Set_LCD_POS(0x00);
	for(i = 0; i < 16; i++)
	{
		Write_LCD_Data(Current_Temp_Display_Buffer[i] );
	}
	Set_LCD_POS(0x40);
	for(i = 0; i < 16; i++)
	{
		Write_LCD_Data(Current_Time_Display_Buffer[i] );
	}
	Set_LCD_POS(0x0E); Write_LCD_Data(0x00);
  Set_LCD_POS(0x0F); Write_LCD_Data('C');
}

//----------------------------------------------
//欢迎界面
//----------------------------------------------
void Welcome()
{
	uchar i;
	LASER    = 0;
	HEATER1  = 0;
	HEATER2  = 0;
	Write_LCD_Command(0x01); DelayXus(5);
	Set_LCD_POS(0x00);
	for(i = 0; i < 16; i++)
	{
		Write_LCD_Data(Welcome_Buffer[i] );
	}
	Set_LCD_POS(0x40);
	for(i = 0; i < 16; i++)
	{
		Write_LCD_Data(Next_Step_Buffer[i] );
	}
	Set_LCD_POS(0x00); Write_LCD_Data(' ');
	Set_LCD_POS(0x01); Write_LCD_Data(' ');
	Set_LCD_POS(0x02); Write_LCD_Data(' ');
	Set_LCD_POS(0x03); Write_LCD_Data(' ');
	Set_LCD_POS(0x0C); Write_LCD_Data(' ');
	Set_LCD_POS(0x0D); Write_LCD_Data(' ');
	Set_LCD_POS(0x0E); Write_LCD_Data(' ');
	Set_LCD_POS(0x0F); Write_LCD_Data(' ');
	delay_ms(1000);
	while(Flag == 0)
	{
	if(BUTTON  ==  0)
		{
			  delay_ms(500);
			  if(BUTTON == 1)
				{
					Flag = 1;
				}
				else if(BUTTON == 0)
				{
					delay_ms(1500);
					if(BUTTON == 0)
					{
						Flag = 1;
					}
				}
		}
	}
}

//----------------------------------------------
//预处理控制步骤
//----------------------------------------------
void Pretreatment_Control()
{
	
	if(Set_Time == 0)
			{
				HEATER1  = 0;
	      HEATER2  = 0;
			  CDTM     = 29;
			  CDTS     = 59;
			  GoalT    = 55;	
				Set_Time = 1;
			}	
			if (TF0 == 1)
			{
				TF0 = 0;
				TH0 = (65536 - 40000) / 256;
	      TL0 = (65536 - 40000) % 256;
				if (T_Count < 8 )
				{
					T_Count = T_Count + 1;
				}
				else
				{
					T_Count = 0;
					if (CDTS > 0)
					{
						CDTS = CDTS - 1;
						if(GoalT > CurrentT)
						{
							HEATER1 = 1;
						}
						else if (GoalT <= CurrentT)
						{
							HEATER1 = 0;
						}
					}
					else
					{
						CDTS = 59;
						if(CDTM > 0)
						{
							CDTM = CDTM - 1;
						}
						else
						{
							Flag = 2;
							HEATER1 = 0;
						}
					}
					
				}
			}
			if(BUTTON  ==  0)
		{
			  delay_ms(500);
			  if(BUTTON == 1)
				{
					Flag = 1;
				}
				else if(BUTTON == 0)
				{
					delay_ms(1500);
					if(BUTTON == 0)
					{
						Flag = 2;
						HEATER1 = 0;
					}
				}
		}
}

//----------------------------------------------
//反应准备界面
//----------------------------------------------
void Ready_To_Go()
{
	uchar i;
	Write_LCD_Command(0x01); DelayXus(5);
	Set_LCD_POS(0x00);
	for(i = 0; i < 16; i++)
	{
		Write_LCD_Data(Ask_Buffer[i] );
	}
	Set_LCD_POS(0x40);
	for(i = 0; i < 16; i++)
	{
		Write_LCD_Data(Next_Step_Buffer[i] );
	}
  Set_LCD_POS(0x00); Write_LCD_Data(' ');
	Set_LCD_POS(0x01); Write_LCD_Data(' ');
	Set_LCD_POS(0x02); Write_LCD_Data(' ');
	Set_LCD_POS(0x03); Write_LCD_Data(' ');
	Set_LCD_POS(0x0C); Write_LCD_Data(' ');
	Set_LCD_POS(0x0D); Write_LCD_Data(' ');
	Set_LCD_POS(0x0E); Write_LCD_Data(' ');
	Set_LCD_POS(0x0F); Write_LCD_Data(' ');
	delay_ms(1000);
	while(Flag == 2)
	{
	if(BUTTON == 0)
		{
			  delay_ms(500);
			  if(BUTTON == 1)
				{
					Flag = 3;
				}
				else if(BUTTON == 0)
				{
					delay_ms(1500);
					if(BUTTON == 0)
					{
						Flag = 3;
					}
				}
		}
	}	
}

//----------------------------------------------
//反应控制步骤1
//----------------------------------------------
void Rection1_Control()
{
	if(Set_Time == 1)
			{
				HEATER1  = 0;
	      HEATER2  = 0;
			  CDTM     = 29;
			  CDTS     = 59;
			  GoalT    = 65;	
				Set_Time = 2;
			}	
			if (TF0 == 1)
			{
				TF0 = 0;
				TH0 = (65536 - 40000) / 256;
	      TL0 = (65536 - 40000) % 256;
				if (T_Count < 8 )
				{
					T_Count = T_Count + 1;
				}
				else
				{
					T_Count = 0;
					if (CDTS > 0)
					{
						CDTS = CDTS - 1;
						if(GoalT > CurrentT)
						{
							HEATER2 = 1;
						}
						else if (GoalT <= CurrentT)
						{
							HEATER2 = 0;
						}
					}
					else
					{
						CDTS = 59;
						if(CDTM > 0)
						{
							CDTM = CDTM - 1;
						}
						else
						{
							Flag    = 4;
							HEATER2 = 0;
						}
					}
					
				}
			}
			if(BUTTON  ==  0)
		{
			  delay_ms(500);
			  if(BUTTON == 1)
				{
					Flag = 3;
				}
				else if(BUTTON == 0)
				{
					delay_ms(1500);
					if(BUTTON == 0)
					{
						Flag = 5;
					}
				}
		}
}

//----------------------------------------------
//反应控制步骤2
//----------------------------------------------
void Rection2_Control()
{
	if(Set_Time == 2)
			{
				HEATER1  = 0;
	      HEATER2  = 0;
			  CDTM     =  9;
			  CDTS     = 59;
			  GoalT    = 80;	
				Set_Time = 0;
			}	
			if (TF0 == 1)
			{
				TF0 = 0;
				TH0 = (65536 - 40000) / 256;
	      TL0 = (65536 - 40000) % 256;
				if (T_Count < 8 )
				{
					T_Count = T_Count + 1;
				}
				else
				{
					T_Count = 0;
					if (CDTS > 0)
					{
						CDTS = CDTS - 1;
						if(GoalT > CurrentT)
						{
							HEATER2 = 1;
						}
						else if (GoalT <= CurrentT)
						{
							HEATER2 = 0;
						}
					}
					else
					{
						CDTS = 59;
						if(CDTM > 0)
						{
							CDTM = CDTM - 1;
						}
						else
						{
							Flag    = 5;
							HEATER2 = 0;
						}
					}
					
				}
			}
}

//----------------------------------------------
//荧光检测界面
//----------------------------------------------
void UV_Detection()
{
	uchar i;
	HEATER1  = 0;
	HEATER2  = 0;
	Set_LCD_POS(0x00);
	for(i = 0; i < 16; i++)
	{
		Write_LCD_Data(Ask_Buffer[i] );
	}
	Set_LCD_POS(0x40);
	for(i = 0; i < 16; i++)
	{
		Write_LCD_Data(Next_Step_Buffer[i] );
	}
  Set_LCD_POS(0x00); Write_LCD_Data('*');
	Set_LCD_POS(0x01); Write_LCD_Data('*');
	Set_LCD_POS(0x02); Write_LCD_Data('*');
	Set_LCD_POS(0x03); Write_LCD_Data('*');
	Set_LCD_POS(0x0C); Write_LCD_Data('*');
	Set_LCD_POS(0x0D); Write_LCD_Data('*');
	Set_LCD_POS(0x0E); Write_LCD_Data('*');
	Set_LCD_POS(0x0F); Write_LCD_Data('*');
	delay_ms(1000);
	while(Flag == 5)
	{
	if(BUTTON == 0)
		{
			  LASER  =  1;
			  delay_ms(500);
			  if(BUTTON == 1)
				{
					delay_ms(1500);
					LASER = 0;
				}
				else if(BUTTON == 0)
				{
					delay_ms(1000);
					if(BUTTON == 0)
					{
						LASER = 0;
						Flag  = 0;
					}
				}
		}
	}	
}

//----------------------------------------------
//主函数
//----------------------------------------------
void main()
{
	TMOD = 0x01;
	TH0 = (65536 - 40000) / 256;
	TL0 = (65536 - 40000) % 256;
	TR0 = 1;	
	LCD_Initialise();
	Read_Temperature_1();
	Read_Temperature();
	Delay(50000);
	while(1)
	{
		if (Flag == 0)  
    {
			STP1_LED       =  0;
			STP2_LED       =  0;
			STP3_LED       =  0;
		  Welcome();
		}
		if (Flag == 1)  
    {
			STP1_LED       =  0;
			STP2_LED       =  1;
			STP3_LED       =  1;
			PH             =  0;
			Set_Time       =  0;
      while(Flag == 1)
      {
				GoalT        = 55;
				Read_Temperature();
				if (DS18B20_IS_OK) 
				{
					Display_Temperature();
				}
				while(PH == 0)
					{
						CDTM     = 88;
			      CDTS     = 88;
						HEATER1  =  1;
						HEATER2  =  0;
						Read_Temperature();
						Delay(50000);
				    if (DS18B20_IS_OK) 
						{
							Display_Temperature();
						}
						while ((GoalT <= CurrentT) && PH == 0)
            {
							HEATER1  =  0;
							Read_Temperature();
							Delay(50000);
							if (DS18B20_IS_OK) 
							{
								Display_Temperature();
							}
							if (BUTTON  ==  0)
							{
								delay_ms(500);
								if(BUTTON == 1)
								{
									PH = 1;
									Flag = 1;
								}
							}
						}
						if(BUTTON  ==  0)
						{
								delay_ms(500);
								if(BUTTON == 1)
								{
									if(CurrentT >= (GoalT-2) )
									{
										PH       = 1;
										Pretreatment_Control();
									}
								}
								else if(BUTTON == 0)
								{
									delay_ms(500);
									if(BUTTON == 0)
									{
										PH       = 1;
										Flag     = 2;
										Set_Time = 1;
										HEATER1  = 0;
									}
								}
						}
					}
				Pretreatment_Control();
				DelayXus(50);
			}				
		} 
		if (Flag == 2)  
    {
			STP1_LED       =  0;
			STP2_LED       =  0;
			STP3_LED       =  1;
			Ready_To_Go();
		}
		if (Flag == 3)  
    {
			STP1_LED       =  1;
			STP2_LED       =  0;
			STP3_LED       =  1;
			PH             =  0;
			Write_LCD_Command(0x01); DelayXus(5);
      while(Flag == 3)
      {
				GoalT        = 65;
				Read_Temperature_1();
				if (DS18B20_IS_OK) Display_Temperature();
				while(PH == 0 )
					{
						CDTM     = 88;
			      CDTS     = 88;
						HEATER1  =  0;
						HEATER2  =  1;
						Read_Temperature_1();
						Delay(50000);
				    if (DS18B20_IS_OK) Display_Temperature();
						
						while ((GoalT <= CurrentT) && PH == 0)
            {
							HEATER2  =  0;
							Read_Temperature_1();
							Delay(50000);
							if (DS18B20_IS_OK) 
							{
								Display_Temperature();
							}
							if (BUTTON  ==  0)
							{
								delay_ms(500);
								if(BUTTON == 1)
								{
									PH = 1;
									Flag = 5;
								}
							}
						}
						
						if(BUTTON  ==  0)
						{
								delay_ms(500);
								if(BUTTON == 1)
								{
									
									if(CurrentT >= (GoalT-1) )
									{
										PH       = 1;
										Rection1_Control();
									}

								}
								else if(BUTTON == 0)
								{
									delay_ms(500);
									if(BUTTON == 0)
									{
										PH = 1;
										Flag = 5;
										HEATER1 = 0;
									}
								}
						}

					}
				PH           =  1;	
				Rection1_Control();
				DelayXus(50);
			}						
		}
		if (Flag == 4)  
    {
			STP1_LED       =  1;
			STP2_LED       =  1;
			STP3_LED       =  0;		
			PH             =  0;
			Write_LCD_Command(0x01); DelayXus(5);
      while(Flag == 4)
      {
				GoalT        = 80;
				Read_Temperature_1();
				if (DS18B20_IS_OK) Display_Temperature();
				while((GoalT > CurrentT) && PH == 0 )
					{
						CDTM     = 88;
			      CDTS     = 88;
						HEATER1  =  0;
						HEATER2  =  1;
						Read_Temperature_1();
						Delay(50000);
				    if (DS18B20_IS_OK) Display_Temperature();					
					}
				PH           =  1;	
				Rection2_Control();
				DelayXus(50);
			}				
		}
		if (Flag == 5)  
    {
			STP1_LED       =  1;
			STP2_LED       =  1;
			STP3_LED       =  1;
		  UV_Detection();
		}
	}
}






