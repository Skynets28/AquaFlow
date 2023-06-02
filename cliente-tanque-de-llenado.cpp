//CLIENTE

//Libraries
#include <ESP8266WiFi.h>//https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFi.h

//Constants
#define LED 2
#define UPDATE_TIME 500
// Define Trig and Echo pin:
// Definimos los pines que vamos a utilizar
#define TRIGGER_PIN 5
#define ECHO_PIN 4

//Parameters
String nom = "Slave0";
const char* ssid = "NethomeWifi2.4";
const char* password = "C4552553b4ny$33";

//Variables
String command;
unsigned long previousRequest = 0;

// Declaramos las variables que vamos a utilizar
float duration;
int distance;
// Definimos la velocidad del sonido en centímetros por segundo
const float SPEED_OF_SOUND_CM_PER_SEC = 34300.0;

// Definimos la distancia máxima a medir en centímetros
const int MAX_DISTANCE_CM = 400;

//Objects
WiFiClient master;
IPAddress server(192, 168, 100, 90);

void setup() {
  //Init Serial USB
  Serial.begin(115200);
  Serial.println(F("Initialize System"));
  //Init ESP8266 Wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.print(nom);
  Serial.print(F(" connected to Wifi! IP address : ")); Serial.println(WiFi.localIP()); // Print the IP address
    // Inicializamos los pines como entradas y salidas
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
}

void loop() {
  requestMaster();
  delay(1000);
}

void requestMaster( ) { /* function requestMaster */
  
    // Enviamos un pulso de 10 microsegundos al pin del trigger
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(15);
  digitalWrite(TRIGGER_PIN, LOW);

  // Medimos la duración del pulso en el pin del echo
  duration = pulseIn(ECHO_PIN, HIGH, 26000);

  // Calculamos la distancia en centímetros
  distance = int(round((duration/58.3)));

  // Verificamos que la distancia esté dentro de los límites establecidos
  
    Serial.print("Distancia: ");
    Serial.print(distance);
    Serial.println(" cm");
  
  ////Request to master
  if ((millis() - previousRequest) > UPDATE_TIME) { // client connect to server every 500ms
    previousRequest = millis();
    if (master.connect(server, 80)) { // Connection to the server
      master.println(nom + ": Hello! my current state is x" + String(distance) + "\r");
    }
  }
}