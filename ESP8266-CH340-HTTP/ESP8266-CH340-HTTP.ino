// 此 Project 用于控制引脚电平, 进而控制继电器


#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <U8g2lib.h>

ESP8266WebServer server(80);                                       //实例化web服务对象
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);  //实例化u8g2对象

/* 变量 */
const String deviceID = "Table_First";  // 设备主机名
const String ssid = "HelloWorld-2.4G";  // WiFi名称
const String password = "730666yyds";   // WiFi密码

#define Length 7                                      //只有7个可用引脚
const int waitWIFI = 120;                             // 等待连接WiFi的时长, 单位秒
const int io[Length] = { 0, 2, 12, 13, 14, 15, 16 };  //可用IO列表, 4,5用于屏幕显示
const bool mode[Length] = { 1, 1, 0, 1, 0, 1, 1 };    //储存引脚模式, 0为输入, 1为输出


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
  bool level[Length] = { 0, 1, 0, 0, 0, 0, 1 };
  String lastIP = "暂无记录";
  Display display;
} data;


/* 函数声明 */
void init_display(bool reverse = false);
void init_WIFI();
void init_GPIO();
void init_server();
void toDisplay(Display data, bool reverse = false);  // 将结构体中的数据显示到屏幕

void setup() {
  Serial.begin(9600);
  init_display();
  init_WIFI();
  init_GPIO();
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
  data.display.revise("已连接服务器", "00:00", "By YangRucheng");
}

void handleRoot() {
  auto html = String("<meta charset='utf-8'>");
  html += String("<h1>欢迎来到ESP8266 Home页面!</h1>");
  html += String("<p>这是用于YangRucheng台灯控制!</p>");
  html += String("<p><a href='./get'>获取引脚和显示状态</a> (JSON格式)</p>");
  html += String("<p><a href='./set'>前往控制页面</a></p>");
  html += String("<p><a href='https://github.com/YangRucheng'>我的GitHub主页</a></p>");
  html += String("<p><a href='https://blog.yangrucheng.top'>我的博客</a></p>");
  html += String("<p>上次执行控制IP: ") + String(data.lastIP) + String("</p>");
  server.send(200, "text/html", html);
}

void handleSET() {
  extern const char* controlHTML;
  server.send(200, "text/html", controlHTML);
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
  toDisplay(data.display);
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

  data.display.revise("正在等待...", "00:00", "By YangRucheng");
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

const char* controlHTML = R"(
<meta charset='UTF-8'>
<title>控制页</title>
<p>欢迎来到ESP8266控制页!</p>
<p>注意: 此页面会记录IP地址, 请慎重操作!</p>

<h2>引脚状态</h2>
<ul id='list'></ul>

<button onclick='submit()'>提交更改</button>
<script>
    let pin = {};

    fetch('./get')
        .then(response => response.json())
        .then(data => {
            pin = data.level;
            update();
        })

    function setPin(key, value) {
        console.log('[临时] 修改引脚状态 Set', key, '=', !value);
        pin[key] = !value;
        update();
    }

    function update() {
        const list = document.getElementById('list');
        while (list.firstChild) {
            list.removeChild(list.firstChild);
        };
        Object.keys(pin).forEach(key => {
            const li = document.createElement('li');
            li.textContent = `${key}:  ${pin[key] ? 'High' : 'Low'}`;
            li.onclick = function () {
                setPin(key, pin[key]);
            };
            list.appendChild(li);
        });
    }

    function submit() {
        fetch('./set', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ 'level': pin })
        })
            .then(response => response.json())
            .then(data => {
                pin = data.level;
                update();
            })
    }
</script>
)";