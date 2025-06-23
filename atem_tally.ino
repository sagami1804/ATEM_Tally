#include <M5StickCPlus.h>
#include <WiFi.h>
#include <Preferences.h>
#include <SkaarhojPgmspace.h>
#include <ATEMbase.h>
#include <ATEMstd.h>
#include <WebServer.h>
#include "index.h"

#define LED_PIN 10
#define NUM_INPUTS 20
#define INITIAL_ORIENTATION 3
#define AUTOUPDATE_ORIENTATION 1
#define GRAY 0x0841
#define GREEN 0x0400
#define RED 0xF800

Preferences preferences;
ATEMstd AtemSwitcher;
WebServer server(80);

IPAddress atemIPAddr;

String ssid;
String pass;
String atemIpStr;

//初期設定
int input = 1;
int previousInput = input;
int previousLabel = label;

boolean previousInputIsProgram = false;
boolean previousInputIsPreview = false;

int orientation = INITIAL_ORIENTATION;
int previousOrientation = orientation;
unsigned long lastOrientationUpdate = millis();
boolean receiveIp = false;

void setup() {
  //LEDを消灯
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  //前回の設定を読み込み
  preferences.begin("tally", false);
  ssid = preferences.getString("ssid", "elecom-44b063");
  pass = preferences.getString("pass", "3w9r885ekic1");
  atemIpStr = preferences.getString("atemIP", "192.168.1.4");
  atemIPAddr.fromString(atemIpStr);
  input = preferences.getInt("input", 1);
  label = preferences.getInt("label", 1);

  //M5Stackを初期化
  M5.begin(true, true, false);
  M5.IMU.Init();
  M5.Lcd.setRotation(orientation);

  //WiFi接続
  M5.Lcd.printf("Connecting to\n%s: ", ssid.c_str());
  WiFi.begin(ssid.c_str(), pass.c_str());
  int retryCount = 0;
  int reconnectCount = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    retryCount++;
    if (retryCount > 6) {
      WiFi.disconnect(true, true);
      WiFi.begin(ssid.c_str(), pass.c_str());
      retryCount = 0;
      reconnectCount++;
    }
    if (reconnectCount > 3) {
      WifiSetting();
      WiFi.disconnect(true, true);
      WiFi.begin(ssid.c_str(), pass.c_str());
      retryCount = 0;
      reconnectCount = 0;
    }
  }
  M5.Lcd.println(" done");

  //ATEM接続
  M5.Lcd.printf("Connecting to ATEM\n<%s>: ", atemIPAddr.toString().c_str());
  AtemSwitcher.begin(atemIPAddr);
  AtemSwitcher.connect();
  M5.Lcd.println("done");
  delay(2000);
}

void loop() {
  M5.update();

  //画面回転
  if (AUTOUPDATE_ORIENTATION && millis() - lastOrientationUpdate > 500) {
    updateOrientation();
    lastOrientationUpdate = millis();
  }

  //ボタンAが押されたらinputを変更
  if (M5.BtnA.wasPressed()) {
    input = (input % NUM_INPUTS) + 1;
    preferences.putInt("input", input);
    updateOrientation();
  }

  //ボタンBが押されたらATEMのinputをタリーのinputに変更
  if (M5.BtnB.wasPressed()) {
    AtemSwitcher.changeProgramInput(input);
  }

  AtemSwitcher.runLoop();

  //ATEMのinput変化を検知
  boolean inputIsProgram = AtemSwitcher.getProgramTally(input);
  boolean inputIsPreview = AtemSwitcher.getPreviewTally(input);

  if ((input != previousInput) || (label != previousLabel) ||
      (inputIsProgram != previousInputIsProgram) ||
      (inputIsPreview != previousInputIsPreview) ||
      (orientation != previousOrientation)) {

    if (inputIsProgram && inputIsPreview) {
      //preview&programの場合
      render(input, label, RED, GREEN, LOW);
    } else if (inputIsProgram) {
      //programの場合
      render(input, label, RED, BLACK, LOW);
    } else if (inputIsPreview) {
      //previewの場合
      render(input, label, GREEN, BLACK, HIGH);
    } else {
      //それ以外の場合
      render(input, label, BLACK, GRAY, HIGH);
    }
  }

  previousInput = input;
  previousLabel = label;
  previousInputIsProgram = inputIsProgram;
  previousInputIsPreview = inputIsPreview;
  previousOrientation = orientation;

  //文字表示を更新
  M5.Lcd.drawString("Tally: " + String(inputIsProgram ? "PGM" : inputIsPreview ? "PVM" : "OFF"), 3, M5.Lcd.height() - 16, 2);
}

//画面表示を更新
void render(int input, int label, unsigned long screenColor, unsigned long labelColor, bool ledValue) {
  //LEDを更新
  digitalWrite(LED_PIN, ledValue);

  //画面を更新
  M5.Lcd.fillScreen(screenColor);
  M5.Lcd.setTextColor(labelColor, screenColor);
  M5.Lcd.setTextDatum(MC_DATUM);
  M5.Lcd.drawString(String(input), M5.Lcd.width() / 2, M5.Lcd.height() / 2, 8);

  //バッテリー状態を更新
  int voltage = Map(M5.Axp.GetBatVoltage(), 3.0, 4.2, 0, 100);
  float current = M5.Axp.GetBatCurrent();

  char batteryStatus[20];
  const char* chargingIcon = current == 0 ? " " : (current > 0 ? "Charging" : "Battery");
  sprintf(batteryStatus, "%s %d%%", chargingIcon, voltage);

  M5.Lcd.setTextDatum(TR_DATUM);
  M5.Lcd.drawString(batteryStatus, M5.Lcd.width() - 1, 1, 2);
}

//画面回転を更新
void updateOrientation() {
  float accX = 0, accY = 0, accZ = 0;
  M5.IMU.getAccelData(&accX, &accY, &accZ);

  if (accZ < .9) {
    if (accX > .6) orientation = 1;
    else if (accX < .4 && accX > -.5) orientation = (accY > 0) ? 0 : 2;
    else orientation = 3;
  }

  if (orientation != previousOrientation) {
    M5.Lcd.setRotation(orientation);
  }
}

//バッテリーインジケーターの計算
int Map(float value, float start1, float stop1, float start2, float stop2) {
  return start2 + (stop2 - start2) * ((value - start1) / (stop1 - start1));
}

//html表示
String htmlMessage() {
  return htmlPage;
}

//htmlを送信
void handleRoot() {
  server.send(200, "text/html", htmlMessage());
}

//設定用htmlサーバー
void handleSetIP() {
  if (server.hasArg("SSID") && server.hasArg("Password") && server.hasArg("atemIP")) {
    ssid = server.arg("SSID");
    pass = server.arg("Password");
    String atemIPStr = server.arg("atemIP");

    if (atemIPAddr.fromString(atemIPStr)) {
      preferences.putString("ssid", ssid);
      preferences.putString("pass", pass);
      preferences.putString("atemIP", atemIPStr);
      server.send(200, "text/html", "<h1>Send Setup</h1><a href='/'>Return</a>");
      receiveIp = true;
    } else {
      server.send(400, "text/html", "<h1>Failed</h1><a href='/'>Return</a>");
    }
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void WifiSetting() {
  const IPAddress ip(192, 168, 1, 1);
  const IPAddress subnet(255, 255, 255, 0);

  M5.Lcd.println("\nFailed WiFi connecting");

  //M5Stackをhtmlサーバーとして起動
  WiFi.softAP("M5Stack", "hoge00xpw");
  delay(100);
  WiFi.softAPConfig(ip, ip, subnet);
  IPAddress apIP = WiFi.softAPIP();
  M5.Lcd.printf("Connecting to M5Stack\nPASS: hoge00xpw\nIP: <%d.%d.%d.%d> ", apIP[0], apIP[1], apIP[2], apIP[3]);

  server.on("/", handleRoot);
  server.on("/setIP", HTTP_POST, handleSetIP);
  server.begin();

  while (!receiveIp) {
    server.handleClient();
  }
  WiFi.disconnect(true);
  M5.Lcd.println("\nFinished");
}
