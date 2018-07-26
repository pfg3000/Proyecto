#include "DHT.h" //Cargamos la librería DHT (temperatura y humedad)
#include <OneWire.h>
#include <DallasTemperature.h>
#include "MQ135.h" //Cargamos la librería MQ135

//********************************** VARIOS ************************************************
#define SALIDASERIAL 1 //Muestra o no, los resultados en el puerto serial. 1 si 0 no.

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

//Variables Medicion de PH
float medicionPH = 0.0; //Valor medido
float PhParametro = 0.0;
float PHmaxParametro = 0.0;
float PHminParametro = 0.0;
float medicionNivelPHmas = 0.0; //Valor medido
float medicionNivelPHmenos = 0.0; //Valor medido

//Variables para medicion de humedad del aire
float medicionHumedad = 0.0; //Valor medido
float humedadMaxParametro = 0.0;
float humedadMinParametro = 0.0;

//Variables para medicion de temperatura del aire
float medicionTemperaturaAire = 0.0; //Valor medido
float temperaturaAireMaxParametro = 0.0;
float temperaturaAireMinParametro = 0.0;

//Variables para medicion de temperatura del agua
float medicionTemperaturaAgua = 0.0; //Valor medido
float TempAguaMaxParametro = 0.0;
float TempAguaMinParametro = 0.0;

//Variables para la medicion de co2
float medicionCO2 = 0.0; //Valor medido
float co2MaxParametro = 0.0;
float co2MinParametro = 0.0;

//Variables para la medicion de ce
float medicionNivelNutrienteA = 0.0; //Valor medido
float medicionNivelNutrienteB = 0.0; //Valor medido
float medicionCE = 0.0; //Valor medido
float ceMaxParametro = 0.0;
float ceMinParametro = 0.0;

//Variables para la medicion de los niveles de los tanques de agua
float medicionNivelTanquePrincial = 0.0; //Valor medido
float medicionNivelTanqueAguaLimpia = 0.0; //Valor medido
float medicionNivelTanqueDesechable = 0.0; //Valor medido

//***********************************************************************************  VARIABLES PARA MEDICIONES >

void setup() {
  if (SALIDASERIAL == 1)
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

}

void loop() {
  float h = dht.readHumidity(); //Se lee la humedad del aire.
  float t = dht.readTemperature(); //Se lee la temperatura del aire.
//
//  float n1 = analogRead(pinNivel1);//Se lee nivel de nutrientes A.
//  float n2 = analogRead(pinNivel2);//Se lee nivel de nutrientes B.
//  float n3 = analogRead(pinNivel3);//Se lee nivel de nutrientes C.
//  float n4 = analogRead(pinNivel4);//Se lee nivel de pH+.
//  float n5 = analogRead(pinNivel5);//Se lee nivel de pH-.

  sensorDS18B20.requestTemperatures();
  float wt = sensorDS18B20.getTempCByIndex(0);//Se lee la temperatura del agua.

  int measure = analogRead(pinPH);//Se lee el pH.
  double voltage = 5 / 1024.0 * measure;
  float ph = 7 + ((2.5 - voltage) / 0.18);

//  float n6 = digitalRead(pinNivel6);//Se lee nivel de agua limpia.
//  float n7 = digitalRead(pinNivel7);//Se lee nivel de agua descartada.
//  float n8 = digitalRead(pinNivel8);//Se lee nivel de tanque principal lleno.
//  float n9 = digitalRead(pinNivel9);//Se lee nivel de tanque principal vacio.

  float co2 = gasSensor.getPPM();//Se mide el co2.
  float rzero = gasSensor.getRZero();//Se mide el rzero para calibrar el co2.

  int sensorValue = 0;
  int outputValue = 0;
  sensorValue = analogRead(pinCE);
  outputValue = map(sensorValue, 0, 1023, 0, 5000);

  if (SALIDASERIAL == 1)
  {
    //Se imprimen las variables
    Serial.println("Humedad: ");
    Serial.println(h);
    Serial.println("Temperatura: ");
    Serial.println(t);

//    Serial.println("Nutrientes A: ");
//    Serial.println(n1);
//    Serial.println("Nutrientes B: ");
//    Serial.println(n2);
//    Serial.println("Nutrientes C: ");
//    Serial.println(n3);
//
//    Serial.println("pH+: ");
//    Serial.println(n4);
//    Serial.println("pH-: ");
//    Serial.println(n5);

    Serial.print("Temperatura agua: ");
    Serial.print(wt);
    Serial.println(" ºC");

    Serial.print("PH: ");
    Serial.print(ph, 3);
    Serial.println("");

//    Serial.print("Tanque de agua limpia ");
//    if (n6 == LOW)
//      Serial.print("No Vacio");
//    else
//      Serial.print("Vacio");
//    Serial.println("");
//
//    Serial.print("Tanque de agua descartada ");
//    if (n7 == LOW)
//      Serial.print("No Lleno");
//    else
//      Serial.print("Lleno");
//    Serial.println("");
//
//    Serial.print("Tanque de agua principal ");
//    if (n8 == LOW)
//      Serial.print("No Lleno");
//    else
//      Serial.print("Lleno");
//    Serial.print("\t");
//    if (n9 == LOW)
//      Serial.print("No Vacio");
//    else
//      Serial.print("Vacio");
//    Serial.println("");

    Serial.println("CO2: ");
    Serial.println(co2);
    Serial.print(" ppm");
    //    Serial.println(": ");
    //    Serial.println(rzero);

    Serial.println("");

    Serial.print("sensor=");
    Serial.print(sensorValue);
    Serial.print("\t output=");
    Serial.println(analogRead(pinCE) * 5.00 / 1024, 2);

    Serial.println("");
  }

  delay(2000); //Se espera 2 segundos para seguir leyendo //datos
}

//************************************************************** FUNCIONES ************************************************************** FUNCIONES
//************************************************************** FUNCIONES ************************************************************** FUNCIONES
//************************************************************** FUNCIONES ************************************************************** FUNCIONES

//************************************************************** < FUNCIONES de encendido y apagado de dispositivos

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
//************************************************************** FUNCIONES de encendido y apagado de dispositivos >
