#include <SoftwareSerial.h>
#include <Wire.h>
#include "wifi.h"
#include <Adafruit_NeoPixel.h>
#define wifi_led 13
#ifdef __AVR__
#include <avr/power.h>
#endif
#define PIXEL_PIN 6
#define PIXEL_COUNT 11
Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

SoftwareSerial mySerial(11, 10); // RX, TX
int time_cnt = 0, cnt = 0, init_flag = 0;

void setup() {
  pinMode(4, INPUT_PULLUP);
  pinMode(13, OUTPUT);
  Serial.begin(9600);
  mySerial.begin(115200);//和涂鸦模块通信
  Serial.println("START!");
  strip.begin();
  wifi_protocol_init();
}

void loop() {
  if (init_flag == 0) {
    time_cnt++;
    if (time_cnt % 6000 == 0) {
      time_cnt = 0;
      cnt ++;
    }
    wifi_stat_led(&cnt);   // Wi-Fi状态处理
  }
  wifi_uart_service();
  myserialEvent();      // 串口接收处理
  key_scan();           // 重置配网按键检测
  if (init_flag == 1) //联网后执行
  {
    control();
  }
}

int switch_led_int = 0;
int work_mode_int = 0;
int color_change_int = 0;
int datachange = 0;
int datachange1 = 0;
int datachange2 = 0;
int datachange3 = 0;
int author = 0;
int weather_now = 0;
int weather_enable = 0;



void control() { //联网后执行循环
  if (switch_led_int == 1 && datachange == 1) {
    weather_enable = 0;
    datachange = 0;
    author = 0;
    strip.clear();
    colorfill(strip.Color(  100, 100, 100), 10);
  } else if (switch_led_int == 0 && datachange == 1) {
    weather_enable = 0;
    datachange = 0;
    author = 0;
    strip.clear();
    colorfill (strip.Color(  0, 0,   0), 10);
  }datachange = 0;

  if (datachange1 == 1) {
    switch (work_mode_int) {
      case 0:{ // 天气模式
        datachange1 = 0;
        author = 0;
        weather_enable = 1;}
        break;
      case 1:{ // 彩虹模式{
        weather_enable = 0;
         datachange1 = 0;
        author = 0;
        rainbow(10);}
        break;
      case 2:{ // 自定义{
        weather_enable = 0;
        datachange1 = 0;
        author = 1;}
        break;
    }
  }
  if (weather_enable == 1) {
    if (weather_now == 1 && datachange3 == 1) { //雨天
      strip.clear();
      colorfill (strip.Color(  0,   0, 135),10);
       datachange3 = 0;
    } else if  (weather_now == 2 && datachange3 == 1) { //晴天
      strip.clear();
      colorfill (strip.Color(135,   0,   0),10);
      datachange3 = 0;
    }
  }
  if (author == 1 && datachange2 == 1) {
    mcu_dp_enum_update(DPID_MODE_CHOOSE,2); //枚举型数据上报;
    switch (color_change_int) {
      case 0: {// R{
        datachange2 = 0;
        strip.clear();
        colorfill(strip.Color(255,   0,   0), 10);}
        break;
      case 1:{ // G{
        datachange2 = 0;
        strip.clear();
        colorfill(strip.Color(0,   255,   0), 10);}
        break;

      case 2:{ // B{
        datachange2 = 0;
        strip.clear();
        colorfill(strip.Color(0,   0,   255), 10);}
        break;

      case 3:{ // W{
        datachange2 = 0;
        strip.clear();
        colorfill(strip.Color(255, 255, 255), 10);}
        break;
    }
  }
}



// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 3 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 3*65536. Adding 256 to firstPixelHue each time
  // means we'll make 3*65536/256 = 768 passes through this outer loop:
  for (long firstPixelHue = 0; firstPixelHue < 3 * 65536; firstPixelHue += 256) {
    for (int i = 0; i < strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

void colorfill(uint32_t color, int wait) {
  for (int i = 0; i < 11; i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);
  }
}

void myserialEvent() {
  if (mySerial.available()) {
    unsigned char ch = (unsigned char)mySerial.read();
    uart_receive_input(ch);
  }
}

void key_scan(void)//重置
{
  static char ap_ez_change = 0;
  unsigned char buttonState  = HIGH;
  buttonState = digitalRead(4);
  if (buttonState == LOW) {
    delay(3000);
    buttonState = digitalRead(4);
    printf("----%d", buttonState);
    if (buttonState == LOW) {
      printf("123\r\n");
      init_flag = 0;
      switch (ap_ez_change) {
        case 0 :
          mcu_set_wifi_mode(SMART_CONFIG);
          break;
        case 1 :
          mcu_set_wifi_mode(AP_CONFIG);
          break;
        default:
          break;
      }
      ap_ez_change = !ap_ez_change;
    }
  }
}

void wifi_stat_led(int *cnt)// WiFi连接灯处理
{
  switch (mcu_get_wifi_work_state())
  {
    case SMART_CONFIG_STATE:  //0x00
      init_flag = 0;
      if (*cnt == 2) {
        *cnt = 0;
      }
      if (*cnt % 2 == 0)  //LED快闪
      {
        digitalWrite(wifi_led, LOW);
      }
      else
      {
        digitalWrite(wifi_led, HIGH);
      }
      break;
    case AP_STATE:  //0x01
      init_flag = 0;
      if (*cnt >= 30) {
        *cnt = 0;
      }
      if (*cnt  == 0)      // LED 慢闪
      {
        digitalWrite(wifi_led, LOW);
      }
      else if (*cnt == 15)
      {
        digitalWrite(wifi_led, HIGH);
      }
      break;

    case WIFI_NOT_CONNECTED:  // 0x02
      digitalWrite(wifi_led, LOW); // LED 熄灭
      break;
    case WIFI_CONNECTED:  // 0x03
      break;
    case WIFI_CONN_CLOUD:  // 0x04
      if ( 0 == init_flag )
      {
        digitalWrite(wifi_led, HIGH);// LED 常亮
        init_flag = 1;// Wi-Fi 连接上后该灯可控
        *cnt = 0;
      }
      break;

    default:
      digitalWrite(wifi_led, LOW);
      break;
  }
}

