
#include <M5Unified.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ==========================================
// 1. 設定
// ==========================================

#define DS18B20_PIN 8
#define MEASUREMENT_INTERVAL 1000

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

// 測定履歴
float history[GRAPH_POINTS];
int historyCount = 0;

unsigned long lastMeasurement = 0;

// ==========================================
// 3. 温度による色の変更
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
// 4. 初期画面描画
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
    8, 152, 304, 80, 10, COLOR_PANEL
  );

  M5.Display.drawRoundRect(
    8, 152, 304, 80, 10, COLOR_BORDER
  );

  // グラフタイトル
  M5.Display.setTextDatum(middle_left);
  M5.Display.setTextFont(2);
  M5.Display.setTextColor(COLOR_WHITE);

  M5.Display.drawString(
    "Temperature Trend", 18, 165
  );
}

// ==========================================
// 5. 温度計アイコン描画
// ==========================================

void drawThermometer(uint16_t color) {

  int x = 37;
  int y = 90;

  // 外枠
  M5.Display.drawRoundRect(
    x - 6, y - 21, 12, 37, 6, COLOR_GRAY
  );

  // 温度計の液柱
  M5.Display.fillRoundRect(
    x - 2, y - 8, 4, 23, 2, color
  );

  // 温度計下部
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
// 6. 温度表示
// ==========================================

void drawTemperature(float temp, bool valid) {

  // メインカードの内部を更新
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

    // 温度を大きく表示
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextFont(4);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(color);

    M5.Display.drawString(
      String(temp, 2), 174, 107
    );

    // 単位
    M5.Display.setTextFont(2);
    M5.Display.setTextSize(2);
    M5.Display.drawString(
      "C", 277, 108
    );
  }

  // 接続状態
  M5.Display.setTextFont(1);
  M5.Display.setTextSize(1);
  M5.Display.setTextDatum(middle_right);

  if (valid) {
    M5.Display.setTextColor(COLOR_GREEN);
    M5.Display.drawString(
      "CONNECTED", 300, 62
    );
  } else {
    M5.Display.setTextColor(COLOR_RED);
    M5.Display.drawString(
      "DISCONNECTED", 300, 62
    );
  }
}

// ==========================================
// 7. 測定履歴を追加
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
// 8. グラフ描画
// ==========================================

void drawGraph() {

  // グラフ領域だけ更新
  M5.Display.fillRect(
    14, 176, 292, 51, COLOR_PANEL
  );

  if (historyCount == 0) {
    return;
  }

  // 温度の最大・最小を取得
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

  // 表示範囲に余裕を持たせる
  float graphMin = minTemp - 0.5;
  float graphMax = maxTemp + 0.5;

  // グラフ描画範囲
  const int xStart = 20;
  const int xEnd = 300;
  const int yTop = 184;
  const int yBottom = 218;

  // 基準線
  M5.Display.drawFastHLine(
    xStart, yBottom, xEnd - xStart, COLOR_BORDER
  );

  M5.Display.drawFastHLine(
    xStart, yTop, xEnd - xStart, COLOR_BORDER
  );

  uint16_t color =
    getTemperatureColor(history[historyCount - 1]);

  // 折れ線
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
    historyCount - 1, 0, GRAPH_POINTS - 1,
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
// 9. 初期設定
// ==========================================

void setup() {

  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(115200);

  // 横向き表示
  M5.Display.setRotation(1);

  // DS18B20設定
  pinMode(DS18B20_PIN, INPUT_PULLUP);

  sensors.begin();
  sensors.setResolution(12);

  // 画面表示
  drawBaseScreen();
  drawTemperature(0, false);

  Serial.println("DS18B20 Start");

  Serial.print("Detected Sensors: ");
  Serial.println(sensors.getDeviceCount());

  // 初回測定をすぐに実行
  lastMeasurement =
    millis() - MEASUREMENT_INTERVAL;
}

// ==========================================
// 10. メイン処理
// ==========================================

void loop() {

  M5.update();

  unsigned long now = millis();

  if (now - lastMeasurement >= MEASUREMENT_INTERVAL) {

    lastMeasurement = now;

    // 温度取得
    sensors.requestTemperatures();
    float temperature = sensors.getTempCByIndex(0);

    // 測定結果の判定
    bool valid =
      (temperature != DEVICE_DISCONNECTED_C);

    if (valid) {

      Serial.print("Temperature: ");
      Serial.print(temperature, 2);
      Serial.println(" C");

      addHistory(temperature);

    } else {

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
}