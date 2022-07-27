// KÜTÜPHANELER EKLENİYOR....
#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include "esp_camera.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <SPIFFS.h>
#include <FS.h>
#include <Firebase_ESP_Client.h>
//Provide the token generation process info.
#include <addons/TokenHelper.h>
#include "addons/RTDBHelper.h"
//----------------------------------------------


//  WIFI ADINIZI VE ŞİFRENİZİ BURAYA ÇİFT TIRNAKLAR ARASINA GİRİNİZ
const char* ssid = "TADOSIS";
const char* password = "26tadosis26";



// tadosis ADINDAKİ FIREBASE PROJEMİZİN AYARLARINI GİRİYORUZ.
#define API_KEY "AIzaSyCxG6WEFQdU8pphW5S2QwJMTm9svLOjdO8" // firebase api keyi " " içine yazınız.

// FIREBASE' DE BULUNAN REALTIME DATABSE'İN URL'İ GİRİLİYOR.
#define DATABASE_URL "tadosis-fe49b-default-rtdb.firebaseio.com" //firebase url " " içine yazınız.

// SADECE CHECKPOINT-3 NOKTASININ BU FIREBASE'E BİR DEĞER GİRMESİ İÇİN AUTHENTICATION OLUŞTURULUYOR. DAHA ÖNCESİNDE FIREBASE KISMINDA BUNUN TANIMLANMIŞ OLMASI GEREKİR.
#define USER_EMAIL "checkpoint3@mail.com"
#define USER_PASSWORD "123456"

// ÇEKİLEN RESMİN FIRABASE'DEKİ STORAGE KAYDEDİLMESİ İÇİN STORAGE BUCKET ID'Sİ GİRİLİYOR.
#define STORAGE_BUCKET_ID "tadosis-fe49b.appspot.com"

// CHECKPOINT-3 DE ÇEKİLEN FOTONUN SPIFSS DOSYA SİSTEMİNDE ÖNCE KAYDOLMASI İÇİN BİR TANE RESİM İSMİ VERİLİYOR.
#define FILE_PHOTO "/data/cp3.jpg"

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
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
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

boolean takeNewPhoto = true;



// WIFI LEDİ İÇİN
#define wifiLed 12
bool baglantiVarMi = false;



FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;


bool taskCompleted = false;



// Check if photo capture was successful
bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( FILE_PHOTO );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}

// Capture Photo and Save it to SPIFFS
void capturePhotoSaveSpiffs( void ) {

  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly
  do {
    // Take a photo with the camera
    Serial.println("Taking a photo...");

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    // Photo file name
    Serial.printf("Picture file name: %s\n", FILE_PHOTO);
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
    // Insert the data in the photo file
    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.print("The picture has been saved in ");
      Serial.print(FILE_PHOTO);
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");

    }
    // Close the file
    file.close();
    esp_camera_fb_return(fb);

    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS);
  } while ( !ok );
}

void initWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println();
  Serial.print("Bağlandı. IP Adresi: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  baglantiVarMi = true;
}

void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }
}

void initCamera() {
  // OV2640 camera module
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

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
}


//----------------------------------------

// ÖĞRENCİ CHECKPOINT-3'E ULAŞTIĞI ZAMAN BU DEĞİŞKENE 1,SİSTEM İLK ÇALIŞTIRILDIĞINDA İSE 0 BİLGİSİ YÜKLENMEKTEDİR.
int x = 0;




void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  initWiFi();
  initSPIFFS();
  // Turn-off the 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  initCamera();

  //Firebase
  // Assign the api key
  configF.api_key = API_KEY;
  configF.database_url = DATABASE_URL;
  //Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;


  //Firebase.signUp(&configF, &auth, "", "");
  //Assign the callback function for the long running token generation task
  configF.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&configF, &auth);
  Firebase.reconnectWiFi(true);

  // FOTOĞRAFIN ÇEKİLMESİ ESNASINDA ESP32-CAM ÜSTÜNDE BULUNAN LEDİN YANMASI İÇİN GEREKLİ AYARLAMALAR YAPILIYOR
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);

  // ESP32 İNTERNETE BAĞLANINCA YEŞİL LEDİN YANMASI İÇİN
  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, LOW);


  // 13 NUMARALI BACAĞI GİRİŞ OLARAK KABUL ETTİRECEK OLURSAK;
  pinMode(13, INPUT);
  Firebase.RTDB.setInt(&fbdo, "CheckPoint-3", x);

  if (baglantiVarMi)
    digitalWrite(wifiLed, HIGH);
  else
    digitalWrite(wifiLed, LOW);
}

void loop() {
  // EĞER CHECKPOINT-3 MICROBIT'INDEN 1 BİLGİSİ GELDİYSE FOTO ÇEKİLMESİ İÇİN GEREKLİ KODLAR ÇALIŞIR.
  if (digitalRead(13) == HIGH)
  {
    while (digitalRead(13) == HIGH);

    // BURADA ÇEKME LEDİ YANMAYA BAŞLAYACAK
    digitalWrite(4, HIGH); // FLASH YANIYOR.

    x = 1;
    if (takeNewPhoto) {
      capturePhotoSaveSpiffs();
      takeNewPhoto = false;
    }
    delay(1);
    if (Firebase.ready() && !taskCompleted) {
      taskCompleted = true;
      Serial.print("Uploading picture... ");
      if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID /* Firebase Storage bucket id */, FILE_PHOTO /* path to local file */, mem_storage_type_flash /* memory storage type, mem_storage_type_flash and mem_storage_type_sd */, "checkPoint3.jpg" /* path of remote file stored in the bucket */, "image/jpeg" /* mime type */)) {
        Serial.printf("\nDownload URL: %s\n", fbdo.downloadURL().c_str());

        // Tekrar foto çekilebilmesi için
        taskCompleted = false;
        takeNewPhoto = true;

        // FOTOĞRAF FİREBASE STORAGE'A YÜKLENDİ İSE ARTIK checkPoint-1 noktasının 1 olması gerekir
        Firebase.RTDB.setInt(&fbdo, "CheckPoint-3", x);
        digitalWrite(4, LOW); // FLASH SÖNÜYOR
        delay(500); // BİRAZ BEKLETMEK İYİ OLACAK GİBİ.
      }
      else {
        Serial.println(fbdo.errorReason());
      }
    }
  }
}
