#include "DHT.h" //Cargamos la librería DHT (temperatura y humedad)
#include <OneWire.h>
#include <DallasTemperature.h>
#include "MQ135.h" //Cargamos la librería MQ135

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
#define SIZE_PAYLOAD     40
#define SIZE_PACKAGE     SIZE_PAYLOAD + 7
#define SIZE_CHECKSUM    3
#define SIZE_MSG_QUEUE   10

String receivedPackage = "";
bool   packageComplete = false;

//************************** Banderas de comandos y errores *********************
bool CMD_BAJAR_TEMPERATURA_AGUA = false;
bool CMD_SUBIR_TEMPERATURA_AGUA = false;
bool ERR_MEDICION_TEMPERATURA_AGUA = false;
bool CMD_SUBIR_PH = false;
bool CMD_BAJAR_PH = false;
bool ERR_MEDICION_PH = false;
bool CMD_ADD_NUTRIENTE_A = false;
bool CMD_ADD_NUTRIENTE_B = false;
bool CMD_ADD_AGUA = false;
bool ERR_MEDICION_CE = false;
bool VENTILAR = false;
bool ERR_MEDICION_HUMEDAD = false;
bool ERR_MEDICION_TEMPERATURA = false;
bool ERR_MEDICION_CO2 = false;

//************************** pH del agua *****************************************
#define pinPH A1 //Seleccionamos el pin que usara el sensor de pH.

//************************** Conductividad Electrica del agua *****************************************
#define pinCE A2 //Seleccionamos el pin que usara el sensor de conductividad electrica.

//************************** CO2 *****************************************
#define pinCO2 A0 //Seleccionamos el pin que usara el sensor de MQ135.
MQ135 gasSensor = MQ135(pinCO2);

//************************* Niveles de liquido *********************************************
#define pinNivel1 24 //Seleccionamos el pin en el que se conectará el sensor de nivel de nutrientes A.
#define pinNivel2 25 //Seleccionamos el pin en el que se conectará el sensor de nivel de nutrientes A.

#define pinNivel3 26 //Seleccionamos el pin en el que se conectará el sensor de nivel de nutrientes B.
#define pinNivel4 27 //Seleccionamos el pin en el que se conectará el sensor de nivel de nutrientes B.

#define pinNivel5 28 //Seleccionamos el pin en el que se conectará el sensor de nivel de pH+.
#define pinNivel6 29 //Seleccionamos el pin en el que se conectará el sensor de nivel de pH+.

#define pinNivel7 30 //Seleccionamos el pin en el que se conectará el sensor de nivel de pH-.
#define pinNivel8 31 //Seleccionamos el pin en el que se conectará el sensor de nivel de pH-.

//************************* Niveles de agua *********************************************
#define pinNivel9 32 //Seleccionamos el pin en el que se conectará el sensor de nivel de agua limpia.
#define pinNivel10 33 //Seleccionamos el pin en el que se conectará el sensor de nivel de agua limpia.

#define pinNivel11 34 //Seleccionamos el pin en el que se conectará el sensor de nivel de agua descartada.
#define pinNivel12 35 //Seleccionamos el pin en el que se conectará el sensor de nivel de agua descartada.

#define pinNivel13 36 //Seleccionamos el pin en el que se conectará el sensor de nivel de tanque principal.
#define pinNivel14 37 //Seleccionamos el pin en el que se conectará el sensor de nivel de tanque principal.

//************************** Temperatura y humedad del aire ********************************
#define DHTTYPE DHT22 //Se selecciona el tipo de sensor medidor de temperatura DHT22.
#define pinDHT 22 //Seleccionamos el pin en el que se conectará el sensor DHT22.
DHT dht(pinDHT, DHTTYPE); //Se inicia una variable que será usada por Arduino para comunicarse con el sensor.

//*********************************** ACTUADORES ***************************************** 16 relé
#define BombaEnfriadoPinRELE1 40 //Seleccionamos el pin en el que se conectará Bomba de agua Enfriado.
#define BombaLlenadoPinRELE2 41 //Seleccionamos el pin en el que se conectará Bomba de agua Llenado.
#define BombaPrincipalPinRELE3 42 //Seleccionamos el pin en el que se conectará Bomba de agua Principal.
#define BombaVaciadoPinRELE4 43 //Seleccionamos el pin en el que se conectará Bomba de agua Vaciado.
#define NutrienteApinRELE5 44 //Seleccionamos el pin en el que se conectará Bomba peristáltica Nutriente A.
#define NutrienteBpinRELE6 45 //Seleccionamos el pin en el que se conectará Bomba peristáltica Nutriente B.
#define pHMasPinRELE7 46 //Seleccionamos el pin en el que se conectará Bomba peristáltica pH+.
#define pHmenosPinRELE8 47 //Seleccionamos el pin en el que se conectará Bomba peristáltica pH-.
#define LucesPinRELE9 48 //Seleccionamos el pin en el que se conectará Luces Led.
#define VentiladoresPinRELE10 49 //Seleccionamos el pin en el que se conectará Ventiladores.
#define CalentadorPinRELE11 50 //Seleccionamos el pin en el que se conectará Calentador.
#define pinRELE12 51 //Seleccionamos el pin en el que se conectará **Sin Asignar.
#define pinRELE13 52 //Seleccionamos el pin en el que se conectará **Sin Asignar.
#define pinRELE14 53 //Seleccionamos el pin en el que se conectará **Sin Asignar.
//#define pinRELE15 44 //Seleccionamos el pin en el que se conectará **Sin Asignar.
//#define pinRELE16 45 //Seleccionamos el pin en el que se conectará **Sin Asignar.

//************************** Temperatura del agua *****************************************
const int pinDS18B20 = 23; //Seleccionamos el pin en el que se conectará el sensor DS18B20.
OneWire oneWireObjeto(pinDS18B20);//Inicializamos la clase.
DallasTemperature sensorDS18B20(&oneWireObjeto);//Inicializamos la clase.


//*********************************************************************************** < VARIABLES PARA MEDICIONES
//Variables para Luz
float hsLuzParametro = 0.0;

//Variables Medición de PH
float medicionPH = 0.0; //Valor medido
float PhParametro = 0.0;
float PHmaxParametro = 0.0;
float PHminParametro = 0.0;
float medicionNivelPHmas = 0.0; //Valor medido
float medicionNivelPHmenos = 0.0; //Valor medido

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
float medicionCE = 0.0; //Valor medido
float ceMaxParametro = 0.0;
float ceMinParametro = 0.0;

//Variables para la medición de los niveles de los tanques de agua
float medicionNivelTanquePrincial = 0.0; //Valor medido
float medicionNivelTanqueAguaLimpia = 0.0; //Valor medido
float medicionNivelTanqueDesechable = 0.0; //Valor medido

//***********************************************************************************  VARIABLES PARA MEDICIONES >

void setup() {
  Serial.begin(9600); //Se inicia la comunicación serial

  dht.begin(); //Se inicia el sensor DHT22.
  sensorDS18B20.begin(); //Se inicia el sensor DS18B20.

  //********* SENSORES
  pinMode(pinNivel1, INPUT); //nivel de nutrientes A.
  pinMode(pinNivel2, OUTPUT); //nivel de nutrientes A.
  pinMode(pinNivel3, INPUT); //nivel de nutrientes B.
  pinMode(pinNivel4, OUTPUT); //nivel de nutrientes B.
  pinMode(pinNivel5, INPUT); //nivel de ph+.
  pinMode(pinNivel6, OUTPUT); //nivel de ph+.
  pinMode(pinNivel7, INPUT); //nivel de ph-.
  pinMode(pinNivel8, OUTPUT); //nivel de ph-.
  pinMode(pinNivel9, INPUT); //agua limpia.
  pinMode(pinNivel10, OUTPUT); //agua limpia.
  pinMode(pinNivel11, INPUT); //agua descartada.
  pinMode(pinNivel12, OUTPUT); //agua descartada.
  pinMode(pinNivel13, INPUT); //tanque principal.
  pinMode(pinNivel14, OUTPUT); //tanque principal.

  pinMode(pinPH, INPUT); //ph.
  pinMode(pinCE, INPUT); //ce.
  pinMode(pinCO2, INPUT); //co2.
  pinMode(pinDHT, INPUT); //temperatura y humedad del aire.
  pinMode(pinDS18B20, INPUT); //temperatura del agua.

  //********* ACTUADORES
  pinMode(BombaEnfriadoPinRELE1, OUTPUT); //Bomba de agua Enfriado.
  pinMode(BombaLlenadoPinRELE2, OUTPUT); //Bomba de agua Llenado.
  pinMode(BombaPrincipalPinRELE3, OUTPUT); //Bomba de agua Principal.
  pinMode(BombaVaciadoPinRELE4, OUTPUT); //Bomba de agua Vaciado.
  pinMode(NutrienteApinRELE5, OUTPUT); //Bomba peristáltica Nutriente A.
  pinMode(NutrienteBpinRELE6, OUTPUT); //Bomba peristáltica Nutriente B.
  pinMode(pHMasPinRELE7, OUTPUT); //Bomba peristáltica pH+.
  pinMode(pHmenosPinRELE8, OUTPUT); //Bomba peristáltica pH-.
  pinMode(LucesPinRELE9, OUTPUT); //Luces Led.
  pinMode(VentiladoresPinRELE10, OUTPUT); //Ventiladores.
  pinMode(CalentadorPinRELE11, OUTPUT); //Calentador.
  pinMode(pinRELE12, OUTPUT); // **Sin Asignar.
  pinMode(pinRELE13, OUTPUT); // **Sin Asignar.
  pinMode(pinRELE14, OUTPUT); // **Sin Asignar.
  //  pinMode(pinRELE15, OUTPUT); // **Sin Asignar.
  //  pinMode(pinRELE16, OUTPUT); // **Sin Asignar.

  //********* Protocolo de comunicación.
  receivedPackage.reserve(200);
}

void loop() {

  //Tomar parametros del cultivo.
  //  if (!obtenerParametros())
  //  {
  //    setParametrosDefault();
  //  }
  hsLuzParametro = 2.0; //horas

  humedadMaxParametro = 60.0; //%
  humedadMinParametro = 40.0; //%
  temperaturaAireMaxParametro = 30.0; //ºc
  temperaturaAireMinParametro = 15.0; //ºc
  co2MaxParametro = 600.0; //ppm
  co2MinParametro = 300.0; //ppm

  temperaturaAguaMaxParametro = 21.0; //ºc
  temperaturaAguaMinParametro = 18.0; //ºc
  PhParametro = 6.5;
  PHmaxParametro = PhParametro * 1.05;
  PHminParametro = PhParametro * 0.95;
  ceMaxParametro = 0.333;
  ceMinParametro = 0.111;

  if (!controlarNivelesPH())
  {
    //Algo falló.
  }

  if (!controlarNivelesTanques())
  {
    //Algo falló.
  }

  if (!analizarAgua())
  {
    //Algo falló.
  }

  if (!analizarAire())
  {
    //Algo falló.
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

  Serial.print("Tanque de agua limpia ");
  Serial.println(medicionNivelTanqueAguaLimpia);
  Serial.print("Tanque de agua descartada ");
  Serial.println(medicionNivelTanqueDesechable);
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

  delay(2000); //Se espera 2 segundos para seguir leyendo //datos
}

//************************************************************** FUNCIONES ************************************************************** FUNCIONES
//************************************************************** FUNCIONES ************************************************************** FUNCIONES
//************************************************************** FUNCIONES ************************************************************** FUNCIONES

//************************************************************** < FUNCIONES de encendido y apagado de dispositivos
//---------------------------------------------------------------------------------------------------------------//
//BombaEnfriadoPinRELE1
bool encenderEnfriador()
{
  digitalWrite(BombaEnfriadoPinRELE1, HIGH);
  return true;
}
bool apagarEnfriador()
{
  digitalWrite(BombaEnfriadoPinRELE1, LOW);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//BombaLlenadoPinRELE2
bool encenderLlenado()
{
  digitalWrite(BombaLlenadoPinRELE2, HIGH);
  return true;
}
bool apagarLlenado()
{
  digitalWrite(BombaLlenadoPinRELE2, LOW);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//BombaPrincipalPinRELE3
bool encenderBombaPrincipal()
{
  digitalWrite(BombaPrincipalPinRELE3, HIGH);
  return true;
}
bool apagarBombaPrincipal()
{
  digitalWrite(BombaPrincipalPinRELE3, LOW);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//BombaVaciadoPinRELE4
bool encenderBombaVaciado()
{
  digitalWrite(BombaVaciadoPinRELE4, HIGH);
  return true;
}
bool apagarBombaVaciado()
{
  digitalWrite(BombaVaciadoPinRELE4, LOW);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//NutrienteApinRELE5
bool encenderNutrientesA()
{
  digitalWrite(NutrienteApinRELE5, HIGH);
  return true;
}
bool apagarBombaNutrientesA()
{
  digitalWrite(NutrienteApinRELE5, LOW);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//NutrienteBpinRELE6
bool encenderNutrientesB()
{
  digitalWrite(NutrienteBpinRELE6, HIGH);
  return true;
}
bool apagarBombaNutrientesB()
{
  digitalWrite(NutrienteBpinRELE6, LOW);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//pHMasPinRELE7
bool encenderPHmas()
{
  digitalWrite(pHMasPinRELE7, HIGH);
  return true;
}
bool apagarPHmas()
{
  digitalWrite(pHMasPinRELE7, LOW);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//pHmenosPinRELE8
bool encenderPHmenos()
{
  digitalWrite(pHmenosPinRELE8, HIGH);
  return true;
}
bool apagarPHmenos()
{
  digitalWrite(pHmenosPinRELE8, LOW);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//LucesPinRELE9
bool encenderLuces()
{
  digitalWrite(LucesPinRELE9, HIGH);
  return true;
}
bool apagarLuces()
{
  digitalWrite(LucesPinRELE9, LOW);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//VentiladoresPinRELE10
bool encenderVentiladores()
{
  digitalWrite(VentiladoresPinRELE10, HIGH);
  return true;
}
bool apagarVentiladores()
{
  digitalWrite(VentiladoresPinRELE10, LOW);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//CalentadorPinRELE11
bool encenderCalentador()
{
  digitalWrite(CalentadorPinRELE11, HIGH);
  return true;
}
bool apagarCalentador()
{
  digitalWrite(CalentadorPinRELE11, LOW);
  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** FUNCIONES de encendido y apagado de dispositivos >
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** < FUNCIONES de medición
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

  if (strcmp(caso, "medicionNivelPHmas") == 0)
  {
    PinTrig = pinNivel5;
    PinEcho = pinNivel6;
    goto continua;
  }

  if (strcmp(caso, "medicionNivelPHmenos") == 0)
  {
    PinTrig = pinNivel7;
    PinEcho = pinNivel8;
    goto continua;
  }

  if (strcmp(caso, "medicionNivelTanqueAguaLimpia") == 0)
  {
    PinTrig = pinNivel9;
    PinEcho = pinNivel10;
    goto continua;
  }

  if (strcmp(caso, "medicionNivelTanqueDesechable") == 0)
  {
    PinTrig = pinNivel11;
    PinEcho = pinNivel12;
    goto continua;
  }

  if (strcmp(caso, "medicionNivelTanquePrincial") == 0)
  {
    PinTrig = pinNivel13;
    PinEcho = pinNivel14;
  }

continua:

  digitalWrite(PinTrig, LOW);
  delayMicroseconds(2);

  digitalWrite(PinTrig, HIGH);
  delayMicroseconds(10);

  float tiempo = pulseIn(PinEcho, HIGH);
  float distancia = (tiempo / 2) / 29.1;

  return distancia;
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
  return sensorDS18B20.getTempCByIndex(0);//Se lee la temperatura del agua.
}
//---------------------------------------------------------------------------------------------------------------//
float medirPH()
{
  int measure = analogRead(pinPH);//Se lee el pH.
  //si es 0 esta conectado pero apagado.
  //SERIAL_PRINT("pinPH:",measure);
  if (measure == 0)
    return 0.0;
  double voltage = 5 / 1024.0 * measure;
  return 7 + ((2.5 - voltage) / 0.18);
}
//---------------------------------------------------------------------------------------------------------------//
float medirCE()
{
  int sensorValue = 0;
  int outputValue = 0;
  sensorValue = analogRead(pinCE);
  //SERIAL_PRINT("pinCE:",sensorValue);
  outputValue = map(sensorValue, 0, 1023, 0, 5000);
  //SERIAL_PRINT("outputValue:",outputValue);
  return sensorValue * 5.00 / 1024;
}
//---------------------------------------------------------------------------------------------------------------//
float medirCO2()
{
  return gasSensor.getPPM();//Se mide el co2.
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
  VENTILAR = false;

  //------------------------------------------------------- Humedad
  medicionHumedad = medirHumedad();

  if (medicionHumedad == -10000)
  {
    medicionHumedad = 0.0;
    ERR_MEDICION_HUMEDAD = true;
    //return false;
  }
  else if (medicionHumedad < humedadMinParametro || medicionHumedad > humedadMaxParametro)
  { //Se requiere alguna accion.
    //encenderVentiladores();
    VENTILAR = true;
  }
  ERR_MEDICION_HUMEDAD = false;

  //-------------------------------------------------------TMP
  medicionTemperaturaAire = medirTemperatura();

  if (medicionTemperaturaAire == -10000)
  {
    medicionTemperaturaAire = 0.0;
    ERR_MEDICION_TEMPERATURA = true;
    //return false;
  }
  else if (medicionTemperaturaAire < temperaturaAireMinParametro || medicionTemperaturaAire > temperaturaAireMaxParametro)
  { //Se requiere alguna accion.
    //encenderVentiladores();
    VENTILAR = true;
  }
  ERR_MEDICION_TEMPERATURA = false;

  //-------------------------------------------------------CO2
  medicionCO2 = medirCO2();
  if (medicionCO2 > 10000)
  {
    medicionCO2 = 0.0;
    ERR_MEDICION_CO2 = true;
    //return false;
  }
  else  if (medicionCO2 < co2MinParametro || medicionCO2 > co2MaxParametro)
  { //Se requiere alguna accion.
    //encenderVentiladores();
    VENTILAR = true;
  }
  ERR_MEDICION_CO2 = false;

  return true;
}
//---------------------------------------------------------------------------------------------------------------//
bool analizarAgua()
{
  //-------------------------------------------------------TMP
  medicionTemperaturaAgua = medirTemperaturaAgua();

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
    { //Bajar temperatura.
      CMD_BAJAR_TEMPERATURA_AGUA = true;
      CMD_SUBIR_TEMPERATURA_AGUA = false;
    }
    else if (medicionTemperaturaAgua > temperaturaAguaMaxParametro)
    { //Subir temperaruta.
      CMD_BAJAR_TEMPERATURA_AGUA = false;
      CMD_SUBIR_TEMPERATURA_AGUA = true;
    }
  }
  else
  { //No hace nada.
    CMD_BAJAR_TEMPERATURA_AGUA = false;
    CMD_SUBIR_TEMPERATURA_AGUA = false;
  }
  ERR_MEDICION_TEMPERATURA_AGUA = false;

  //-------------------------------------------------------PH
  medicionPH = medirPH();

  if (medicionPH == 0 )
  { //Sensor desconectado o apagado.
    medicionPH = 0.0;
    ERR_MEDICION_PH = true;
    //return false;
  }
  else if (medicionPH < 0 )
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
    else if (medicionPH > PHmaxParametro)
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
  ERR_MEDICION_PH = false;

  //-------------------------------------------------------CE
  medicionCE = medirCE();

  if (medicionCE == 0 )
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
    else if (medicionPH > ceMaxParametro)
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
  ERR_MEDICION_CE = false;

  return true;
}
//---------------------------------------------------------------------------------------------------------------//
bool controlarNivelesPH()
{
  medicionNivelPHmas = medirNivel("medicionNivelPHmas");
  medicionNivelPHmenos = medirNivel("medicionNivelPHmenos");

  return true;
}
//---------------------------------------------------------------------------------------------------------------//
bool controlarNivelesNutrintes()
{
  medicionNivelNutrienteA = medirNivel("medicionNivelNutrie");
  medicionNivelNutrienteB = medirNivel("medicionNivelNutrienteB");

  return true;
}
//---------------------------------------------------------------------------------------------------------------//
bool controlarNivelesTanques()
{
  medicionNivelTanqueAguaLimpia = medirNivel("medicionNivelTanqueAguaLimpia");
  medicionNivelTanqueDesechable = medirNivel("medicionNivelTanqueDesechable");
  medicionNivelTanquePrincial = medirNivel("medicionNivelTanquePrincial");

  return true;
}
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** FUNCIONES de control >
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** < FUNCIONES para el protocolo de paquetes entre ARDUINO y ESP8266
void serialEvent() //Funcion que captura los eventos del puerto serial.
{
  static int state = 0;
  if (Serial.available()) {
    char inChar = (char)Serial.read();

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
    }
  }
}

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
  if (cmd > 0 && cmd < 9)
  {
    //if (cmd == 6) {  // unico comando proveniente desde la raspberry con datos;
    memcpy (data, payLoad + 2, (length - 2)*sizeof(char));
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
bool  checkPackageComplete(void)
{
  if (packageComplete)
  {
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
  }
  return false;
}
//---------------------------------------------------------------------------------------------------------------//
//************************************************************** FUNCIONES para el protocolo de paquetes entre ARDUINO y ESP8266 >
//---------------------------------------------------------------------------------------------------------------//
