
#include <M5Unified.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>

#include "secrets.h"

// ==========================================
// 1. 設定
// ==========================================

#define DS18B20_PIN 8

// 温度測定間隔
#define MEASUREMENT_INTERVAL 1000

// AWS送信間隔
#define PUBLISH_INTERVAL 5000

// 再接続間隔
#define WIFI_RECONNECT_INTERVAL 10000
#define MQTT_RECONNECT_INTERVAL 5000

// MQTTポート
#define AWS_IOT_PORT 8883

// グラフに保持する測定回数
#define GRAPH_POINTS 40

// 画面カラー（RGB565）
#define COLOR_BG      0x080F
#define COLOR_PANEL   0x10A4
#define COLOR_BORDER  0x2949
#define COLOR_WHITE   0xFFFF
#define COLOR_GRAY    0xAD55
#define COLOR_GREEN   0x4FEA
#define COLOR_BLUE    0x4DFF
#define COLOR_ORANGE  0xFD20
#define COLOR_RED     0xF9A7

// ==========================================
// 2. センサ設定
// ==========================================

OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

// ==========================================
// 3. AWS設定
// ==========================================

WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

// ==========================================
// 4. 変数
// ==========================================

// 測定履歴
float history[GRAPH_POINTS];
int historyCount = 0;

// 最新の測定値
float currentTemperature = 0.0;

// 最新の測定時刻（UNIXTIME）
time_t measurementTimestamp = 0;

// 測定データが有効か
bool temperatureValid = false;

// NTP設定済み
bool ntpConfigured = false;

// 最終実行時刻
unsigned long lastMeasurement = 0;
unsigned long lastPublish = 0;
unsigned long lastWiFiAttempt = 0;
unsigned long lastMQTTAttempt = 0;
unsigned long lastStatusDraw = 0;

// 送信状態
enum PublishStatus {
  PUBLISH_WAIT,
  PUBLISH_SUCCESS,
  PUBLISH_FAILED,
  PUBLISH_NO_DATA
};

PublishStatus publishStatus = PUBLISH_WAIT;

// ==========================================
// 5. 温度による色の変更
// ==========================================

uint16_t getTemperatureColor(float temp) {

  if (temp < 15.0) {
    return COLOR_BLUE;
  } else if (temp < 30.0) {
    return COLOR_GREEN;
  } else {
    return COLOR_ORANGE;
  }
}

// ==========================================
// 6. 初期画面描画
// ==========================================

void drawBaseScreen() {

  M5.Display.fillScreen(COLOR_BG);

  // ヘッダー
  M5.Display.fillRoundRect(
    8, 7, 304, 33, 8, COLOR_PANEL
  );

  M5.Display.setTextDatum(middle_left);
  M5.Display.setTextFont(2);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(COLOR_WHITE);

  M5.Display.drawString(
    "DS18B20", 18, 23
  );

  M5.Display.setTextDatum(middle_right);
  M5.Display.setTextColor(COLOR_GRAY);

  M5.Display.drawString(
    "CoreS3", 302, 23
  );

  // メインカード
  M5.Display.fillRoundRect(
    8, 46, 304, 99, 12, COLOR_PANEL
  );

  M5.Display.drawRoundRect(
    8, 46, 304, 99, 12, COLOR_BORDER
  );

  // グラフカード
  M5.Display.fillRoundRect(
    8, 152, 304, 67, 10, COLOR_PANEL
  );

  M5.Display.drawRoundRect(
    8, 152, 304, 67, 10, COLOR_BORDER
  );

  // グラフタイトル
  M5.Display.setTextDatum(middle_left);
  M5.Display.setTextFont(2);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(COLOR_WHITE);

  M5.Display.drawString(
    "Temperature Trend", 18, 165
  );
}

// ==========================================
// 7. 温度計アイコン描画
// ==========================================

void drawThermometer(uint16_t color) {

  int x = 37;
  int y = 90;

  M5.Display.drawRoundRect(
    x - 6, y - 21, 12, 37, 6, COLOR_GRAY
  );

  M5.Display.fillRoundRect(
    x - 2, y - 8, 4, 23, 2, color
  );

  M5.Display.fillCircle(
    x, y + 16, 10, COLOR_PANEL
  );

  M5.Display.drawCircle(
    x, y + 16, 10, COLOR_GRAY
  );

  M5.Display.fillCircle(
    x, y + 16, 6, color
  );
}

// ==========================================
// 8. 温度表示
// ==========================================

void drawTemperature(float temp, bool valid) {

  M5.Display.fillRoundRect(
    10, 48, 300, 95, 10, COLOR_PANEL
  );

  M5.Display.setTextDatum(middle_left);
  M5.Display.setTextFont(2);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(COLOR_GRAY);

  M5.Display.drawString(
    "TEMPERATURE", 61, 65
  );

  if (!valid) {

    drawThermometer(COLOR_RED);

    M5.Display.setTextFont(4);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(COLOR_RED);
    M5.Display.setTextDatum(middle_center);

    M5.Display.drawString(
      "Sensor Error", 185, 102
    );

  } else {

    uint16_t color = getTemperatureColor(temp);

    drawThermometer(color);

    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextFont(4);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(color);

    M5.Display.drawString(
      String(temp, 2), 174, 107
    );

    M5.Display.setTextFont(2);
    M5.Display.setTextSize(2);

    M5.Display.drawString(
      "C", 277, 108
    );
  }

  // センサ状態
  M5.Display.setTextFont(1);
  M5.Display.setTextSize(1);
  M5.Display.setTextDatum(middle_right);

  if (valid) {

    M5.Display.setTextColor(COLOR_GREEN);
    M5.Display.drawString(
      "SENSOR OK", 300, 62
    );

  } else {

    M5.Display.setTextColor(COLOR_RED);
    M5.Display.drawString(
      "SENSOR NG", 300, 62
    );
  }
}

// ==========================================
// 9. 測定履歴を追加
// ==========================================

void addHistory(float temp) {

  if (historyCount < GRAPH_POINTS) {

    history[historyCount] = temp;
    historyCount++;

  } else {

    for (int i = 0; i < GRAPH_POINTS - 1; i++) {
      history[i] = history[i + 1];
    }

    history[GRAPH_POINTS - 1] = temp;
  }
}

// ==========================================
// 10. グラフ描画
// ==========================================

void drawGraph() {

  // グラフ描画領域
  M5.Display.fillRect(
    14, 176, 292, 39, COLOR_PANEL
  );

  if (historyCount == 0) {
    return;
  }

  float minTemp = history[0];
  float maxTemp = history[0];

  for (int i = 1; i < historyCount; i++) {

    if (history[i] < minTemp) {
      minTemp = history[i];
    }

    if (history[i] > maxTemp) {
      maxTemp = history[i];
    }
  }

  float graphMin = minTemp - 0.5;
  float graphMax = maxTemp + 0.5;

  const int xStart = 20;
  const int xEnd = 300;
  const int yTop = 180;
  const int yBottom = 211;

  // 基準線
  M5.Display.drawFastHLine(
    xStart, yBottom,
    xEnd - xStart, COLOR_BORDER
  );

  M5.Display.drawFastHLine(
    xStart, yTop,
    xEnd - xStart, COLOR_BORDER
  );

  uint16_t color =
    getTemperatureColor(history[historyCount - 1]);

  // 折れ線描画
  for (int i = 1; i < historyCount; i++) {

    int x1 = map(
      i - 1, 0, GRAPH_POINTS - 1,
      xStart, xEnd
    );

    int x2 = map(
      i, 0, GRAPH_POINTS - 1,
      xStart, xEnd
    );

    int y1 = yBottom -
      (history[i - 1] - graphMin) *
      (yBottom - yTop) /
      (graphMax - graphMin);

    int y2 = yBottom -
      (history[i] - graphMin) *
      (yBottom - yTop) /
      (graphMax - graphMin);

    M5.Display.drawLine(
      x1, y1, x2, y2, color
    );
  }

  // 最新値に丸印
  int latestX = map(
    historyCount - 1,
    0, GRAPH_POINTS - 1,
    xStart, xEnd
  );

  int latestY = yBottom -
    (history[historyCount - 1] - graphMin) *
    (yBottom - yTop) /
    (graphMax - graphMin);

  M5.Display.fillCircle(
    latestX, latestY, 3, color
  );
}

// ==========================================
// 11. 通信状態表示
// ==========================================

void drawConnectionStatus() {

  // 画面下部をクリア
  M5.Display.fillRect(
    0, 222, 320, 18, COLOR_BG
  );

  M5.Display.setTextFont(1);
  M5.Display.setTextSize(1);
  M5.Display.setTextDatum(middle_left);

  // Wi-Fi接続状態
  if (WiFi.status() == WL_CONNECTED) {

    M5.Display.setTextColor(COLOR_GREEN);
    M5.Display.drawString(
      "WiFi OK", 12, 231
    );

  } else {

    M5.Display.setTextColor(COLOR_RED);
    M5.Display.drawString(
      "WiFi NG", 12, 231
    );
  }

  // AWS接続状態
  if (mqttClient.connected()) {

    M5.Display.setTextColor(COLOR_GREEN);
    M5.Display.drawString(
      "AWS OK", 115, 231
    );

  } else {

    M5.Display.setTextColor(COLOR_RED);
    M5.Display.drawString(
      "AWS NG", 115, 231
    );
  }

  // データ送信状態
  M5.Display.setTextDatum(middle_right);

  switch (publishStatus) {

    case PUBLISH_SUCCESS:
      M5.Display.setTextColor(COLOR_GREEN);
      M5.Display.drawString(
        "SENT", 305, 231
      );
      break;

    case PUBLISH_FAILED:
      M5.Display.setTextColor(COLOR_RED);
      M5.Display.drawString(
        "FAILED", 305, 231
      );
      break;

    case PUBLISH_NO_DATA:
      M5.Display.setTextColor(COLOR_ORANGE);
      M5.Display.drawString(
        "NO DATA", 305, 231
      );
      break;

    default:
      M5.Display.setTextColor(COLOR_GRAY);
      M5.Display.drawString(
        "WAIT", 305, 231
      );
      break;
  }
}

// ==========================================
// 12. Wi-Fi接続開始
// ==========================================

void connectWiFi() {

  Serial.println("Connecting to Wi-Fi...");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  lastWiFiAttempt = millis();
}

// ==========================================
// 13. Wi-Fi接続管理
// ==========================================

void maintainWiFi() {

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if (millis() - lastWiFiAttempt >=
      WIFI_RECONNECT_INTERVAL) {

    Serial.println("Retrying Wi-Fi...");

    WiFi.disconnect();

    WiFi.begin(
      WIFI_SSID,
      WIFI_PASSWORD
    );

    lastWiFiAttempt = millis();
  }
}

// ==========================================
// 14. NTP時刻同期
// ==========================================

void maintainTime() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!ntpConfigured) {

    // UTCで時刻同期
    configTime(
      0,
      0,
      "pool.ntp.org",
      "time.google.com"
    );

    ntpConfigured = true;

    Serial.println("NTP configured");
  }
}

// ==========================================
// 15. 時刻同期確認
// ==========================================

bool isTimeValid() {

  time_t now = time(nullptr);

  // 明らかに不正なUNIXTIMEを除外
  return now >= 1700000000;
}

// ==========================================
// 16. AWS IoT Core接続
// ==========================================

void connectAWS() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (mqttClient.connected()) {
    return;
  }

  // 時刻同期完了まで接続しない
  if (!isTimeValid()) {

    Serial.println("Waiting for NTP...");
    return;
  }

  Serial.println(
    "Connecting to AWS IoT Core..."
  );

  bool connected = mqttClient.connect(
    AWS_IOT_CLIENT_ID
  );

  if (connected) {

    Serial.println(
      "AWS IoT Core connected"
    );

  } else {

    Serial.print(
      "AWS connection failed. State: "
    );

    Serial.println(mqttClient.state());
  }
}

// ==========================================
// 17. AWS接続管理
// ==========================================

void maintainAWS() {

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (mqttClient.connected()) {

    mqttClient.loop();
    return;
  }

  if (millis() - lastMQTTAttempt >=
      MQTT_RECONNECT_INTERVAL) {

    lastMQTTAttempt = millis();

    connectAWS();
  }
}

// ==========================================
// 18. 温度測定
// ==========================================

void measureTemperature() {

  pinMode(DS18B20_PIN, INPUT_PULLUP);

  // 測定開始
  sensors.requestTemperatures();

  // 温度取得
  float temperature =
    sensors.getTempCByIndex(0);

  // 測定時刻を取得
  time_t timestamp = time(nullptr);

  // 測定結果の判定
  bool valid =
    (temperature != DEVICE_DISCONNECTED_C) &&
    (temperature >= -55.0) &&
    (temperature <= 125.0);

  temperatureValid = valid;

  if (valid) {

    currentTemperature = temperature;

    // 時刻が有効な場合のみ保持
    if (timestamp >= 1700000000) {
      measurementTimestamp = timestamp;
    } else {
      measurementTimestamp = 0;
    }

    Serial.print("Temperature: ");
    Serial.print(currentTemperature, 2);
    Serial.println(" C");

    Serial.print("UNIXTIME: ");
    Serial.println((long long)measurementTimestamp);

    // グラフに追加
    addHistory(currentTemperature);

  } else {

    measurementTimestamp = 0;

    Serial.println(
      "Error: DS18B20 not connected"
    );
  }

  // 画面更新
  M5.Display.startWrite();

  drawTemperature(temperature, valid);
  drawGraph();

  M5.Display.endWrite();
}

// ==========================================
// 19. AWSへ測定データ送信
// ==========================================

void publishTemperature() {

  // 温度が無効
  if (!temperatureValid) {

    publishStatus = PUBLISH_NO_DATA;

    Serial.println(
      "Publish skipped: Invalid temperature"
    );

    return;
  }

  // 時刻が無効
  if (measurementTimestamp == 0) {

    publishStatus = PUBLISH_NO_DATA;

    Serial.println(
      "Publish skipped: Invalid timestamp"
    );

    return;
  }

  // AWS未接続
  if (!mqttClient.connected()) {

    publishStatus = PUBLISH_FAILED;

    Serial.println(
      "Publish skipped: AWS disconnected"
    );

    return;
  }

  // ========================================
  // JSON作成
  // ========================================

  char payload[256];

  snprintf(
    payload,
    sizeof(payload),
    "{\"device_id\":\"%s\",\"timestamp\":%lld,\"temperature\":%.2f}",
    AWS_IOT_CLIENT_ID,
    (long long)measurementTimestamp,
    currentTemperature
  );

  // ========================================
  // AWS IoT Coreへ送信
  // ========================================

  bool result = mqttClient.publish(
    AWS_IOT_PUBLISH_TOPIC,
    payload
  );

  if (result) {

    publishStatus = PUBLISH_SUCCESS;

    Serial.print("Published: ");
    Serial.println(payload);

  } else {

    publishStatus = PUBLISH_FAILED;

    Serial.println("Publish failed");
  }
}

// ==========================================
// 20. 初期設定
// ==========================================

void setup() {

  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(115200);

  // 横向き表示
  M5.Display.setRotation(1);

  // ========================================
  // DS18B20初期化
  // ========================================

  pinMode(
    DS18B20_PIN,
    INPUT_PULLUP
  );

  sensors.begin();
  sensors.setResolution(12);

  Serial.println("DS18B20 Start");

  Serial.print("Detected Sensors: ");
  Serial.println(sensors.getDeviceCount());

  // ========================================
  // 画面初期化
  // ========================================

  drawBaseScreen();
  drawTemperature(0, false);
  drawConnectionStatus();

  // ========================================
  // AWS証明書設定
  // ========================================

  secureClient.setCACert(
    AWS_CERT_CA
  );

  secureClient.setCertificate(
    AWS_CERT_CRT
  );

  secureClient.setPrivateKey(
    AWS_CERT_PRIVATE
  );

  secureClient.setTimeout(3000);

  // MQTT設定
  mqttClient.setServer(
    AWS_IOT_ENDPOINT,
    AWS_IOT_PORT
  );

  mqttClient.setSocketTimeout(3);
  mqttClient.setKeepAlive(30);

  // ========================================
  // Wi-Fi接続開始
  // ========================================

  connectWiFi();

  // 初回測定をすぐに開始
  lastMeasurement =
    millis() - MEASUREMENT_INTERVAL;

  lastPublish = millis();

  lastMQTTAttempt =
    millis() - MQTT_RECONNECT_INTERVAL;
}

// ==========================================
// 21. メイン処理
// ==========================================

void loop() {

  M5.update();

  // ========================================
  // Wi-Fi接続管理
  // ========================================

  maintainWiFi();

  // ========================================
  // NTP時刻同期
  // ========================================

  maintainTime();

  // ========================================
  // AWS接続管理
  // ========================================

  maintainAWS();

  unsigned long now = millis();

  // ========================================
  // 温度測定（1秒間隔）
  // ========================================

  if (now - lastMeasurement >=
      MEASUREMENT_INTERVAL) {

    lastMeasurement = now;

    measureTemperature();
  }

  // ========================================
  // AWS送信（5秒間隔）
  // ========================================

  if (now - lastPublish >=
      PUBLISH_INTERVAL) {

    lastPublish = now;

    publishTemperature();
  }

  // ========================================
  // 通信状態表示（1秒間隔）
  // ========================================

  if (now - lastStatusDraw >= 1000) {

    lastStatusDraw = now;

    M5.Display.startWrite();

    drawConnectionStatus();

    M5.Display.endWrite();
  }

  delay(10);
}
