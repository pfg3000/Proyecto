#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
//#include <WiFiClient.h>
//#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#include <Time.h>
#include <Wire.h>
#include <TimeAlarms.h>

#include "FS.h"
#include "SPISlave.h"

#define DEBUG
//********************************** << PROTOCOLO DE COMUNICACION *****************************
#ifdef  DEBUG
#define DELIMITER_CHARACTER '|'
#define START_CHARACTER '#'
#define END_CHARACTER '*'
#define SERIAL_PRINT(x,y)        Serial.print(x);Serial.println(y);
#endif

#ifndef DEBUG
#define DELIMITER_CHARACTER (int8_t)0xAE
#define START_CHARACTER (int8_t)0x91
#define END_CHARACTER (int8_t)0x92
#define SERIAL_PRINT(x,y)        Serial.print(x);Serial.println(y);
#endif

#define ERROR_CODE              (-1)
#define ERROR_BAD_CHECKSUM      (-2)
#define ERROR_READ_CARD         (-3)
#define CLOSE                    5
#define OPEN                     65

#define PACKAGE_FORMAT   "%c%c%s%c%s%c%c" //start character + delimiter character + payLoad + delimiter character + checksum + delimiter character + end character.
#define SIZE_PAYLOAD     25
#define SIZE_PACKAGE     SIZE_PAYLOAD + 7
#define SIZE_CHECKSUM    3
#define SIZE_MSG_QUEUE   10

String receivedPackage = "";
int COMANDO = 0;
bool enviarPaquete = true;
bool enviarPaquete2 = true;
unsigned long timeout;
unsigned long timeout2;
#define SEND_PACKAGE_TIME 45000
#define SEND_PACKAGE2_TIME 15000
bool mutexSPI = true;// para que no se pise el SPI con la generacion del jSon.
bool mutexWifi = true;// para que no se pise el ingreso del nuevo SSID y Pass con la generacion del jSon.

//********************************** PROTOCOLO DE COMUNICACION >> *****************************
//*********************************************************************************************
//********************************** << SSID **************************************************
String pral = "<html>"
              "<meta http-equiv='Content-Type' content='text/html; charset=utf-8'/>"
              "<title>SmartZ WIFI</title> <style type='text/css'> body,td,th { color: #036; } body { background-color: #999; } </style> </head>"
              "<body> "
              "<h1>SmartZ WIFI</h1><br>"
              "<form action='config' method='get' target='pantalla'>"
              "<fieldset align='left' style='border-style:solid; border-color:#336666; width:200px; height:180px; padding:10px; margin: 5px;'>"
              "<legend><strong>Configurar WI-FI</strong></legend>"
              "SSID: <br> <input name='ssid' type='text' size='15'/> <br><br>"
              "PASSWORD: <br> <input name='pass' type='password' size='15'/> <br><br>"
              "<input type='submit' value='Comprobar conexion' />"
              "</fieldset>"
              "</form>"
              "<iframe id='pantalla' name='pantalla' src='' width=900px height=400px frameborder='0' scrolling='no'></iframe>"
              "</body>"
              "</html>";

ESP8266WebServer server(80);

String ssid_leido;
String pass_leido;
int ssid_tamano = 0;
int pass_tamano = 0;
char ssid[50];
char password[50];
//********************************** SSID >> **************************************************

//-------------------VARIABLES GLOBALES--------------------------

bool CONFIGURAR_ALARMAS = true;
AlarmId loggit;

char host[99];
String strhost = "clientes.webbuilders.com.ar";// donde tengo la web
String strurl = "/testSmartZ.php"; // metodo

String jsonForUpload;
String configPackage = "";
String downloadConfig = "";

String idDispositivo; // identificador unico de la placa ESP
int Notificaciones;
int NotificacionesReal = 15;
int CantidadHorasLuz;
int HoraInicioLuz;
float PH_aceptable;

float medicionHumedad;
float medicionCO2;
float medicionTemperaturaAire;
float medicionTemperaturaAgua;
float medicionPH;
float medicionCE;
float medicionNivelTanquePrincial;
float medicionNivelTanqueAguaLimpia;
float medicionNivelTanqueDesechable;
float medicionNivelPHmas;
float medicionNivelPHmenos;
float medicionNivelNutrienteA;
float medicionNivelNutrienteB;
char Alert[50];
char Error1[50];

//********* Declaracion de variables para cada color R G B
int rled = 2; // Pin para led rojo
int bled = 4; // Pin para led azul
int gled = 0;  // Pin para led verde

void setup() {
  //********* Se inicializan pines PWM como salida
  pinMode(rled, OUTPUT);
  pinMode(bled, OUTPUT);
  pinMode(gled, OUTPUT);

  //********* Protocolo de comunicación.
  receivedPackage.reserve(200);

  // Inicia Serial
  Serial.begin(9600);
  Serial.setDebugOutput(true);

  idDispositivo = String(ESP.getChipId());
  SERIAL_PRINT("chipId: ", idDispositivo);

  char idDispChar[25];
  String(idDispositivo).toCharArray(idDispChar, String(idDispositivo).length()+1);

  WiFi.softAP("SmartZ", idDispChar);     //Nombre que se mostrara en las redes wifi

  server.on("/", []() {
    server.send(200, "text/html", pral);
  });
  server.on("/config", wifi_conf);
  server.begin();
  Serial.println("Webserver iniciado...");

  Serial.println(lee(70));
  Serial.println(lee(1));
  Serial.println(lee(30));
  intento_conexion();



  if (!SPIFFS.begin())
  {
    Serial.println("Failed to mount file system");
    //return;
  }
  if (!loadConfig())
  {
    Serial.println("Failed to load config");
  }
  else
  {
    Serial.println("Config loaded");
    //Serial.println(configPackage);
    //delay(200);
    //enviarPaquete();
  }

  //*************************************************************************** SPI

  SPISlave.onData([](uint8_t * data, size_t len) {
    String message = String((char *)data);
    (void) len;
    //Serial.println(message);
    Serial.printf("Recibo: %s\n", (char *)data);
    receivedPackage = message;

    char cmd[] = "00";
    char payLoad[25];
    char package[32];
    char aux[22];
    if (mutexSPI)
    {
      if (checkPackageComplete()) {
        strcpy(payLoad, "00");
        strcpy(payLoad, "");
        strcpy(package, "");
        strcpy(aux, "");

        switch (COMANDO) {
          case 16:
            strcpy(cmd, "16");
            parameterToJson("HorasLuz", intTOstring(CantidadHorasLuz)).toCharArray(aux, 22);
            snprintf(payLoad, sizeof(payLoad), "%s%c%s", cmd, (char)DELIMITER_CHARACTER, aux);
            strcpy(package, preparePackage(payLoad, strlen(payLoad)));
            break;
          case 17:
            strcpy(cmd, "17");
            parameterToJson("HoraIniLuz", intTOstring(HoraInicioLuz)).toCharArray(aux, 22);
            snprintf(payLoad, sizeof(payLoad), "%s%c%s", cmd, (char)DELIMITER_CHARACTER, aux);
            strcpy(package, preparePackage(payLoad, strlen(payLoad)));
            break;
          case 18:
            strcpy(cmd, "18");
            parameterToJson("pHaceptable", intTOstring(PH_aceptable)).toCharArray(aux, 22);
            snprintf(payLoad, sizeof(payLoad), "%s%c%s", cmd, (char)DELIMITER_CHARACTER, aux);
            strcpy(package, preparePackage(payLoad, strlen(payLoad)));
            break;
          default:
            completarLargo(intTOstring(COMANDO), 2, 1).toCharArray(cmd, 3);
            snprintf(payLoad, sizeof(payLoad), "%s%c{\"OK\":\"%s\"}", cmd, (char)DELIMITER_CHARACTER, cmd);
            strcpy(package, preparePackage(payLoad, strlen(payLoad)));
            break;
        }
      }
      else
      {
        strcpy(cmd, "99");
        strcpy(payLoad, "");
        strcpy(package, "");
        strcpy(aux, "");

        snprintf(payLoad, sizeof(payLoad), "%s%c%s", cmd, (char)DELIMITER_CHARACTER, "{\"ER\":\"99\"}");
        strcpy(package, preparePackage(payLoad, strlen(payLoad)));
      }
    }
    else
    {
      strcpy(cmd, "98");
      strcpy(payLoad, "");
      strcpy(package, "");
      strcpy(aux, "");

      snprintf(payLoad, sizeof(payLoad), "%s%c%s", cmd, (char)DELIMITER_CHARACTER, "{\"BS\":\"BUSY\"}");
      strcpy(package, preparePackage(payLoad, strlen(payLoad)));
    }
    Serial.print(package);
    SPISlave.setData(package);
  });

  SPISlave.onDataSent([]() {
    Serial.println("fue enviado");
  });

  SPISlave.onStatus([](uint32_t data) {
    Serial.printf("Status: %u\n", data);
    SPISlave.setStatus(millis()); //set next status
  });

  SPISlave.onStatusSent([]() {
    Serial.println("Status Sent");
  });

  SPISlave.begin();
  SPISlave.setStatus(millis());

}

//--------------------------LOOP--------------------------------
void loop() {
  server.handleClient();

  //if (CONFIGURAR_ALARMAS)
  //{
  //setTime(0, 0, 0, 1, 1, 2000);
  //loggit = Alarm.timerRepeat(NotificacionesReal, enviardatos);
  //Alarm.timerRepeat(63, upDatos);
  ////Alarm.disable();
  //CONFIGURAR_ALARMAS = false;
  // }

  //digitalClockDisplay();

  if (downloadConfig != "" && downloadConfig != configPackage)
  {
    delay(10);
    if (!saveConfig())
    {
      Serial.println("Failed to save config");
    }
    else
    {
      Serial.println("Config saved");
      //Serial.println(downloadConfig);
      if (NotificacionesReal != Notificaciones) {
        NotificacionesReal = Notificaciones;
        //        CONFIGURAR_ALARMAS = true;
        //        Alarm.disable(loggit);
      }
      if (!loadConfig())
      {
        Serial.println("Failed to load config");
      }
      delay(200);
      //enviarPaquete();
    }
  }

  if (enviarPaquete) {
    timeout = millis();
    enviarPaquete = false;
  }
  if ((millis() - timeout > SEND_PACKAGE_TIME)) {
    Serial.println("GENERANDO JSON");
    upDatos();
    enviarPaquete = true;
  }

  if (enviarPaquete2) {
    timeout2 = millis();
    enviarPaquete2 = false;
  }
  if ((millis() - timeout2 > SEND_PACKAGE2_TIME)) {
    Serial.println("GENERANDO JSON");
    enviardatos();
    enviarPaquete2 = true;
  }

  checkStatusWifi();
  delay(2000);
}
//---------------------------------------------------------------------------------------------------------------//
String parameterToJson(String nombre, String valor)
{
  return "{\"" + nombre + "\":\"" + valor + "\"}";
}
//---------------------------------------------------------------------------------------------------------------//
String floatTOstring(float x)//Convertir de float a String
{
  return String(x);
}
//---------------------------------------------------------------------------------------------------------------//
String intTOstring(int x)//Convertir de int a String
{
  return String(x);
}
//---------------------------------------------------------------------------------------------------------------//
void upDatos()
{
  mutexSPI = false;
  String linea;
  if (WiFi.status() == WL_CONNECTED)
  {
    jsonForUpload = generarJson();
    jsonForUpload = "http://clientes.webbuilders.com.ar/testSmartZ.php?dato=" + jsonForUpload;
    char url[1000];
    jsonForUpload.toCharArray(url, jsonForUpload.length() + 1);
    //    SERIAL_PRINT("jsonForUpload: ", jsonForUpload);
    //    SERIAL_PRINT("URL: ", url);
    HTTPClient http;
    http.begin(url);

    int httpCode = http.GET();
    if (httpCode > 0) {
      linea  = http.getString();
      SERIAL_PRINT("linea=", linea);
    }
    else
    {
      SERIAL_PRINT("error http ", httpCode);
    }
    http.end();   //Close connection
  }
  mutexSPI = true;
}
//---------------------------------------------------------------------------------------------------------------//
void enviardatos()
{
  String linea;
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin("http://clientes.webbuilders.com.ar/testSmartZ.php?dato={\"id\":\"12625275\",\"Notificacion\":\"015\",\"HorasLuz\":\"02\",\"HoraInicioLuz\":\"21\",\"pHaceptable\":\"0007\"}");

    int httpCode = http.GET();
    if (httpCode > 0) {
      linea  = http.getString();
      SERIAL_PRINT("linea=", linea);
    }
    else
    {
      SERIAL_PRINT("error http ", httpCode);
    }
    http.end();   //Close connection
  }
  /*
    String datos;
    int distancia = 0;
    //crear Json

    StaticJsonBuffer<200> jsonBuffer1;
    JsonObject& json1 = jsonBuffer1.createObject();
    json1["Distancia"] = distancia;
    //  Serial.println();
    //  json.prettyPrintTo(Serial);
    String dato;
    json1.printTo(dato);
    //datos = "dato=" + dato;
    datos = "dato={\"id\":\"12625275\",\"Notificacion\":\"015\",\"HorasLuz\":\"02\",\"HoraInicioLuz\":\"21\",\"pHaceptable\":\"0007\"}";

    String linea = "error";
    //WiFiClientSecure client; //esto es para https
    WiFiClient client;
    strhost.toCharArray(host, 100);
    // Serial.println(host);
    if (!client.connect(host, 80)) {
      Serial.println("Fallo de conexion");
      //return linea;
    }

    client.print(String("PUT ") + strurl + " HTTP/1.1" + "\r\n" +
                 "Host: " + strhost + "\r\n" +
                 "Accept: *//*" + "*\r\n" +
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
  //return linea;
  }
  }
  // Lee todas las lineas que recibe del servidor y las imprime por la terminal serial
  while (client.available())
  {
  linea = client.readStringUntil('\r');
  }

  SERIAL_PRINT("linea=", linea);
*/

  //linea = "{\"id\":\"12625275\",\"Notificacion\":\"015\",\"HorasLuz\":\"02\",\"HoraInicioLuz\":\"21\",\"pHaceptable\":\"0007\"}";
  if (linea != "") {
    StaticJsonBuffer<500> jsonBuffer;
    char json[300];
    linea.toCharArray(json, 300);
    JsonObject& root = jsonBuffer.parseObject(json);
    //idDispositivo = root["id"];
    Notificaciones = root["Notificacion"];
    CantidadHorasLuz = root["HorasLuz"];
    HoraInicioLuz = root["HoraInicioLuz"];
    PH_aceptable = root["pHaceptable"];

    //  SERIAL_PRINT("id=", id);
    //  SERIAL_PRINT("N=", Notificacion);
    //  SERIAL_PRINT("CHL=", HorasLuz);
    //  SERIAL_PRINT("HL=", HoraInicioLuz);
    //  SERIAL_PRINT("PH=", pHaceptable);

    downloadConfig = "";
    root.printTo(downloadConfig);
  }
  else
  {
    downloadConfig = "";
  }

  /*  char package1[144];
    char package[151];

    linea.toCharArray(package1, 151);

    strcpy(package, preparePackage(package1, strlen(package1)));
    Serial.flush();

    Serial.println("");
    for (int i = 0; i < strlen(package); i++) {
      Serial.write(package[i]);
      if (i % 63 == 0)
        delay(500);
    }
  */
  /*
    const size_t bufferSize = JSON_OBJECT_SIZE(1) + 145;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    char json[146];
    linea.toCharArray(json, 146);
    JsonObject& root = jsonBuffer.parseObject(json);
    //root.prettyPrintTo(Serial);
    downloadConfig = "";
    root.printTo(downloadConfig);
    //float Distancia = root["Distancia"]; // 3.44

    //return linea;*/
}

//---------------------------------------------------------------------------------------------------------------//
void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println();
}
//---------------------------------------------------------------------------------------------------------------//
void printDigits(int digits)
{
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** < FUNCIONES para el protocolo de paquetes entre ARDUINO y ESP8266
//---------------------------------------------------------------------------------------------------------------//
char* calcChecksum(const char *data, int length)
{
  static char checksum[SIZE_CHECKSUM];
  int result = 0;

  for (int idx = 0; idx < length; idx ++)
  {
    result = result ^ data[idx];
  }

  if (result <= 15)
  {
    if (result <= 9)
    {
      checksum[0] = '0';
      checksum[1] = result + 48;
      checksum[2] = '\0';
    }
    else
    {
      checksum[0] = '0';
      checksum[1] = result + 87;
      checksum[2] = '\0';
    }
  }
  else
  {
    int rest;
    int quotient;
    char numHexa[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    rest = result % 16;
    quotient = result / 16;
    checksum[2] = '\0';
    checksum[1] = numHexa[rest];
    checksum[0] = numHexa[quotient];
  }
  return checksum;
}
//---------------------------------------------------------------------------------------------------------------//
char *preparePackage(const char *payLoad, int length)
{
  char checksum[SIZE_CHECKSUM];
  memcpy(checksum, calcChecksum(payLoad, length), 3 * sizeof(char));
  checksum[2] = '\0';
  static char package[SIZE_PACKAGE];
  snprintf(package, sizeof(package), PACKAGE_FORMAT, START_CHARACTER, DELIMITER_CHARACTER, payLoad, DELIMITER_CHARACTER, checksum, DELIMITER_CHARACTER, END_CHARACTER);
  return package;
}
//---------------------------------------------------------------------------------------------------------------//
bool compareChecksum(const char *checksumA, const char *checksumB)
{
  bool result = true;

  for (int idx = 0; idx < SIZE_CHECKSUM; idx++)
  {
    if (checksumA[idx] != checksumB[idx])
    {
      result = false;
    }
  }
  return result;
}
//---------------------------------------------------------------------------------------------------------------//
char *getChecksumFromReceivedPackage(const char *package, int length)
{
  static char checksum[SIZE_CHECKSUM];
  byte countDelimiterCharacter = 0;

  int  index = 0;
  bool findOK = false;
  for (; index < length; index++)
  {
    if (package[index] == END_CHARACTER)
    {
      findOK = true;
      break;
    }
  }

  if (!findOK)
  {
    return NULL;
  }

  index -= 3;
  for (int idx = 0; idx < SIZE_CHECKSUM - 1; idx++)
  {
    checksum[idx] = package[index + idx];
  }
  checksum[2] = '\0';
  return checksum;
}
//---------------------------------------------------------------------------------------------------------------//
bool validatePackage(const char *package, int length)
{
  return compareChecksum(getChecksumFromReceivedPackage(package, length), calcChecksum(package + 2, length - 7));
  // + 2 para saltearme el start_character y el -7 es (caracter de inicio + primer delimitador + 1 delimitador antes del checksum +  2 caracteres del checksum + ultimo delimitador
  // + caracter de fin.
}
//---------------------------------------------------------------------------------------------------------------//
char *disarmPackage(const char *package, int length)
{
  static char payLoad[SIZE_PAYLOAD];
  memcpy(payLoad, package + 2, (length - 7 )*sizeof(char)); // +3 para salterame el caracter inicial(1) + primer delimitador(1); -7 para no copiar caracter de inicio
  // de paquete(1), primer delimitador(1), delimitador antes del checksum(1) el checksum(2) el limitador despues del checksum(1) y el caracter de fin de paquete(1).
  payLoad[length - 7] = '\0';
  return payLoad;
}
//---------------------------------------------------------------------------------------------------------------//
int disarmPayLoad(const char *payLoad, int length, char *data) //*********************************************************************************************************** VER
{
  char command[3];
  memcpy(command, payLoad, 2 * sizeof(char));
  command[2] = '\0';

  int cmd = atoi(command);
  if (cmd > 0 && cmd < 100)
  {
    //if (cmd == 6) {  // unico comando proveniente desde la raspberry con datos;
    memcpy (data, payLoad + 3, (length - 2)*sizeof(char));
    data[length - 2] = '\0';
    // }
    return cmd;
  }
  return ERROR_CODE; //retCode error;
}
//---------------------------------------------------------------------------------------------------------------//
int proccesPackage(String package, int length, char *data)
{
  SERIAL_PRINT("\n> Recieved Package: ", package);
  char  recievedPack[SIZE_PACKAGE];
  char  payLoad[SIZE_PAYLOAD];
  package.toCharArray(recievedPack, sizeof(recievedPack));
  if (validatePackage(recievedPack, strlen(recievedPack)))
  {
    SERIAL_PRINT("> Checksum valid!!", "");
    strcpy(payLoad, disarmPackage(recievedPack, strlen(recievedPack)));
    SERIAL_PRINT("> PayLoad Disarmed: ", payLoad);

    int command = disarmPayLoad(payLoad, strlen(payLoad), data);
    if (command != -1)
    {
      SERIAL_PRINT("> Command: ", command);
      SERIAL_PRINT("> Data: ", data);
      return command;
    }
    else
    {
      return ERROR_CODE;
    }
  }
  else
  {
    return ERROR_BAD_CHECKSUM;
  }
}
//---------------------------------------------------------------------------------------------------------------//
bool checkPackageComplete(void)
{
  char data[50];
  COMANDO = 0;
  int retCmd = proccesPackage(receivedPackage, receivedPackage.length(), data);

  if (retCmd == ERROR_CODE)
  {
    SERIAL_PRINT("> Error unknown command !!", "");
    return false;
  }
  else if (retCmd == ERROR_BAD_CHECKSUM)
  {
    SERIAL_PRINT("> Error bad checksum!!", "");
    return false;
  }
  else
  {
    StaticJsonBuffer<200> jsonBuffer;
    //    char auxiliar[200];
    //    strcpy(auxiliar, data);
    JsonObject& root = jsonBuffer.parseObject(data);

    const char *Alertas;
    const char *Errores;

    COMANDO = retCmd;
    SERIAL_PRINT("iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii ", COMANDO);
    switch (retCmd) {
      case 1:
        medicionHumedad = root["HumAire"];
        SERIAL_PRINT("HumAire ", medicionHumedad);
        break;
      case 2:
        medicionCO2 = root["CO2"];
        SERIAL_PRINT("CO2 ", medicionCO2);

        break;
      case 3:
        medicionTemperaturaAire = root["TempAire"];
        SERIAL_PRINT("TempAire ", medicionTemperaturaAire);

        break;
      case 4:
        medicionTemperaturaAgua = root["TempAgua"];
        SERIAL_PRINT("TempAgua ", medicionTemperaturaAgua);

        break;
      case 5:
        medicionPH = root["PH"];
        SERIAL_PRINT("PH ", medicionPH);

        break;
      case 6:
        medicionCE = root["CE"];
        SERIAL_PRINT("CE ", medicionCE);

        break;
      case 7:
        medicionNivelTanquePrincial = root["NivelTanqueP"];
        SERIAL_PRINT("NivelTanqueP ", medicionNivelTanquePrincial);

        break;
      case 8:
        medicionNivelTanqueAguaLimpia = root["NivelTanqueL"];
        SERIAL_PRINT("NivelTanqueL ", medicionNivelTanqueAguaLimpia);

        break;
      case 9:
        medicionNivelTanqueDesechable = root["NivelTanqueD"];
        SERIAL_PRINT("NivelTanqueD ", medicionNivelTanqueDesechable);

        break;
      case 10:
        medicionNivelPHmas = root["NivelPh+"];
        SERIAL_PRINT("NivelPh+ ", medicionNivelPHmas);

        break;
      case 11:
        medicionNivelPHmenos = root["NivelPh-"];
        SERIAL_PRINT("NivelPh- ", medicionNivelPHmenos);

        break;
      case 12:
        medicionNivelNutrienteA = root["NivelNutA"];
        SERIAL_PRINT("NivelNutA ", medicionNivelNutrienteA);

        break;
      case 13:
        medicionNivelNutrienteB = root["NivelNutA"];
        SERIAL_PRINT("NivelNutA ", medicionNivelNutrienteB);

        break;
      case 14:
        Alertas = root["A"];
        sprintf(Alert, "%s", Alertas);
        SERIAL_PRINT("A ", Alertas);

        break;
      case 15:
        Errores = root["E"];
        sprintf(Error1, "%s", Errores);
        SERIAL_PRINT("E ", Errores);

        break;
      case 16:
        //COMANDO = 1;
        SERIAL_PRINT("HsL", "");

        break;
      case 17:
        //COMANDO = 2;
        SERIAL_PRINT("HiL", "");

        break;
      case 18:
        //COMANDO = 3;
        SERIAL_PRINT("phA", "");

        break;
    }
  }
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
String generarJson()
{
  String a(Alert);
  String e(Error1);

  String resultado;
  resultado.reserve(1000);
  resultado += "{";
  resultado += parameterToJsonHalf("chipID", idDispositivo);
  resultado += parameterToJsonHalf("HumedadAire", floatTOstring(medicionHumedad));
  resultado += parameterToJsonHalf("NivelCO2", floatTOstring(medicionCO2));
  resultado += parameterToJsonHalf("TemperaturaAire", floatTOstring(medicionTemperaturaAire));
  resultado += parameterToJsonHalf("TemperaturaAguaTanquePrincipal", floatTOstring(medicionTemperaturaAgua));
  resultado += parameterToJsonHalf("MedicionPH", floatTOstring(medicionPH));
  resultado += parameterToJsonHalf("MedicionCE", floatTOstring(medicionCE));
  resultado += parameterToJsonHalf("NivelTanquePrincipal", intTOstring(medicionNivelTanquePrincial));
  resultado += parameterToJsonHalf("NivelTanqueLimpia", intTOstring(medicionNivelTanqueAguaLimpia));
  resultado += parameterToJsonHalf("NivelTanqueDescarte", intTOstring(medicionNivelTanqueDesechable));
  resultado += parameterToJsonHalf("NivelPhMas", intTOstring(medicionNivelPHmas));
  resultado += parameterToJsonHalf("NivelPhMenos", intTOstring(medicionNivelPHmenos));
  resultado += parameterToJsonHalf("NivelNutrienteA", intTOstring(medicionNivelNutrienteA));
  resultado += parameterToJsonHalf("NivelNutrienteB", intTOstring(medicionNivelNutrienteB));
  resultado += parameterToJsonHalf("Alertas", a);
  resultado += parameterToJsonHalf("Errores", e);
  resultado += "}";
  /*
    //crear Json
    StaticJsonBuffer<1000> jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();

    json["chipID"] = idDispositivo;
    json["HumedadAire"] = medicionHumedad;
    json["NivelCO2"] = medicionCO2;
    json["TemperaturaAire"] = medicionTemperaturaAire;
    json["TemperaturaAguaTanquePrincipal"] = medicionTemperaturaAgua;
    json["MedicionPH"] = medicionPH;
    json["MedicionCE"] = medicionCE;
    json["NivelTanquePrincipal"] = medicionNivelTanquePrincial;
    json["NivelTanqueLimpia"] = medicionNivelTanqueAguaLimpia;
    json["NivelTanqueDescarte"] = medicionNivelTanqueDesechable;
    json["NivelPhMas"] = medicionNivelPHmas;
    json["NivelPhMenos"] = medicionNivelPHmenos;
    json["NivelNutrienteA"] = medicionNivelNutrienteA;
    json["NivelNutrienteB"] = medicionNivelNutrienteB;

    String a(Alert);
    json["Alertas"] = a;

    String e(Error1);
    json["Errores"] = e;

    //  Serial.println();
    json.prettyPrintTo(Serial);
    //  String dato;
    //  json.printTo(dato);
    //
    //  char package1[513];
    //  char package[520];
    //  dato.toCharArray(package1, 513);
    //
    //  strcpy(package, preparePackage(package1, strlen(package1)));
    //  SERIAL_PRINT("package: ", package);
    //
    //  Serial1.println(package);
    String aux;
    json.printTo(aux);
    return aux;
  */
  return resultado;
}
//---------------------------------------------------------------------------------------------------------------//
String parameterToJsonHalf(String nombre, String valor)
{
  return "\"" + nombre + "\":\"" + valor + "\"";
}
//---------------------------------------------------------------------------------------------------------------//
String completarLargo(String x, int largo, int lado) // lado: 1 Derecha 2 Izquierda. ** Completar el largo con 000000
{
  //  SERIAL_PRINT("x: ", x);
  //  SERIAL_PRINT("length: ", x.length());

  if (x.length() < largo) {
    for (int i = 0 ; i <= largo - x.length(); i++)
      if (lado == 1)
        x = "0" + x;
      else
        x = x + "0";
  }
  //  SERIAL_PRINT("x2: ", x);
  return x;
}
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** FUNCIONES para el protocolo de paquetes entre ARDUINO y ESP8266 >
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** < FUNCIONES para FileSystem
//---------------------------------------------------------------------------------------------------------------//
bool loadConfig()
{
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile)
  {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024)
  {
    Serial.println("Config file size is too large");
    return false;
  }

  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<300> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());
  //json.prettyPrintTo(Serial);
  configPackage = "";
  json.printTo(configPackage);
  configFile.close();
  if (!json.success())
  {
    Serial.println("Failed to parse config file");
    return false;
  }

  //idDispositivo = json["idDispositivo"];
  Notificaciones = json["Notificaciones"];
  CantidadHorasLuz = json["CantidadHorasLuz"];
  HoraInicioLuz = json["HoraInicioLuz"];
  PH_aceptable = json["PH_aceptable"];

  SERIAL_PRINT("id=", idDispositivo);
  SERIAL_PRINT("N=", Notificaciones);
  SERIAL_PRINT("CHL=", CantidadHorasLuz);
  SERIAL_PRINT("HL=", HoraInicioLuz);
  SERIAL_PRINT("PH=", PH_aceptable);

  return true;
}
//---------------------------------------------------------------------------------------------------------------//
bool saveConfig() {
  StaticJsonBuffer<300> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(downloadConfig);

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile)
  {
    Serial.println("Failed to open config file for writing");
    return false;
  }
  Serial.println("");
  Serial.println("");
  //json.printTo(Serial);
  Serial.println("");
  Serial.println("");
  json.printTo(configFile);
  configFile.close();
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** FUNCIONES para FileSystem >
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** < FUNCIONES guardar SSID
//---------------------------------------------------------------------------------------------------------------//
String arregla_simbolos(String a) {
  a.replace("%C3%A1", "á");
  a.replace("%C3%A9", "é");
  a.replace("%C3%A", "i");
  a.replace("%C3%B3", "ó");
  a.replace("%C3%BA", "ú");
  a.replace("%21", "!");
  a.replace("%23", "#");
  a.replace("%24", "$");
  a.replace("%25", "%");
  a.replace("%26", "&");
  a.replace("%27", "/");
  a.replace("%28", "(");
  a.replace("%29", ")");
  a.replace("%3D", "=");
  a.replace("%3F", "?");
  a.replace("%27", "'");
  a.replace("%C2%BF", "¿");
  a.replace("%C2%A1", "¡");
  a.replace("%C3%B1", "ñ");
  a.replace("%C3%91", "Ñ");
  a.replace("+", " ");
  a.replace("%2B", "+");
  a.replace("%22", "\"");
  return a;
}

void wifi_conf() {
  int cuenta = 0;
  SERIAL_PRINT("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA ", COMANDO);
  String getssid = server.arg("ssid"); //Recibimos los valores que envia por GET el formulario web
  String getpass = server.arg("pass");

  getssid = arregla_simbolos(getssid); //Reemplazamos los simbolos que aparecen cun UTF8 por el simbolo correcto
  getpass = arregla_simbolos(getpass);

  ssid_tamano = getssid.length() + 1;  //Calculamos la cantidad de caracteres que tiene el ssid y la clave
  pass_tamano = getpass.length() + 1;

  getssid.toCharArray(ssid, ssid_tamano); //Transformamos el string en un char array ya que es lo que nos pide WIFI.begin()
  getpass.toCharArray(password, pass_tamano);

  Serial.println(ssid);     //para depuracion
  Serial.println(password);
  colorRGB(1);
  WiFi.begin(ssid, password);     //Intentamos conectar
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    cuenta++;
    if (cuenta > 20) {
      colorRGB(3);
      graba(70, "noconfigurado");
      server.send(200, "text/html", String("<h2>No se pudo realizar la conexion<br>no se guardaron los datos.</h2>"));
      return;
    }
  }
  Serial.print(WiFi.localIP());
  graba(70, "configurado");
  graba(1, getssid);
  graba(30, getpass);
  server.send(200, "text/html", String("<h2>Conexion exitosa a: "
                                       + getssid + "<br> El pass ingresado es: " + getpass + "<br>Datos correctamente guardados."));
  colorRGB(2);
}

void graba(int addr, String a) {
  SERIAL_PRINT("GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG ", COMANDO);
  int tamano = (a.length() + 1);
  Serial.print(tamano);
  char inchar[30];    //'30' Tamaño maximo del string
  a.toCharArray(inchar, tamano);
  EEPROM.write(addr, tamano);
  for (int i = 0; i < tamano; i++) {
    addr++;
    EEPROM.write(addr, inchar[i]);
  }
  EEPROM.commit();
}

String lee(int addr) {
  SERIAL_PRINT("LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL ", COMANDO);
  String nuevoString;
  int valor;
  int tamano = EEPROM.read(addr);
  for (int i = 0; i < tamano; i++) {
    addr++;
    valor = EEPROM.read(addr);
    nuevoString += (char)valor;
  }
  return nuevoString;
}

void intento_conexion() {
  SERIAL_PRINT("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB ", COMANDO);
  colorRGB(1);
  if (lee(70).equals("configurado")) {
    ssid_leido = lee(1);      //leemos ssid y password
    pass_leido = lee(30);

    Serial.println(ssid_leido);  //Para depuracion
    Serial.println(pass_leido);

    ssid_tamano = ssid_leido.length() + 1;  //Calculamos la cantidad de caracteres que tiene el ssid y la clave
    pass_tamano = pass_leido.length() + 1;

    ssid_leido.toCharArray(ssid, ssid_tamano); //Transf. el String en un char array ya que es lo que nos pide WiFi.begin()
    pass_leido.toCharArray(password, pass_tamano);

    int cuenta = 0;
    WiFi.begin(ssid, password);      //Intentamos conectar
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      cuenta++;
      if (cuenta > 20) {
        Serial.println("Fallo al conectar");
        return;
      }
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Conexion exitosa a: ");
    Serial.println(ssid);
    Serial.println(WiFi.localIP());
    colorRGB(2);
  }
}

void checkStatusWifi()
{
  if (WiFi.status() == WL_CONNECTED)
    colorRGB(2);
  else
    colorRGB(1);
}

void colorRGB(int color)//0 off, 1 Rojo, 2 Verde, 3 Amarillo
{
  switch ( color)
  {
    case 1:
      analogWrite(rled, 255); // Se enciende color rojo
      analogWrite(bled, 0); // Se apaga color azul
      analogWrite(gled, 0); // Se apaga colo verde
      break;
    case 2:
      analogWrite(rled, 0); // Se enciende color rojo
      analogWrite(bled, 0); // Se apaga color azul
      analogWrite(gled, 255); // Se apaga colo verde
      break;
    case 3:
      analogWrite(rled, 255); // Se enciende color amarillo
      analogWrite(gled, 255); // Mezclando r = 255 / g = 255 / b = 0
      analogWrite(bled, 0); // Se apaga colo verde
      break;
    default:
      analogWrite(rled, 0); // Se enciende color rojo
      analogWrite(bled, 0);  // Se apaga color azul
      analogWrite(gled, 0);  // Se apaga colo verde
  }
}
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** FUNCIONES guardar SSID >
//---------------------------------------------------------------------------------------------------------------//
