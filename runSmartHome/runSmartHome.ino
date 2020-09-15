#include <stdio.h>
#include <stdlib.h>
#include <SoftwareSerial.h>
#include <U8glib.h>
#include <Wire.h>
#include <dht11.h>
#include "edp.c"
#define ADDR 0b0100011//GY30"L"模式
#define DHT11_PIN 3//DHT11引脚
#define KEY  "cwqaGogCtE685AvvvtT79nTtQt0="    //修改为自己的API_KEY
#define ID   "627376552"    //修改为自己的设备ID
edp_pkt *pkt;
dht11 DHT;
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);  // I2C / TWI
char T[5] = {0}; //定义温度
char H[4] = {0}; //定义湿度
char I[6] = {0}; //定义光照强度
int set_T = 30; //温度阈值
int set_H = 50; //湿度阈值
int set_L = 200; //光照强度阈值
char S_T[5] = {0}; //温度阈值
char S_H[5] = {0}; //湿度阈值
SoftwareSerial mySerial1(8, 9);//ESP8266软串口
void setup() {
  //串口初始化
  Serial.begin(9600);//调试串口
  mySerial1.begin(9600);//该串口连接至ESP8266
  while (!Serial) {
    ; //等待串口连接
  }
  digitalWrite(13, LOW);//EDP连接指示灯
  //OLED初始化
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255, 255, 255);
  }
  itoa(set_T, S_T, 10);
  itoa(set_H, S_H, 10);
  u8g.firstPage();//屏幕开始界面
  do {
    u8g.setFont(u8g_font_unifont);
    u8g.drawStr( 10, 30, "Welcome to Use");
    u8g.drawStr( 28, 50, "Smart Home");
  } while ( u8g.nextPage() );
  delay(3000);
  //ESP8266初始化
  esp8266_init();
  //光照模块初始化
  illumination_init();
  //温湿度模块初始化
  pinMode(12, OUTPUT);//发光风扇输出端口
  pinMode(6, OUTPUT); //雾化片输出端口
}
void loop() {
  unsigned char pkt_type;
  edp_pkt rcv_pkt;
  static int edp_connect = 0;
  int tmp;
  //EDP 连接
  if (!edp_connect)
  {
    while (mySerial1.available()) mySerial1.read(); //清空串口接收缓存
    packetSend(packetConnect(ID, KEY));             //发送EPD连接包
    while (!mySerial1.available());                 //等待EDP连接应答
    if ((tmp = mySerial1.readBytes(rcv_pkt.data, sizeof(rcv_pkt.data))) > 0 && (rcv_pkt.data[0] == 0x20 && rcv_pkt.data[2] == 0x00 && rcv_pkt.data[3] == 0x00))
    {
      edp_connect = 1;
      digitalWrite(13, HIGH);//LED灯亮
    }
    packetSend(packetDataSaveTrans(NULL, "Set_Temperature", S_T)); //将初始温度阈值上传至数据流
    packetSend(packetDataSaveTrans(NULL, "Set_Humidity", S_H));//将初始湿度阈值上传至数据流
    packetClear(&rcv_pkt);//清空返回数据
  }
  packetSend(packetDataSaveTrans(NULL, "Temperature", T));//将温度值上传至数据流
  packetSend(packetDataSaveTrans(NULL, "Humidity", H));//将湿度值上传至数据流
  packetSend(packetDataSaveTrans(NULL, "Illumination", I)); //将光照强度上传至数据流
  delay(500);
  //读取云端控制指令
  while (mySerial1.available())
  {
    readEdpPkt(&rcv_pkt);//读取EDP包
    if (isEdpPkt(&rcv_pkt))//判断EDP包是否完整
    {
      pkt_type = rcv_pkt.data[0];
      switch (pkt_type)
      {
        case CMDREQ:
          char edp_command[50];
          char edp_cmd_id[40];
          long id_len, cmd_len, rm_len;
          char datastr[20];
          char val[10];
          memset(edp_command, 0, sizeof(edp_command));
          memset(edp_cmd_id, 0, sizeof(edp_cmd_id));
          edpCommandReqParse(&rcv_pkt, edp_cmd_id, edp_command, &rm_len, &id_len, &cmd_len);
          delay(50);
          Serial.print("cmd: ");
          Serial.println(edp_command);//打印控制命名
          sscanf(edp_command, "%[^:]:%s", datastr, val);//datastr：命令数据流名称，val：命令数据流值，均为字符串形式
          if (strstr(datastr, "Set_Temperature") != NULL) //如果是设置温度阈值
          {
            set_T = atoi(val); //将字符串转为数字传给阈值
            itoa(set_T, S_T, 10);//将阈值转为字符串用于显示
          }
          else if (strstr(datastr, "Set_Humidity") != NULL) //如果设置湿度阈值
          {
            set_H = atoi(val); //将字符串转为数字传给阈值
            itoa(set_H, S_H, 10);//将阈值转为字符串用于显示
          }
          packetSend(packetDataSaveTrans(NULL, datastr, val)); //将新数据值上传至数据流，更新阈值显示
          break;
        default:
          Serial.print("unknown type: ");
          Serial.println(pkt_type, HEX);
          break;
      }
    }
  }
  if (rcv_pkt.len > 0)
  {
    packetClear(&rcv_pkt);
  }
  //温湿度模块
  DHT.read(DHT11_PIN);//读取温湿度
  sprintf(H, "%d", DHT.humidity); //湿度int型转换char型
  dtostrf(DHT.temperature, 6, 2, T); //温度int型转换char型
  Serial.print("温度：");
  Serial.println(DHT.temperature, 2);//打印温度（两位小数）
  Serial.print("湿度：");
  Serial.println(DHT.humidity, DEC);//打印湿度
  if (DHT.temperature > set_T) //如果温度高于阈值
  {
    digitalWrite(12, HIGH);//风扇开始运行
  }
  else {
    digitalWrite(12, LOW);
  }
  if (DHT.humidity < set_H) //如果湿度低于阈值
  {
    digitalWrite(6, HIGH);//雾化器运行
  }
  else {
    digitalWrite(6, LOW);
  }
  //光照模块
  illumination();
  //OLED显示模块
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_unifont);
    u8g.drawStr( 0, 10, "T:");
    u8g.drawStr( 70, 10, "S_T:");
    u8g.drawStr( 0, 30, "H:");
    u8g.drawStr( 70, 30, "S_H:");
    u8g.drawStr( 0, 50, "I:");
    u8g.drawStr( 10, 10, T);
    u8g.drawStr( 100, 10, S_T);
    u8g.drawStr( 20, 30, H);
    u8g.drawStr( 100, 30, S_H);
    u8g.drawStr( 20, 50, I);
  } while ( u8g.nextPage() );
}
//发送AT指令
void sendcmd(char * cmd)
{
  mySerial1.write(cmd);
  delay(100);
  if (strstr(cmd, "AT+CIPSEND\r\n") && mySerial1.find("OK"))//如果WIFI连接正常
  {
    Serial.println("OK");
    u8g.firstPage();
    do {
      u8g.setFont(u8g_font_unifont);
      u8g.drawStr( 25, 40, "CONNECT OK");
    } while ( u8g.nextPage() );
    delay(500);
  }
  while (mySerial1.available())//清空串口缓存
  {
    delay(100);
    mySerial1.read();
  }
}
//esp8266初始化函数
void esp8266_init()
{
  u8g.firstPage();//屏幕显示WIFI正在连接
  do {
    u8g.setFont(u8g_font_unifont);
    u8g.drawStr( 5, 40, "WIFI CONNECTING");
  } while ( u8g.nextPage() );
  sendcmd("AT\r\n");
  delay(1000);
  sendcmd("AT+CWMODE=3\r\n");
  delay(2000);
  sendcmd("AT+RST\r\n");
  delay(2000);
  /*//修改WIFI连接时使用
    sendcmd("AT+CWJAP=\"HUAWEI nova 2 Plus\",\"zjh19990316\"\r\n");
    delay(1000);
  */
  sendcmd("AT+CIPSTART=\"TCP\",\"183.230.40.39\",876\r\n");
  delay(1000);
  sendcmd("AT+CIPMODE=1\r\n");
  delay(1000);
  sendcmd("AT+CIPSEND\r\n");
  delay(1000);
}
//光照模块初始化
void illumination_init()
{
  Wire.begin();
  Wire.beginTransmission(ADDR);
  Wire.write(0b00000001);
  Wire.endTransmission();
  pinMode(11, OUTPUT);//发光二极管输出端口
}
//光照强度数据读取
void illumination()
{
  int L;//光照强度
  // reset
  Wire.beginTransmission(ADDR);
  Wire.write(0b00000111);
  Wire.endTransmission();
  delay(100);
  Wire.beginTransmission(ADDR);
  Wire.write(0b00100000);
  Wire.endTransmission();
  // typical read delay 120ms
  delay(100);
  Wire.requestFrom(ADDR, 2); // 2byte every time
  for (L = 0; Wire.available() >= 1; ) {
    char c = Wire.read();
    //Serial.println(c, HEX);
    L = (L << 8) + (c & 0xFF);
  }
  L = L / 1.2; //光照强度int型
  memset(I, 0, sizeof(I));
  sprintf(I, "%d", L); //光照强度int型转为char型
  Serial.print("lx: ");
  Serial.println(I);
  if (L < set_L) //如果光照强度低于阈值（当前设定400）
  {
    digitalWrite(11, HIGH);//点亮LED灯
  }
  else {
    digitalWrite(11, LOW);
  }
  delay(100);
}
//EDP包发送函数（后面函数均与EDP传输有关）
void packetSend(edp_pkt * pkt)
{
  if (pkt != NULL)
  {
    mySerial1.write(pkt->data, pkt->len);    //串口发送
    mySerial1.flush();
    free(pkt);              //回收内存
  }
}
bool readEdpPkt(edp_pkt * p)
{
  int tmp;
  if ((tmp = mySerial1.readBytes(p->data + p->len, sizeof(p->data))) > 0 )
  {
    p->len += tmp;
  }
  return true;
}
