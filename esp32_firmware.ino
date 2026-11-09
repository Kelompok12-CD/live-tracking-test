#include <SPI.h>
#include <MFRC522.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// Configuration Pin RFID RC522 (SPI)
#define SS_PIN 5
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);

// Configuration Pin SIM800L (Dipindah dari SPI)
#define MODEM_RX 4
#define MODEM_TX 2

// Configuration Pin GPS NEO-6M
#define GPS_RX 16
#define GPS_TX 17

HardwareSerial serialGPS(1);
HardwareSerial serialGSM(2);
TinyGPSPlus gps;

const char APN[] = "internet"; 
const String THINGSPEAK_API_KEY = "4ZLJLA90TBUGVYFC"; 

String lastUID = "NO_TAP";

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi Bus SPI & Sensor RFID
  SPI.begin(); 
  rfid.PCD_Init();
  
  // Inisialisasi Communication Serial GPS & GSM
  serialGPS.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  serialGSM.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);

  delay(2000);
  initSIM800L();
  Serial.println("System ESP32 RFID + GPS Ready!");
}

void loop() {
  // 1. Scanning RFID Card
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    lastUID = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      lastUID += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
      lastUID += String(rfid.uid.uidByte[i], HEX);
    }
    lastUID.toUpperCase();
    Serial.print("Card Scanned UID: ");
    Serial.println(lastUID);
    
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  // 2. Read GPS & Send to ThingSpeak
  while (serialGPS.available() > 0) {
    if (gps.encode(serialGPS.read())) {
      if (gps.location.isValid()) {
        float latitude = gps.location.lat();
        float longitude = gps.location.lng();
        
        Serial.printf("Lat: %.6f, Lng: %.6f, UID: %s\n", latitude, longitude, lastUID.c_str());
        
        sendDataToThingSpeak(latitude, longitude, lastUID);
        
        // Reset state RFID & Delay untuk kuota rate limit ThingSpeak
        lastUID = "NO_TAP";
        delay(15000); 
      }
    }
  }
}

void initSIM800L() {
  sendATCommand("AT", 1000);
  sendATCommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", 1000);
  String apnCmd = "AT+SAPBR=3,1,\"APN\",\"" + String(APN) + "\"";
  sendATCommand(apnCmd.c_str(), 1000);
  sendATCommand("AT+SAPBR=1,1", 3000);
}

void sendDataToThingSpeak(float lat, float lng, String rfidTag) {
  sendATCommand("AT+HTTPINIT", 1000);
  sendATCommand("AT+HTTPPARA=\"CID\",1", 1000);
  
  // Format API HTTP GET ke ThingSpeak
  String url = "http://api.thingspeak.com/update?api_key=" + THINGSPEAK_API_KEY + 
               "&field1=" + String(lat, 6) + 
               "&field2=" + String(lng, 6) + 
               "&field3=" + rfidTag;
  
  String urlCmd = "AT+HTTPPARA=\"URL\",\"" + url + "\"";
  sendATCommand
