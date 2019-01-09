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
