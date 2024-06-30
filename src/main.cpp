#include <Arduino.h>
//Inisialisasi library
#include <Wire.h>
#include <TinyGPS++.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <lorawan.h>

//Defining Pin.
#define buzzer 26
#define led 33  
#define button 32
#define gpsPower 25
#define RXD2 16
#define TXD2 17

LiquidCrystal_I2C lcd(0x27,20,4);
HardwareSerial neogps(1);
TinyGPSPlus gps;


//interval setup 

const unsigned long interval = 1000;  
const unsigned long interval_off = 60000;   // 10 s interval to send message
unsigned long previousMillis = 0;  // will store last time message sent


int port, channel, freq;
int warning;
float getLat,getLon;
unsigned int counter = 0; 



//LoRaWAN Setup
const char *devAddr = "1B7D8104";
const char *nwkSKey = "6DEEE0DF1A4B11D5C0C7ED8490DBE0AD";
const char *appSKey = "2AF50E98BB8707AF6AFBAA0887F5C50E";
char myStr[128];
byte outStr[255];
byte recvStatus = 0;
bool newmessage = false;

const sRFM_pins RFM_pins = {
  .CS = 15,
  .RST = 0,
  .DIO0 = 27,
  .DIO1 = 2,
};
String dataSend = "";


void buzzerBlink(){
    digitalWrite(buzzer,HIGH);
    delay(500);
    digitalWrite(buzzer,LOW);
    delay(1000);
}

void waitForValidGPS() {
  unsigned long gpsTimeout = millis();
  const unsigned long timeoutPeriod = 30000;  // 30 seconds timeout
  digitalWrite(gpsPower,HIGH);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for GPS");

  while (!gps.location.isValid()) {
    while (neogps.available()) {
      gps.encode(neogps.read());
    }

    if (millis() - gpsTimeout > timeoutPeriod) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("GPS Timeout");
      Serial.println("GPS fix not acquired. Timeout.");
      return;
    }

    lcd.setCursor(0, 1);
    lcd.print("Invalid GPS");
    delay(1000);  // Update every second
  }
  
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("GPS Acquired");
  getLat = gps.location.lat();
  getLon = gps.location.lng();
  lcd.setCursor(0, 1);
  lcd.print(getLat);
  digitalWrite(gpsPower,LOW);
}


void setup() {

  pinMode(button, INPUT);
  pinMode(led, OUTPUT);
  pinMode(buzzer,OUTPUT);
  pinMode(gpsPower,OUTPUT);
  // digitalWrite(gpsPower,HIGH);
  Serial.begin(115200); //untuk memulai sebuah serial komunikasi => 9600/115200 --> Bebas lo mau pake yang mana
  neogps.begin(9600, SERIAL_8N1, RXD2, TXD2); // memulai komunikasi dengan GPS Module 
  // if (!lora.init()) {
  //   Serial.println("RFM95 not detected");
  //   delay(5000);
  //   return;
  // }
  lora.init();
  lcd.init();                      // initialize the lcd 
  lcd.backlight();

//   //modem sleep
  btStop();
  WiFi.disconnect(true);  // Disconnect from the network
  WiFi.mode(WIFI_OFF);  // Switch WiFi off
  // digitalWrite(buzzer,HIGH);

//LoRaWAN Setup
  lora.setDeviceClass(CLASS_A);
  lora.setDataRate(SF12BW125);
  lora.setFramePortTx(5);
  lora.setChannel(MULTI);
  lora.setTxPower(15);
  lora.setNwkSKey(nwkSKey);
  lora.setAppSKey(appSKey);
  lora.setDevAddr(devAddr);
}

void loop(){
  //to keep mappi read serial even its power off
    while (neogps.available()){
    gps.encode(neogps.read());
  }
    int data = digitalRead(button);

  /*only if using oled*/
  // display.clearDisplay();
  // display.setTextSize(1);
  // display.setTextColor(SSD1306_WHITE);
  // display.setCursor(0, 0);

  if (gps.location.isValid()) {
    getLat = gps.location.lat();
    getLon = gps.location.lng();
    //just to show
    // display.print(F("Lat: "));
    // display.println(gps.location.lat(), 6);
    // display.print(F("Lng: "));
    // display.println(gps.location.lng(), 6);
  } else {
    //show that gps not found
    // display.println(F("Lat: Invalid"));
    // display.println(F("Lng: Invalid"));
  }
       
  if (data == 1){
      digitalWrite(led,HIGH);
    // state awal di pencet
    digitalWrite(gpsPower,HIGH);
    setCpuFrequencyMhz(240);

    // lcd.backlight();
    // lcd.setCursor(0, 0);
    // lcd.print("Lat: ");
    // lcd.print(gps.location.lat(), 6); // 6 decimal places
    
    // lcd.setCursor(0, 1);
    // lcd.print("Lon: ");
    // lcd.print(gps.location.lng(), 6); // 6 decimal places
    
    lcd.setCursor(0, 0);
    lcd.print("DANGER!");
    //kirim data
     if (millis() - previousMillis > interval) {
    previousMillis = millis();
    digitalWrite(buzzer,HIGH);
    delay(500);
    digitalWrite(buzzer,LOW);
    // digitalWrite(gpsPower,HIGH);

    // while (gps.location.lat() == 0 && gps.location.lng() == 0 )
    // {
    //   Serial.println("waiting for gps");
    // }
    

    String latitude = String(getLat, 8);
    String longitude = String(getLon, 8);
    // dataSend = "{\"Btn\": " + (String)warning + ", \"Lat\": "+ latitude +", \"Lon\": "+ longitude +", \"ShipID\": "+ shipID +"}";
    // dataSend.toCharArray(myStr,100);
    // dataSend.toCharArray(myStr,dataSend.length()+1);

    // sprintf(myStr, "Lora Counter-%d", counter++);
     dataSend = "{\"Btn\": " + (String)data + ", \"Lat\": "+ latitude +", \"Lon\": "+ longitude +"}";
    dataSend.toCharArray(myStr,100);
    //dataSend.toCharArray(myStr,dataSend.length()+1);

    
    Serial.print("Sending: ");
    Serial.println(myStr);
    lora.sendUplink(myStr, strlen(myStr), 0);
    port = lora.getFramePortTx();
    channel = lora.getChannel();
    freq = lora.getChannelFreq(channel);
    Serial.print(F("fport: "));    Serial.print(port);Serial.print(" ");
    Serial.print(F("Ch: "));    Serial.print(channel);Serial.print(" ");
    Serial.print(F("Freq: "));    Serial.print(freq);Serial.println(" ");
  }

  // Check Lora RX
  lora.update();
  recvStatus = lora.readDataByte(outStr);
    Serial.println(recvStatus);
  if (recvStatus) {
    newmessage = true;
    int counter = 0;
    port = lora.getFramePortRx();
    channel = lora.getChannelRx();
    freq = lora.getChannelRxFreq(channel);

    for (int i = 0; i < recvStatus; i++)
    {
      if (((outStr[i] >= 32) && (outStr[i] <= 126)) || (outStr[i] == 10) || (outStr[i] == 13))
        counter++;
    }
    if (port != 0)
    {
      if (counter == recvStatus)
      {
        Serial.print(F("Received String : "));
        for (int i = 0; i < recvStatus; i++)
        {
          Serial.print(char(outStr[i]));
        }
      }
      else
      {
        Serial.print(F("Received Hex: "));
        for (int i = 0; i < recvStatus; i++)
        {
          Serial.print(outStr[i], HEX); Serial.print(" ");
        }
      }
      Serial.println();
      Serial.print(F("fport: "));    Serial.print(port);Serial.print(" ");
      Serial.print(F("Ch: "));    Serial.print(channel);Serial.print(" ");
      Serial.print(F("Freq: "));    Serial.println(freq);Serial.println(" ");
    }
    else
    {
      Serial.print(F("Received Mac Cmd : "));
      for (int i = 0; i < recvStatus; i++)
      {
        Serial.print(outStr[i], HEX); Serial.print(" ");
      }
      Serial.println();
      Serial.print(F("fport: "));    Serial.print(port);Serial.print(" ");
      Serial.print(F("Ch: "));    Serial.print(channel);Serial.print(" ");
      Serial.print(F("Freq: "));    Serial.println(freq);Serial.println(" ");
    }
  }

  }
  else{
    warning=0;
    digitalWrite(led,LOW);
    lcd.noBacklight();
    setCpuFrequencyMhz(80);
    // lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Safe     ");
    //turn off the power
    digitalWrite(gpsPower,LOW);
    // Serial.println(data);
    // digitalWrite(led,LOW);
    // algoritma untuk bangun 3 menit untuk send data
    if (millis() - previousMillis > interval_off) {
    previousMillis = millis();
    waitForValidGPS();
    digitalWrite(buzzer,HIGH);
    delay(500);
    digitalWrite(buzzer,LOW);


  
    String latitude = String(getLat, 8);
    String longitude = String(getLon, 8);
    dataSend = "{\"Btn\": " + (String)data + ", \"Lat\": "+ latitude +", \"Lon\": "+ longitude +"}";
    dataSend.toCharArray(myStr,100);

    
    Serial.print("Sending: ");
    Serial.println(myStr);
    lora.sendUplink(myStr, strlen(myStr), 0);
    port = lora.getFramePortTx();
    channel = lora.getChannel();
    freq = lora.getChannelFreq(channel);
    Serial.print(F("fport: "));    Serial.print(port);Serial.print(" ");
    Serial.print(F("Ch: "));    Serial.print(channel);Serial.print(" ");
    Serial.print(F("Freq: "));    Serial.print(freq);Serial.println(" ");
  }

  // Check Lora RX
  lora.update();

  recvStatus = lora.readDataByte(outStr);
  Serial.println(recvStatus);
  if (recvStatus) {
    newmessage = true;
    int counter = 0;
    port = lora.getFramePortRx();
    channel = lora.getChannelRx();
    freq = lora.getChannelRxFreq(channel);

    for (int i = 0; i < recvStatus; i++)
    {
      if (((outStr[i] >= 32) && (outStr[i] <= 126)) || (outStr[i] == 10) || (outStr[i] == 13))
        counter++;
    }
    if (port != 0)
    {
      if (counter == recvStatus)
      {
        Serial.print(F("Received String : "));
        for (int i = 0; i < recvStatus; i++)
        {
          lcd.clear();
          lcd.setCursor(1,0);
          lcd.print("DALAM COVERAGE");
          lcd.setCursor(4,1);
          lcd.print("GATEWAY!");
          delay(5000);
          lcd.clear();
          Serial.print(char(outStr[i]));
        }
      }
      else
      {
        Serial.print(F("Received Hex: "));
        for (int i = 0; i < recvStatus; i++)
        {
          Serial.print(outStr[i], HEX); Serial.print(" ");
        }
      }
      Serial.println();
      Serial.print(F("fport: "));    Serial.print(port);Serial.print(" ");
      Serial.print(F("Ch: "));    Serial.print(channel);Serial.print(" ");
      Serial.print(F("Freq: "));    Serial.println(freq);Serial.println(" ");
    }
    else
    {
      Serial.print(F("Received Mac Cmd : "));
      for (int i = 0; i < recvStatus; i++)
      {
        Serial.print(outStr[i], HEX); Serial.print(" ");
      }
      Serial.println();
      Serial.print(F("fport: "));    Serial.print(port);Serial.print(" ");
      Serial.print(F("Ch: "));    Serial.print(channel);Serial.print(" ");
      Serial.print(F("Freq: "));    Serial.println(freq);Serial.println(" ");
    }
  }

  }
}


/* Cek LCD and GPS*/
// TinyGPSPlus gps;
// LiquidCrystal_I2C lcd(0x27, 16, 2); // Adjust the address (0x27) if needed

// // // Define the serial connections for the GPS
// #define GPS_RX_PIN 16
// #define GPS_TX_PIN 17

// HardwareSerial gpsSerial(1);

// void setup() {
//   // Start the hardware serial for ESP32
//   Serial.begin(115200);
//   pinMode(gpsPower,OUTPUT);
//   digitalWrite(gpsPower,HIGH);  
//   // Start the serial for GPS
//   gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  
//   // Initialize the LCD
//   lcd.init();
//   lcd.backlight();
  
//   // Initial LCD message
//   lcd.setCursor(0, 0);
//   lcd.print("Waiting for");
//   lcd.setCursor(0, 1);
//   lcd.print("GPS signal...");
// }

// void loop() {
//   // Read the GPS data
//   while (gpsSerial.available() > 0) {
//     gps.encode(gpsSerial.read());
//   }

//   // Check if there is a valid location
//   if (gps.location.isUpdated()) {
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Lat: ");
//     lcd.print(gps.location.lat(), 6); // 6 decimal places
    
//     lcd.setCursor(0, 1);
//     lcd.print("Lon: ");
//     lcd.print(gps.location.lng(), 6); // 6 decimal places
    
//     // Also print to Serial Monitor
//     Serial.print("Latitude: ");
//     Serial.println(gps.location.lat(), 6);
//     Serial.print("Longitude: ");
//     Serial.println(gps.location.lng(), 6);
//   } else {
//     // If no valid GPS data, print "No GPS"
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("No GPS");
//     lcd.setCursor(0, 1);
//     lcd.print("Satellites");
    
//     // Also print to Serial Monitor
//     Serial.println("No GPS Satellites");
//   }
  
//   delay(1000); // Update every second
// }

/* cek power control and buzzer*/
// void setup(){
//   pinMode(buzzer,OUTPUT);
//   // pinMode(12,OUTPUT);
//   // pinMode(33,OUTPUT);
// }

// void loop(){
//   digitalWrite(buzzer,HIGH);
//   // digitalWrite(13,HIGH);
//   // digitalWrite(33,HIGH);
//   delay(1000);
//   digitalWrite(buzzer,LOW);
//   // digitalWrite(13,LOW);
//   // digitalWrite(33,LOW);
//   delay(2000);
// }

/*Power Control with Push Button*/
// void setup(){
//   pinMode(buzzer,OUTPUT);
//   pinMode(led,OUTPUT);
//   pinMode(gpsPower,OUTPUT);
//   pinMode(32,INPUT);
// }

// void loop(){
//   int data = digitalRead(32);
//   if (data == 1){
//     digitalWrite(buzzer,HIGH);
//     digitalWrite(led,HIGH);
//     digitalWrite(gpsPower,HIGH);
//   } else{
//     digitalWrite(buzzer,LOW);
//     digitalWrite(led,LOW);
//     digitalWrite(gpsPower,LOW);
//   }
// }





