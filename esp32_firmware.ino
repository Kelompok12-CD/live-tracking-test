#include <TinyGPS++.h>
#include <HardwareSerial.h>

#define MODEM_RX 18
#define MODEM_TX 19
#define GPS_RX 16
#define GPS_TX 17

HardwareSerial serialGPS(1);
HardwareSerial serialGSM(2);
TinyGPSPlus gps;

const char APN[] = "internet"; // APN Provider Anda
// Endpoint REST API Supabase
const char SUPABASE_URL[] = "https://PROJECT_ID_ANDA.supabase.co/rest/v1/gps_logs";
const char SUPABASE_KEY[] = "Bearer API_KEY_ANON_ANDA";

void setup() {
  Serial.begin(115200);
  serialGPS.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  serialGSM.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);

  delay(3000);
  initSIM800L();
}

void loop() {
  while (serialGPS.available() > 0) {
    if (gps.encode(serialGPS.read())) {
      if (gps.location.isValid()) {
        float latitude = gps.location.lat();
        float longitude = gps.location.lng();
        
        sendDataToSupabase(latitude, longitude);
        delay(5000); // Kirim koordinat setiap 5 detik
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

void sendDataToSupabase(float lat, float lng) {
  sendATCommand("AT+HTTPINIT", 1000);
  sendATCommand("AT+HTTPPARA=\"CID\",1", 1000);
  
  String urlCmd = "AT+HTTPPARA=\"URL\",\"" + String(SUPABASE_URL) + "\"";
  sendATCommand(urlCmd.c_str(), 1000);
  sendATCommand("AT+HTTPPARA=\"CONTENT\",\"application/json\"", 1000);
  sendATCommand("AT+HTTPPARA=\"USERDATA\",\"apikey: API_KEY_ANON_ANDA\"", 1000);
  
  String jsonData = "{\"lat\":" + String(lat, 6) + ",\"lng\":" + String(lng, 6) + "}";
  String dataCmd = "AT+HTTPDATA=" + String(jsonData.length()) + ",10000";
  
  sendATCommand(dataCmd.c_str(), 1000);
  sendATCommand(jsonData.c_str(), 1000);
  sendATCommand("AT+HTTPACTION=1", 2000); // Exec POST Request
  sendATCommand("AT+HTTPTERM", 1000);
}

void sendATCommand(const char* cmd, int timeout) {
  serialGSM.println(cmd);
  long int time = millis();
  while ((time + timeout) > millis()) {
    while (serialGSM.available()) {
      Serial.write(serialGSM.read());
    }
  }
}
