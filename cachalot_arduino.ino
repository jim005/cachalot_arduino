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

int DEBUG = 1;

// Libraories
#include <ArduinoJson.h>
#include "DHT.h"


// -------------[ CABLAGE ]--------------------

// Cable de la sonde de 0(20%) a 4(100%)
const int CAPTEUR_GPIO[] = {
  3, 4, 5, 6, 7
};
const int CAPTEUR_GPIO_COUNT = 5;           // the number of pins (i.e. the length of the array)

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
const int GPIO_RELAY_1 = 8;
bool RELAY_1_STATUS;


// Variables
const int waterIsOver = 0;  // value when water is triggering switch

float WATER_LEVEL_MEASURE;
long lastReadingTime = 0;
char linebuf[80];
int charcount = 0;


// Setup Ethernet controler for ENC28J60
#include <UIPEthernet.h>
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x70}; // assign a MAC address for the ethernet controller.
IPAddress ip(192, 168, 1, 70);
IPAddress gateway(192, 168, 1, 2);
IPAddress subnet(255, 255, 255, 0);
IPAddress myDns(1, 1, 1, 1); // 1 public dns
EthernetServer server(80); // Initialize the Ethernet server library





void setup() {

  // GPIO_RELAY_1 module prepared
  // pinMode(GPIO_RELAY_1, OUTPUT);
  // digitalWrite(GPIO_RELAY_1, HIGH);


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

    WATER_LEVEL_MEASURE = getWaterLevel();


    temperature = getTemperature();
    humidity = getHumidity();
    heatIndex = getHeatIndex();
    
    RELAY_1_STATUS = 0;
   // RELAY_1_STATUS = !(digitalRead(GPIO_RELAY_1));


    // debug
    if (DEBUG == 1) {

      Serial.println (WATER_LEVEL_MEASURE);

      Serial.print(F("Humidity: "));
      Serial.print(humidity);

      Serial.print(F("%  Temperature: "));
      Serial.print(temperature);
      Serial.print(F("°C "));

      Serial.print(F("Heat index: "));
      Serial.print(heatIndex);
      Serial.print(F("°C "));


      Serial.print(F("Relai n°1 : "));
      Serial.print(RELAY_1_STATUS);


    }


    // timestamp the last time you got a reading:
    lastReadingTime = millis();
  }




  // listen for incoming Ethernet connections:
  listenForEthernetClients();

}

float getTemperature() {
  return dht.readTemperature();
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

    if ((int) digitalRead(CAPTEUR_GPIO[4]) == waterIsOver) {
      WATER_LEVEL =  CAPTEUR_4_DISTANCE;
      break;
    }
    if ((int) digitalRead(CAPTEUR_GPIO[3]) == waterIsOver) {
      WATER_LEVEL =  CAPTEUR_3_DISTANCE;
      break;
    }
    if ((int) digitalRead(CAPTEUR_GPIO[2]) == waterIsOver) {
      WATER_LEVEL =  CAPTEUR_2_DISTANCE;
      break;
    }
    if ((int) digitalRead(CAPTEUR_GPIO[1]) == waterIsOver) {
      WATER_LEVEL =  CAPTEUR_1_DISTANCE;
      break;
    }
    if ((int) digitalRead(CAPTEUR_GPIO[0]) == waterIsOver) {
      WATER_LEVEL =  CAPTEUR_0_DISTANCE;
      break;
    }

    WATER_LEVEL = 0;
    break;


  }


  Serial.println(WATER_LEVEL);

  for (int thisPin = 0; thisPin < CAPTEUR_GPIO_COUNT; thisPin++) {
    Serial.print("GPIO ");
    Serial.print (CAPTEUR_GPIO[thisPin]);
    Serial.print(" : ");
    Serial.println ((int) digitalRead(CAPTEUR_GPIO[thisPin]));
  }


  return WATER_LEVEL;

}


// Display dashboard page with on/off button for GPIO_GPIO_RELAY_1_1
// It also print Temperature in C and F
void dashboardPageJson(EthernetClient &client) {

  const size_t capacity = JSON_OBJECT_SIZE(6);
  
  DynamicJsonDocument doc(capacity);
  doc["name"] = "cachalot";
  doc["waterLevel"] = WATER_LEVEL_MEASURE;
  doc["temperature"] = temperature;
  doc["heatIndex"] = heatIndex;
  doc["humidity"] = humidity;
  doc["relay1Status"] = RELAY_1_STATUS;


  Serial.print(F("Sending: "));
  serializeJson(doc, Serial);
  Serial.println();

  // Write response headers
  client.println(F("HTTP/1.0 200 OK"));
  client.println(F("Content-Type: application/json"));
  client.println(F("Connection: close"));
  client.print(F("Content-Length: "));
  client.println(measureJsonPretty(doc));
  client.println();

  serializeJsonPretty(doc, client);   // Write JSON document

}



void listenForEthernetClients() {

  // listen for incoming clients
  EthernetClient client = server.available();

  // Do we have a client?
  if (!client) return;


  Serial.println(F("New client"));

  memset(linebuf, 0, sizeof(linebuf));
  charcount = 0;
  // an http request ends with a blank line
  boolean currentLineIsBlank = true;

  // Read the request (we ignore the content in this example)
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      //read char by char HTTP request
      linebuf[charcount] = c;
      if (charcount < sizeof(linebuf) - 1) charcount++;
      // if you've gotten to the end of the line (received a newline
      // character) and the line is blank, the http request has ended,
      // so you can send a reply

      if (c == '\n' && currentLineIsBlank) {
        dashboardPageJson(client);
        break;
      }
      if (c == '\n') {
        if (strstr(linebuf, "GET /relay_1_off") > 0) {
          digitalWrite(GPIO_RELAY_1, HIGH);          
          client.println(F("<meta http-equiv=\"refresh\" content=\"0;url=/\" />"));

        }
        else if (strstr(linebuf, "GET /relay_1_on") > 0) {
          digitalWrite(GPIO_RELAY_1, LOW); 
          client.println(F("<meta http-equiv=\"refresh\" content=\"0;url=/\" />"));
        }

        // you're starting a new line
        currentLineIsBlank = true;
        memset(linebuf, 0, sizeof(linebuf));
        charcount = 0;
      }
      else if (c != '\r') {
        // you've gotten a character on the current line
        currentLineIsBlank = false;
      }
    }
  }


  // give the web browser time to receive the data
  delay(1);
  // close the connection:
  client.stop();
  Serial.println("client disconnected");
}
