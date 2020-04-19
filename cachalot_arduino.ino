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

const int waterIsOver = 0;  // value when water is triggering switch


/// Jeedom
const String JEEDOM_IP =  "192.168.1.33";
const String JEEDOM_VIRTUAL_APIKEY = "JiVW21qvOqNPVHe5tbXbxOhLMhK1OfM8";
const int JEEDOM_VIRTUAL_COMMANDE_ID = 749;
const int   PORT     = 80;


// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  // make the pushbutton's pin an input:
  for (int thisPin = 0; thisPin < CAPTEUR_GPIO_COUNT; thisPin++) {
    pinMode(CAPTEUR_GPIO[thisPin], INPUT);
  }
}

float measure() {

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

  // debug
  if (DEBUG == 1) {
    for (int thisPin = 0; thisPin < CAPTEUR_GPIO_COUNT; thisPin++) {
      Serial.print("GPIO ");
      Serial.print (CAPTEUR_GPIO[thisPin]);
      Serial.print(" : ");
      Serial.println ((int) digitalRead(CAPTEUR_GPIO[thisPin]));
    }
  }

  return WATER_LEVEL;

}

// the loop routine runs over and over again forever:
void loop() {

  float valeur = measure();
  Serial.println (valeur);


  // Send info to Jeedom server
  String baseUrl = "/core/api/jeeApi.php?apikey=";
  baseUrl += JEEDOM_VIRTUAL_APIKEY;
  baseUrl += "&type=virtual&id=";
  
  String urlToUpdateJeedom = baseUrl + JEEDOM_VIRTUAL_COMMANDE_ID;
  urlToUpdateJeedom += "&value=";
  urlToUpdateJeedom += String(valeur);

  sendToJeedom(urlToUpdateJeedom);


  delay(2000);

}


boolean sendToJeedom(String urlToUpdateJeedom) {
  Serial.print("Requesting URL: ");
  Serial.println(JEEDOM_IP + urlToUpdateJeedom);
  // http.begin(JEEDOM_IP, PORT, urlToUpdateJeedom);
  //  int httpCode = http.GET();
  Serial.println("closing connection");
  //  http.end();

  delay(1000);
}
