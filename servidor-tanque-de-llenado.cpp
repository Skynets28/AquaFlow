//Server  

//Libraries
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFi.h
#include <iostream>
#include <string>
#include <sstream>
using namespace std;
using std::stoi;

//Constants
#define NUM_SLAVES 1
#define LED 2
#define BOMBA 13

//Parameters
String nom = "Master";
const char* ssid = "NethomeWifi2.4";
const char* password = "C4552553b4ny$33";

//Variables
bool sendCmd = false;
String slaveCmd = "0";
String slaveState = "0";
int medida;
int medida2;
int contador = 0;
int contador2 = 0; 
float porcentaje;
int capacidadDelTanque = 350;
stringstream ss;
string str;
bool estado = 0;
bool estadoAnt = 0;

//Variables para el timer
unsigned long previousMillis = 0;  // Variable para almacenar el tiempo anterior
const unsigned long interval = 120000;  // Retraso de una hora en milisegundos
unsigned long currentMillis;

//Objects
WiFiServer server(80);
WiFiClient browser;
IPAddress ip(192, 168, 100, 90);
IPAddress gateway(192, 168, 100, 1);
IPAddress subnet(255, 255, 255, 0);

void setup() {
  //Init Serial USB
  Serial.begin(115200);
  Serial.println(F("Initialize System"));
  //Init ESP8266 Wifi
  WiFi.config(ip, gateway, subnet);       // forces to use the fix IP
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  server.begin();
  Serial.print(nom);
  Serial.print(F(" connected to Wifi! IP address : http://")); Serial.println(WiFi.localIP()); // Print the IP address
  pinMode(LED, OUTPUT);
  pinMode(BOMBA, OUTPUT);
}

void loop() {
  clientRequest();
}

void clientRequest( ) { /* funcion clientRequest */
  ////Revisa si el cliente esta conectado
  WiFiClient client = server.available();
  client.setTimeout(50);
  if (client) {
    if (client.connected()) {
      //Imprime la direccion ip del cliente
      Serial.print(" ->");Serial.println(client.remoteIP());
      String request = client.readStringUntil('\r'); //receive el mensaje del cliente
      
      if (request.indexOf("Slave0") == 0) {
        //Manejo de la solicitud del esclavo
        Serial.print("From "); Serial.println(request);
        int index = request.indexOf(":");
        String slaveid = request.substring(0, index);
        slaveState = request.substring(request.indexOf("x") + 1, request.length());
        Serial.print("state received: "); Serial.println(slaveState);
        //Obtencion de la medida del sensor
        medida = slaveState.toInt();
        //Porcentaje de llenado del tanque
        porcentaje = ((medida-20)*100)/capacidadDelTanque;
        Serial.print("\nPorcentaje de llenado = ");Serial.print(porcentaje); Serial.print("%");
        //Encendido o apagado de la bomba
        if (porcentaje >= 80) {
          digitalWrite(BOMBA, HIGH); // apaga la bomba
          contador = 0;
          Serial.println(contador);
        } else {
          digitalWrite(BOMBA, LOW);  // enciende la bomba
          contador++;
          Serial.println(contador);
        }
        if (contador >= 60){
            //delay(120000);
            medida2 = slaveState.toInt();
          if((medida2 - medida)<15){
            //delay(120000);
            digitalWrite(BOMBA, HIGH);  // Apaga la bomba
            do{
              delay(1000);
              contador2++;
              Serial.println(contador2);
            }while (contador2<=60);
            contador2 = 0;
          }
          contador=0;
        }
        webpage(client, porcentaje);
        delay(1000);

        client.print(nom);
        if (sendCmd) {
          sendCmd = false;
          client.println(": Ok " + slaveid + "! Set state to x" + String(slaveCmd) + "\r");
        } else {
          client.println(": Hi " + slaveid + "!\r"); // sends the answer to the client
        }
        client.stop();                // terminates the connection with the client
      } else {
        Serial.print("From Browser : "); Serial.println(request);
        client.flush();
        //handleRequest(request);
        webpage(client, porcentaje);
      }
    }
  }
}

void webpage(WiFiClient browser, float porcentaje) { /* function webpage */
  ////Send webpage to browser
  String porcentajeStr = String(porcentaje, 2);

  browser.println("HTTP/1.1 200 OK");
  browser.println("Content-Type: text/html");
  browser.println(""); //  do not forget this one
  browser.println("<!DOCTYPE HTML>");
  browser.println("<html>");
  browser.println("<head>");
  browser.println("<meta charset=\"UTF-8\"><meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Sistema de agua</title>/div>");
  browser.println("<style>");
  browser.println("*{padding:0;margin:0;}header{background-color:#38b6ff;text-align:center;height:60px;width:100%;}body{font-family:system-ui,-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Oxygen,Ubuntu,Cantarell,'Open Sans','Helvetica Neue',sans-serif;background-color:black;}div{display:flex;height:80vh;width:100vw;align-items:center;justify-content:center;}");
  browser.println("h3{color:white;text-align:center;}h2{font-size:7rem;position:absolute;}.border{color:white;text-shadow:-1px -1px 0 white,1px -1px 0 white,1px 1px 0 white,-1px 1px 0 white,;}.wave{color:#09f;animation:wave 3s ease-in-out infinite;}@keyframes wave{0%,100%{clip-path:polygon(0% 47%,10% 48%,33% 54%,54% 60%,70% 61%,84% 59%,100% 52%,100% 100%,0% 100%);}50%{clip-path:polygon(0% 60%,15% 65%,34% 66%,51% 62%,67% 50%,84% 45%,100% 46%,100% 100%,0% 100%);}}");
  browser.println("</style>");
  browser.println("</head>");
  browser.println("<body>");
  browser.println("<header><a href=\"https://casinfraestructura.com.mx/\"><img src=\"https://casinfraestructura.com.mx/wp-content/uploads/2021/09/cropped-Diseno-sin-titulo-28-1.png\" alt=\"\"></a></header><div><h2 class=\"border\">TRESAL</h2><h2 class=\"wave\">TRESAL</h2></div>");
  browser.print("<h3>Porcentaje de llenado =");
  browser.print(porcentajeStr);
  browser.print("%");
  browser.println("</h3>");
  int pinState = digitalRead(BOMBA);
  if (pinState == HIGH) {
    browser.print("<h3>ESTATUS DE LA BOMBA: APAGADA</h3>");
  } else {
    browser.print("<h3>ESTATUS DE LA BOMBA: ENCENDIDA</h3>");
  }
  browser.println("</body></html>");
  delay(1);
}

