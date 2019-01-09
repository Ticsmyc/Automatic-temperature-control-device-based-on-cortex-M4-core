##基于cortex m4内核的自动温控装置

####  内容

使用cortex m4开发板、温湿度传感器、电机，实现温度湿度控制。

#### 器件

- [x] NUCLEO-L476RG开发板 x 1

- [x] 温度湿度传感器DHT11  x 1
- [x] 微型130电机+风扇叶  x 1 
- [x] 电机驱动L9110S  x 1
- [x] HD44780 Text LCD  x 1
- [x] 按键 x 1
- [x] LED灯 x 1
- [x] 母对母杜邦线若干

#### 功能及特点

- 使用温度湿度传感器进行测量。
- 当温度超过设定阈值且湿度未超过时，风扇正转送风。
- 当湿度超过设定阈值且温度未超过时，风扇反转吸风除湿。
- 当温度和湿度均超过设定阈值时，LED灯闪烁，风扇正转。
- 使用一个按键控制风扇是否工作。
- 用一块LCD屏幕显示当前温度湿度，以及风扇状态。
- 通过串口通信，将温度湿度发送到PC端。
#### 设计原理
- DHT11传感器

  1.引脚介绍：

  Pin1：(VDD)，电源引脚，供电电压为3~5.5V。

  Pin2：（DATA），串行数据，单总线。 

  Pin3:（NC），空脚。 

  Pin4（VDD），接地端，电源负极。

  2.数据帧的描述：

  DATA用于微处理器与DHT11之间的通讯和同步，采用单总线数据格式，一次通讯时间4ms左右，数据分小数部分和整数部分，具体格式在下面说明，当前小数部分用于以后扩展，现读出为零。操作流程如下:

  一次完整的数据传输为40bit，高位先出。

  其数据格式：8bit湿度整数数据+8bit湿度小数数据+8bi温度整数数据+8bit温度小数数据

  数据传送正确时校验和数据等于“8bit湿度整数数据+8bit湿度小数数据+8bi温度整数数据+8bit温度小数数据”所得结果的末8位。

  3.时序描述：

  用户MCU发送一次开始信号后，DHT11从低功耗模式转换到高速模式，等待主机开始信号结束后，DHT11发送响应信号，送出40bit的数据，并触发一次信号采集，用户可选择读取部分数据。

- 温湿度传感器 DHT11 说明： https://blog.csdn.net/u013151320/article/details/50389624
- 直流电机说明 ： https://blog.csdn.net/teavamc/article/details/77429519

#### 遇到的一些问题

###### DHT11传感器使用时：

1. 数据难处理，40bit数据分为五组，较复杂。
2. 初期没有LCD屏幕，试图使用串口将传感器得到的数据传到PC。串口通信报错，发现是因为定义了模拟输出，与串口通信冲突。

###### 电机使用时：

1. 正转反转问题： 使用模拟信号输出至电机驱动模块，发现无法改变方向。试图在电机上加四根线，发现两边都接地了。。最后发现应该用数字信号输出到驱动.....



#### 代码

```c
#include "mbed.h"
#include "TextLCD.h"

//管脚绑定
DigitalOut myled(PA_5);  //LED灯
DigitalIn botton(PC_13); //按键
DigitalOut IN1(PH_0); //电机控制信号1
DigitalOut IN2(PH_1);//电机控制信号2
DigitalIn DHT11_DQ_IN(PA_6); //温湿度传感器输入
Serial pc(SERIAL_TX, SERIAL_RX); //串口通信
TextLCD lcd(PC_8,PC_6,PC_5,PB_2,PB_1,PB_15);  //LCD屏幕


int temperature=0;//温度整数位
int humidity=0; //湿度整数位
int t=0;//温度小数位
int h=0;//湿度小数位
int mark=0;//正转1反转2不转0标志位
int kaiguan=1;//风扇开关

void DispTemp()
{
    //LCD显示
    lcd.locate(0,0);
    lcd.printf("Temp: ");
    lcd.locate(6,0);
    lcd.printf("%d.%d",temperature,t);
    lcd.locate(0,1);
    lcd.printf("Humi: ");
    lcd.locate(6,1);
    lcd.printf("%d.%d",humidity,h);
    lcd.locate(14,1);
    //正转反转
    if(mark==1)
        lcd.printf("R");
    else if(mark==2)
        lcd.printf("L");
    else
        lcd.printf("N");
    //风扇开关
    lcd.locate(13,0);
    if(kaiguan==1)
        lcd.printf("ON ");
    if(kaiguan==0)
        lcd.printf("OFF");
}

void DHT11_Rst(void)
{
//使能DHT11(要发送一个持续至少18ms的0信号)
    DigitalOut DHT11_DQ_Out(PA_6);    //将PA_6改为输出,发送使能信号
    DHT11_DQ_Out=0;     //置0
    wait_ms(20);       //保持至少18ms
    DHT11_DQ_Out=1;     //恢复1
    wait_us(30);
    DigitalIn DHT11_DQ_IN(PA_6);//将PA_6换回输入状态

}
//从DHT11读取一位
int DHT11_Read_Bit(void)
{
    while (DHT11_DQ_IN == 0);
    wait_us(40);
    if (DHT11_DQ_IN == 1)
    {
        while (DHT11_DQ_IN == 1);
        return 1;
    }
    else
    {
        return 0;
    }
}
//从DHT11读取一个字节
//返回读到的数据
int DHT11_Read_Byte(void)
{
    int i,dat;
    dat=0;
    for (i=0;i<8;i++)//每组数据有8bit,每次右移一位
    {
        dat<<=1;
        dat|=DHT11_Read_Bit();
    }
    return dat;
}
//从DHT11读取一次数据
//temp:温度值(范围:0~50°)
//humi:湿度值(范围:20%~90%)
//返回值：0,正常;1,读取失败
int DHT11_Read_Data(int *temp,int *humi,int *h, int *t)
{
    int buf[5];
    int i;
    DHT11_Rst(); //先让DHT11开始工作
    if(DHT11_DQ_IN==0)
    {
        while (DHT11_DQ_IN == 0);
        while (DHT11_DQ_IN == 1);
        for(i=0;i<5;i++)//读取40位数据 ,5组,每组8bit. 前四组分别是温度湿度的整数位和小数位, 第五组是校验位
        {
            buf[i]=DHT11_Read_Byte();
        }
        if((buf[0]+buf[1]+buf[2]+buf[3])==buf[4])  //校验
        {
            *humi=buf[0];
            *temp=buf[2];
            *h=buf[1];
            *t=buf[3];
        }
    }
    else return 1;//DHT11未工作,读取失败
    return 0;
}

int main() {
    while(1) {
        //传感器读取温度湿度数据
        DHT11_Read_Data(&temperature,&humidity,&h,&t);

        //串口通信,将数据发回PC
        pc.printf("temperature: %d.%d , humidity: %d.%d\n", temperature,t , humidity,h);
        wait(0.2);

        //判断风扇开关的开闭
        if(botton==0)
        {
            if(kaiguan==0)  kaiguan=1;
            else kaiguan=0;
        }

        //LCD显示
        DispTemp();

        //风扇正转反转逻辑
        if(temperature>=26 && humidity >=50 && kaiguan==1)

        {
            //如果温度湿度都高
            //led开始闪烁
            myled = 1; // LED is ON
            wait(0.2); // 200 ms
            myled = 0; // LED is OFF
            wait(0.2); // 1 sec
            //风扇正转
            IN1=1;
            IN2=0;
            //修改风扇正转标志位
            mark=1;
        }
       else  if(temperature>=26 && humidity <=50 &&kaiguan==1)
        {
            //温度高湿度低,正转
            IN1=1;
            IN2=0;
            mark=1;

        }
       else  if(temperature<=26 && humidity >=50&&kaiguan==1)
        {
            //温度低湿度高,反转

             IN1=0;
             IN2=1;
             mark=2;
        }
        else
        {
            //否则LED不亮,风扇不转,标志位为0
            IN1=0;
            IN2=0;
            myled=0;
            mark=0;
            }
    }
}

```



