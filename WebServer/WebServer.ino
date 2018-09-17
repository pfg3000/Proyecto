#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <WiFiClientSecure.h>

#include <Time.h>
#include <Wire.h>
#include <TimeAlarms.h>

#include "FS.h"

#include "SPISlave.h"

#define DEBUG
//********************************** PROTOCOLO DE COMUNICACION *****************************
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
bool   packageComplete = false;

//-------------------VARIABLES GLOBALES--------------------------

bool CONFIGURAR_ALARMAS = true;

const char *ssid = "PolkoNet";
const char *password = "polkotite8308!";

unsigned long previousMillis = 0;

char host[99];
String strhost = "clientes.webbuilders.com.ar";
String strurl = "/testSmartZ.php"; // donde tengo la web
String chipid = ""; //identificador unico de la placa de m

String configPackage = "";
String downloadConfig = "";

int NotificacionesReal = 15;
int CantidadHorasLuzReal = 3;
float PH_aceptableReal = 7;
const char* idDispositivo;
int Notificaciones;
int CantidadHorasLuz;
int HoraInicioLuz;
float PH_aceptable;

AlarmId loggit;

void setup() {
  //********* Protocolo de comunicación.
  receivedPackage.reserve(200);

  int contconexion = 0;
  // Inicia Serial
  Serial.begin(9600);
  //Serial.setDebugOutput(true);

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

  if (!SPIFFS.begin())
  {
    Serial.println("Failed to mount file system");
    return;
  }

  if (!loadConfig())
  {
    Serial.println("Failed to load config");
  }
  else
  {
    Serial.println("Config loaded");
    //Serial.println(configPackage);
    delay(200);
    enviarPaquete();
  }

  //***************************************************************************

  SPISlave.onData([](uint8_t * data, size_t len) {
    String message = String((char *)data);
    (void) len;
    Serial.println(message);
    //    if (message.equals("Hello Slave!")) {
    //      SPISlave.setData("Hello Master!");
    //    } else if (message.equals("Are you alive?")) {
    //      char answer[33];
    //      sprintf(answer, "Alive for %lu seconds!", millis() / 1000);
    //      SPISlave.setData(answer);
    receivedPackage = message;
    if (checkPackageComplete()) {
      char cmd[] = "98";
      char payLoad[25];
      char package[32];
      snprintf(payLoad, sizeof(payLoad), "%s%c%s", cmd, (char)DELIMITER_CHARACTER, "OK");
      strcpy(package, preparePackage(payLoad, strlen(payLoad)));
      SPISlave.setData(package);

    } else {
      char cmd[] = "99";
      char payLoad[25];
      char package[32];

      snprintf(payLoad, sizeof(payLoad), "%s%c%s", cmd, (char)DELIMITER_CHARACTER, "ER");
      strcpy(package, preparePackage(payLoad, strlen(payLoad)));
      SPISlave.setData(package);
    }
    Serial.printf("Question: %s\n", (char *)data);
  });

  SPISlave.onDataSent([]() {
    Serial.println("Answer Sent");
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

  //SPISlave.setData("Ask me a question!");
}

//--------------------------LOOP--------------------------------
void loop() {
  //Serial.println("");
  if (CONFIGURAR_ALARMAS)
  {
    setTime(0, 0, 0, 1, 1, 2000);
    loggit = Alarm.timerRepeat(10, enviardatos);
    //Alarm.disable();
    CONFIGURAR_ALARMAS = false;
  }

  //digitalClockDisplay();

  //  if (SoftSerial.available()) {
  //    Serial.println("SIIIII");
  //    mySerialFunction();
  //  }
  //serialEvent();

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
        CONFIGURAR_ALARMAS = true;
        Alarm.disable(loggit);
      }
      if (!loadConfig())
      {
        Serial.println("Failed to load config");
      }
      delay(200);
      enviarPaquete();
    }
  }
  Alarm.delay(1000);
}

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

bool enviarPaquete()
{
  delay(100);
  char cmd[] = "01";
  char payLoad[25];
  char package[32];
  char aux[22];

  parameterToJson("HorasLuz", intTOstring(CantidadHorasLuz)).toCharArray(aux, 30);

  snprintf(payLoad, sizeof(payLoad), "%s%c%s", cmd, (char)DELIMITER_CHARACTER, aux);
  strcpy(package, preparePackage(payLoad, strlen(payLoad)));
  Serial.print(package);
  //SPISlave.setData(package);

  delay(100);

  strcpy(cmd, "02");
  strcpy(payLoad, "");
  strcpy(package, "");
  strcpy(aux, "");

  parameterToJson("HoraInicioLuz", intTOstring(HoraInicioLuz)).toCharArray(aux, 30);

  snprintf(payLoad, sizeof(payLoad), "%s%c%s", cmd, (char)DELIMITER_CHARACTER, aux);
  strcpy(package, preparePackage(payLoad, strlen(payLoad)));
  Serial.print(package);
  //SPISlave.setData(package);

  delay(100);

  strcpy(cmd, "03");
  strcpy(payLoad, "");
  strcpy(package, "");
  strcpy(aux, "");

  parameterToJson("pHaceptable", intTOstring(PH_aceptable)).toCharArray(aux, 30);

  snprintf(payLoad, sizeof(payLoad), "%s%c%s", cmd, (char)DELIMITER_CHARACTER, aux);
  strcpy(package, preparePackage(payLoad, strlen(payLoad)));
  Serial.print(package);
  //SPISlave.setData(package);

  return true;
}
/*void mySerialFunction() {
  static int state = 0;
  if (Serial.available()) {
    char inChar = (char)Serial.read();
    //Serial.println(inChar);
    if (inChar == START_CHARACTER && state == 0)
    {
      state = 1;
      receivedPackage = inChar;
    }

    else if (inChar == START_CHARACTER && state == 1)
    {
      state = 1;
      receivedPackage = inChar;
    }

    else if (inChar != END_CHARACTER && state == 1)
    {
      receivedPackage += inChar;
    }

    else if (inChar == END_CHARACTER && state == 1)
    {
      receivedPackage += inChar;
      state = 0;
      packageComplete = true;
      Serial.println("");
      Serial.println(receivedPackage);
      Serial.println("");
    }
  }
  }*/

void enviarMensaje() {
  int distancia = 0;
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
  //  Serial.println();
  //  json.prettyPrintTo(Serial);
  String dato;
  json.printTo(dato);
  //enviardatos("dato=" + dato);

  /*
    //  Pedir el Json del servidor
    if (WiFi.status() == WL_CONNECTED)
    {
      HTTPClient http;  //Object of class HTTPClient
      //http.begin("http://jsonplaceholder.typicode.com/users/1");
      http.begin("http://clientes.webbuilders.com.ar/testSmartZ.php");

      int httpCode = http.GET();
      //Check the returning code
      if (httpCode > 0) {
        // Get the request response payload
        String payload = http.getString();
        // TODO: Parsing
        Serial.print(payload);
      }
      http.end();   //Close connection
    }*/

}

//-------

void enviardatos()
{
  String linea;
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin("http://clientes.webbuilders.com.ar/testSmartZ.php?dato={\"idDispositivo\":\"2263874F-4820-4E31-9E37-58E77DD25494\",\"Notificaciones\":\"015\",\"CantidadHorasLuz\":\"02\",\"HoraInicioLuz\":\"21\",\"PH_aceptable\":\"07\"}");

    int httpCode = http.GET();
    if (httpCode > 0) {
      linea  = http.getString();
      //SERIAL_PRINT("linea=", linea);
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
    datos = "dato={\"idDispositivo\":\"2263874F-4820-4E31-9E37-58E77DD25494\",\"Notificaciones\":\"015\",\"CantidadHorasLuz\":\"02\",\"HoraInicioLuz\":\"21\",\"PH_aceptable\":\"07\"}";

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

  linea = "{\"idDispositivo\":\"2263874F-4820-4E31-9E37-58E77DD25494\",\"Notificaciones\":\"015\",\"CantidadHorasLuz\":\"02\",\"HoraInicioLuz\":\"21\",\"PH_aceptable\":\"07\"}";

  StaticJsonBuffer<500> jsonBuffer;
  char json[300];
  linea.toCharArray(json, 300);
  JsonObject& root = jsonBuffer.parseObject(json);
  idDispositivo = root["idDispositivo"];
  Notificaciones = root["Notificaciones"];
  CantidadHorasLuz = root["CantidadHorasLuz"];
  HoraInicioLuz = root["HoraInicioLuz"];
  PH_aceptable = root["PH_aceptable"];

  //  SERIAL_PRINT("id=", idDispositivo);
  //  SERIAL_PRINT("N=", Notificaciones);
  //  SERIAL_PRINT("CHL=", CantidadHorasLuz);
  //  SERIAL_PRINT("HL=", HoraInicioLuz);
  //  SERIAL_PRINT("PH=", PH_aceptable);

  downloadConfig = "";
  root.printTo(downloadConfig);


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
  */
  //return linea;
}

//-------------------------------------------------------------------------


void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println();
}

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
//void serialEvent() //Funcion que captura los eventos del puerto serial.
//{
//  static int state = 0;
//  if (Serial.available()) {
//
//    while (Serial.available()) {
//      char inChar = (char)Serial.read();
//
//      Serial.print(inChar);
//
//    }
//    Serial.println("pong");
//    /*
//      char inChar = (char)Serial.read();
//
//      if (inChar == START_CHARACTER && state == 0)
//      {
//      state = 1;
//      receivedPackage = inChar;
//      }
//
//      else if (inChar == START_CHARACTER && state == 1)
//      {
//      state = 1;
//      receivedPackage = inChar;
//      }
//
//      else if (inChar != END_CHARACTER && state == 1)
//      {
//      receivedPackage += inChar;
//      }
//
//      else if (inChar == END_CHARACTER && state == 1)
//      {
//      receivedPackage += inChar;
//      state = 0;
//      packageComplete = true;
//      Serial.print(receivedPackage);
//      }*/
//  }
//}

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
int proccesPackage(String package, int length)
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
    char data[50];
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
  //  if (packageComplete)
  //  {
  packageComplete = false;
  int retCmd = proccesPackage(receivedPackage, receivedPackage.length());

  if (retCmd == ERROR_CODE)
  {
    SERIAL_PRINT("> Error unknown command !!", "");
  }
  else if (retCmd == ERROR_BAD_CHECKSUM)
  {
    SERIAL_PRINT("> Error bad checksum!!", "");
  }
  else
  {
    /*      switch (retCmd) {
            case OPEN_DOOR:
              SERIAL_PRINT("> Command Open Door", "");
              openDoor = true;
              break;

            case CLOSE_DOOR:
              SERIAL_PRINT("> Command Close Door", "");
              closeDoor = true;
              break;

            case CARD_VALID:
              SERIAL_PRINT("> Command Card Valid", "");
              openDoor = true;
              break;

            case CARD_NOT_VALID:
              SERIAL_PRINT("> Command Card not Valid", "");
              push_Msg_inQueue(MSG_INVALID_CARD);
              refreshDisplay = true;
              break;

            case ACK_BUTTON_PRESSED:
              SERIAL_PRINT("> Command ACK button pressed", "");
              break;

            case ACK_OPEN_DOOR:
              SERIAL_PRINT(F("> Command ACK Open Door"), "");
              break;
          }*/
  }
  return true;
  //  }
  //  return false;
}
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** FUNCIONES para el protocolo de paquetes entre ARDUINO y ESP8266 >
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

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
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

  idDispositivo = json["idDispositivo"];
  Notificaciones = json["Notificaciones"];
  CantidadHorasLuz = json["CantidadHorasLuz"];
  HoraInicioLuz = json["HoraInicioLuz"];
  PH_aceptable = json["PH_aceptable"];

  //  SERIAL_PRINT("id=", idDispositivo);
  //  SERIAL_PRINT("N=", Notificaciones);
  //  SERIAL_PRINT("CHL=", CantidadHorasLuz);
  //  SERIAL_PRINT("HL=", HoraInicioLuz);
  //  SERIAL_PRINT("PH=", PH_aceptable);

  //  const char* serverName = json["serverName"];
  //  const char* accessToken = json["accessToken"];
  //
  //  // Real world application would store these values in some variables for
  //  // later use.
  //
  //  Serial.print("Loaded serverName: ");
  //  Serial.println(serverName);
  //  Serial.print("Loaded accessToken: ");
  //  Serial.println(accessToken);
  return true;
}

bool saveConfig() {
  StaticJsonBuffer<300> jsonBuffer;
  //  JsonObject& json = jsonBuffer.createObject();
  //  json["serverName"] = "Pdrrrrrm";
  //  json["accessToken"] = "128du9as8du12eoue8da98h123ueh9h98";
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
