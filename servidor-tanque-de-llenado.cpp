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
#define NUM_SLAVES 2
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
int numeroMuestra1 = 0;
int numeroMuestra2 = 0;
int status;
float porcentaje1;
float porcentaje2;
float muestraPorcentaje1=0;
float muestraPromedio1=0;
float muestraPorcentaje2=0;
float muestraPromedio2=0;
int capacidadDelTanque1 = 300;
int capacidadDelTanque2 = 300;
stringstream ss;
string str;
string respuesta;
bool estado = 0;
bool estadoAnt = 0;
bool responseSlave0 = true;
bool responseSlave1 = false;
bool estatusCisterna = false;

//Variables para el timer
unsigned long previousMillis = 0;  // Variable para almacenar el tiempo anterior
const unsigned long interval = 120000;  // Limite de tiempo de solicitud del Slave0
unsigned long currentMillis;
unsigned long lastSlaveCommunicationTime1 = 0;  // Tiempo en milisegundos de la última comunicación con el Slave
unsigned long lastSlaveCommunicationTime2 = 0;  // Tiempo en milisegundos de la última comunicación con el Slave

//Objects
WiFiServer serverP(80); //Puerto Servidor primario
WiFiServer serverS(8080); //Puerto Servidor Secundario
WiFiServer serverW(443);
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
  serverP.begin();
  serverS.begin();
  serverW.begin();
  Serial.print(nom);
  Serial.print(F(" connected to Wifi! IP address : http://")); Serial.println(WiFi.localIP()); // Print the IP address
  pinMode(LED, OUTPUT);
  pinMode(BOMBA, OUTPUT);
  digitalWrite(BOMBA, HIGH);//Apaga la bomba
}

void loop() {
  clientRequest8080();
  clientRequest80();
  if(!estatusCisterna){
    status = 1;
  }else{
    status = 2;
  }
  currentMillis = millis();
  if ((currentMillis - lastSlaveCommunicationTime1) > interval && (currentMillis - lastSlaveCommunicationTime2) > interval) {
    // No se ha recibido información del Slave1, apagar la bomba
    digitalWrite(BOMBA, HIGH);
    Serial.println("No se recibió información del Slave 0 y del slave 1. Bomba apagada.");
    status = 3;
  }else if ((currentMillis - lastSlaveCommunicationTime1) > interval && (currentMillis - lastSlaveCommunicationTime2) < interval){
    // No se ha recibido información del Slave1, apagar la bomba
    digitalWrite(BOMBA, HIGH);
    Serial.println("No se recibió información del Slave 1. Bomba apagada.");
    status = 4;
  }else if ((currentMillis - lastSlaveCommunicationTime1) < interval && (currentMillis - lastSlaveCommunicationTime2) > interval){
    // No se ha recibido información del Slave0, apagar la bomba
    digitalWrite(BOMBA, HIGH);
    Serial.println("No se recibió información del Slave 0. Bomba apagada.");
    status = 5;
  }
  clientRequest465(muestraPromedio1, muestraPromedio2, status);
  delay(500);
}

//Funcion de solicitud del cliente puerto 8080
void clientRequest8080(){
  WiFiClient clientS = serverS.available();
  clientS.setTimeout(50);
  if(clientS){
    if(clientS.connected()){
      //Imprimir IP del cliente
      Serial.print("\nIP del cliente --> "); Serial.print(clientS.remoteIP());
      //Recepcion del mesaje del cliente
      String solicitud = clientS.readStringUntil('\r');
      if(solicitud.indexOf("Slave1") == 0){
        //Manejo de la solicitud del Slave0
        //Actualizar el tiempo de la última comunicación con el Slave
        lastSlaveCommunicationTime2 = millis();
        //Imprimir datos de la solicitud
        Serial.print("From "); Serial.print(solicitud);
        int index = solicitud.indexOf(":");
        String slaveId = solicitud.substring(0 , index);
        slaveState = solicitud.substring(solicitud.indexOf("x") + 1, solicitud.length());
        Serial.print("Estado recibido: "); Serial.print(slaveState);
        //Obtencion de la medida del sensor
        medida1 = slaveState.toInt();
        //Porcentaje de llenado del tanque
        porcentaje1 = 100 - (((medida1)*100)/capacidadDelTanque1);
        muestraPorcentaje1 = muestraPorcentaje1 + porcentaje1;
        numeroMuestra1++;
        Serial.print("\nPorcentaje de llenado = ");Serial.print(porcentaje1); Serial.print("%"); Serial.print(" Numero de Muestra = ");Serial.print(numeroMuestra1);
        //Evaluacion del nevel de agua de la 
        if(numeroMuestra1 == 40){
          muestraPromedio1 = (muestraPorcentaje1/40);
          if (muestraPromedio1 >= 50) {
            estatusCisterna = true; // apaga la bomba
            Serial.println("Hay agua en la cisterna");
          } else {
            estatusCisterna = false;  // enciende la bomba
            Serial.println("No hay agua en la cisterna");
          }
          numeroMuestra1 = 0;
          muestraPorcentaje1 = 0;
        }
        clientS.print(nom);
        if (sendCmd) {
          sendCmd = false;
          clientS.println(": Ok " + slaveId + "! Set state to x" + String(slaveCmd) + "\r");
        } else {
          clientS.println(": Hi " + slaveId + "!\r"); // sends the answer to the client
        }
      }
    } 
  }
}
//Funcion de solicitud del cliente puerto 80
void clientRequest80(){
  //Revisar si el cliente esta conectado
  WiFiClient clientP = serverP.available();
  clientP.setTimeout(50);
  if(clientP){
    if(clientP.connected()){
      //Imprimir IP del cliente
      Serial.print("\nIP del cliente --> "); Serial.print(clientP.remoteIP());
      //Recepcion del mesaje del cliente
      String solicitud = clientP.readStringUntil('\r');
      if(solicitud.indexOf("Slave0") == 0){
        //Manejo de la solicitud del Slave0
        //Actualizar el tiempo de la última comunicación con el Slave
        lastSlaveCommunicationTime1 = millis();
        //Imprimir datos de la solicitud
        Serial.print("From "); Serial.print(solicitud);
        int index = solicitud.indexOf(":");
        String slaveId = solicitud.substring(0 , index);
        slaveState = solicitud.substring(solicitud.indexOf("x") + 1, solicitud.length());
        Serial.print("Estado recibido: "); Serial.print(slaveState);
        //Obtencion de la medida del sensor
        medida2 = slaveState.toInt();
        //Porcentaje de llenado del tanque
        porcentaje2 = 100 - (((medida2)*100)/capacidadDelTanque2);
        muestraPorcentaje2 = muestraPorcentaje2 + porcentaje2;
        numeroMuestra2++;
        Serial.print("\nPorcentaje de llenado = ");Serial.print(porcentaje2); Serial.print("%"); Serial.print(" Numero de Muestra = ");Serial.print(numeroMuestra2);
        //Encendido y apagado de la bomba
        if(numeroMuestra2 == 40){
          muestraPromedio2 = (muestraPorcentaje2/40);
          if (muestraPromedio2 >= 90) {
              digitalWrite(BOMBA, HIGH); // apaga la bomba
              Serial.println("Bomba apagada");
          }else if (muestraPromedio2 < 50){
              if(estatusCisterna){
                digitalWrite(BOMBA, LOW);  // enciende la bomba
                Serial.println("Bomba encendida");
              }else{
                Serial.println("No hay agua en la cisterna, no se encendera la bomba");
                digitalWrite(BOMBA, HIGH); // apaga la bomba
              }
          }
          numeroMuestra2 = 0;
          muestraPorcentaje2 = 0;   
        }
        clientP.print(nom);
        if (sendCmd) {
          sendCmd = false;
          clientP.println(": Ok " + slaveId + "! Set state to x" + String(slaveCmd) + "\r");
        } else {
          clientP.println(": Hi " + slaveId + "!\r"); // sends the answer to the client
        }
      }
    }
  }
}

void clientRequest465(float muestraPromedio1, float muestraPromedio2, int status){
  WiFiClient clientW = serverW.available();
  clientW.setTimeout(50);
  if(clientW){
    if(clientW.connected()){
      //Imprimir IP del cliente
      Serial.print("IP del cliente --> "); Serial.print(clientW.remoteIP());
      //Recepcion del mesaje del cliente
      String solicitud = clientW.readStringUntil('\r');
      Serial.print("From Browser : "); Serial.println(solicitud);
      clientW.flush();
      //handleRequest(solicitud);
      webpage(clientW, muestraPromedio1, muestraPromedio2, status);
    }
  }
}

//Funcion de la pagina web
void webpage(WiFiClient browser, float muestraPromedio1, float muestraPromedio2, int status) { /* function webpage */
  ////Send webpage to browser
  String porcentajeStr1 = String(muestraPromedio1, 2);
  String porcentajeStr2 = String(muestraPromedio2, 2);

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
  browser.print("<h3>Porcentaje de llenado del tanque =");
  browser.print(porcentajeStr2);
  browser.print("%");
  browser.println("</h3>");
  browser.print("<h3>Porcentaje de llenado de la cisterna =");
  browser.print(porcentajeStr1);
  browser.print("%");
  browser.println("</h3>");
  int pinState = digitalRead(BOMBA);
  if (pinState == HIGH) {
    browser.print("<h3>ESTATUS DE LA BOMBA: APAGADA</h3>");
  } else {
    browser.print("<h3>ESTATUS DE LA BOMBA: ENCENDIDA</h3>");
  }
  if(status == 1){
    browser.print("<h3>(Funcionando correctamente, No hay agua en la cisterna)</h3>");
  }else if(status == 2){
    browser.print("<h3>(Funcionando correctamente)</h3>");
  }else if(status == 3){
    browser.print("<h3>(No se reciben datos del sensor de nivel del cisterna y del tanque)</h3>");
  }else if(status == 4){
    browser.print("<h3>(No se reciben datos del sensor de nivel de la cisterna)</h3>");
  }else if(status == 5){
    browser.print("<h3>(No se reciben datos del sensor de nivel de la tanque)</h3>");
  }
  browser.println("</body></html>");
  delay(1);
}