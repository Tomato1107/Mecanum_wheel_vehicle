#include "esp_camera.h"
#include <WiFi.h>

//
// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled
//

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER

// Hardware Horizontal Mirror, 0 or 1 (overrides default board setting)
#define H_MIRROR 0

// Hardware Vertical Flip , 0 or 1 (overrides default board setting)
#define V_FLIP 1

// Browser Rotation (one of: -90,0,90, default 0)
#define CAM_ROTATION 0


#if !defined(CAM_ROTATION)
    #define CAM_ROTATION 0
#endif
int myRotation = CAM_ROTATION;

#include "camera_pins.h"
#include "robot_pins.h"

const char* ssid = "********";  //input your wifi settings
const char* password = "********";

//ESP32 SoftAP Configration
/*const char ssid[] = "ESP32_TEST";
const char password[] = "12345678";
const IPAddress ip(192,168,11,7);
const IPAddress subnet(255,255,255,0);
*/

//
#define COUNT_LOW 1638
#define COUNT_HIGH 7864
#define TIMER_WIDTH 16
#include "esp32-hal-ledc.h"
unsigned int S_0=0, S_1=0, S_2=0, S_3=0;



void startCameraServer();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  
  ledcSetup(2, 50, TIMER_WIDTH); // channel 1, 50 Hz, 16-bit width
  ledcAttachPin(12, 2);   // GPIO 22 assigned to channel 1
  S_0 = map(90, 0, 180, COUNT_LOW, COUNT_HIGH);
  ledcWrite(2, S_0); 
  
  ledcSetup(3, 50, TIMER_WIDTH); // channel 1, 50 Hz, 16-bit width
  ledcAttachPin(13, 3);   // GPIO 22 assigned to channel 1
  S_1 = map(90, 0, 180, COUNT_LOW, COUNT_HIGH);
  ledcWrite(3, S_1); 
      
  ledcSetup(4, 50, TIMER_WIDTH); // channel 2, 50 Hz, 16-bit width
  ledcAttachPin(2, 4);   // GPIO 19 assigned to channel 2
  S_2 = map(90, 0, 180, COUNT_LOW, COUNT_HIGH);
  ledcWrite(4, S_2);
  
  ledcSetup(5, 50, TIMER_WIDTH); // channel 1, 50 Hz, 16-bit width
  ledcAttachPin(15, 5);   // GPIO 22 assigned to channel 1
  S_3 = map(90, 0, 180, COUNT_LOW, COUNT_HIGH);
  ledcWrite(5, S_3);     
  
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
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  //initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);//flip it back
    s->set_brightness(s, 1);//up the blightness just a bit
    s->set_saturation(s, -2);//lower the saturation
  }
  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

// Config can override mirror and flip
#if defined(H_MIRROR)
    s->set_hmirror(s, H_MIRROR);
#endif
#if defined(V_FLIP)
    s->set_vflip(s, V_FLIP);
#endif


  //SoftAP
  /*WiFi.softAP(ssid,password);
  delay(100);
  WiFi.softAPConfig(ip,ip,subnet);
  IPAddress myIP = WiFi.softAPIP();
  */

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");


  //Serial.println("ESP32 SoftAP Mode start.");
  //Serial.print("SSID:");
  //Serial.println(ssid);

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  //Serial.print(myIP);
  Serial.println("' to connect");

  pinMode(FLASH_GPIO_NUM, OUTPUT);

  /*
  //Test for the position and rotation of each servo motors
  S_0 = map(170, 0, 180, COUNT_LOW, COUNT_HIGH);
  ledcWrite(1, S0); 
  delay(1000);
  ledcWrite(1, 90);
  
  S_1 = map(170, 0, 180, COUNT_LOW, COUNT_HIGH);
  ledcWrite(2, S1);
  delay(1000);
  ledcWrite(2, 90);

  
  S_2 = map(170, 0, 180, COUNT_LOW, COUNT_HIGH);
  ledcWrite(3, S2); 
  delay(1000);
  ledcWrite(3, 90);

  
  S_3 = map(170, 0, 180, COUNT_LOW, COUNT_HIGH);
  ledcWrite(4, S3);  
  delay(1000);
  ledcWrite(4, 90);*/
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(10000);
}
