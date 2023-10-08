// 此 Project 用于控制舵机开关灯


#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <U8g2lib.h>
#include <Servo.h>

ESP8266WebServer server(80);                                       //实例化web服务对象
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);  //实例化u8g2对象
Servo servo1, servo2;                                              //建立电机对象

/* 变量 */
const String deviceID = "Motor";  // 设备主机名
const String ssid = "HelloWorld-2.4G";  // WiFi名称
const String password = "730666yyds";   // WiFi密码

#define Length 5                              //只有7个可用引脚
const int waitWIFI = 120;                     // 等待连接WiFi的时长, 单位秒
const int io[Length] = { 0, 2, 13, 15, 16 };  //可用IO列表, 4,5用于屏幕显示
const bool mode[Length] = { 1, 1, 1, 1, 1 };  //储存引脚模式, 0为输入, 1为输出


struct Display {  //显示内容
  String line1, line2, time;
  int weather;

  void revise(String _line1 = "", String _time = "", String _line2 = "", int _weather = 0) {  //修改
    line1 = _line1 != "null" ? _line1 : line1;
    line2 = _line2 != "null" ? _line2 : line2;
    time = _time != "null" ? _time : time;
    weather = _weather != NULL ? _weather : weather;
  }
};


struct Data {  //引脚和显示
  bool level[Length] = { 0, 1, 0, 0, 1 };
  String lastIP = "暂无记录";
  Display display;
} data;


/* 函数声明 */
void init_display(bool reverse = false);
void init_WIFI();
void init_GPIO();
void init_server();
void init_servo();
void toDisplay(Display data, bool reverse = false);  // 将结构体中的数据显示到屏幕

void setup() {
  Serial.begin(9600);
  init_display();
  init_WIFI();
  init_GPIO();
  init_servo();
  init_server();
}

void loop() {
  server.handleClient();
}

void init_server() {
  server.on("/", handleRoot);
  server.on("/set", HTTP_POST, handlePOST);  //设置引脚状态和显示内容, 输入模式的引脚将被忽略
  server.on("/get", HTTP_GET, handleGET);    //获取引脚状态和显示内容, 输出模式的引脚将被跳过
  server.on("/set", HTTP_GET, handleSET);    //获取引脚状态和显示内容, 输出模式的引脚将被跳过
  server.onNotFound(handleNotFound);
  server.begin();
  data.display.revise("正在连接WIFI...", "00:00", "By YangRucheng");
  toDisplay(data.display);
}

void handleRoot() {
  auto html = String("<meta charset='utf-8'>");
  html += String("<h1>欢迎来到ESP8266 Home页面!</h1>");
  html += String("<p>这是用于730寝室大灯控制!</p>");
  html += String("<p><a href='./get'>获取引脚和显示状态</a> (JSON格式)</p>");
  html += String("<p><a href='./set'>前往控制页面</a></p>");
  html += String("<p><a href='https://github.com/YangRucheng'>我的GitHub主页</a></p>");
  html += String("<p><a href='https://blog.yangrucheng.top'>我的博客</a></p>");
  html += String("<p>上次执行控制IP: ") + String(data.lastIP) + String("</p>");
  server.send(200, "text/html", html);
}

void handleSET() {
  auto html = String("<meta charset='utf-8'>");
  html += String("<p>欢迎来到ESP8266控制页!</p>");
  html += String("<p>暂未完成!</p>");
  html += String("<p>注意: 此页面会记录IP地址, 请慎重操作!</p>");
  server.send(200, "text/html", html);
  data.lastIP = server.client().remoteIP().toString();
}

void handleGET() {
  DynamicJsonDocument doc(512);
  for (int i = 0; i < Length; i++)  //更新输入模式的状态
    if (mode[i] == 0)
      data.level[i] = digitalRead(io[i]);
  for (int i = 0; i < Length; i++)
    doc["level"][String(io[i])] = data.level[i];
  doc["display"]["line1"] = data.display.line1;
  doc["display"]["time"] = data.display.time;
  doc["display"]["line2"] = data.display.line2;
  doc["display"]["weather"] = data.display.weather;
  doc["msg"] = "Powered By YangRucheng";
  doc["random"] = random(10000, 99999);
  doc["lastIP"] = data.lastIP;
  char buffer[512];
  serializeJson(doc, buffer);
  server.send(200, "application/json", buffer);
}

void handlePOST() {
  if (server.hasArg("plain") == false) {
    server.send(400, "text/html", "<meta charset='utf-8'>Error! 请求中无请求体!");
    return;
  }

  String body = server.arg("plain");
  DynamicJsonDocument doc(512);                             //堆内存中声明一个JsonDocument对象
  DeserializationError error = deserializeJson(doc, body);  //反序列化JSON数据
  if (error) {
    server.send(400, "text/html", "<meta charset='utf-8'>Error! 解析JSON错误!");
    return;
  }

  for (int i = 0; i < Length; i++) {
    if (mode[i] == 1 and doc["level"].containsKey(String(io[i]))) {
      data.level[i] = doc["level"][String(io[i])];
      digitalWrite(io[i], data.level[i] ? HIGH : LOW);
    }
  }
  data.display.revise(
    doc["display"]["line1"],
    doc["display"]["time"],
    doc["display"]["line2"],
    doc["display"]["weather"]);

  if (doc["msg"] != NULL) {
    const int on1 = 0;
    const int on2 = 0;

    const int mid1 = 60;
    const int mid2 = 0;

    const int off1 = 60;
    const int off2 = 40;

    String msg = doc["msg"].as<const char*>();

    if (msg.substring(0, 2) == String("on")) {
      servo1.write(on1);
      servo2.write(on2);
      delay(200);
      servo1.write(mid1);
      servo2.write(mid2);
    }

    if (msg.substring(0, 3) == String("off")) {
      servo1.write(off1);
      servo2.write(off2);
      delay(200);
      servo1.write(mid1);
      servo2.write(mid2);
    }
  }
  handleGET();  //返回get请求内容
}

void handleNotFound() {
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "Not Found! Redirct!");
}

/************************************/
/* 通用函数 */
/************************************/

/* 将结构体中的数据显示到屏幕 */
void toDisplay(Display data, bool reverse) {
  char const* icon_index[4] = { "@", "A", "C", "E" };  //图标在天气字库里分别代表 阴(0x40)，云，雨，晴

  if (data.weather < 0 || data.weather >= 4) {
    // 无显示
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    return;
  }

  if (reverse == false) {
    // 正向显示
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy16_t_gb2312);  //设置字体
    u8g2.setCursor(0, 14);
    u8g2.print(data.line1);
    u8g2.setFont(u8g2_font_logisoso24_tr);  //设置字体
    u8g2.setCursor(0, 43);
    u8g2.print(data.time);
    u8g2.setFont(u8g2_font_open_iconic_weather_4x_t);  //天气字体
    u8g2.drawStr(90, 48, icon_index[data.weather]);
    u8g2.setFont(u8g2_font_wqy15_t_gb2312);  //设置字体
    u8g2.setCursor(0, 61);
    u8g2.print(data.line2);
    u8g2.sendBuffer();
    return;
  }
  if (reverse == true) {
    // 反向显示
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy16_t_gb2312);  //设置字体
    u8g2.setCursor(0, 14);
    u8g2.print(data.line1);
    u8g2.setFont(u8g2_font_logisoso24_tr);  //设置字体
    u8g2.setCursor(0, 43);
    u8g2.print(data.time);
    u8g2.setFont(u8g2_font_open_iconic_weather_4x_t);  //天气字体
    u8g2.drawStr(90, 46, icon_index[data.weather]);
    u8g2.setFont(u8g2_font_wqy15_t_gb2312);  //设置字体
    u8g2.setCursor(0, 61);
    u8g2.print(data.line2);
    u8g2.sendBuffer();
    return;
  }
}

/* 初始化WIFI */
void init_WIFI() {
  Serial.print("Connecting to WIFI");
  int count = waitWIFI * 2;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.hostname("ESP-" + deviceID);  //设置新的hostname
  while (WiFi.status() != WL_CONNECTED && count) {
    Serial.print(".");
    count--;
  }
   if (count) {
    Serial.printf("\nSuccess Connected to WIFI\n");
    Serial.println(WiFi.localIP());
  } else {
    Serial.printf("\nFail Connected to WIFI\n");
  }
  WiFi.setAutoConnect(true);  // 设置自动连接

  data.display.revise("正在启动服务...", "00:00", "By YangRucheng");
  toDisplay(data.display);
}

/* 初始化GPIO */
void init_GPIO() {
  for (int i = 0; i < Length; i++) {
    pinMode(io[i], mode[i] ? OUTPUT : INPUT_PULLDOWN_16);
    if (mode[i] == 1)
      digitalWrite(io[i], data.level[i] ? HIGH : LOW);
  }
}

/* 初始化显示模块 */
void init_display(bool reverse) {
  u8g2.begin();
  u8g2.enableUTF8Print();
  if (reverse == true)
    u8g2.setDisplayRotation(U8G2_R2);

  data.display.revise("正在连接WIFI...", "00:00", "By YangRucheng");
  toDisplay(data.display);
}

/* 初始化舵机模块 */
void init_servo() {
  servo1.attach(12);
  servo2.attach(14);
  servo1.write(60);
  servo2.write(0);
}






// //----------------------------------------------------------------------------
// #include <WebSocketsClient.h>
// #include <ArduinoJson.h>
// #include <ESP8266WiFi.h>
// #include <U8g2lib.h>
// #include <ctime>

// WebSocketsClient Client;                                           // 实例化WebSocketsClient对象
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);  //实例化u8g2对象

// /* 必须唯一的变量 */
// String deviceID = "lamp1";  //设备唯一ID
// /* 可修改变量 */
// const String ssid = "HelloWorld-2.4G";  // WiFi名称
// const String password = "730666yyds";   // WiFi密码
// IPAddress gateway(10, 10, 1, 1);        // WiFi网关地址
// IPAddress host(10, 10, 3, 2);           // ESP8266的IP地址
// IPAddress netmask(255, 255, 0, 0);      // WiFi子网掩码
// String server_host = "10.10.1.2";       // WebSocket服务器IP地址
// int server_port = 2233;                 // WebSocket服务器端口
// int waitWIFI = 120;                     // 等待连接WiFi的时长, 单位秒


// struct Display {
//   String line1, line2, time;
//   int weather;
//   Display(String _line1 = "", String _time = "", String _line2 = "", int _weather = 0) {
//     line1 = _line1, line2 = _line2, time = _time;
//     weather = _weather;
//   }
//   void revise(String _line1 = "", String _time = "", String _line2 = "", int _weather = 0) {
//     line1 = _line1, line2 = _line2, time = _time;
//     weather = _weather;
//   }
// };

// Display display;  //当前显示的内容

// /* 函数声明 */
// void init_display(bool reverse = false);
// void init_WIFI();
// void init_webosocket();
// void init_servo();
// void toDisplay(Display data, bool reverse = false);  // 将结构体中的数据显示到屏幕

// void setup() {
//   Serial.begin(9600);
//   init_servo();
//   init_display();
//   init_WIFI();
//   init_webosocket();
// }

// void loop() {
//   Client.loop();
// }




// void init_WIFI() {  // 初始化WIFI
//   Serial.print("Connecting to WIFI");
//   int count = waitWIFI * 2;
//   WiFi.config(host, gateway, netmask);
//   WiFi.begin(ssid, password);
//   WiFi.hostname("ESP-" + deviceID);  //设置新的hostname
//   while (WiFi.status() != WL_CONNECTED && count) {
//     Serial.print(".");
//     count--;
//   }
//   if (count)
//     Serial.printf("\nSuccess Connected to WIFI\n");
//   else
//     Serial.printf("\nFail Connected to WIFI\n");
//   ;
//   WiFi.setAutoConnect(true);  // 设置自动连接

//   display.revise("正在连接服务器...", "00:00", "By YangRucheng");
//   toDisplay(display);
// }

// void init_webosocket() {  // 启动WebSocket客户端
//   Client.begin(server_host, server_port, "/iot/device/" + deviceID);
//   Serial.println("Websocket running!");
//   Client.setReconnectInterval(5000);       //断线重连
//   Client.enableHeartbeat(15000, 3000, 2);  //启用Ping心跳包
//   Client.onEvent(webSocketEvent);          // 指定事件处理函数

//   display.revise("已连接服务器", "00:00", "By YangRucheng");
//   toDisplay(display);
// }

// void init_display(bool reverse) {
//   u8g2.begin();
//   u8g2.enableUTF8Print();
//   if (reverse == true)
//     u8g2.setDisplayRotation(U8G2_R2);

//   display.revise("正在连接WIFI...", "00:00", "By YangRucheng");
//   toDisplay(display);
// }

// void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
//   switch (type) {
//     case WStype_DISCONNECTED:
//       Serial.printf("Websocket disconnected! Reconnect in 5 seconds!\n");
//       break;
//     case WStype_CONNECTED:
//       Serial.printf("Websocket success connected to: %s\n", payload);
//       break;
//     case WStype_TEXT:
//       Serial.printf("Websocket get text: %s\n", payload);
//       handle((char*)payload);  //处理消息
//       break;
//     case WStype_PING:
//       Serial.printf("Websocket get heartbeat ping\n");
//       break;
//     case WStype_PONG:
//       Serial.printf("Websocket get heartbeat pong\n");
//       break;
//   }
// }

// /* 处理传来的文本, 反序列化并调用对应函数 */
// void handle(String data_str) {
//   DynamicJsonDocument doc(512);                                 //堆内存中声明一个JsonDocument对象
//   DeserializationError error = deserializeJson(doc, data_str);  //反序列化JSON数据
//   if (error) return;

//   String device = doc["device"];
//   String type = doc["type"];

//   if (type.compareTo("heartbeat") == 0) {  //心跳包
//     Serial.println("heartbeat!");
//     return;
//   }

//   if (type.compareTo("set") == 0) {  //修改
//     Serial.println("setGPIO!");

//     if (doc["data"]["display"] != NULL) {
//       display.revise(
//         doc["data"]["display"]["line1"].as<const char*>(),
//         doc["data"]["display"]["time"].as<const char*>(),
//         doc["data"]["display"]["line2"].as<const char*>(),
//         (int)doc["data"]["display"]["weather"]);
//       toDisplay(display);
//     }


//     return;
//   }

//   Serial.println("Error Data!");
// }


// /* 将结构体中的数据显示到屏幕 */
// void toDisplay(Display data, bool reverse) {
//   char const* icon_index[4] = { "@", "A", "C", "E" };  //图标在天气字库里分别代表 阴(0x40)，云，雨，晴

//   if (data.weather < 0 || data.weather >= 4) {
//     // 无显示
//     u8g2.clearBuffer();
//     u8g2.sendBuffer();
//     return;
//   }

//   if (reverse == false) {
//     // 正向显示
//     u8g2.clearBuffer();
//     u8g2.setFont(u8g2_font_wqy16_t_gb2312);  //设置字体
//     u8g2.setCursor(0, 14);
//     u8g2.print(data.line1);
//     u8g2.setFont(u8g2_font_logisoso24_tr);  //设置字体
//     u8g2.setCursor(0, 43);
//     u8g2.print(data.time);
//     u8g2.setFont(u8g2_font_open_iconic_weather_4x_t);  //天气字体
//     u8g2.drawStr(90, 48, icon_index[data.weather]);
//     u8g2.setFont(u8g2_font_wqy15_t_gb2312);  //设置字体
//     u8g2.setCursor(0, 61);
//     u8g2.print(data.line2);
//     u8g2.sendBuffer();
//     return;
//   }
//   if (reverse == true) {
//     // 反向显示
//     u8g2.clearBuffer();
//     u8g2.setFont(u8g2_font_wqy16_t_gb2312);  //设置字体
//     u8g2.setCursor(0, 14);
//     u8g2.print(data.line1);
//     u8g2.setFont(u8g2_font_logisoso24_tr);  //设置字体
//     u8g2.setCursor(0, 43);
//     u8g2.print(data.time);
//     u8g2.setFont(u8g2_font_open_iconic_weather_4x_t);  //天气字体
//     u8g2.drawStr(90, 46, icon_index[data.weather]);
//     u8g2.setFont(u8g2_font_wqy15_t_gb2312);  //设置字体
//     u8g2.setCursor(0, 61);
//     u8g2.print(data.line2);
//     u8g2.sendBuffer();
//     return;
//   }
// }
