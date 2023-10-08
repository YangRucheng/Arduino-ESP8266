// 此 Project 用于产生 WiFi 热点


#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

ESP8266WebServer server(8080);       // 实例化web服务对象
ESP8266WebServer serverDefault(80);  // 实例化web服务对象

const char* ssid = "XTU-FREE";            // SSID
const IPAddress IP(172, 16, 0, 32);       // 自定义的IP地址
const IPAddress gateway(172, 16, 0, 32);  // 自定义的网关
const IPAddress subnet(255, 0, 0, 0);     // 自定义的子网掩码

void setup() {
  Serial.begin(9600);
  WiFi.softAPConfig(IP, gateway, subnet);
  WiFi.softAP(ssid);
  Serial.print("AP IP address: ");
  Serial.println(IP);

  serverDefault.onNotFound(handleNotFoundDefault);
  server.onNotFound(handleNotFound);
  server.on("/login", handleLogin);
  serverDefault.begin();
  server.begin();
}

void loop() {
  server.handleClient();
  serverDefault.handleClient();
}

void handleNotFoundDefault() {
  serverDefault.sendHeader("Location", "http://172.16.0.32:8080/login");
  serverDefault.send(302, "text/plain", "");
}

void handleNotFound() {
  server.sendHeader("Location", "/login");
  server.send(302, "text/plain", "");
}

void handleLogin() {
  auto html = String("<meta charset='utf-8'>");
  html += String("<title>登录到网络</title>");
  html += String("<h1>钓鱼WIFI</h1>");
  html += String("<p>这是一个钓鱼WIFI, 请仔细甄别!</p>");
  html += String("<p>距离你输入密码仅有一步之遥!</p>");
  html += String("<p>不用感谢我们为你的账号安全所做的!</p>");
  server.send(200, "text/html", html);
}
