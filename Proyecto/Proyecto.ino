//***************************** SmartZ - Arduino Mega 2560 *******************************
//***************************** SmartZ - Arduino Mega 2560 *******************************
//***************************** SmartZ - Arduino Mega 2560 *******************************

#include "DHT.h"               //Cargamos la librería DHT (temperatura y humedad)
#include <OneWire.h>           //(sensor de temperatura del agua)
#include <DallasTemperature.h> //(sensor de temperatura del agua)
#include "MQ135.h"             //Cargamos la librería MQ135 (sensor de calidad de aire)
#include <ArduinoJson.h>       //Para poder trabajar con estructuras jSon
#include <DS1307RTC.h>         //Reloj de tiempo real
#include <Time.h>              //Reloj de tiempo real
#include <Wire.h>              //Reloj de tiempo real
#include <TimeAlarms.h>        //Alarmas
#include <SPI.h>               //Protocolo d ecomunicacion

//********************************** << PROTOCOLO SPI *****************************
class ESPSafeMaster
{
private:
  uint8_t _ss_pin;
  void _pulseSS()
  {
    digitalWrite(_ss_pin, HIGH);
    delayMicroseconds(5);
    digitalWrite(_ss_pin, LOW);
  }

public:
  ESPSafeMaster(uint8_t pin) : _ss_pin(pin) {}
  void begin()
  {
    pinMode(_ss_pin, OUTPUT);
    _pulseSS();
  }

  uint32_t readStatus()
  {
    _pulseSS();
    SPI.transfer(0x04);
    uint32_t status = (SPI.transfer(0) | ((uint32_t)(SPI.transfer(0)) << 8) | ((uint32_t)(SPI.transfer(0)) << 16) | ((uint32_t)(SPI.transfer(0)) << 24));
    _pulseSS();
    return status;
  }

  void writeStatus(uint32_t status)
  {
    _pulseSS();
    SPI.transfer(0x01);
    SPI.transfer(status & 0xFF);
    SPI.transfer((status >> 8) & 0xFF);
    SPI.transfer((status >> 16) & 0xFF);
    SPI.transfer((status >> 24) & 0xFF);
    _pulseSS();
  }

  void readData(uint8_t *data)
  {
    _pulseSS();
    SPI.transfer(0x03);
    SPI.transfer(0x00);
    for (uint8_t i = 0; i < 32; i++)
    {
      data[i] = SPI.transfer(0);
    }
    _pulseSS();
  }

  void writeData(uint8_t *data, size_t len)
  {
    uint8_t i = 0;
    _pulseSS();
    SPI.transfer(0x02);
    SPI.transfer(0x00);
    while (len-- && i < 32)
    {
      SPI.transfer(data[i++]);
    }
    while (i++ < 32)
    {
      SPI.transfer(0);
    }
    _pulseSS();
  }

  String readData()
  {
    char data[33];
    data[32] = 0;
    readData((uint8_t *)data);
    return String(data);
  }

  void writeData(const char *data)
  {
    writeData((uint8_t *)data, strlen(data));
  }
};

ESPSafeMaster esp(SS);
//********************************** PROTOCOLO SPI >> *****************************

#define DEBUG
//********************************** PROTOCOLO DE COMUNICACION *****************************
#ifdef DEBUG
#define DELIMITER_CHARACTER '|'
#define START_CHARACTER '#'
#define END_CHARACTER '*'
#define SERIAL_PRINT(x, y) \
  Serial.print(x);         \
  Serial.println(y);
#endif

#ifndef DEBUG
#define DELIMITER_CHARACTER (int8_t)0xAE
#define START_CHARACTER (int8_t)0x91
#define END_CHARACTER (int8_t)0x92
#define SERIAL_PRINT(x, y) \
  Serial.print(x);         \
  Serial.println(y);
#endif

#define ERROR_CODE (-1)         //Valor de error
#define ERROR_BAD_CHECKSUM (-2) //Valor error de checksum

#define PACKAGE_FORMAT "%c%c%s%c%s%c%c" //Formato del paquete: start character + delimiter character + payLoad + delimiter character + checksum + delimiter character + end character.
#define SIZE_PAYLOAD 25                 //Tamaño del payload
#define SIZE_PACKAGE SIZE_PAYLOAD + 7   //Tamaño del paquete
#define SIZE_CHECKSUM 3                 //Tamaño del checksum

String receivedPackage = "";    //Donde recibo el paquete de la ESP.
bool enviarPaquete = true;      //Bandera para el envio de paquetes.
unsigned long timeoutPaquete;   //Timeout para el envio de paquetes.
int contadorRechazos = 0;       //Contador de paquetes rechazados en el envio a la ESP
#define RECHAZOS_MAXIMOS 15     //Rechazos maximos de paquetes enviados a la ESP
#define SEND_PACKAGE_TIME 30000 //Cada x tiempo se envian los paquetes a la ESP

//************************** Banderas de comandos y errores, variables varias *********************
#define apago HIGH
#define prendo LOW

bool CMD_SISTEMA_ENCENDIDO = true; //Bandera para iniciar el funcionamiento de SmartZ
int rled = 7;                      // Pin para led rojo
int gled = 6;                      // Pin para led verde

//Comandos
bool CMD_BAJAR_TEMPERATURA_AGUA = false;
bool CMD_SUBIR_TEMPERATURA_AGUA = false;
bool CMD_SUBIR_PH = false;
bool CMD_BAJAR_PH = false;
bool CMD_ADD_NUTRIENTE_A = false;
bool CMD_ADD_NUTRIENTE_B = false;
bool CMD_ADD_AGUA = false;
bool CMD_VACIAR_AGUA = false;
bool CMD_VENTILAR = false;
//Errores
bool ERR_MEDICION_TEMPERATURA_AGUA = false;
bool ERR_MEDICION_PH = false;
bool ERR_MEDICION_CE = false;
bool ERR_MEDICION_HUMEDAD = false;
bool ERR_MEDICION_TEMPERATURA = false;
bool ERR_MEDICION_CO2 = false;
bool ERR_MEDICION_NIVEL_PH_MAS = false;
bool ERR_MEDICION_NIVEL_PH_MEN = false;
bool ERR_MEDICION_NIVEL_NUT_A = false;
bool ERR_MEDICION_NIVEL_NUT_B = false;
bool ERR_MEDICION_NIVEL_PRINCIPAL = false;
bool ERR_INGRESO_AGUA_LIMPIA = false;
bool ERR_SALIDA_AGUA_DESCARTE = false;
bool ERR_ENVIO_INFORMACION = false;
//Alertas
bool ALT_MINIMO_PH_MAS = false;
bool ALT_MINIMO_PH_MEN = false;
bool ALT_MINIMO_NUT_A = false;
bool ALT_MINIMO_NUT_B = false;
bool ALT_MINIMO_PRINCIPAL = false;
bool ALT_MAXIMO_PRINCIPAL = false;
bool ALT_RTC_DESCONFIGURADO = false;
bool ALT_RTC_DESCONECTADO = false;
bool ALT_VACIADO_REALIZADO = false;
bool ALT_LLENADO_REALIZADO = false;

unsigned long timeoutRecirculacion = 0; //Timeout para la recirculacion de agua entre ajustes de ph o nutrientes.
#define RECIRCULACION_TIME 15000    //Cada x tiempo se vuelven a realizar los analisis de agua / aire.
bool RECIRCULACION = false;

#define CANTIDAD_INTENTOS 5          //Cantidad de intentos de vaciado o llenado de agua por segundo.
#define CANTIDAD_MEDICIONES 10       //Cantidad de mediciones de nivel (para aminorar el error)
#define TIEMPO_GOTEO_NUTRIENTES 4000 //4 segundos x cada 5 ml -> tirara 30 ml entre los 2 nutrientes
#define TIEMPO_GOTEO_PH 2000         //1 segundos x cada 1 ml aproximado
//#define TIEMPO_BOMBA_AGUA 10         //segundos x cada litro
#define TIEMPO_LECTURA_NIVEL 5000 //segundos de lectura de nivel en llenado / vaciado
//#define CM_POR_LITRO 2               //cm x cada litro

#define TIEMPO_VENTILACION 6000 //Tiempo en el que se ejecuta el timer de apagado de ventilador.

//Indicadores de dispositivos en funcionamiento
bool VENTILADOR_FUNCIONANDO = false;
bool BOMBA_FUNCIONANDO = false;
bool ENFRIADOR_FUNCIONANDO = false;
bool CALENTADOR_FUNCIONANDO = false;

bool CONFIGURAR_ALARMAS = true; //Para setear la configuracion de las alarmas (Luces)
//Alarmas de encendido y apagado de luces.
AlarmId idAlarmON;  //ID de la alarma de encendido de las luces.
AlarmId idAlarmOFF; //ID de la alarma de apagado de las luces.

//************************** pH del agua *****************************************
#define pinPH A1 //Seleccionamos el pin que usara el sensor de pH.

//************************** Conductividad Electrica del agua *****************************************
#define pinCE A2 //Seleccionamos el pin que usara el sensor de conductividad electrica.

//************************** CO2 *****************************************
#define pinCO2 A0 //Seleccionamos el pin que usara el sensor de MQ135.
MQ135 gasSensor = MQ135(pinCO2);

//************************* Niveles de liquido *********************************************
#define pinNivel1 24 //Seleccionamos el pin en el que se conectará el sensor de nivel de nutrientes A. T
#define pinNivel2 25 //Seleccionamos el pin en el que se conectará el sensor de nivel de nutrientes A. E

#define pinNivel3 26 //Seleccionamos el pin en el que se conectará el sensor de nivel de nutrientes B. T
#define pinNivel4 27 //Seleccionamos el pin en el que se conectará el sensor de nivel de nutrientes B. E

//#define pinNivel5 29 //Seleccionamos el pin en el que se conectará el sensor de nivel de pH+.
#define pinNivel6 A15 //Seleccionamos el pin en el que se conectará el sensor de nivel de pH+.

//#define pinNivel7 31 //Seleccionamos el pin en el que se conectará el sensor de nivel de pH-.
#define pinNivel8 A14 //Seleccionamos el pin en el que se conectará el sensor de nivel de pH-.

//************************* Niveles de agua *********************************************
//#define pinNivel9 32 //Seleccionamos el pin en el que se conectará el sensor de nivel de agua limpia.

#define pinNivel10 33 //Seleccionamos el pin en el que se conectará el sensor de nivel tanque vacio.
#define pinNivel11 34 //Seleccionamos el pin en el que se conectará el sensor de nivel tanque minimo.
#define pinNivel12 35 //Seleccionamos el pin en el que se conectará el sensor de nivel tanque lleno.

#define pinNivel13 36 //Seleccionamos el pin en el que se conectará el sensor de nivel de tanque principal. T
#define pinNivel14 37 //Seleccionamos el pin en el que se conectará el sensor de nivel de tanque principal. E

//************************** Temperatura y humedad del aire ********************************
#define DHTTYPE DHT22     //Se selecciona el tipo de sensor medidor de temperatura DHT22.
#define pinDHT 22         //Seleccionamos el pin en el que se conectará el sensor DHT22.
DHT dht(pinDHT, DHTTYPE); //Se inicia una variable que será usada por Arduino para comunicarse con el sensor.

//*********************************** ACTUADORES ***************************************** 16 relé
#define BombaEnfriadoPinRELE1 40  //Seleccionamos el pin en el que se conectará Bomba de agua Enfriado.
#define BombaLlenadoPinRELE2 41   //Seleccionamos el pin en el que se conectará Valvula de Llenado.
#define BombaPrincipalPinRELE3 42 //Seleccionamos el pin en el que se conectará Bomba de agua Principal.
#define BombaVaciadoPinRELE4 43   //Seleccionamos el pin en el que se conectará Bomba de agua Vaciado.
#define NutrienteApinRELE5 44     //Seleccionamos el pin en el que se conectará Bomba peristáltica Nutriente A.
#define NutrienteBpinRELE6 45     //Seleccionamos el pin en el que se conectará Bomba peristáltica Nutriente B.
#define pHMasPinRELE7 46          //Seleccionamos el pin en el que se conectará Bomba peristáltica pH+.
#define pHmenosPinRELE8 47        //Seleccionamos el pin en el que se conectará Bomba peristáltica pH-.
#define LucesPinRELE9 48          //Seleccionamos el pin en el que se conectará Luces Led.
#define VentiladoresPinRELE10 49  //Seleccionamos el pin en el que se conectará Ventiladores.
#define CalentadorPinRELE11 32    //Seleccionamos el pin en el que se conectará Calentador.
#define PHpinRELE12 38            //Seleccionamos el pin en el que se conectará el sensor de ph.
#define CEpinRELE13 39            //Seleccionamos el pin en el que se conectará el sensor de ce.
#define PeltierpinRELE14 30       //Seleccionamos el pin en el que se conectará el peltier.
#define BombaVaciadoPinRELE15 31  //Seleccionamos el pin en el que se conectará la Bomba de agua de vaciado.
#define pinRELE16 5               //Seleccionamos el pin en el que se conectará reset de NODEMCU.

//******************************************************************* Temperatura del agua
const int pinDS18B20 = 23;                       //Seleccionamos el pin en el que se conectará el sensor DS18B20.
OneWire oneWireObjeto(pinDS18B20);               //Inicializamos la clase.
DallasTemperature sensorDS18B20(&oneWireObjeto); //Inicializamos la clase.

//*********************************************************************************** < VARIABLES PARA MEDICIONES
//Variables para Luz
int hsLuzParametroOriginal = 0;
int hsLuzParametro = 0;
int horaInicioLuzOriginal = -1;
int horaInicioLuz = 0;
bool lucesEncendidas = false;

//Variables Medición de PH
float medicionPH = 0.0; //Valor medido
float PhParametro = 7.0;
float PHmaxParametro = 8.0;
float PHminParametro = 6.0;
float medicionNivelPHmas = 0.0;   //Valor medido
float medicionNivelPHmenos = 0.0; //Valor medido
float minimoNivelPHmas = 500.0;
float minimoNivelPHmenos = 500.0;

//Variables para medición de humedad del aire
float medicionHumedad = 0.0; //Valor medido
float humedadMaxParametro = 0.0;
float humedadMinParametro = 0.0;

//Variables para medición de temperatura del aire
float medicionTemperaturaAire = 0.0; //Valor medido
float temperaturaAireMaxParametro = 0.0;
float temperaturaAireMinParametro = 0.0;

//Variables para medición de temperatura del agua
float medicionTemperaturaAgua = 0.0; //Valor medido
float temperaturaAguaMaxParametro = 0.0;
float temperaturaAguaMinParametro = 0.0;

//Variables para la medición de co2
float medicionCO2 = 0.0; //Valor medido
float co2MaxParametro = 0.0;
float co2MinParametro = 0.0;

//Variables para la medición de ce
float medicionNivelNutrienteA = 0.0; //Valor medido
float medicionNivelNutrienteB = 0.0; //Valor medido
float medicionCE = 0.0;              //Valor medido
float ceMaxParametro = 0.0;
float ceMinParametro = 0.0;
float maximoNivelNutrienteA = 0.0;
float maximoNivelNutrienteB = 0.0;
float minimoNivelNutrienteA = 0.0;
float minimoNivelNutrienteB = 0.0;
float pisoTanqueNutrienteA = 0.0;
float pisoTanqueNutrienteB = 0.0;

//Variables para la medición de los niveles de los tanques de agua
float medicionNivelTanquePrincial = 0.0; //Valor medido
float minimoNivelTanquePrincial = 0.0;
float maximoNivelTanquePrincial = 0.0;
bool tanqueLleno = false; //Flotante de tanque lleno.
bool tanqueMedio = false; //Flotante de tanque medio.
bool tanqueVacio = true;  //Flotante de tanque vacio.
float pisoTanqueAguaPrincipal = 0.0;
//***********************************************************************************  VARIABLES PARA MEDICIONES >

void setup()
{
  Serial.begin(9600); //Se inicia la comunicación serial

  dht.begin();           //Se inicia el sensor DHT22.
  sensorDS18B20.begin(); //Se inicia el sensor DS18B20.

  //********* LED
  pinMode(rled, OUTPUT); //led rojo.
  pinMode(gled, OUTPUT); //led verde.
  //********* SENSORES
  pinMode(pinNivel1, OUTPUT); //nivel de nutrientes A.
  pinMode(pinNivel2, INPUT);  //nivel de nutrientes A.
  pinMode(pinNivel3, OUTPUT); //nivel de nutrientes B.
  pinMode(pinNivel4, INPUT);  //nivel de nutrientes B.
  //pinMode(pinNivel5, OUTPUT); //nivel de ph+.
  pinMode(pinNivel6, INPUT); //nivel de ph+.
  //pinMode(pinNivel7, OUTPUT); //nivel de ph-.
  pinMode(pinNivel8, INPUT); //nivel de ph-.
  // pinMode(pinNivel9, OUTPUT); //agua limpia.
  pinMode(pinNivel10, INPUT);  //Tanque vacio.
  pinMode(pinNivel11, INPUT);  //Tanque minimo.
  pinMode(pinNivel12, INPUT);  //Tanque lleno.
  pinMode(pinNivel13, OUTPUT); //tanque principal.
  pinMode(pinNivel14, INPUT);  //tanque principal.

  pinMode(pinPH, INPUT);      //ph.
  pinMode(pinCE, INPUT);      //ce.
  pinMode(pinCO2, INPUT);     //co2.
  pinMode(pinDHT, INPUT);     //temperatura y humedad del aire.
  pinMode(pinDS18B20, INPUT); //temperatura del agua.

  //********* ACTUADORES
  pinMode(BombaEnfriadoPinRELE1, OUTPUT);  //Bomba de agua Enfriado.
  pinMode(BombaLlenadoPinRELE2, OUTPUT);   //Bomba de agua Llenado.
  pinMode(BombaPrincipalPinRELE3, OUTPUT); //Bomba de agua Principal.
  pinMode(BombaVaciadoPinRELE4, OUTPUT);   //Valvula de agua Vaciado.
  pinMode(NutrienteApinRELE5, OUTPUT);     //Bomba peristáltica Nutriente A.
  pinMode(NutrienteBpinRELE6, OUTPUT);     //Bomba peristáltica Nutriente B.
  pinMode(pHMasPinRELE7, OUTPUT);          //Bomba peristáltica pH+.
  pinMode(pHmenosPinRELE8, OUTPUT);        //Bomba peristáltica pH-.
  pinMode(LucesPinRELE9, OUTPUT);          //Luces Led.
  pinMode(VentiladoresPinRELE10, OUTPUT);  //Ventiladores.
  pinMode(CalentadorPinRELE11, OUTPUT);    //Calentador.
  pinMode(PHpinRELE12, OUTPUT);            // **pH.
  pinMode(CEpinRELE13, OUTPUT);            // **CE.
  pinMode(PeltierpinRELE14, OUTPUT);       // **Sin Asignar.
  pinMode(BombaVaciadoPinRELE15, OUTPUT);  //Bomba de agua Vaciado.
  pinMode(pinRELE16, OUTPUT);              //Reset WiFi.

  apagarTodo(true, 0);
  SERIAL_PRINT("INICIO SETUP", "");
  //********* Protocolo de comunicación.
  receivedPackage.reserve(200);

  SPI.begin();
  //  SPI.setDataMode(SPI_MODE3);
  //  SPI.setClockDivider(SPI_CLOCK_DIV16);
  //  SPI.setBitOrder(MSBFIRST);

  esp.begin();
}

void loop()
{
  if (ERR_ENVIO_INFORMACION)
  { //Si hubo error en el envio de paquetes parpadea el led rojo.
    colorRGB(0);
    delay(500);
    colorRGB(1);
    delay(500);
    colorRGB(0);
    delay(500);
    colorRGB(1);
    delay(500);
    colorRGB(0);
  }

  if (CMD_SISTEMA_ENCENDIDO)
  { //Si el sistema esta encendido se activa led verde.
    colorRGB(2);
  }
  else
  { //Si el sistema no esta encendido se activa led rojo.
    colorRGB(1);
  }

  // Tomar parametros del cultivo.
  // Tomamos el parametro de horaInicioLuz y de hsLuzParametro
  // seteamos el reloj con la hora del reloj RTC
  // luego seteamos la alarma inicial y la alarma final para encender y apagar las luces.
  if (enviarPaquete) //Envio los paquetes a la ESP
  {
    timeoutPaquete = millis();
    enviarPaquete = false;
    enviarInformacion();
  }
  if (millis() - timeoutPaquete > SEND_PACKAGE_TIME)
  {
    Serial.println("GENERANDO JSON");
    enviarInformacion();
    generarJson();
    enviarPaquete = true;
  }

  SERIAL_PRINT("hsLuzParametro: ", hsLuzParametro);
  SERIAL_PRINT("horaInicioLuz: ", horaInicioLuz);
  SERIAL_PRINT("PhParametro: ", PhParametro);
  SERIAL_PRINT("Encendido: ", CMD_SISTEMA_ENCENDIDO);

  humedadMaxParametro = 60.0;         //%
  humedadMinParametro = 40.0;         //%
  temperaturaAireMaxParametro = 30.0; //ºc
  temperaturaAireMinParametro = 15.0; //ºc
  co2MaxParametro = 600.0;            //ppm
  co2MinParametro = 300.0;            //ppm

  temperaturaAguaMaxParametro = 21.0; //ºc
  temperaturaAguaMinParametro = 18.0; //ºc
  PHmaxParametro = PhParametro * 1.05;
  PHminParametro = PhParametro * 0.95;
  ceMaxParametro = 0.8;
  ceMinParametro = 0.4;

  maximoNivelNutrienteA = 3;
  maximoNivelNutrienteB = 3;
  minimoNivelNutrienteA = 10;
  minimoNivelNutrienteB = 10;
  pisoTanqueNutrienteA = 13.0;
  pisoTanqueNutrienteB = 13.0;

  minimoNivelTanquePrincial = 14.0;
  maximoNivelTanquePrincial = 5.0;
  pisoTanqueAguaPrincipal = 18.0;

 /* if ((hsLuzParametro != hsLuzParametroOriginal && hsLuzParametroOriginal != 0) || (horaInicioLuz != horaInicioLuzOriginal && horaInicioLuzOriginal != -1))
  { //Si vino de la ESP algun cambio de valores, reconfiguro las alarmas.
    hsLuzParametroOriginal = hsLuzParametro;
    horaInicioLuzOriginal = horaInicioLuz;
    CONFIGURAR_ALARMAS = true;
    Alarm.disable(idAlarmON);
    Alarm.disable(idAlarmOFF);
  }
*/
  //if (CONFIGURAR_ALARMAS)
 // { //Realizo la configuracion de las alarmas.
    tmElements_t datetime;
    if (RTC.read(datetime) == true)
    {
      //tomo la hora del RTC
      setTime(datetime.Hour, datetime.Minute, datetime.Second, datetime.Month, datetime.Day, datetime.Year);
      SERIAL_PRINT("Hora Actual", datetime.Hour);
      SERIAL_PRINT("Hora inicio luz", horaInicioLuz);
      SERIAL_PRINT("Cantidad de horas luz", hsLuzParametro);
      // Creo las alarmas
      if (hsLuzParametro > 0)
      {
        //idAlarmON = Alarm.alarmRepeat(horaInicioLuz, 0, 0, encenderLuces);
        //idAlarmOFF = Alarm.alarmRepeat(ajustarHoras(horaInicioLuz + hsLuzParametro), 0, 0, apagarLuces);
        //chequeo hora actual y hora parametro
        if(lucesEncendidas && datetime.Hour == ajustarHoras(horaInicioLuz + hsLuzParametro)){
          SERIAL_PRINT("APAGO LUCES", 0);
          apagarLuces();
          lucesEncendidas = false;
        }else
        {
          if(!lucesEncendidas && datetime.Hour == horaInicioLuz ){
            SERIAL_PRINT("ENCIENDO LUCES", 0);
            encenderLuces();
            lucesEncendidas = true;
          }
        }
      }else{
        if(lucesEncendidas){
          apagarLuces();
        }
          lucesEncendidas = false;
      }
      //CONFIGURAR_ALARMAS = false;
    }
    else
    {
      if (RTC.chipPresent())
      {
        // El reloj esta detenido, es necesario ponerlo a tiempo
        ALT_RTC_DESCONFIGURADO = true;
        generarAlerta("ALT_RTC_DESCONFIGURADO");
      }
      else
      {
        // No se puede comunicar con el RTC en el bus I2C, revisar las conexiones.
        ALT_RTC_DESCONECTADO = true;
        generarAlerta("ALT_RTC_DESCONECTADO");
      }
    }
    // CONFIGURAR_ALARMAS = false;
  //}
  digitalClockDisplay();

  if (!controlarNivelesPH())
  {
    //Algo falló. Alertar
    if (ERR_MEDICION_NIVEL_PH_MAS == true)
    {
      generarAlerta("ERR_MEDICION_NIVEL_PH_MAS");
    }
    if (ERR_MEDICION_NIVEL_PH_MEN == true)
    {
      generarAlerta("ERR_MEDICION_NIVEL_PH_MEN");
    }
  }
  else
  {
    //Alerto si hubo nivel bajo.
    if (ALT_MINIMO_PH_MAS == true)
    {
      generarAlerta("ALT_MINIMO_PH_MAS");
    }
    if (ALT_MINIMO_PH_MEN == true)
    {
      generarAlerta("ALT_MINIMO_PH_MEN");
    }
  }

  if (!controlarNivelesNutrintes())
  {
    //Algo falló. Alertar
    if (ERR_MEDICION_NIVEL_NUT_A == true)
    {
      generarAlerta("ERR_MEDICION_NIVEL_NUT_A");
    }
    if (ERR_MEDICION_NIVEL_NUT_B == true)
    {
      generarAlerta("ERR_MEDICION_NIVEL_NUT_B");
    }
  }
  else
  {
    //Alerto si hubo nivel bajo.
    if (ALT_MINIMO_NUT_A == true)
    {
      generarAlerta("ALT_MINIMO_NUT_A");
    }
    if (ALT_MINIMO_NUT_B == true)
    {
      generarAlerta("ALT_MINIMO_NUT_B");
    }
  }

  if (!controlarNivelesTanques())
  {
    //Algo falló. Alertar
    if (ERR_MEDICION_NIVEL_PRINCIPAL == true)
    {
      generarAlerta("ERR_MEDICION_NIVEL_PRINCIPAL");
    }
  }
  else
  {
    //Alerto si hubo nivel bajo o alto.
    if (ALT_MINIMO_PRINCIPAL == true)
    {
      generarAlerta("ALT_MINIMO_PRINCIPAL");
    }
    if (ALT_MAXIMO_PRINCIPAL == true)
    {
      generarAlerta("ALT_MAXIMO_PRINCIPAL");
    }
    if (maximoNivelTanquePrincial < medicionNivelTanquePrincial && !CMD_VACIAR_AGUA)
    { //Solo agrego agua limpia.
      SERIAL_PRINT("BAJO NIVEL TANQUE PRINCIPAL ", "1");
      apagarTodo(false, 5000);

      if (!llenarTanque(false))
      {
        //Algo falló. Alertar
        if (ERR_INGRESO_AGUA_LIMPIA == true)
        {
          generarAlerta("ERR_INGRESO_AGUA_LIMPIA");
        }
      }
      else
      {
        if (ALT_LLENADO_REALIZADO == true)
        {
          generarAlerta("ALT_LLENADO_REALIZADO");
          encenderBombaPrincipal();
        }
      }
    }
  }

  /******************** VACIAR TANQUE *****************************/
  if (CMD_VACIAR_AGUA && !tanqueVacio)
    {
      CMD_VACIAR_AGUA = false;
      if (!vaciarTanque())
      {
        //Algo falló. Alertar
        if (ERR_SALIDA_AGUA_DESCARTE == true)
        {
          generarAlerta("ERR_SALIDA_AGUA_DESCARTE");
        }
      }
      else
      {
        generarAlerta("ALT_VACIADO_REALIZADO");
        CMD_SISTEMA_ENCENDIDO = false;
      }
    }
  /********************* SISTEMA ENCENDIDO *************************/

  if (CMD_SISTEMA_ENCENDIDO)
  {

    if (RECIRCULACION) //Activo el modo de recirculacion de agua hasta la proxima medicion.
    {
      if(millis() - timeoutRecirculacion > RECIRCULACION_TIME)
      {
        RECIRCULACION = false;
      }
      
    }else{
      timeoutRecirculacion = millis();
      RECIRCULACION = true;
    }
    
    //CMD_VACIAR_AGUA = true;
    //tanqueVacio = false;

    if (tanqueVacio) //
    {
      apagarTodo(true, 0);
      SERIAL_PRINT("TANQUE VACIO", "");

      if (!llenarTanque(true))
      {
        //Algo falló. Alertar
        if (ERR_INGRESO_AGUA_LIMPIA == true)
        {
          generarAlerta("ERR_INGRESO_AGUA_LIMPIA");
        }
        if (ERR_MEDICION_NIVEL_PRINCIPAL == true)
        {
          generarAlerta("ERR_MEDICION_NIVEL_PRINCIPAL");
        }
      }
      else
      {
        if (ALT_LLENADO_REALIZADO == true)
        {
          generarAlerta("ALT_LLENADO_REALIZADO");
          encenderBombaPrincipal();
        }
      }
    }

    if (!RECIRCULACION)
    {
      if (!analizarAgua())
      {
        //Algo falló. Alertar
        if (ERR_MEDICION_TEMPERATURA_AGUA == true)
        {
          generarAlerta("ERR_MEDICION_TEMPERATURA_AGUA");
        }
        if (ERR_MEDICION_PH == true)
        {
          generarAlerta("ERR_MEDICION_PH");
        }
        if (ERR_MEDICION_CE == true)
        {
          generarAlerta("ERR_MEDICION_CE");
        }
      }
      else
      {
        if (CMD_BAJAR_TEMPERATURA_AGUA)
        {
          generarAlerta("CMD_BAJAR_TEMPERATURA_AGUA");
          encenderEnfriador();
          apagarCalentador();
        }
        if (CMD_SUBIR_TEMPERATURA_AGUA)
        {
          generarAlerta("CMD_SUBIR_TEMPERATURA_AGUA");
          apagarEnfriador();
          encenderCalentador();
        }
        if (!CMD_BAJAR_TEMPERATURA_AGUA && !CMD_SUBIR_TEMPERATURA_AGUA)
        {
          generarAlerta("!CMD_BAJAR_TEMPERATURA_AGUA && !CMD_SUBIR_TEMPERATURA_AGUA");
          apagarCalentador();
          apagarEnfriador();
        }

        if (CMD_SUBIR_PH)
        {
          generarAlerta("CMD_SUBIR_PH_IN");
          encenderPHmas();
          delay(TIEMPO_GOTEO_PH);
          generarAlerta("CMD_SUBIR_PH_OUT");
          apagarPHmas();
          //apagarPHmenos();
          //RECIRCULACION = true;
        }

        if (CMD_BAJAR_PH)
        {
          generarAlerta("CMD_BAJAR_PH_IN");
          encenderPHmenos();
          delay(TIEMPO_GOTEO_PH);
          generarAlerta("CMD_BAJAR_PH_OUT");
          apagarPHmenos();
          //apagarPHmas();
          //RECIRCULACION = true;
        }

        if (!CMD_SUBIR_PH && !CMD_BAJAR_PH)
        {
          generarAlerta("!CMD_SUBIR_PH && !CMD_BAJAR_PH");
          apagarPHmas();
          apagarPHmenos();
        }

        if ((CMD_ADD_NUTRIENTE_A || CMD_ADD_NUTRIENTE_B) && (!ALT_MINIMO_NUT_A && !ALT_MINIMO_NUT_B)) // Si hay q agregar A o B y ambos tienen nivel ok
        {
          generarAlerta("CMD_ADD_NUTRIENTE_A");
          generarAlerta("CMD_ADD_NUTRIENTE_B");
          SERIAL_PRINT("Agregar Nutrientes", 0);
          encenderNutrientesA();
          encenderNutrientesB();
          SERIAL_PRINT("BOMBAS Nutrientes ENCENDIDAS", 0);
          delay(TIEMPO_GOTEO_NUTRIENTES);
          apagarBombaNutrientesA();
          apagarBombaNutrientesB();
          SERIAL_PRINT("BOMBAS Nutrientes APAGADAS", 0);
          //RECIRCULACION = true;
        }
        SERIAL_PRINT("CHEQUEO NUTRIENTES TACHOS", 0);
        if (!CMD_ADD_NUTRIENTE_A && !CMD_ADD_NUTRIENTE_B)
        {
          generarAlerta("!CMD_ADD_NUTRIENTE_A && !CMD_ADD_NUTRIENTE_B");
          apagarBombaNutrientesA();
          apagarBombaNutrientesB();
        }

        SERIAL_PRINT("AGREGAR AGUA??? ", CMD_ADD_AGUA);
        if (CMD_ADD_AGUA)
        {
          generarAlerta("CMD_ADD_AGUA");
          apagarTodo(false, 5000);

          //Vaciamos un poco de agua por concentracion de nutrientes.
          encenderBombaVaciado();
          delay(15000);
          apagarBombaVaciado();

          if (!llenarTanque(false))
          {
            //Algo falló. Alertar
            if (ERR_INGRESO_AGUA_LIMPIA == true)
            {
              generarAlerta("ERR_INGRESO_AGUA_LIMPIA");
            }
          }
          else
          {
            if (ALT_LLENADO_REALIZADO == true)
            {
              generarAlerta("ALT_LLENADO_REALIZADO");
              encenderBombaPrincipal();
            }
          }
          //RECIRCULACION = true;
        }
      }
    }

    if (!analizarAire())
    {
      //Algo falló. Alertar
      if (ERR_MEDICION_HUMEDAD == true)
      {
        generarAlerta("ERR_MEDICION_HUMEDAD");
      }
      if (ERR_MEDICION_TEMPERATURA == true)
      {
        generarAlerta("ERR_MEDICION_TEMPERATURA");
      }
      if (ERR_MEDICION_CO2 == true)
      {
        generarAlerta("ERR_MEDICION_CO2");
      }
    }
    else
    {
      if (CMD_VENTILAR) //Si es necesario ventilar, lo enciendo, luego seteo una alarma para que se apague.
      {
        if (!VENTILADOR_FUNCIONANDO)
        {
          generarAlerta("CMD_VENTILAR");
          encenderVentiladores();
          Alarm.timerOnce(TIEMPO_VENTILACION, apagarVentiladores);
        }
      }
    }
  }
  else
  { //apago todo
    apagarTodo(true, 0);
    SERIAL_PRINT("SISTEMA APAGADO", "");
  }

  //Se imprimen las variables
  Serial.print("Humedad: ");
  Serial.print(medicionHumedad);
  Serial.println(" %");

  Serial.print("Temperatura: ");
  Serial.print(medicionTemperaturaAire);
  Serial.println(" ºC");

  Serial.print("Nutrientes A: ");
  Serial.println(medicionNivelNutrienteA);
  Serial.print("Nutrientes B: ");
  Serial.println(medicionNivelNutrienteB);

  Serial.print("pH+: ");
  Serial.println(medicionNivelPHmas);
  Serial.print("pH-: ");
  Serial.println(medicionNivelPHmenos);

  Serial.print("Temperatura agua: ");
  Serial.print(medicionTemperaturaAgua);
  Serial.println(" ºC");

  Serial.print("PH: ");
  Serial.println(medicionPH, 3);

  // Serial.print("Tanque de agua limpia ");
  // Serial.println(medicionNivelTanqueAguaLimpia);
  // Serial.print("Tanque de agua descartada ");
  // Serial.println(medicionNivelTanqueDesechable);
  Serial.print("Tanque de agua principal ");
  Serial.println(medicionNivelTanquePrincial);

  Serial.print("CO2: ");
  Serial.print(medicionCO2);
  Serial.println(" ppm");

  //    Serial.println("sensor=");
  //    Serial.print(sensorValue);
  Serial.print("CE: ");
  Serial.println(medicionCE);

  Serial.println("");

  //  Serial.println("");
  //  Serial.println("");

  Alarm.delay(1000); //delay(2000); //Se espera 2 segundos para seguir leyendo //datos
}

//************************************************************** FUNCIONES ************************************************************** FUNCIONES
//************************************************************** FUNCIONES ************************************************************** FUNCIONES
//************************************************************** FUNCIONES ************************************************************** FUNCIONES
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** < FUNCIONES de encendido y apagado de dispositivos
//---------------------------------------------------------------------------------------------------------------//
bool resetWifi()
{
  digitalWrite(pinRELE16, prendo);
  delay(1000);
  digitalWrite(pinRELE16, apago);
}
//---------------------------------------------------------------------------------------------------------------//
bool apagarTodo(bool todo, int timeDelay)
{
  apagarBombaNutrientesA();
  apagarBombaNutrientesB();
  apagarBombaPrincipal();
  apagarBombaVaciado();
  apagarCE();
  apagarLlenado();
  apagarPH();
  apagarPHmas();
  apagarPHmenos();

  if (todo == true)
  {
    apagarCalentador();
    apagarLuces();
    apagarEnfriador();
    apagarVentiladores();
  }

  if (timeDelay != 0)
    delay(timeDelay);

  SERIAL_PRINT("APAGAR TODO ", todo);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//BombaEnfriadoPinRELE1
bool encenderEnfriador()
{
  ENFRIADOR_FUNCIONANDO = true;
  digitalWrite(BombaEnfriadoPinRELE1, prendo);
  digitalWrite(PeltierpinRELE14, prendo);
  return true;
}
bool apagarEnfriador()
{
  ENFRIADOR_FUNCIONANDO = false;
  digitalWrite(BombaEnfriadoPinRELE1, apago);
  digitalWrite(PeltierpinRELE14, apago);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//BombaLlenadoPinRELE2
bool encenderLlenado()
{
  digitalWrite(BombaLlenadoPinRELE2, prendo);
  return true;
}
bool apagarLlenado()
{
  digitalWrite(BombaLlenadoPinRELE2, apago);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//BombaPrincipalPinRELE3
bool encenderBombaPrincipal()
{
  BOMBA_FUNCIONANDO = true;
  digitalWrite(BombaPrincipalPinRELE3, prendo);
  return true;
}
bool apagarBombaPrincipal()
{
  BOMBA_FUNCIONANDO = false;
  digitalWrite(BombaPrincipalPinRELE3, apago);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//BombaVaciadoPinRELE4
bool encenderBombaVaciado()
{
  digitalWrite(BombaVaciadoPinRELE4, prendo);
  digitalWrite(BombaVaciadoPinRELE15, prendo);
  return true;
}
bool apagarBombaVaciado()
{
  digitalWrite(BombaVaciadoPinRELE4, apago);
  digitalWrite(BombaVaciadoPinRELE15, apago);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//NutrienteApinRELE5
bool encenderNutrientesA()
{
  digitalWrite(NutrienteApinRELE5, prendo);
  return true;
}
bool apagarBombaNutrientesA()
{
  digitalWrite(NutrienteApinRELE5, apago);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//NutrienteBpinRELE6
bool encenderNutrientesB()
{
  digitalWrite(NutrienteBpinRELE6, prendo);
  return true;
}
bool apagarBombaNutrientesB()
{
  digitalWrite(NutrienteBpinRELE6, apago);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//pHMasPinRELE7
bool encenderPHmas()
{
  digitalWrite(pHMasPinRELE7, prendo);
  return true;
}
bool apagarPHmas()
{
  digitalWrite(pHMasPinRELE7, apago);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//pHmenosPinRELE8
bool encenderPHmenos()
{
  digitalWrite(pHmenosPinRELE8, prendo);
  return true;
}
bool apagarPHmenos()
{
  digitalWrite(pHmenosPinRELE8, apago);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//LucesPinRELE9
void encenderLuces()
{
  digitalWrite(LucesPinRELE9, prendo);
  //Serial.print("encenderLuces");
  //return true;
}
void apagarLuces()
{
  digitalWrite(LucesPinRELE9, apago);
  //Serial.print("apagarLuces");
  //return true;
}
//---------------------------------------------------------------------------------------------------------------//
//VentiladoresPinRELE10
bool encenderVentiladores()
{
  digitalWrite(VentiladoresPinRELE10, prendo);
  VENTILADOR_FUNCIONANDO = true;
  Serial.println("encenderVentiladores");
  return true;
}
void apagarVentiladores()
{
  digitalWrite(VentiladoresPinRELE10, apago);
  VENTILADOR_FUNCIONANDO = false;
  Serial.println("apagarVentiladores");
  //return true;
}
//---------------------------------------------------------------------------------------------------------------//
//CalentadorPinRELE11
bool encenderCalentador()
{
  CALENTADOR_FUNCIONANDO = true;
  digitalWrite(CalentadorPinRELE11, prendo);
  return true;
}
bool apagarCalentador()
{
  CALENTADOR_FUNCIONANDO = false;
  digitalWrite(CalentadorPinRELE11, apago);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//PHpinRELE12
bool encenderPH()
{
  digitalWrite(PHpinRELE12, prendo);
  return true;
}
bool apagarPH()
{
  digitalWrite(PHpinRELE12, apago);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//CEpinRELE13
bool encenderCE()
{
  digitalWrite(CEpinRELE13, prendo);
  return true;
}
bool apagarCE()
{
  digitalWrite(CEpinRELE13, apago);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** FUNCIONES de encendido y apagado de dispositivos >
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** < FUNCIONES de medición
//---------------------------------------------------------------------------------------------------------------//
bool nivelTanque(char caso[])
{
  bool resultado = false;
  int estado = 0;
  if (strcmp(caso, "tanqueLleno") == 0)
  {
    estado = digitalRead(pinNivel12);
    goto continua;
  }
  if (strcmp(caso, "tanqueMedio") == 0)
  {
    estado = digitalRead(pinNivel11);
    goto continua;
  }
  if (strcmp(caso, "tanqueVacio") == 0)
  {
    estado = digitalRead(pinNivel10);
    if (estado == 0)
      estado = HIGH;
    else
      estado = LOW;
    goto continua;
  }

continua:
  // SERIAL_PRINT("Estado", estado);
  switch (estado)
  {
  case LOW:
    resultado = false;
    break;
  case HIGH:
    resultado = true;
    break;
  }
  // SERIAL_PRINT("Resultado", resultado);

  return resultado;
}
//---------------------------------------------------------------------------------------------------------------//
float medirNivelLiquido(char caso[])
{
  float resultado = 0.0;
  if (strcmp(caso, "medicionNivelPHmas") == 0)
  {
    resultado = analogRead(pinNivel6);
    goto continua;
  }

  if (strcmp(caso, "medicionNivelPHmenos") == 0)
  {
    resultado = analogRead(pinNivel8);
    goto continua;
  }

continua:

  return resultado;
}
//---------------------------------------------------------------------------------------------------------------//
float medirNivel(char caso[])
{
  int PinTrig;
  int PinEcho;

  if (strcmp(caso, "medicionNivelNutrienteA") == 0)
  {
    PinTrig = pinNivel1;
    PinEcho = pinNivel2;
    goto continua;
  }

  if (strcmp(caso, "medicionNivelNutrienteB") == 0)
  {
    PinTrig = pinNivel3;
    PinEcho = pinNivel4;
    goto continua;
  }

  // if (strcmp(caso, "medicionNivelPHmas") == 0)
  // {
  //   PinTrig = pinNivel5;
  //   PinEcho = pinNivel6;
  //   goto continua;
  // }

  // if (strcmp(caso, "medicionNivelPHmenos") == 0)
  // {
  //   PinTrig = pinNivel7;
  //   PinEcho = pinNivel8;
  //   goto continua;
  // }

  if (strcmp(caso, "medicionNivelTanquePrincial") == 0)
  {
    PinTrig = pinNivel13;
    PinEcho = pinNivel14;
  }

continua:

  //  digitalWrite(PinTrig, LOW);
  //  delayMicroseconds(2);
  //
  //  digitalWrite(PinTrig, HIGH);
  //  delayMicroseconds(10);
  //
  //  float tiempo = pulseIn(PinEcho, HIGH);
  //  float distancia = (tiempo / 2) / 29.1;
  //  return distancia;

  float distancias[CANTIDAD_MEDICIONES], sum = 0, promedio = 0;
  for (int i = 0; i < CANTIDAD_MEDICIONES; i++)
    distancias[i] = 0;

  for (int i = 0; i < CANTIDAD_MEDICIONES; i++)
  {
    digitalWrite(PinTrig, LOW);
    delayMicroseconds(2);
    digitalWrite(PinTrig, HIGH);
    delayMicroseconds(10);

    float tiempo = pulseIn(PinEcho, HIGH);
    float distancia = (tiempo / 2) / 29.1;
    //distancia = 340m/s*tiempo;

    //Serial.println(distancia);
    distancias[i] = distancia;
    delay(10);
  }
  int cant = 0, CantCeros = 0;

  for (int i = 0; i < CANTIDAD_MEDICIONES; i++)
  {
    if (distancias[i] != 0)
    {
      sum = sum + distancias[i];
      cant++;
    }
    else
      CantCeros++;
  }
  if (CantCeros >= 2)
    return 0; //Serial.print("Falla en sensor / desconectado ");

  //Serial.println(CantCeros);

  promedio = sum / cant;
  //Serial.println(promedio);
  return promedio;
}
//---------------------------------------------------------------------------------------------------------------//
float medirHumedad()
{
  return dht.readHumidity(); //Se lee la humedad del aire.
}
//---------------------------------------------------------------------------------------------------------------//
float medirTemperatura()
{
  return dht.readTemperature(); //Se lee la temperatura del aire.
}
//---------------------------------------------------------------------------------------------------------------//
float medirTemperaturaAgua()
{
  sensorDS18B20.requestTemperatures();
  return sensorDS18B20.getTempCByIndex(0); //Se lee la temperatura del agua.
}
//---------------------------------------------------------------------------------------------------------------//
float medirPH()
{
  apagarCE();
  encenderPH();
  delay(2000);
  int measure = analogRead(pinPH); //Se lee el pH.
  //si es 0 esta conectado pero apagado.
  //SERIAL_PRINT("pinPH:",measure);
  if (measure == 0)
    return 0.0;
  double voltage = 5 / 1024.0 * measure;
  apagarPH();
  return 7 + ((2.5 - voltage) / 0.18);
}
//---------------------------------------------------------------------------------------------------------------//
float medirCE()
{
  apagarPH();
  encenderCE();
  delay(2000);
  int sensorValue = 0;
  int outputValue = 0;
  sensorValue = analogRead(pinCE);
  //SERIAL_PRINT("pinCE:",sensorValue);
  outputValue = map(sensorValue, 0, 1023, 0, 5000);
  delay(500);
  //SERIAL_PRINT("outputValue:",outputValue);
  apagarCE();
  return sensorValue * 5.00 / 1024;
}
//---------------------------------------------------------------------------------------------------------------//
float medirCO2()
{
  return gasSensor.getPPM(); //Se mide el co2.
  //float rzero = gasSensor.getRZero();//Se mide el rzero para calibrar el co2.
  //    Serial.println(":::::rzero ");
  //    Serial.println(rzero);
}
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** FUNCIONES de medición >
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** < FUNCIONES de control
//---------------------------------------------------------------------------------------------------------------//
bool analizarAire()
{
  CMD_VENTILAR = false;

  //------------------------------------------------------- Humedad
  medicionHumedad = medirHumedad();
  ERR_MEDICION_HUMEDAD = false;
  if (medicionHumedad == -10000)
  {
    medicionHumedad = 0.0;
    ERR_MEDICION_HUMEDAD = true;
    //return false;
  }
  else if (medicionHumedad < humedadMinParametro || medicionHumedad > humedadMaxParametro)
  { //Se requiere alguna accion.
    //encenderVentiladores();
    CMD_VENTILAR = true;
  }

  //-------------------------------------------------------TMP
  medicionTemperaturaAire = medirTemperatura();
  ERR_MEDICION_TEMPERATURA = false;
  if (medicionTemperaturaAire == -10000)
  {
    medicionTemperaturaAire = 0.0;
    ERR_MEDICION_TEMPERATURA = true;
    //return false;
  }
  else if (medicionTemperaturaAire < temperaturaAireMinParametro || medicionTemperaturaAire > temperaturaAireMaxParametro)
  { //Se requiere alguna accion.
    //encenderVentiladores();
    CMD_VENTILAR = true;
  }

  //-------------------------------------------------------CO2
  medicionCO2 = medirCO2();
  ERR_MEDICION_CO2 = false;
  if (medicionCO2 > 10000)
  {
    medicionCO2 = 0.0;
    ERR_MEDICION_CO2 = true;
    //return false;
  }
  else if (medicionCO2 < co2MinParametro || medicionCO2 > co2MaxParametro)
  { //Se requiere alguna accion.
    //encenderVentiladores();
    CMD_VENTILAR = true;
  }
  if (ERR_MEDICION_HUMEDAD || ERR_MEDICION_TEMPERATURA || ERR_MEDICION_CO2)
    return false;
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
bool analizarAgua()
{
  apagarTodo(false, 5000);
  //-------------------------------------------------------TMP
  medicionTemperaturaAgua = medirTemperaturaAgua();
  ERR_MEDICION_TEMPERATURA_AGUA = false;
  if (medicionTemperaturaAgua < -100)
  {
    medicionTemperaturaAgua = 0.0;
    ERR_MEDICION_TEMPERATURA_AGUA = true;
    //return false;
  }
  else if (medicionTemperaturaAgua < temperaturaAguaMinParametro || medicionTemperaturaAgua > temperaturaAguaMaxParametro)
  { //Se requiere alguna accion.
    //encenderEnfriador();
    if (medicionTemperaturaAgua < temperaturaAguaMinParametro)
    { //Subir temperatura.
      CMD_BAJAR_TEMPERATURA_AGUA = false;
      CMD_SUBIR_TEMPERATURA_AGUA = true;
    }
    else //if (medicionTemperaturaAgua > temperaturaAguaMaxParametro)
    {    //Bajar temperaruta.
      CMD_BAJAR_TEMPERATURA_AGUA = true;
      CMD_SUBIR_TEMPERATURA_AGUA = false;
    }
  }
  else
  { //No hace nada.
    CMD_BAJAR_TEMPERATURA_AGUA = false;
    CMD_SUBIR_TEMPERATURA_AGUA = false;
  }

  //-------------------------------------------------------PH
  //goto aaa;
  medicionPH = medirPH();
  ERR_MEDICION_PH = false;
  if (medicionPH == 0)
  { //Sensor desconectado o apagado.
    medicionPH = 0.0;
    ERR_MEDICION_PH = true;
    //return false;
  }
  else if (medicionPH < 0)
  { //Sonda desconectada.
    medicionPH = 0.0;
    ERR_MEDICION_PH = true;
    //return false;
  }
  else if (medicionPH < PHminParametro || medicionPH > PHmaxParametro)
  { //Se requiere alguna accion.
    if (medicionPH < PHminParametro)
    {
      //encenderPHmas();
      CMD_SUBIR_PH = true;
      CMD_BAJAR_PH = false;
    }
    else //if (medicionPH > PHmaxParametro)
    {
      //encenderPHmenos();
      CMD_SUBIR_PH = false;
      CMD_BAJAR_PH = true;
    }
  }
  else
  { //No hace nada.
    CMD_SUBIR_PH = false;
    CMD_BAJAR_PH = false;
  }
  //aaa:
  //-------------------------------------------------------CE
  medicionCE = medirCE();
  SERIAL_PRINT("medicionCE ", medicionCE);
  SERIAL_PRINT("ceMinParametro ", ceMinParametro);
  SERIAL_PRINT("ceMaxParametro ", ceMaxParametro);
  ERR_MEDICION_CE = false;
  if (medicionCE == 0)
  { //Sensor desconectado o apagado.
    medicionCE = 0.0;
    ERR_MEDICION_CE = true;
    //return false;
  }
  else if (medicionCE < ceMinParametro || medicionCE > ceMaxParametro)
  { //Se requiere alguna accion.
    if (medicionCE < ceMinParametro)
    {
      //encenderPHmas();
      CMD_ADD_NUTRIENTE_A = true;
      CMD_ADD_NUTRIENTE_B = true;
      CMD_ADD_AGUA = false;
    }
    else //if (medicionPH > ceMaxParametro)
    {
      //encenderPHmenos();
      CMD_ADD_NUTRIENTE_A = false;
      CMD_ADD_NUTRIENTE_B = false;
      CMD_ADD_AGUA = true;
    }
  }
  else
  { //No hace nada.
    CMD_ADD_NUTRIENTE_A = false;
    CMD_ADD_NUTRIENTE_B = false;
    CMD_ADD_AGUA = false;
  }

  encenderBombaPrincipal();
  if (ERR_MEDICION_TEMPERATURA_AGUA || ERR_MEDICION_PH || ERR_MEDICION_CE)
    return false;
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
bool controlarNivelesPH()
{
  //------------------------------------------------------- PH+
  ERR_MEDICION_NIVEL_PH_MAS = false;
  ALT_MINIMO_PH_MAS = false;
  medicionNivelPHmas = medirNivelLiquido("medicionNivelPHmas");
  if (medicionNivelPHmas == 0)
  {
    //   ERR_MEDICION_NIVEL_PH_MAS = true;
    //   //return false;
    // }
    // else if (medicionNivelPHmas < minimoNivelPHmas)
    // {
    ALT_MINIMO_PH_MAS = true;
  }

  //------------------------------------------------------- PH-
  ERR_MEDICION_NIVEL_PH_MEN = false;
  ALT_MINIMO_PH_MEN = false;
  medicionNivelPHmenos = medirNivelLiquido("medicionNivelPHmenos");
  if (medicionNivelPHmenos == 0)
  {
    //   ERR_MEDICION_NIVEL_PH_MEN = true;
    //   //return false;
    // }
    // else if (medicionNivelPHmenos < minimoNivelPHmenos)
    // {
    ALT_MINIMO_PH_MEN = true;
  }

  if (ERR_MEDICION_NIVEL_PH_MAS || ERR_MEDICION_NIVEL_PH_MEN)
    return false;

  return true;
}
//---------------------------------------------------------------------------------------------------------------//
bool controlarNivelesNutrintes()
{
  //------------------------------------------------------- Nut A
  ERR_MEDICION_NIVEL_NUT_A = false;
  ALT_MINIMO_NUT_A = false;
  medicionNivelNutrienteA = medirNivel("medicionNivelNutrienteA");
  if (medicionNivelNutrienteA == 0)
  {
    ERR_MEDICION_NIVEL_NUT_A = true;
    //return false;
  }
  else if (medicionNivelNutrienteA > minimoNivelNutrienteA)
  {
    ALT_MINIMO_NUT_A = true;
  }

  //------------------------------------------------------- Nut B
  ERR_MEDICION_NIVEL_NUT_B = false;
  ALT_MINIMO_NUT_B = false;
  medicionNivelNutrienteB = medirNivel("medicionNivelNutrienteB");
  if (medicionNivelNutrienteB == 0)
  {
    ERR_MEDICION_NIVEL_NUT_B = true;
    //return false;
  }
  else if (medicionNivelNutrienteB > minimoNivelNutrienteB)
  {
    ALT_MINIMO_NUT_B = true;
  }
  if (ERR_MEDICION_NIVEL_NUT_A || ERR_MEDICION_NIVEL_NUT_B)
    return false;

  return true;
}
//---------------------------------------------------------------------------------------------------------------//
bool controlarNivelesTanques()
{
  apagarTodo(false, 5000);
  //------------------------------------------------------- Agua limpia
  /*  ERR_MEDICION_NIVEL_LIMPIA = false;
  medicionNivelTanqueAguaLimpia = medirNivel("medicionNivelTanqueAguaLimpia");
  if (medicionNivelTanqueAguaLimpia == 0) {
    ERR_MEDICION_NIVEL_LIMPIA = true;
    //return false;
  }
  else if (medicionNivelTanqueAguaLimpia > minimoNivelTanqueAguaLimpia) {
    ALT_MINIMO_LIMPIA = true;
  }
  else if (medicionNivelTanqueAguaLimpia < maximoNivelTanqueAguaLimpia) {
    ALT_MAXIMO_LIMPIA = true;
  }
  else {
    ALT_MINIMO_LIMPIA = false;
    ALT_MAXIMO_LIMPIA = false;
  }*/

  //------------------------------------------------------- Agua descarte
  /*  ERR_MEDICION_NIVEL_DESCARTE = false;
  medicionNivelTanqueDesechable = medirNivel("medicionNivelTanqueDesechable");
  if (medicionNivelTanqueDesechable == 0) {
    ERR_MEDICION_NIVEL_DESCARTE = true;
    //return false;
  }
  else if (medicionNivelTanqueDesechable > minimoNivelTanqueDesechable) {
    ALT_MINIMO_DESCARTE = true;
  }
  else if (medicionNivelTanqueDesechable < maximoNivelTanqueDesechable) {
    ALT_MAXIMO_DESCARTE = true;
  }
  else {
    ALT_MINIMO_DESCARTE = false;
    ALT_MAXIMO_DESCARTE = false;
  }*/

  //------------------------------------------------------- Tanque principal
  tanqueLleno = nivelTanque("tanqueLleno");
  tanqueMedio = nivelTanque("tanqueMedio");
  tanqueVacio = nivelTanque("tanqueVacio");

  SERIAL_PRINT("tanqueLleno ", tanqueLleno);
  SERIAL_PRINT("tanqueMedio ", tanqueMedio);
  SERIAL_PRINT("tanqueVacio ", tanqueVacio);

  ERR_MEDICION_NIVEL_PRINCIPAL = false;
  medicionNivelTanquePrincial = medirNivel("medicionNivelTanquePrincial");
  if (medicionNivelTanquePrincial == 0)
  {
    ERR_MEDICION_NIVEL_PRINCIPAL = true;
    //return false;
  }
  else if (medicionNivelTanquePrincial > minimoNivelTanquePrincial)
  {
    ALT_MINIMO_PRINCIPAL = true;
  }
  else if (medicionNivelTanquePrincial < maximoNivelTanquePrincial)
  {
    ALT_MAXIMO_PRINCIPAL = true;
  }
  else
  {
    ALT_MINIMO_PRINCIPAL = false;
    ALT_MAXIMO_PRINCIPAL = false;
  }

  if (ERR_MEDICION_NIVEL_PRINCIPAL) //ERR_MEDICION_NIVEL_LIMPIA || ERR_MEDICION_NIVEL_DESCARTE ||
    return false;

  if (!tanqueMedio || tanqueVacio)
    ALT_MINIMO_PRINCIPAL = true;

  encenderBombaPrincipal();
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
bool vaciarTanque()
{
  ERR_SALIDA_AGUA_DESCARTE = false;
  ALT_VACIADO_REALIZADO = false;
  bool error = false;
  apagarTodo(true,2000);
  encenderBombaVaciado();
  float nivel = medirNivel("medicionNivelTanquePrincial");
  float nivelAux = 0;
  int intentos = 0;
  while (!nivelTanque("tanqueVacio") && !error)
  {
    delay(TIEMPO_LECTURA_NIVEL*2);
    nivelAux = medirNivel("medicionNivelTanquePrincial");
    SERIAL_PRINT("INGRESO AL VACIADO", "");
    if (nivel < nivelAux)
    {
      nivel = nivelAux;
      intentos = 0;
    }
    else
    {
      intentos++;
    }
    if (intentos == CANTIDAD_INTENTOS)
    {
      error = true;
      ERR_SALIDA_AGUA_DESCARTE = true;
    }
    SERIAL_PRINT("INGRESO AL LLENADO ::: INTENTO ", intentos);
  }
  apagarBombaVaciado();
  if (!error)
  {
    ALT_VACIADO_REALIZADO = true;
    return true;
  }
  return false;
}
//---------------------------------------------------------------------------------------------------------------//
bool llenarTanque(bool vacio)
{
  //SERIAL_PRINT("INGRESO AL LLENADO", "");
  ERR_INGRESO_AGUA_LIMPIA = false;
  ALT_LLENADO_REALIZADO = false;
  bool error = false;
  encenderLlenado();
  float nivel = medirNivel("medicionNivelTanquePrincial");
  if (nivel == 0)
  {
    ERR_MEDICION_NIVEL_PRINCIPAL = true;
    error = true;
  }
  float nivelAux = 0;
  int intentos = 0;

  //SERIAL_PRINT("NIVEL ", nivel);

  //bool level = ;
  //SERIAL_PRINT("aaaa ", aaaa);
  //SERIAL_PRINT("error ", error);

  while (!nivelTanque("tanqueLleno") && !error)
  {
    SERIAL_PRINT("INGRESO AL LLENADO", "");

    delay(TIEMPO_LECTURA_NIVEL*2);
    nivelAux = medirNivel("medicionNivelTanquePrincial");

    //SERIAL_PRINT("NIVEL AUX ", nivelAux);

    if (nivelAux == 0)
    {
      ERR_MEDICION_NIVEL_PRINCIPAL = true;
      error = true;
    }
    if (nivel > nivelAux)
    {
      nivel = nivelAux;
      intentos = 0;
    }
    else
    {
      intentos++;
    }
    if (intentos >= CANTIDAD_INTENTOS)
    {
      error = true;
      ERR_INGRESO_AGUA_LIMPIA = true;
    }
    SERIAL_PRINT("INGRESO AL LLENADO ::: INTENTO ", intentos);
  }

  SERIAL_PRINT("APAGAR LLENADO", "");
  apagarLlenado();
  SERIAL_PRINT("LLENADO APAGADO", "");

  if (vacio && !error)
  {
    generarAlerta("CMD_ADD_15NUTRIENTE_A");
    generarAlerta("CMD_ADD_15NUTRIENTE_B");
    encenderNutrientesA();
    encenderNutrientesB();
    delay(TIEMPO_GOTEO_NUTRIENTES * 3);
    apagarBombaNutrientesA();
    apagarBombaNutrientesB();
  }

  if (!error)
  {
    ALT_LLENADO_REALIZADO = true;
    return true;
  }
  return false;
}
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** FUNCIONES de control >
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** < FUNCIONES para el protocolo de paquetes entre ARDUINO y ESP8266
//---------------------------------------------------------------------------------------------------------------//
char *calcChecksum(const char *data, int length)
{
  static char checksum[SIZE_CHECKSUM];
  int result = 0;

  for (int idx = 0; idx < length; idx++)
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
    char numHexa[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
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

  int index = 0;
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
  memcpy(payLoad, package + 2, (length - 7) * sizeof(char)); // +3 para salterame el caracter inicial(1) + primer delimitador(1); -7 para no copiar caracter de inicio
  // de paquete(1), primer delimitador(1), delimitador antes del checksum(1) el checksum(2) el limitador despues del checksum(1) y el caracter de fin de paquete(1).
  payLoad[length - 7] = '\0';
  return payLoad;
}
//---------------------------------------------------------------------------------------------------------------//
int disarmPayLoad(const char *payLoad, int length, char *data)
{
  char command[3];
  memcpy(command, payLoad, 2 * sizeof(char));
  command[2] = '\0';

  int cmd = atoi(command);
  if (cmd > 0 && cmd < 100)
  {
    //if (cmd == 6) {  // unico comando proveniente desde la raspberry con datos;
    memcpy(data, payLoad + 3, (length - 2) * sizeof(char));
    data[length - 2] = '\0';
    // check json 
    if(data[length - 4] == '"'){
      data[length - 3] = '}';
    }
    SERIAL_PRINT("\n> DATA PAYLOAD DISARM", data);
    SERIAL_PRINT("\n> DATA PAYLOAD DISARM LAST CHAR", data[length - 4]);
    SERIAL_PRINT("\n> DATA LENGHT PAYLOAD DISARM", length);
    // }
    return cmd;
  }
  return ERROR_CODE; //retCode error;
}
//---------------------------------------------------------------------------------------------------------------//
int proccesPackage(String package, int length, char *data)
{
  SERIAL_PRINT("\n> Recieved Package: ", package);
  char recievedPack[SIZE_PACKAGE];
  char payLoad[SIZE_PAYLOAD];
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
      SERIAL_PRINT("> Data Lenght: ", sizeof(data));
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
bool checkPackageComplete(const char *com)
{
  char data[50];
  int retCmd = proccesPackage(receivedPackage, receivedPackage.length(), data);

  if (retCmd == ERROR_CODE)
  {
    SERIAL_PRINT("> Error, comando desconocido !!", "");
    return false;
  }
  else if (retCmd == ERROR_BAD_CHECKSUM)
  {
    SERIAL_PRINT("> Error de checksum !!", "");
    return false;
  }
  else
  {
    StaticJsonBuffer<100> jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(data);
    Serial.print("********* JSON **********");
    root.printTo(Serial);
    if (0 < retCmd && 16 > retCmd)
    {
      if (strcmp(com, root["OK"]) != 0)
      {
        SERIAL_PRINT("ER", "Paquete no esperado");
        return false;
      }
    }
    else
    {
      if (atoi(com) != retCmd && (retCmd != 99 && retCmd != 98))
      {
        SERIAL_PRINT("ER", "Paquete no esperado");
        return false;
      }
    }

    switch (retCmd)
    {
    case 16:
      hsLuzParametro = root["HorasLuz"];
      SERIAL_PRINT("HorasLuz ", hsLuzParametro);
      break;
    case 17:
      horaInicioLuz = root["HoraInicioLuz"];
      SERIAL_PRINT("HoraInicioLuz", horaInicioLuz);
      break;
    case 18:
      PhParametro = root["pHaceptable"];
      SERIAL_PRINT("pHaceptable", PhParametro);
      break;
    case 19:
      if (root["pwr"] == 1)
        CMD_SISTEMA_ENCENDIDO = true;
      else
        CMD_SISTEMA_ENCENDIDO = false;
      SERIAL_PRINT("pwr", CMD_SISTEMA_ENCENDIDO);
      break;
    case 20:
      if (root["Vaciado"] == 1)
        CMD_VACIAR_AGUA = true;
      else
        CMD_VACIAR_AGUA = false;
      SERIAL_PRINT("Vaciado ", CMD_VACIAR_AGUA);
      break;
    case 98:
      SERIAL_PRINT("BS", "BUSY");
      return false;
      break;
    case 99:
      SERIAL_PRINT("ER", "ERROR");
      return false;
      break;
    default:
      SERIAL_PRINT("OK ", retCmd);
      break;
    }
  }
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** FUNCIONES para el protocolo de paquetes entre ARDUINO y ESP8266 >
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** < FUNCIONES para la generacion de alertas
//---------------------------------------------------------------------------------------------------------------//
String floatTOstring(float x) //Convertir de float a String
{
  return String(x);
}
//---------------------------------------------------------------------------------------------------------------//
String intTOstring(int x) //Convertir de int a String
{
  return String(x);
}
//---------------------------------------------------------------------------------------------------------------//
String completarLargo(String x, int largo, int lado) // lado: 1 Derecha 2 Izquierda. ** Completar el largo con 000000
{
  //  SERIAL_PRINT("x: ", x);
  //  SERIAL_PRINT("length: ", x.length());

  if (x.length() < largo)
  {
    for (int i = 0; i <= largo - x.length(); i++)
      if (lado == 1)
        x = "0" + x;
      else
        x = x + "0";
  }
  //  SERIAL_PRINT("x2: ", x);
  return x;
}
//---------------------------------------------------------------------------------------------------------------//
int nivelTOporcentaje(float x, float techo, float piso) //Convertir la medicion de nivel en un %
{
  float aux = mapf(x, piso, techo, -1, 99);
  aux = (aux > 99 ? 99 : aux);
  aux = (aux < -1 ? -1 : aux);

  return (int)aux;
}
double mapf(double val, double in_min, double in_max, double out_min, double out_max)
{
  return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
//---------------------------------------------------------------------------------------------------------------//
String generarArrayAlertas()
{ //Se genera un array con todas las alertas existentes 0=false 1=true

  String aux = "";

  aux += ALT_MINIMO_PH_MAS ? "1" : "0";
  aux += ALT_MINIMO_PH_MEN ? "1" : "0";
  aux += ALT_MINIMO_NUT_A ? "1" : "0";
  aux += ALT_MINIMO_NUT_B ? "1" : "0";
  aux += ALT_MINIMO_PRINCIPAL ? "1" : "0";
  aux += ALT_MAXIMO_PRINCIPAL ? "1" : "0";
  // aux += ALT_MINIMO_LIMPIA ? "1" : "0";
  // aux += ALT_MAXIMO_DESCARTE ? "1" : "0";
  aux += ALT_RTC_DESCONFIGURADO ? "1" : "0";
  aux += ALT_RTC_DESCONECTADO ? "1" : "0";

  //  aux += ALT_MAXIMO_LIMPIA ? "1" : "0";
  //  aux += ALT_MINIMO_DESCARTE ? "1" : "0";

  return aux;
}
//---------------------------------------------------------------------------------------------------------------//
String generarArrayErrores()
{ //Se genera un array con todas los errores existentes 0=false 1=true

  String aux = "";
  aux += ERR_MEDICION_PH ? "1" : "0";
  aux += ERR_MEDICION_CE ? "1" : "0";
  aux += ERR_MEDICION_HUMEDAD ? "1" : "0";
  aux += ERR_MEDICION_TEMPERATURA ? "1" : "0";
  aux += ERR_MEDICION_CO2 ? "1" : "0";
  aux += ERR_MEDICION_NIVEL_PH_MAS ? "1" : "0";
  aux += ERR_MEDICION_NIVEL_PH_MEN ? "1" : "0";
  aux += ERR_MEDICION_NIVEL_NUT_A ? "1" : "0";
  aux += ERR_MEDICION_NIVEL_NUT_B ? "1" : "0";
  aux += ERR_MEDICION_NIVEL_PRINCIPAL ? "1" : "0";
  // aux += ERR_MEDICION_NIVEL_LIMPIA ? "1" : "0";
  // aux += ERR_MEDICION_NIVEL_DESCARTE ? "1" : "0";
  aux += ERR_MEDICION_TEMPERATURA_AGUA ? "1" : "0";
  aux += ERR_ENVIO_INFORMACION ? "1" : "0";

  return aux;
}
//---------------------------------------------------------------------------------------------------------------//
bool generarJson()
{
  String auxArray = generarArrayAlertas();
  String auxArray2 = generarArrayErrores();

  //crear Json
  StaticJsonBuffer<290> jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();
  json["HumedadAire"] = completarLargo(floatTOstring(medicionHumedad), 5, 1);
  json["NivelCO2"] = completarLargo(floatTOstring(medicionCO2), 8, 1);
  json["TemperaturaAire"] = completarLargo(floatTOstring(medicionTemperaturaAire), 5, 1);
  json["TemperaturaAguaTanquePrincipal"] = completarLargo(floatTOstring(medicionTemperaturaAgua), 5, 1);
  json["MedicionPH"] = completarLargo(floatTOstring(medicionPH), 6, 1);
  json["MedicionCE"] = completarLargo(floatTOstring(medicionCE), 6, 1);
  json["NivelTanquePrincipal"] = intTOstring(nivelTOporcentaje(medicionNivelTanquePrincial, maximoNivelTanquePrincial, pisoTanqueAguaPrincipal));
  // json["NivelTanqueLimpia"] = intTOstring(nivelTOporcentaje(medicionNivelTanqueAguaLimpia, maximoNivelTanqueAguaLimpia, pisoTanqueAguaLimpia));
  // json["NivelTanqueDescarte"] = intTOstring(nivelTOporcentaje(medicionNivelTanqueDesechable, maximoNivelTanqueDesechable, pisoTanqueAguaDescartada));
  json["NivelPhMas"] = intTOstring(medicionNivelPHmas);
  json["NivelPhMenos"] = intTOstring(medicionNivelPHmenos);
  json["NivelNutrienteA"] = intTOstring(nivelTOporcentaje(medicionNivelNutrienteA, maximoNivelNutrienteA, pisoTanqueNutrienteA));
  json["NivelNutrienteB"] = intTOstring(nivelTOporcentaje(medicionNivelNutrienteB, maximoNivelNutrienteB, pisoTanqueNutrienteB));
  json["Alertas"] = auxArray;
  json["Errores"] = auxArray2;

  json.prettyPrintTo(Serial);

  return true;
}
//---------------------------------------------------------------------------------------------------------------//
bool enviarInformacion()
{
  String auxArray = generarArrayAlertas();
  String auxArray2 = generarArrayErrores();
  contadorRechazos = 0;
  ERR_ENVIO_INFORMACION = false;
  for (int i = 1; i <= 20; i++)
  {
    bool resultado = false;
    SERIAL_PRINT("iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii: ", i);
    switch (i)
    {
    case 1:
      resultado = sendPackage("HumAire", i, completarLargo(floatTOstring(medicionHumedad), 5, 1));
      break;
    case 2:
      resultado = sendPackage("CO2", i, completarLargo(floatTOstring(medicionCO2), 8, 1));
      break;
    case 3:
      resultado = sendPackage("TempAire", i, completarLargo(floatTOstring(medicionTemperaturaAire), 5, 1));
      break;
    case 4:
      resultado = sendPackage("TempAgua", i, completarLargo(floatTOstring(medicionTemperaturaAgua), 5, 1));
      break;
    case 5:
      resultado = sendPackage("PH", i, completarLargo(floatTOstring(medicionPH), 6, 1));
      break;
    case 6:
      resultado = sendPackage("CE", i, completarLargo(floatTOstring(medicionCE), 6, 1));
      break;
    case 7:
      resultado = sendPackage("NivelTanqueP", i, intTOstring(nivelTOporcentaje(medicionNivelTanquePrincial, maximoNivelTanquePrincial, pisoTanqueAguaPrincipal)));
      break;
    case 8:
      //     resultado=sendPackage("NivelTanqueL", i, intTOstring(nivelTOporcentaje(medicionNivelTanqueAguaLimpia, maximoNivelTanqueAguaLimpia, pisoTanqueAguaLimpia)));
      resultado = true;
      break;
    case 9:
      //   resultado=sendPackage("NivelTanqueD", i, intTOstring(nivelTOporcentaje(medicionNivelTanqueDesechable, maximoNivelTanqueDesechable, pisoTanqueAguaDescartada)));
      resultado = true;
      break;
    case 10:
      resultado = sendPackage("NivelPh+", i, intTOstring(medicionNivelPHmas));
      break;
    case 11:
      resultado = sendPackage("NivelPh-", i, intTOstring(medicionNivelPHmenos));
      break;
    case 12:
      resultado = sendPackage("NivelNutA", i, intTOstring(nivelTOporcentaje(medicionNivelNutrienteA, maximoNivelNutrienteA, pisoTanqueNutrienteA)));
      break;
    case 13:
      resultado = sendPackage("NivelNutB", i, intTOstring(nivelTOporcentaje(medicionNivelNutrienteB, maximoNivelNutrienteB, pisoTanqueNutrienteB)));
      break;
    case 14:
      resultado = sendPackage("A", i, auxArray);
      break;
    case 15:
      resultado = sendPackage("E", i, auxArray2);
      break;
    case 16:
      resultado = sendPackage("HsL", i, "16");
      break;
    case 17:
      resultado = sendPackage("HiL", i, "17");
      break;
    case 18:
      resultado = sendPackage("phA", i, "18");
      break;
    case 19:
      resultado = sendPackage("pwr", i, "19");
      break;
    case 20:
      resultado = sendPackage("Vaciado", i, "20");
      break;
    }

    if (!resultado)
    {
      contadorRechazos++;
      i--;
    }
    else
    {
      contadorRechazos = 0;
    }

    if (contadorRechazos > RECHAZOS_MAXIMOS)
    {
      ERR_ENVIO_INFORMACION = true;
      resetWifi();
      return false;
    }
  }
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
bool sendPackage(String nombre, int cmd, String valor)
{
  delay(10);
  char command[3];
  completarLargo(intTOstring(cmd), 2, 1).toCharArray(command, 3);
  char payLoad[25];
  char package[32];
  char aux[22];

  parameterToJson(nombre, valor).toCharArray(aux, 22);

  snprintf(payLoad, sizeof(payLoad), "%s%c%s", command, (char)DELIMITER_CHARACTER, aux);
  strcpy(package, preparePackage(payLoad, strlen(payLoad)));
  Serial.println(package);

  return send(package, command);
}
//---------------------------------------------------------------------------------------------------------------//
String parameterToJson(String nombre, String valor)
{
  return "{\"" + nombre + "\":\"" + valor + "\"}";
}
//---------------------------------------------------------------------------------------------------------------//
bool send(const char *message, const char *com)
{
  delay(10);
  receivedPackage = "";

  SERIAL_PRINT("Envio: ", message);
  esp.writeData(message);
  delay(25);
  receivedPackage = esp.readData();
  SERIAL_PRINT("Recibo: ", receivedPackage);
  Serial.println();

  return checkPackageComplete(com);

  //SPI.beginTransaction(SPISettings(SPI_CLOCK_DIV16, MSBFIRST, SPI_MODE3));
  //SPI.endTransaction ();
}
//---------------------------------------------------------------------------------------------------------------//
bool generarAlerta(String mensaje)
{
  Serial.println(mensaje);
}
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** FUNCIONES para la generacion de alertas >
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** < FUNCIONES para el reloj RTC
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
int ajustarHoras(int x)
{
  if (x >= 24)
  {              //si la cuenta supero las 24hs
    if (x == 24) //si son 24 lo paso a 00hs
      return 0;
    else
    { //si son mas de 24hs, lo ajusto a la hora real
      return x - 24;
    }
  }
  return x; //como es menor a 24 queda igual
}
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** FUNCIONES para el reloj RTC >
//---------------------------------------------------------------------------------------------------------------//

void colorRGB(int color) //0 off, 1 Rojo, 2 Verde
{
  switch (color)
  {
  case 1:
    digitalWrite(rled, HIGH); // Se enciende color rojo
    digitalWrite(gled, LOW);  // Se apaga colo verde
    break;
  case 2:
    digitalWrite(rled, LOW);  // Se enciende color rojo
    digitalWrite(gled, HIGH); // Se apaga colo verde
    break;
  default:
    digitalWrite(rled, LOW); // Se enciende color rojo
    digitalWrite(gled, LOW); // Se apaga colo verde
  }
}