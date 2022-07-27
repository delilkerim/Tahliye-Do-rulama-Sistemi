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

// SADECE CHECKPOINT-1 NOKTASININ BU FIREBASE'E BİR DEĞER GİRMESİ İÇİN AUTHENTICATION OLUŞTURULUYOR. DAHA ÖNCESİNDE FIREBASE KISMINDA BUNUN TANIMLANMIŞ OLMASI GEREKİR.
#define USER_EMAIL "checkpoint1@mail.com"
#define USER_PASSWORD "123456"


// WIFI LEDİ İÇİN
#define wifiLed 12
bool baglantiVarMi = false;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;

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

// ÖĞRENCİ CHECKPOINT-1'E ULAŞTIĞI ZAMAN BU DEĞİŞKENE 1,SİSTEM İLK ÇALIŞTIRILDIĞINDA İSE 0 BİLGİSİ YÜKLENMEKTEDİR.
int x = 0;

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  initWiFi();

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

  // ESP32 İNTERNETE BAĞLANINCA YEŞİL LEDİN YANMASI İÇİN
  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, LOW);

  // 13 NUMARALI BACAĞI GİRİŞ OLARAK KABUL ETTİRECEK OLURSAK;
  pinMode(13, INPUT);
  Firebase.RTDB.setInt(&fbdo, "CheckPoint-1", x);

  if (baglantiVarMi)
    digitalWrite(wifiLed, HIGH);
  else
    digitalWrite(wifiLed, LOW);
}

void loop() {
  // EĞER CHECKPOINT-1 MICROBIT'INDEN 1 BİLGİSİ GELDİYSE FIREBASE DURUMU 1 OLARAK KAYDEDİLECEKTİR.
  if (digitalRead(13) == HIGH)
  {
    while (digitalRead(13) == HIGH);    
    x = 1;    
    if (Firebase.ready()) {         
        // checkPoint-1 noktasının 1 olması gerekir
        Firebase.RTDB.setInt(&fbdo, "CheckPoint-1", x);        
        delay(500); // BİRAZ BEKLETMEK İYİ OLACAK GİBİ.
      }
      else {
        Serial.println(fbdo.errorReason());
      }
    }
  }
