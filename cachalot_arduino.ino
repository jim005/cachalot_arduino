//----------------------------------------
// Affichage du niveau d'eau de la cuve "Cachalot"
//
//
// Sources et inspirations (many thanks) :
//   https://www.fred-j.org/?p=364
//
// Capteur NO / Hauteurs par rapport au sol : 200 mm, 405 mm, 615 mm, 825 mm, 1035 mm
//
//
//
// -------------------------------------------

bool DEBUG = 1;

// Libraories
#include <ArduinoJson.h>
#include <SPI.h>
#include "DHT.h"


// Cable de la sonde de 0(20%) a 4(100%)
const uint8_t CAPTEUR_GPIO[] = { 3, 4, 5, 6, 7};
const uint8_t CAPTEUR_GPIO_COUNT = 5;           // the number of pins (i.e. the length of the array)

const float CAPTEUR_0_DISTANCE = 20;
const float CAPTEUR_1_DISTANCE = 40.5;
const float CAPTEUR_2_DISTANCE = 61.5;
const float CAPTEUR_3_DISTANCE = 82.5;
const float CAPTEUR_4_DISTANCE = 103.5;

// Capteur DHT11 : temperature + humidity
#define DHTPIN 2         // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11  // DHT 11
DHT dht(DHTPIN, DHTTYPE);
float temperature;
float humidity;
float heatIndex;


// Relai
const uint8_t GPIO_RELAY_1 = 8;
bool RELAY_1_STATUS;
const uint8_t GPIO_RELAY_2 = 9;
bool RELAY_2_STATUS;


// Variables
const boolean waterIsOver = false;  // value when water is triggering switch
float WATER_LEVEL_MEASURE;
uint32_t lastReadingTime = 0;
 

// Setup Ethernet controler for ENC28J60
#include <UIPEthernet.h>
#include <EasyWebServer.h>

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x70}; // assign a MAC address for the ethernet controller.
IPAddress ip(192, 168, 1, 70);
IPAddress gateway(192, 168, 1, 2);
IPAddress subnet(255, 255, 255, 0);
IPAddress myDns(1, 1, 1, 1); // 1 public dns
EthernetServer server(80); // Initialize the Ethernet server library




void setup() {

  // GPIO_RELAY_1 module prepared
  pinMode(GPIO_RELAY_1, OUTPUT);
  digitalWrite(GPIO_RELAY_1, HIGH);


  // initialize serial communication at 9600 bits per second:
  // debug
  if (DEBUG == 1) {
    Serial.begin(9600);
    while (!Serial) continue;
  }
  // make the pin an input:
  for (int thisPin = 0; thisPin < CAPTEUR_GPIO_COUNT; thisPin++) {
    pinMode(CAPTEUR_GPIO[thisPin], INPUT);
  }

  // Start DHT11
  dht.begin();

  // Initialize Ethernet device
  Ethernet.begin(mac, ip, myDns, gateway, subnet);

  // Start to listen
  server.begin();

  // give the sensor and Ethernet shield time to set up:
  delay(1000);


  // debug : display IP from DHCP
  Serial.println(F("Server is ready."));
  Serial.print(F("Please connect to http://"));
  Serial.println(Ethernet.localIP());


}


void loop() {

  // check for a reading no more than once a second.
  if (millis() - lastReadingTime > 5000) {

    delay(2000);
    
    WATER_LEVEL_MEASURE = getWaterLevel();
    temperature = getTemperature();
    humidity = getHumidity();
    heatIndex = getHeatIndex();
    RELAY_1_STATUS = !(digitalRead(GPIO_RELAY_1));

    // timestamp the last time you got a reading:
    lastReadingTime = millis();
    
    // debug
    if (DEBUG == 1) {

      Serial.print (F("Waterlevel : "));
      Serial.print (WATER_LEVEL_MEASURE);
      Serial.print (F(" cm "));

      Serial.print(F("Humidity: "));
      Serial.print(humidity);
      Serial.print(F("% "));

      Serial.print(F("Temperature: "));
      Serial.print(temperature);
      Serial.print(F("°C "));

      Serial.print(F("Heat index: "));
      Serial.print(heatIndex);
      Serial.print(F("°C "));

      Serial.print(F("Relai n°1 : "));
      Serial.print(RELAY_1_STATUS);
      Serial.print(F(" "));

      Serial.print(F("Relai n°2 : "));
      Serial.print(RELAY_2_STATUS);

    }

  }


  // Listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("New client!");
    EasyWebServer w(client);                    // Read and parse the HTTP Request
    w.serveUrl("/", rootPage);                  // Root page
    w.serveUrl("/json", dashboardJSON, EWS_TYPE_JSON); // JSON page
    w.serveUrl("/relay_1/off", relay_1_off);
    w.serveUrl("/relay_1/on", relay_1_on);
    w.serveUrl("/relay_2/off", relay_2_off);
    w.serveUrl("/relay_2/on", relay_2_on);
    w.serveUrl("/debug/analog", analogSensorPage);    // Analog sensor page
    w.serveUrl("/debug/digital", digitalSensorPage);  // Digital sensor page
  }
}

void rootPage(EasyWebServer &w) {
  w.client.println(F("<!DOCTYPE HTML>"));
  w.client.println(F("<html><head><title>Cachalot</title>"));
  w.client.println(F("<meta name=\"robots\" content=\"noindex\">"));
  w.client.println(F("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"));
  w.client.println(F("</head><body style=\"padding: 0.5em;font-family: sans-serif;\">"));
  w.client.println(F("<p>Welcome to my little web server.</p>"));

  w.client.print(F("Values :"));
  w.client.print(F("<ul>"));

  w.client.print(F("<li>waterLevel : "));
  w.client.print(WATER_LEVEL_MEASURE);
  w.client.print(F("</li>"));

  w.client.print(F("<li>temperature : "));
  w.client.print(temperature); 
  w.client.print(F("</li>"));
  
  w.client.print(F("<li>humidity : "));
  w.client.print(humidity);
  w.client.print(F("</li>"));

  w.client.print(F("<li>heatIndex : "));
  w.client.print(heatIndex);
  w.client.print(F("</li>"));

  w.client.print(F("<li>relay1Status : "));
  w.client.print(RELAY_1_STATUS);
  w.client.println(F(" <a href='/relay_1/on'>ON</a> <a href='/relay_1/off'>OFF</a>"));
  w.client.print(F("</li>"));

  w.client.print(F("<li>relay2Status : "));
  w.client.print(RELAY_2_STATUS);
  w.client.println(F(" <a href='/relay_2/on'>ON</a> <a href='/relay_2/off'>OFF</a>"));
  w.client.print(F("</li>"));

  w.client.print(F("</ul>"));


  
  w.client.println(F("<hr />"));
  w.client.println(F("<p>Debug : <a href='/debug/analog'>analog sensors</a>, <a href='/debug/digital'>digital sensors</a></p>"));
  w.client.println(F("<p>More : <a href='/json'>values in JSON</a></p>"));
  w.client.println(F("</body></html>"));

}

void dashboardJSON(EasyWebServer &w) {


  w.client.println(F("{"));

  w.client.print(F("\"waterLevel\":"));
  w.client.print(WATER_LEVEL_MEASURE);
  w.client.print(F(","));

  w.client.print(F("\"temperature\":"));
  w.client.print(temperature);
  w.client.print(F(","));

  w.client.print(F("\"humidity\":"));
  w.client.print(humidity);
  w.client.print(F(","));

  w.client.print(F("\"heatIndex\":"));
  w.client.print(heatIndex);
  w.client.print(F(","));

  w.client.print(F("\"relay1Status\":"));
  w.client.print(RELAY_1_STATUS);
  w.client.print(F(","));

  w.client.print(F("\"relay2Status\":"));
  w.client.print(RELAY_2_STATUS);
 // w.client.print(F(""));

  w.client.print(F("}"));



}


void relay_1_on(EasyWebServer &w) {

  digitalWrite(GPIO_RELAY_1, LOW);
  w.client.println(F("<meta http-equiv=\"refresh\" content=\"0;url=/\" />"));

}

void relay_1_off(EasyWebServer &w) {

  digitalWrite(GPIO_RELAY_1, HIGH);
  w.client.println(F("<meta http-equiv=\"refresh\" content=\"0;url=/\" />"));

}


void relay_2_on(EasyWebServer &w) {

  digitalWrite(GPIO_RELAY_2, LOW);
  w.client.println(F("<meta http-equiv=\"refresh\" content=\"0;url=/\" />"));

}

void relay_2_off(EasyWebServer &w) {

  digitalWrite(GPIO_RELAY_2, HIGH);
  w.client.println(F("<meta http-equiv=\"refresh\" content=\"0;url=/\" />"));

}



void analogSensorPage(EasyWebServer &w) {
  w.client.println(F("<!DOCTYPE HTML>"));
  w.client.println(F("<html><head><title>Analog Sensors</title></head><body>"));
  for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
    int sensorReading = analogRead(analogChannel); // Note that analogReaad uses CHANNELS insead of pin.
    w.client.print(F("analog input "));
    w.client.print(analogChannel);
    w.client.print(F(" is "));
    w.client.print(sensorReading);
    w.client.println(F("<br />"));
  }
  w.client.println(F("<p><a href='/'>Home</a></body></html>"));
}

void digitalSensorPage(EasyWebServer &w) {
  w.client.println(F("<!DOCTYPE HTML>"));
  w.client.println(F("<html><head><title>Digital Sensors</title></head><body>"));
  for (int digitalPin = 2; digitalPin < 8; digitalPin++) {
    int sensorReading = digitalRead(digitalPin);
    w.client.print(F("digital pin "));
    w.client.print(digitalPin);
    w.client.print(F(" is "));
    w.client.print(sensorReading);
    w.client.println(F("<br />"));
  }
  w.client.println(F("<p><a href='/'>Home</a></body></html>"));
}


float getTemperature() {

  float t = dht.readTemperature();
  return t;
}

float getHumidity() {
  return dht.readHumidity();
}
float getHeatIndex () {
  return dht.computeHeatIndex(temperature, humidity, false);
}

float getWaterLevel() {

  float WATER_LEVEL = 0;


  while (true) {

    if ((bool) digitalRead(CAPTEUR_GPIO[4]) == waterIsOver) {
      WATER_LEVEL =  CAPTEUR_4_DISTANCE;
      break;
    }
    if ((bool) digitalRead(CAPTEUR_GPIO[3]) == waterIsOver) {
      WATER_LEVEL =  CAPTEUR_3_DISTANCE;
      break;
    }
    if ((bool) digitalRead(CAPTEUR_GPIO[2]) == waterIsOver) {
      WATER_LEVEL =  CAPTEUR_2_DISTANCE;
      break;
    }
    if ((bool) digitalRead(CAPTEUR_GPIO[1]) == waterIsOver) {
      WATER_LEVEL =  CAPTEUR_1_DISTANCE;
      break;
    }
    if ((bool) digitalRead(CAPTEUR_GPIO[0]) == waterIsOver) {
      WATER_LEVEL =  CAPTEUR_0_DISTANCE;
      break;
    }

    WATER_LEVEL = 0;
    break;

  }

  return WATER_LEVEL;

}
