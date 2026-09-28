#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_wifi.h"

// Wi-Fi
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// AI-Thinker ESP32-CAM pins
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WebServer server(80);

void handleStream() {

  WiFiClient client = server.client();

  client.setNoDelay(true);

  server.sendContent(
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
    "Cache-Control: no-cache\r\n"
    "Connection: close\r\n\r\n"
  );

  while (client.connected()) {

    camera_fb_t *fb = esp_camera_fb_get();

    if (fb == NULL) {
      Serial.println("Camera capture failed");
      delay(10);
      continue;
    }

    server.sendContent("--frame\r\n");
    server.sendContent("Content-Type: image/jpeg\r\n");
    server.sendContent("Content-Length: " + String(fb->len) + "\r\n\r\n");

    client.write(fb->buf, fb->len);

    server.sendContent("\r\n");

    esp_camera_fb_return(fb);

    delay(30);
  }

  Serial.println("Stream client disconnected");
}


void setup() {

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(1000);

  // -------------------------
  // Camera configuration
  // -------------------------

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

  // Keep QVGA because this is currently working
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;

  if (psramFound()) {
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  }

  // -------------------------
  // Initialize camera
  // -------------------------

  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK) {

    Serial.printf(
      "Camera Init Failed: 0x%x\n",
      err
    );

    return;
  }

  Serial.println("Camera initialized");


  // -------------------------
  // Camera tuning
  // -------------------------

  sensor_t *s = esp_camera_sensor_get();

  if (s != NULL) {

    s->set_brightness(s, 1);
    s->set_contrast(s, 1);
    s->set_saturation(s, 0);
    s->set_sharpness(s, 1);

    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);

    s->set_exposure_ctrl(s, 1);
    s->set_aec2(s, 1);

    s->set_gain_ctrl(s, 1);

    s->set_bpc(s, 1);
    s->set_wpc(s, 1);
  }


  // -------------------------
  // Wi-Fi
  // -------------------------

  WiFi.mode(WIFI_STA);

  WiFi.setSleep(false);

  esp_wifi_set_ps(WIFI_PS_NONE);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wi-Fi connected");

  Serial.print("ESP32-CAM IP: ");
  Serial.println(WiFi.localIP());


  // -------------------------
  // Web server
  // -------------------------

  server.on("/", []() {

    server.send(
      200,
      "text/html",
      "<html>"
      "<body style='margin:0;background:black;'>"
      "<img src='/stream' "
      "style='width:100%;height:auto;'>"
      "</body>"
      "</html>"
    );
  });

  server.on("/stream", handleStream);

  server.begin();

  Serial.println("Web server started");
}


void loop() {

  server.handleClient();

  delay(1);
}