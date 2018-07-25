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
#define pinNivel1 A3 //Seleccionamos el pin en el que se conectará el sensor de nivel de nutrientes A.
#define pinNivel2 A4 //Seleccionamos el pin en el que se conectará el sensor de nivel de nutrientes B.
#define pinNivel3 A5 //Seleccionamos el pin en el que se conectará el sensor de nivel de nutrientes C.
#define pinNivel4 A6 //Seleccionamos el pin en el que se conectará el sensor de nivel de pH+.
#define pinNivel5 A7 //Seleccionamos el pin en el que se conectará el sensor de nivel de pH-.

//************************* Niveles de liquido *********************************************
#define pinNivel6 25 //Seleccionamos el pin en el que se conectará el sensor de nivel de agua limpia.
#define pinNivel7 26 //Seleccionamos el pin en el que se conectará el sensor de nivel de agua descartada.
#define pinNivel8 27 //Seleccionamos el pin en el que se conectará el sensor de nivel de tanque principal lleno.
#define pinNivel9 28 //Seleccionamos el pin en el que se conectará el sensor de nivel de tanque principal vacio.

//************************** Temperatura y humedad del aire ********************************
#define DHTTYPE DHT22 //Se selecciona el tipo de sensor medidor de temperatura DHT22.
#define pinDHT 22 //Seleccionamos el pin en el que se conectará el sensor DHT22.
DHT dht(pinDHT, DHTTYPE); //Se inicia una variable que será usada por Arduino para comunicarse con el sensor.

//*********************************** ACTUADORES ***************************************** 16 relé
#define BombaEnfriadoPinRELE1 30 //Seleccionamos el pin en el que se conectará Bomba de agua Enfriado.
#define BombaLlenadoPinRELE2 31 //Seleccionamos el pin en el que se conectará Bomba de agua Llenado.
#define BombaPrincipalPinRELE3 32 //Seleccionamos el pin en el que se conectará Bomba de agua Principal.
#define BombaVaciadoPinRELE4 33 //Seleccionamos el pin en el que se conectará Bomba de agua Vaciado.
#define NutrienteApinRELE5 34 //Seleccionamos el pin en el que se conectará Bomba peristáltica Nutriente A.
#define NutrienteBpinRELE6 35 //Seleccionamos el pin en el que se conectará Bomba peristáltica Nutriente B.
#define NutrienteCpinRELE7 36 //Seleccionamos el pin en el que se conectará Bomba peristáltica Nutriente C.
#define pHMasPinRELE8 37 //Seleccionamos el pin en el que se conectará Bomba peristáltica pH+.
#define pHmenosPinRELE9 38 //Seleccionamos el pin en el que se conectará Bomba peristáltica pH-.
#define LucesPinRELE10 39 //Seleccionamos el pin en el que se conectará Luces Led.
#define VentiladoresPinRELE11 40 //Seleccionamos el pin en el que se conectará Ventiladores.
#define CalentadorPinRELE12 41 //Seleccionamos el pin en el que se conectará Calentador.
#define pinRELE13 42 //Seleccionamos el pin en el que se conectará **Sin Asignar.
#define pinRELE14 43 //Seleccionamos el pin en el que se conectará **Sin Asignar.
#define pinRELE15 44 //Seleccionamos el pin en el que se conectará **Sin Asignar.
#define pinRELE16 45 //Seleccionamos el pin en el que se conectará **Sin Asignar.

//************************** Temperatura del agua *****************************************
const int pinDS18B20 = 28; //Seleccionamos el pin en el que se conectará el sensor DS18B20.
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
float ceMax = 0.0
float ceMin = 0.0

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

  //********* ENTRADAS
  pinMode(pinNivel1, INPUT); //nivel de nutrientes A.
  pinMode(pinNivel2, INPUT); //nivel de nutrientes B.
  pinMode(pinNivel3, INPUT); //nivel de nutrientes C.
  pinMode(pinNivel4, INPUT); //nivel de ph+.
  pinMode(pinNivel5, INPUT); //nivel de ph-.
  pinMode(pinNivel6, INPUT); //agua limpia.
  pinMode(pinNivel7, INPUT); //agua descartada.
  pinMode(pinNivel8, INPUT); //tanque principal lleno.
  pinMode(pinNivel9, INPUT); //tanque principal vacio.
  pinMode(pinPH, INPUT); //ph.
  pinMode(pinCE, INPUT); //ce.
  pinMode(pinCO2, INPUT); //co2.
  pinMode(pinDHT, INPUT); //temperatura y humedad del aire.
  pinMode(pinDS18B20, INPUT); //temperatura del agua.

  //********* SALIDAS
  pinMode(BombaEnfriadoPinRELE1, OUTPUT); //Bomba de agua Enfriado.
  pinMode(BombaLlenadoPinRELE2, OUTPUT); //Bomba de agua Llenado.
  pinMode(BombaPrincipalPinRELE3, OUTPUT); //Bomba de agua Principal.
  pinMode(BombaVaciadoPinRELE4, OUTPUT); //Bomba de agua Vaciado.
  pinMode(NutrienteApinRELE5, OUTPUT); //Bomba peristáltica Nutriente A.
  pinMode(NutrienteBpinRELE6, OUTPUT); //Bomba peristáltica Nutriente B.
  pinMode(NutrienteCpinRELE7, OUTPUT); //Bomba peristáltica Nutriente C.
  pinMode(pHMasPinRELE8, OUTPUT); //Bomba peristáltica pH+.
  pinMode(pHmenosPinRELE9, OUTPUT); //Bomba peristáltica pH-.
  pinMode(LucesPinRELE10, OUTPUT); //Luces Led.
  pinMode(VentiladoresPinRELE11, OUTPUT); //Ventiladores.
  pinMode(CalentadorPinRELE12, OUTPUT); //Calentador.
  pinMode(pinRELE13, OUTPUT); // **Sin Asignar.
  pinMode(pinRELE14, OUTPUT); // **Sin Asignar.
  pinMode(pinRELE15, OUTPUT); // **Sin Asignar.
  pinMode(pinRELE16, OUTPUT); // **Sin Asignar.

}

void loop() {
  float h = dht.readHumidity(); //Se lee la humedad del aire.
  float t = dht.readTemperature(); //Se lee la temperatura del aire.

  float n1 = analogRead(pinNivel1);//Se lee nivel de nutrientes A.
  float n2 = analogRead(pinNivel2);//Se lee nivel de nutrientes B.
  float n3 = analogRead(pinNivel3);//Se lee nivel de nutrientes C.
  float n4 = analogRead(pinNivel4);//Se lee nivel de pH+.
  float n5 = analogRead(pinNivel5);//Se lee nivel de pH-.

  sensorDS18B20.requestTemperatures();
  float wt = sensorDS18B20.getTempCByIndex(0);//Se lee la temperatura del agua.

  int measure = analogRead(pinPH);//Se lee el pH.
  double voltage = 5 / 1024.0 * measure;
  float ph = 7 + ((2.5 - voltage) / 0.18);

  float n6 = digitalRead(pinNivel6);//Se lee nivel de agua limpia.
  float n7 = digitalRead(pinNivel7);//Se lee nivel de agua descartada.
  float n8 = digitalRead(pinNivel8);//Se lee nivel de tanque principal lleno.
  float n9 = digitalRead(pinNivel9);//Se lee nivel de tanque principal vacio.

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

    Serial.println("Nutrientes A: ");
    Serial.println(n1);
    Serial.println("Nutrientes B: ");
    Serial.println(n2);
    Serial.println("Nutrientes C: ");
    Serial.println(n3);

    Serial.println("pH+: ");
    Serial.println(n4);
    Serial.println("pH-: ");
    Serial.println(n5);

    Serial.print("Temperatura agua: ");
    Serial.print(wt);
    Serial.println(" ºC");

    Serial.print("PH: ");
    Serial.print(ph, 3);
    Serial.println("");

    Serial.print("Tanque de agua limpia ");
    if (n6 == LOW)
      Serial.print("No Vacio");
    else
      Serial.print("Vacio");
    Serial.println("");

    Serial.print("Tanque de agua descartada ");
    if (n7 == LOW)
      Serial.print("No Lleno");
    else
      Serial.print("Lleno");
    Serial.println("");

    Serial.print("Tanque de agua principal ");
    if (n8 == LOW)
      Serial.print("No Lleno");
    else
      Serial.print("Lleno");
    Serial.print("\t");
    if (n9 == LOW)
      Serial.print("No Vacio");
    else
      Serial.print("Vacio");
    Serial.println("");

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

//NutrienteCpinRELE7
bool encenderNutrientesC()
{
  digitalWrite(NutrienteCpinRELE7, HIGH);
  return true;
}
bool apagarBombaNutrientesC()
{
  digitalWrite(NutrienteCpinRELE7, LOW);
  return true;
}

//pHMasPinRELE8
bool encenderPHmas()
{
  digitalWrite(pHMasPinRELE8, HIGH);
  return true;
}
bool apagarPHmas()
{
  digitalWrite(pHMasPinRELE8, LOW);
  return true;
}

//pHmenosPinRELE9
bool encenderPHmenos()
{
  digitalWrite(pHmenosPinRELE9, HIGH);
  return true;
}
bool apagarPHmenos()
{
  digitalWrite(pHmenosPinRELE9, LOW);
  return true;
}

//LucesPinRELE10
bool encenderLuces()
{
  digitalWrite(LucesPinRELE10, HIGH);
  return true;
}
bool apagarLuces()
{
  digitalWrite(LucesPinRELE10, LOW);
  return true;
}

//VentiladoresPinRELE11
bool encenderVentiladores()
{
  digitalWrite(VentiladoresPinRELE11, HIGH);
  return true;
}
bool apagarVentiladores()
{
  digitalWrite(VentiladoresPinRELE11, LOW);
  return true;
}

//CalentadorPinRELE12
bool encenderCalentador()
{
  digitalWrite(CalentadorPinRELE12, HIGH);
  return true;
}
bool apagarCalentador()
{
  digitalWrite(CalentadorPinRELE12, LOW);
  return true;
}
//************************************************************** FUNCIONES de encendido y apagado de dispositivos >
