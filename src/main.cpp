#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <pgmspace.h>

// ref: https://it-taro.com/esp32_ov2640_gas1/
//https://script.google.com/macros/s/AKfycby1eAPpGDa5R9tX2YRUotVcNQj_ehg6jZUllJjKrgr-hE3FJgbyPylXgTr7zGviIzwq/exec

//*************************************************************************
//  ESP32CamGdriveAPI Ver2022.5.2
//  Arduino Board : ESP32 by Espressif Systems ver 1.0.6
//  Written by IT-Taro
//***********************************************************************
#include <WiFiClientSecure.h>

// ##################### Google,Wi-Fi設定(環境設定) #####################
String clientId           = "413730907193-g2q（省略）dt.apps.googleusercontent.com"; // 【★変更要】
String clientSecret       = "GOCSPX-gFpRtBOst（省略）zAIBl85PxxDQ"; // 【★変更要】
String refreshToken       = "1//0eUyA3JT-zY4E（省略）KP3V8JG8M-lk"; // 【★変更要】
String driveFolder        = "1fpiVQeoy（省略）a3s8kub38q"; // GoogleDrive保存フォルダId【★変更要】

const char *ssid        = "##### SSID #####";
const char *password    = "### PASSWORD ###";
// ###################################################################
const char* refreshServer = "oauth2.googleapis.com";
const char* refreshUri    = "/token";
const char* apiServer     = "www.googleapis.com";
const char* apiUri        = "/upload/drive/v3/files?uploadType=multipart";
String accessToken        = "";

// ピン配置
const byte LED_PIN      = 4;  // LED緑

// CAMERA_MODEL_M5_UNIT_CAM
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    15
#define XCLK_GPIO_NUM     27
#define SIOD_GPIO_NUM     25
#define SIOC_GPIO_NUM     23

#define Y9_GPIO_NUM       19
#define Y8_GPIO_NUM       36
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       39
#define Y5_GPIO_NUM        5
#define Y4_GPIO_NUM       34
#define Y3_GPIO_NUM       35
#define Y2_GPIO_NUM       32
#define VSYNC_GPIO_NUM    22
#define HREF_GPIO_NUM     26
#define PCLK_GPIO_NUM     21

WiFiClientSecure httpsClient;
camera_fb_t * fb;
bool ledFlag          = true; // LED制御フラグ
int waitingTime      = 30000; //Wait 30 seconds to google response.

void setup() {
  // ####### 処理開始 #######
  Serial.begin(115200);
  Serial.println();
  Serial.println("Start!"); 

  // ####### カメラ設定 #######
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;  // UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
  config.jpeg_quality = 10;
  config.fb_count = 1;
  // Newly Added! for "No PSRAM" Board "ESP32 ver 2.0.2" and "M5Stack-Unit-Cam"
  //config.fb_location = CAMERA_FB_IN_DRAM;
  // 2.0.2ではエラーが出るので上記記載が必要だがエラーが出なくなってもPingでロスが多発するため
  // 1.0.6(1.0.4)では上記記述は不要で安定動作する（1.0.xではPing確認でも安定動作）
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(3000);
    ESP.restart();
  }

  // ####### PIN設定開始 #######
  pinMode ( LED_PIN, OUTPUT );

  // ####### 無線Wi-Fi接続 #######
  WiFi.begin ( ssid, password );
  while ( WiFi.status() != WL_CONNECTED ) { // 接続するまで無限ループ処理
    // 接続中はLEDを１秒毎に点滅する
    ledFlag = !ledFlag;
    digitalWrite(LED_PIN, ledFlag);
    delay ( 1000 );
    Serial.print ( "." );
  }
  // Wi-Fi接続完了（IPアドレス表示）
  Serial.print ( "Wi-Fi Connected! IP address: " );
  Serial.println ( WiFi.localIP() );
  Serial.println ( );
  // Wi-Fi接続時はLED点灯（Wi-Fi接続状態）
  digitalWrite ( LED_PIN, true );

  // ####### NTP設定 #######
  configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");

  // ####### HTTPS証明書設定 #######
  // Serverの証明書チェックをしない（1.0.5以降で必要）【rootCAでルート証明書を設定すれば解決できる】
  httpsClient.setInsecure();//skip verification
  //httpsClient.setCACert(rootCA);// 事前にWebブラウザなどでルート証明書を取得しrootCA設定も可

  // ####### 画像取得 #######
  Serial.println("Start get JPG");
  getCameraJPEG();
  // ####### アクセストークン取得 #######
  Serial.println("Start get AccessToken");
  getAccessToken();
  // ####### GoogleDrive保存 #######
  Serial.println("Start Post GoogleDrive");
  postGoogleDriveByAPI();

  Serial.println("API Completed!!!");
}

// Loop処理
void loop() {
  delay(100);
}

//  画像をPOST処理（送信）
void postGoogleDriveByAPI() {

  Serial.println("Connect to " + String(apiServer));
  if (httpsClient.connect(apiServer, 443)) {
    Serial.println("Connection successful");

    // 時間を取得してファイル名を作成
    struct tm timeInfo;
    char preFilename[16];
    getLocalTime(&timeInfo);
    sprintf(preFilename, "%04d%02d%02d_%02d%02d%02d",
          timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
          timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
    String saveFilename = "esp32_" + String(preFilename) + ".jpg";

    String metadata = "--foo_bar_baz\r\n"
                      "Content-Type: application/json; charset=UTF-8\r\n\r\n"
                      "{\"name\":\"" + saveFilename + "\",\"parents\":[\"" + driveFolder + "\"]}\r\n\r\n"; // parentsは保存するフォルダ
                      //"{\"name\":\"" + saveFilename + "\"}\r\n\r\n"; // 保存フォルダ指定なしもOK（TOPにファイルが作成される）
    String startBoundry = "--foo_bar_baz\r\n"
                          "Content-Type:image/jpeg\r\n\r\n";
    String endBoundry   = "\r\n--foo_bar_baz--";

    unsigned long contentsLength = metadata.length() + startBoundry.length() + fb->len + endBoundry.length();
    String header = "POST " + String(apiUri) + " HTTP/1.1\r\n" +
                    "HOST: " + String(apiServer) + "\r\n" +
                    "Connection: close\r\n" +
                    "content-type: multipart/related; boundary=foo_bar_baz\r\n" +
                    "content-length: " + String(contentsLength) + "\r\n" +
                    "authorization: Bearer " + accessToken + "\r\n\r\n";

    Serial.println("Send JPEG DATA by API");
    httpsClient.print(header);
    httpsClient.print(metadata);
    httpsClient.print(startBoundry);
    //  JPEGデータは1000bytesに区切ってPOST
    unsigned long dataLength = fb->len;
    uint8_t*      bufAddr    = fb->buf;
    for(unsigned long i = 0; i < dataLength ;i=i+1000) {
      if ( (i + 1000) < dataLength ) {
        httpsClient.write(( bufAddr + i ), 1000);
      } else if (dataLength%1000 != 0) {
        httpsClient.write(( bufAddr + i ), dataLength%1000);
      }
    }
    httpsClient.print(endBoundry);

    Serial.println("Waiting for response.");
    long int StartTime=millis();
    while (!httpsClient.available()) {
      Serial.print(".");
      delay(100);
      if ((StartTime+waitingTime) < millis()) {
        Serial.println();
        Serial.println("No response.");
        break;
      }
    }
    Serial.println();
    while (httpsClient.available()) {
      Serial.print(char(httpsClient.read()));
    }
    /*Serial.println("Recieving Reply");
    while (httpsClient.connected()) {
      String retLine = httpsClient.readStringUntil('\n');
      //Serial.println("retLine:" + retLine);
      int okStartPos = retLine.indexOf("200 OK");
      if (okStartPos >= 0) {
        Serial.println("200 OK");
        break;
      }
    }*/
  } else {
    Serial.println("Connected to " + String(refreshServer) + " failed.");
  }
  httpsClient.stop();
}

// リフレッシュトークンでアクセストークンを更新（アクセストークン有効時間は1時間）
void getAccessToken() {
  accessToken = "None";
  // ####### アクセストークン取得 #######
  Serial.println("Connect to " + String(refreshServer));
  if (httpsClient.connect(refreshServer, 443)) {
    Serial.println("Connection successful");

    String body = "client_id="     + clientId     + "&" +
                  "client_secret=" + clientSecret + "&" +
                  "refresh_token=" + refreshToken + "&" +
                  "grant_type=refresh_token";

    // Send Header
    httpsClient.println("POST "  + String(refreshUri) + " HTTP/1.1");
    httpsClient.println("Host: " + String(refreshServer));
    httpsClient.println("content-length: " + (String)body.length());
    httpsClient.println("Content-Type: application/x-www-form-urlencoded");
    httpsClient.println();
    // Send Body
    httpsClient.println(body);

    Serial.println("Recieving Token");
    while (httpsClient.connected()) {
      String retLine = httpsClient.readStringUntil('\n');
      //Serial.println("retLine:" + retLine);
      int tokenStartPos = retLine.indexOf("access_token");
      if (tokenStartPos >= 0) {
        tokenStartPos     = retLine.indexOf("\"", tokenStartPos) + 1;
        tokenStartPos     = retLine.indexOf("\"", tokenStartPos) + 1;
        int tokenEndPos   = retLine.indexOf("\"", tokenStartPos);
        accessToken       = retLine.substring(tokenStartPos, tokenEndPos); 
        Serial.println("AccessToken:"+accessToken);
        break;
      }
    }
  } else {
    Serial.println("Connected to " + String(refreshServer) + " failed.");
  }
  httpsClient.stop();
  if (accessToken == "None") {
    Serial.println("Get AccessToken Failed. Restart ESP32!");
    delay(3000);
    ESP.restart();
  }
}

// OV2640でJPEG画像取得
void getCameraJPEG(){
  //  数回撮影してAE(自動露出)をあわせる
  esp_camera_fb_get();
  esp_camera_fb_get();
  fb = esp_camera_fb_get();  //JPEG取得
  if (!fb) {
    Serial.printf("Camera capture failed");
  }
  // 撮影終了処理
  esp_camera_fb_return(fb);
  Serial.printf("JPG: %uB ", (uint32_t)(fb->len));
}