
#ifndef SECRETS_H
#define SECRETS_H

// ==========================================
// 1. Wi-Fi設定
// ==========================================

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// ==========================================
// 2. AWS IoT Core設定
// ==========================================

// AWS IoT Coreのデバイスデータエンドポイント
#define AWS_IOT_ENDPOINT ""

// MQTTクライアントID（一意の名前）
#define AWS_IOT_CLIENT_ID ""

// MQTT送信先トピック
#define AWS_IOT_PUBLISH_TOPIC ""

// ==========================================
// 3. AWS IoT Core 証明書
// ==========================================

// Amazon Root CA 1
static const char AWS_CERT_CA[] = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";

// デバイス証明書
static const char AWS_CERT_CRT[] = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";

// 秘密鍵
static const char AWS_CERT_PRIVATE[] = R"EOF(
-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----
)EOF";

#endif
