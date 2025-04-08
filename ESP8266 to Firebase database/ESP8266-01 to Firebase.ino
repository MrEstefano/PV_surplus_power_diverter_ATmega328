/***********************************************************************************/
/*    program name: ESP8266 datalogging to Firebase Real time database 
/*    programer name:  Stefan Zakutansky
/*    date:            8th February 2024
/*    program function: Receive data from ATmega328 with start anfd and
/*                      end-markers combined. Once separated, wrap all together and 
/*                      and send it to data base 
/*    Credits to: Rui Santos, Robin2, JANAK13
/*
/************************************************************************************/

#include <Wire.h>
#include <Arduino.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "---------------"
#define WIFI_PASSWORD "-----------------"

// Insert Firebase project API Key
#define API_KEY "-"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "----------------------"
#define USER_PASSWORD "--------------"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "-----------------------------------------"

#define RX 0   // - arduino TX(PD2)  ->  esp-01 RX (GPIO0)
#define TX 2    //  - arduino RX(PD3)  ->  esp-01 TX (GPIO2)

int ledState = LOW;

unsigned long previousMillis = 0;
const long interval = 1000;
// temporary array for use when parsing
const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];        
// variables to hold the parsed data
float power = 0.0;
float diverted = 0.0;
float temperature = 0.0;
//Define software serial object in the class
SoftwareSerial ArduinoUno(RX,TX);   //D1=5=rx  D2=4=Tx
// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
// Parent Node (to be updated in every loop)
String parentPath;
FirebaseJson json;
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
// Variable to save USER UID
String uid;
// Database main path (to be updated in setup with the user UID)
String databasePath;

// Database child nodes
String powerPath = "/power";
String divertedPath = "/diverted";
String temperaturePath = "/temperature";
String timePath = "/timestamp";
// Variable to save current epoch time
int timestamp;
boolean recvInProgress = false;
boolean newData = false;
byte ndx = 0;
static char startMarker = '<';
static char endMarker = '>';
char rc;
// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 18000;
int NrOfCycles=0;
int connectionOK = 0;


// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

void setup(){
 // pinMode(LED_BUILTIN, OUTPUT);
  //init. hardware serial callout
  Serial.begin(115200);

  //init. software serial callout
  ArduinoUno.begin(9600);

  //init. wifi callout
  initWiFi();

  //init. time gathering callout
  timeClient.begin();

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  //check if connected to sewrver and set the baud rate
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }

  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";
}



void loop(){
  //check if data are comming through and if previous data already sent
  if (ArduinoUno.available()>0 && newData == false){
    //store incoming
    while (ArduinoUno.available()>0){
      rc = ArduinoUno.read();
      //figgure out the start of the incoming strinhg 
      if (recvInProgress == true) {
        if (rc != endMarker) {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
        }
        else {
            receivedChars[ndx] = '\0'; // terminate the string
            recvInProgress = false;
            ndx = 0;
            newData = true;
        }
      }
      else if (rc == startMarker) {
          recvInProgress = true;
      }  
    }
  }

  if (newData == true ){
    //if new data in the buffer, break the down in sepoarate blocks and assigne the type  
    strcpy(tempChars,receivedChars);
    // this temporary copy is necessary to protect the original data
    // because strtok() used in parseData() replaces the commas with \0
    char * strtokIndx; // this is used by strtok() as an index

    strtokIndx = strtok(tempChars,",");    // get the first part - the string
    power = atof(strtokIndx);     // copy it to messageFromPC

    strtokIndx = strtok(NULL, ",");        // this continues where the previous call left off
    diverted = atof(strtokIndx);      // convert this part to an integer

    strtokIndx = strtok(NULL, ",");
    temperature = atof(strtokIndx);        // convert this part to a float

    // transmission of data every 16 seconds for Appinventor to be ready to receive data
    if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
      sendDataPrevMillis = millis();
      //output incoming data to hardware serial / verification of data is actualy processed
      Serial.print("Float ");
      Serial.println(power);
      Serial.print("Float ");
      Serial.println(diverted);
      Serial.print("Float ");
      Serial.println(temperature);
      //Get current timestamp
      timestamp = getTime();
      Serial.print ("time: ");
      Serial.println (timestamp);
      //build the string for dispatch to server
      parentPath = databasePath + "/" + String(timestamp);
      //assigne data to json
      json.set(powerPath.c_str(), String(power));
      json.set(divertedPath.c_str(), String(diverted));  
      json.set(temperaturePath.c_str(), String(temperature));
      json.set(timePath, String(timestamp));
      Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
      //data has been set flag     
      newData = false;
    }
  }
  /*
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if (ledState == LOW) {
      ledState = HIGH;  // Note that this switches the LED *off*
    } else {
      ledState = LOW;  // Note that this switches the LED *on*
    }
    
     Serial.print("Float ");
  }
  */
}
