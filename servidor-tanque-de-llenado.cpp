//Server  

//Libraries
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFi.h
#include <iostream>
#include <cmath>
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
const char* ssid = "Skynets";
const char* password = "SkyPw0l1";

//Variables
bool sendCmd = false;
String slaveCmd = "0";
String slaveState = "0";
int medida;
int medida1;
int medida2;
int diferencia;
int contador = 0;
int contador2 = 0; 
int numeroMuestra = 0;
int status;
float porcentaje;
float muestraPorcentaje=0;
float muestraPromedio=0;
int capacidadDelTanque = 400;
stringstream ss;
string str;
string respuesta;
bool estado = 0;
bool estadoAnt = 0;

//Variables para el timer
unsigned long previousMillis = 0;  // Variable para almacenar el tiempo anterior
const unsigned long interval = 20000;  // Limite de tiempo de solicitud del Slave0
unsigned long currentMillis;
unsigned long lastSlaveCommunicationTime = 0;  // Tiempo en milisegundos de la última comunicación con el Slave

//Objects
WiFiServer server(80);
WiFiClient browser;
IPAddress ip(192, 168, 1, 177);
IPAddress gateway(192, 168, 1, 1);
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
  client.setTimeout(500);
  if (client) {
    if (client.connected()) {
      //Imprime la direccion ip del cliente
      Serial.print(" ->");Serial.println(client.remoteIP());
      String request = client.readStringUntil('\r'); //receive el mensaje del cliente
      
      if (request.indexOf("Slave0") == 0) {
        // Actualizar el tiempo de la última comunicación con el Slave
        lastSlaveCommunicationTime = millis();
        //Manejo de la solicitud del esclavo
        Serial.print("From "); Serial.println(request);
        int index = request.indexOf(":");
        String slaveid = request.substring(0, index);
        slaveState = request.substring(request.indexOf("x") + 1, request.length());
        Serial.print("state received: "); Serial.println(slaveState);
        //Obtencion de la medida del sensor
        medida = slaveState.toInt();
        //Porcentaje de llenado del tanque
        porcentaje = 100 - (((medida)*100)/capacidadDelTanque);
        muestraPorcentaje = muestraPorcentaje + porcentaje;
        numeroMuestra++;
        Serial.print("\nPorcentaje de llenado = ");Serial.print(porcentaje); Serial.print("%");
        status = 1;
        //Encendido o apagado de la bomba
        if(numeroMuestra == 40){
          muestraPromedio = (muestraPorcentaje/40);
          if (muestraPromedio >= 90) {
          digitalWrite(BOMBA, HIGH); // apaga la bomba
          contador = 0;
          Serial.println(contador);
        } else {
          digitalWrite(BOMBA, LOW);  // enciende la bomba
          contador++;
          Serial.println(contador);
        }
        if(contador==1){
          medida1 = muestraPromedio;
        }
        if (contador >= 6){
            //delay(120000);
            medida2 = muestraPromedio;
            diferencia = abs(medida1 - medida2);
          if(diferencia<20){
            //delay(120000);
            digitalWrite(BOMBA, HIGH);  // Apaga la bomba
            do{
              contador2++;
              Serial.println(contador2);
              WiFiClient client = server.available();
              client.setTimeout(500);
              if (client) {
                ////Revisa si el cliente esta conectado
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
                    porcentaje = 100 - (((medida)*100)/capacidadDelTanque);
                    Serial.print("\nPorcentaje de llenado = ");Serial.print(porcentaje); Serial.print("%");
                    client.print(nom);
                    if (sendCmd) {
                      sendCmd = false;
                      client.println(": Ok " + slaveid + "! Set state to x" + String(slaveCmd) + "\r");
                    } else {
                      client.println(": Hi " + slaveid + "!\r"); // sends the answer to the client
                    }
                  } else {
                    //Obtencion de la medida del sensor
                    medida = slaveState.toInt();
                    //Porcentaje de llenado del tanque
                    porcentaje = 100 - (((medida)*100)/capacidadDelTanque);
                    Serial.print("\nPorcentaje de llenado = ");Serial.print(porcentaje); Serial.print("%");
                    Serial.print("From Browser : "); Serial.println(request);
                    client.flush();
                    status = 2;
                    //handleRequest(request);
                    webpage(client, porcentaje, status);
                  }
                }  
              }
              delay(1000);   //client.stop();  // Terminates the connection with the client
            }while (contador2<=7200);
            contador2 = 0;
          }
          contador=0;
          medida1=0;
          medida2=0;
        }
        muestraPorcentaje = 0;
        numeroMuestra = 0;
        }
        client.print(nom);
        if (sendCmd) {
          sendCmd = false;
          client.println(": Ok " + slaveid + "! Set state to x" + String(slaveCmd) + "\r");
        } else {
          client.println(": Hi " + slaveid + "!\r"); // sends the answer to the client
        }
      } else {
        Serial.print("From Browser : "); Serial.println(request);
        client.flush();
        //handleRequest(request);
        webpage(client, porcentaje, status);
      }
    }
    //client.stop();  // Terminates the connection with the client
  }
  // Verificar si ha pasado mucho tiempo desde la última comunicación con el Slave
  currentMillis = millis();
  if (currentMillis - lastSlaveCommunicationTime > interval) {
    // No se ha recibido información del Slave, apagar la bomba
    digitalWrite(BOMBA, HIGH);
    Serial.println("No se recibió información del Slave. Bomba apagada.");
    status = 3;
  }
  delay(500);
}

void webpage(WiFiClient browser, float porcentaje, int status) { /* function webpage */
  ////Send webpage to browser
  String porcentajeStr = String(porcentaje, 2);

  browser.println("HTTP/1.1 200 OK");
  browser.println("Content-Type: text/html");
  browser.println(""); //  do not forget this one
  browser.println("<!DOCTYPE HTML>");
  browser.println("<html>");
  browser.println("<head>");
  browser.println("<meta charset=\"UTF-8\"><meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Sistema de agua</title>");
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
  if(status == 1){
    browser.print("<h3>(Funcionando correctamente)</h3>");
  }else if (status == 2){
    browser.print("<h3>(No hubo flujo de agua, en espera para volver a intentar)</h3>");
  } else if(status == 3){
    browser.print("<h3>(No se reciben datos del sensor de nivel)</h3>");
  }
  browser.println("</body></html>");
  delay(1);
}

