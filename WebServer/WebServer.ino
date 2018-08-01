#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <WiFiClientSecure.h>

//-------------------VARIABLES GLOBALES--------------------------

const char *ssid = "Zoylofull";
const char *password = "1234432112";

unsigned long previousMillis = 0;

char host[99];
String strhost = "clientes.webbuilders.com.ar";
String strurl = "/testSmartZ.php"; // donde tengo la web
String chipid = ""; //identificador unico de la placa de m

//-------Función para Enviar Datos a la Base de Datos SQL--------

String enviardatos(String datos) 
{
      String linea = "error";
      //WiFiClientSecure client; //esto es para https
      WiFiClient client;
      strhost.toCharArray(host, 100);
     // Serial.println(host);
      if (!client.connect(host, 80)) {
        Serial.println("Fallo de conexion");
        return linea;
      }
    
      client.print(String("PUT ") + strurl + " HTTP/1.1" + "\r\n" +
                   "Host: " + strhost + "\r\n" +
                   "Accept: */*" + "*\r\n" +
                   "Content-Length: " + datos.length() + "\r\n" +
                   "Content-Type: application/x-www-form-urlencoded" + "\r\n" +
                   "\r\n" + datos);
      delay(10);
    
      //Serial.print("Enviando datos a SQL...");
    
      unsigned long timeout = millis();
      while (client.available() == 0) 
      {
        if (millis() - timeout > 5000) 
        {
          Serial.println("Cliente fuera de tiempo!");
          client.stop();
          return linea;
        }
      }
      // Lee todas las lineas que recibe del servidor y las imprime por la terminal serial
      while (client.available()) 
      {
        linea = client.readStringUntil('\r');
      }
      Serial.println(linea);
/*
      const size_t bufferSize = JSON_OBJECT_SIZE(1) + 20;
      DynamicJsonBuffer jsonBuffer(bufferSize);
      const char* json = "{\"Distancia\":3.44}";
      JsonObject& root = jsonBuffer.parseObject(json);
      float Distancia = root["Distancia"]; // 3.44
  */    
     return linea;
}

//-------------------------------------------------------------------------

void setup() {

  // Inicia Serial
  Serial.begin(115200);
  Serial.println("");

  Serial.print("chipId: ");
  chipid = String(ESP.getChipId());
  Serial.println(chipid);

  // Conexión WIFI
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED and contconexion < 50) { //Cuenta hasta 50 si no se puede conectar lo cancela
    ++contconexion;
    delay(500);
    Serial.print(".");
  }
  // Si quiero usar IP fija
  if (contconexion < 50) {
//   para usar con ip fija
//   IPAddress ip(192, 168, 0, 201);
//   IPAddress gateway(192, 168, 0, 1);
//   IPAddress subnet(255, 255, 255, 0);
//   IPAddress dns(8, 8, 8, 8);
//   WiFi.config(ip, dns, gateway, subnet);
//   WiFi.setDNS(dns);

    Serial.println("");
    Serial.println("WiFi conectado");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("");
    Serial.println("Error de conexion");
  }
}

//--------------------------LOOP--------------------------------
void loop() {
    /*
    unsigned long currentMillis = millis();
    int i=0;
    if (currentMillis - previousMillis >= 10000) { //envia la  cada 10 segundos
        previousMillis = currentMillis;
        enviardatos("chipid=" + chipid + "&=" + String(i, 2));
        Serial.println(i);
        i++;
    }*/
  
    //crear Json
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["Distancia"] = distancia;
    Serial.println();
    json.prettyPrintTo(Serial);
    String dato;
    json.printTo(dato);
    enviardatos("dato=" + dato);









/*

  //  Pedir el Json del servidor
if (WiFi.status() == WL_CONNECTED) 
{
    HTTPClient http;  //Object of class HTTPClient
    //http.begin("http://jsonplaceholder.typicode.com/users/1");
    http.begin("http://clientes.webbuilders.com.ar/test.txt");
    
    int httpCode = http.GET();
    //Check the returning code                                                                  
    if (httpCode > 0) {
      // Get the request response payload
      String payload = http.getString();
      // TODO: Parsing
      Serial.print(payload);
    }
    http.end();   //Close connection
  }
    
  */

  
  delay(1000);


}


