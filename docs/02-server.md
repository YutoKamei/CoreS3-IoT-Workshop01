# STEP 2：測定した温度をAWSへ送ろう

[🏠 トップ](../README.md) | [← STEP 1](01-temperature.md) | [次へ：STEP 3 →](03-smartphone.md)

## このSTEPの目標

**M5Stack CoreS3をWi-Fiに接続し、測定した温度をAWS IoT Coreへ送信します。**

STEP 1ではCoreS3本体とパソコンに温度を表示しました。今回は、離れた場所にデータを届ける機能を追加します。

## 1. Wi-FiとAWS IoT Coreの役割

```text
DS18B20 → M5Stack CoreS3 → Wi-Fi → AWS IoT Core
          温度を測定・表示       MQTTで送信
                                    ↓
                           後続の保存・表示処理
                           （講師が別途設定）
```

- **Wi-Fi**：CoreS3をネットワークに接続する仕組みです。
- **AWS IoT Core**：IoT機器から届くメッセージを受け取るクラウドサービスです。
- **MQTT**：小さなデータをやり取りする際によく使われる通信方式です。
- **トピック**：MQTTでメッセージの送信先を区別するための名前です。
- **TLS**：通信内容を暗号化する仕組みです。今回は証明書による認証も使用します。
- **JSON**：データを「名前と値」の組み合わせで表す形式です。

> [!NOTE]
> 今回のプログラムは**TLSで保護されたMQTT通信**を使用します。接続先のポート番号は `8883` です。

## 2. AWSへ送信するデータを見てみよう

今回のプログラムでは、次のようなJSONを送信します（値は例です）。

```json
{
  "device_id": "IoT-Workshop-CoreS3-01",
  "timestamp": 1795500000,
  "temperature": 24.50
}
```

| 項目 | 意味 |
| --- | --- |
| `device_id` | CoreS3を識別する名前。`AWS_IOT_CLIENT_ID` の値を使用します。 |
| `timestamp` | 温度を測定した時刻を表すUNIX時刻（秒単位）です。 |
| `temperature` | 測定した温度（℃）です。 |

**温度測定は約1秒ごと、AWSへの送信は約5秒ごと**に行います。したがって、通常は毎秒の測定値すべてではなく、送信時点で保持している最新の測定値を送ります。

> [!NOTE]
> CoreS3はWi-Fi接続後にNTPを使用して時刻を合わせます。
>
> 時刻が有効になる前は、AWSへの接続や温度データの送信が行われません。

## 3. 自分のデバイス番号を確認する

講師から指定されたデバイス番号を確認しましょう。

- 自分のデバイス番号：`IoT-Workshop-CoreS3-____`
- 送信先：講師が設定した **AWS IoT Core**
- MQTTトピック：講師が指定した値

他のグループとデータが混ざらないよう、デバイス番号やトピックが割り当てられています。

> [!IMPORTANT]
> **ほかのグループの設定値に変更しないでください。**
>
> デバイス番号とMQTTトピックは、講師が配布する設定ファイルで指定されています。

## 4. プログラムと設定ファイルを用意する

### 4-1. STEP 2のプログラムを開く

今回は、STEP 1の温度測定・画面表示に加えて、Wi-Fi接続とAWS IoT Coreへの送信機能を備えたプログラムを使用します。

1. 講師が指定した[STEP 2の完成版プログラム](../code/step2/step2.ino)を開き、コードをコピーして新しいスケッチに貼り付けます。
2. プログラムの冒頭に `#include "secrets.h"` があることを確認します。

```cpp
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>
#include "secrets.h"
```

今回新しく使用する `PubSubClient` ライブラリは、Arduino IDEのライブラリマネージャーから導入できます。研修では講師が事前にインストールしておきます。

### 4-2. `secrets_sample.h` を確認し、`secrets.h` を受け取る

GitHubには、接続設定の書き方を示す **[`secrets_sample.h`（サンプル設定ファイル）](../code/step2/secrets_sample.h)** を用意しています。

`secrets_sample.h` では、Wi-FiのSSID・パスワード、AWS IoT Coreのエンドポイント、クライアントID、MQTTトピック、証明書・秘密鍵の**記入場所**を確認できます。

ただし、値は空欄またはダミーなので、そのままではAWS IoT Coreへ接続できません。

実習では、必要な接続情報が設定された **`secrets.h` を講師から各PCへ直接配布**します。`secrets.h` 自体はGitHubでは公開しません。

1. GitHubで [`secrets_sample.h`](../code/step2/secrets_sample.h) を開き、どのような設定項目があるか確認します。
2. 講師から、自分のCoreS3用の `secrets.h` を受け取ります。
3. Arduino IDEで開いている `.ino` ファイルと**同じスケッチフォルダ**に `secrets.h` を配置します。
4. ファイル名が **`secrets.h`** になっていることを確認します（`secrets_sample.h` のままでは読み込まれません）。
5. 設定内容は変更せず、そのまま使用します。

フォルダの配置例：

```text
CoreS3-IoT-Workshop01_04/
├── CoreS3-IoT-Workshop01_04.ino
└── secrets.h           ← 講師から受け取る（公開しない）
```

> [!IMPORTANT]
> **`secrets.h` は公開しないでください。**
>
> `secrets.h` にはWi-Fiパスワード、デバイス証明書、秘密鍵が含まれます。
>
> GitHub、SNS、公開フォルダなどにアップロードせず、ファイルの中身をスクリーンショットにも載せないでください。
>
> 誤って公開してしまった場合は、自分で削除するだけで済ませず、すぐに講師へ知らせてください。

### 4-3. `secrets_sample.h` で設定項目を確認する

公開されている [`secrets_sample.h`](../code/step2/secrets_sample.h) を見ながら、次の設定項目の役割を確認しましょう。

実際の `secrets.h` には講師が各デバイス向けの値を設定しています。

| 設定名 | 役割 |
| --- | --- |
| `WIFI_SSID` | 接続するWi-Fiネットワークの名前 |
| `WIFI_PASSWORD` | Wi-Fiのパスワード |
| `AWS_IOT_ENDPOINT` | AWS IoT Coreの接続先 |
| `AWS_IOT_CLIENT_ID` | CoreS3を識別するクライアントID |
| `AWS_IOT_PUBLISH_TOPIC` | MQTTメッセージの送信先トピック |
| `AWS_CERT_CA` | 接続先の信頼性を確認するためのCA証明書 |
| `AWS_CERT_CRT` | デバイスを認証するための証明書 |
| `AWS_CERT_PRIVATE` | デバイス認証に用いる秘密鍵 |

> [!TIP]
> **なぜ設定ファイルを分けるの？**
>
> 温度測定などのプログラムと、パスワード・秘密鍵などの設定情報を分けて管理するためです。
>
> 同じプログラムを使っても、デバイスごとに異なる設定ファイルを組み合わせられます。

## 5. プログラムを書き込んで送信する

1. CoreS3をUSB Type-CケーブルでPCに接続します。
2. Arduino IDEのボードを **M5CoreS3** に設定し、正しいポートを選択します。
3. `secrets.h` がスケッチフォルダにあることを確認します。
4. Arduino IDEの **書き込み（→）** ボタンをクリックします。
5. 書き込みが完了したら、シリアルモニタを **115200 baud** で開きます。
6. Wi-Fi接続、時刻同期、AWS接続、温度データ送信の順に状態を確認します。

シリアルモニタの表示例（数値や順番は実行環境によって異なります）：

```text
DS18B20 Start
Detected Sensors: 1
Connecting to Wi-Fi...
NTP configured
Connecting to AWS IoT Core...
AWS IoT Core connected
Temperature: 24.50 C
UNIXTIME: 1795500000
Published: {"device_id":"IoT-Workshop-CoreS3-01","timestamp":1795500000,"temperature":24.50}
```

> [!NOTE]
> Wi-Fiに接続した直後は、NTPで時刻を合わせるため、AWS接続まで少し時間がかかる場合があります。
>
> `Published:` は、プログラムがMQTTクライアントから送信を開始できたことを表します。
>
> AWS IoT Coreが受信したことやデータベースに保存されたことまで保証する表示ではありません。
>
> 受信状況は講師側で確認します。

### 5-1. CoreS3の画面で通信状態を確認する

今回のプログラムでは、温度のほかに画面下部へ通信状態を表示します。

| 表示 | 意味 |
| --- | --- |
| `WiFi OK` / `WiFi NG` | Wi-Fiの接続状態 |
| `AWS OK` / `AWS NG` | AWS IoT CoreへのMQTT接続状態 |
| `WAIT` | まだ送信結果がない状態 |
| `SENT` | 直近のMQTT送信処理が成功した状態 |
| `FAILED` | 直近の送信処理に失敗した状態 |
| `NO DATA` | 有効な温度または測定時刻がなく送信を見送った状態 |

> [!NOTE]
> `SENT` は最後に試みた送信の結果を表示するもので、サーバでの保存完了を表すものではありません。
>
> また、表示は次の送信処理まで残ります。

## 6. プログラムの重要な部分

| コード | 意味 |
| --- | --- |
| `#include "secrets.h"` | 講師が配布した接続設定を読み込む |
| `#define DS18B20_PIN 8` | DS18B20のデータ端子（GPIO8） |
| `#define MEASUREMENT_INTERVAL 1000` | 温度測定間隔（約1秒） |
| `#define PUBLISH_INTERVAL 5000` | AWSへの送信間隔（約5秒） |
| `#define AWS_IOT_PORT 8883` | TLSを使用するMQTTの接続ポート |
| `connectWiFi();` | Wi-Fi接続を開始する |
| `maintainTime();` | NTPによる時刻同期を開始する |
| `maintainAWS();` | AWSとの接続を管理する |
| `measureTemperature();` | 温度を測定し、測定時刻とともに保持する |
| `publishTemperature();` | 最新の温度をJSONにし、MQTTで送信する |
| `mqttClient.publish(...)` | 指定されたMQTTトピックへ送信する |
| `drawConnectionStatus();` | CoreS3の画面に通信状態を描画する |

## 基本課題

- [ ] `secrets.h` を指定のスケッチフォルダに配置できた
- [ ] CoreS3にプログラムを書き込めた
- [ ] CoreS3の画面に `WiFi OK` が表示された
- [ ] CoreS3の画面に `AWS OK` が表示された
- [ ] 温度測定が継続していることを確認できた
- [ ] シリアルモニタに `Published:` が表示された
- [ ] 講師と一緒にAWS IoT Coreでの受信を確認できた

## チャレンジ課題

### チャレンジA（やさしい）：送信間隔を変更しよう

プログラムの次の設定を確認しましょう。

```cpp
#define PUBLISH_INTERVAL 5000
```

`5000` を `10000` に変更すると、送信間隔はどう変わるでしょうか？ 温度を**測定する間隔**との違いも考えてみましょう。

### チャレンジB（ふつう）：Wi-Fi接続が切れたらどうなる？

Wi-FiやAWSへの接続が切れた場合、CoreS3の画面表示やシリアルモニタにどのような変化が起きるか、プログラムを読んで考えましょう。

※ 実際に通信を切断する実験は、講師の指示がある場合のみ行ってください。

### チャレンジC（難しい）：送信成功と保存成功は同じ？

`mqttClient.publish(...)` が成功した場合、何が分かり、何がまだ確認できないでしょうか？ 

講師にAWS側の受信状況を見せてもらい、比較してみましょう。

## 考えてみよう

- なぜCoreS3で測定した温度をサーバへ送るのでしょうか？
- Wi-Fiがつながらない場合でも、CoreS3で温度を測定できますか？

---

[← STEP 1](01-temperature.md) | **[次へ：STEP 3 スマホ確認 →](03-smartphone.md)**
