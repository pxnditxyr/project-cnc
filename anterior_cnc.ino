#include <ESP32Time.h>
#include <Preferences.h>
#include <TFT_eSPI.h> // Graphics and font library for ILI9341 driver chip
#include <SPI.h>
#include <DHT.h>
ESP32Time rtc;
//Comandos para guardar datos en la memoria del ESP
Preferences dato_riego;
Preferences dato_fumigado;
TFT_eSPI tft = TFT_eSPI();  // Invoke library
//Botones de mando
#define btn_ON_riego 5
#define btn_ON_fumigado 13
#define btn_OFF 16
//Modulo rele
#define bmb_riego 32
#define bmb_fumigado 33
//Sensores DHT
#define DHTPIN1 12
#define DHTTYPE1 DHT11
#define DHTPIN3 14
#define DHTTYPE3 DHT11
#define DHTPIN4 27
#define DHTTYPE4 DHT11
//Sensores de humedad del suelo
#define HUM1 36       //A0
#define HUM2 39       //A3
//Sensores de flujo
#define FLJ1 34
#define FLJ2 35
//Driver motor paso a paso DVR825
#define enable_dvr 21
#define DIR 1
#define STEP 22
//Variables para control de motores
int steps_rev = 2500;
int aux = 0;
int i, j;
//Consumo de agua
volatile int NumPulsos; //variable para la cantidad de pulsos recibidos
float factor_conversion = 7.11; //para convertir de frecuencia a caudal
float volumen_riego = 0;
float volumen_fumigado = 0;
long dt = 0; //variación de tiempo por cada bucle
long t0 = 0; //millis() del bucle anterior
//Variables para crear un temporizador
int seg = 0;
int minn = 0;
int hora = 0;
//Volumen en litros de los tanques
const int tq1_2 = 10;
float porcentaje_tq_riego = 0;
float porcentaje_tq_fumigado = 0;
float volumen_tq_riego = 10;
float volumen_tq_fumigado = 10;
float volumen_tq_riego_2 = 0;
float volumen_tq_fumigado_2 = 0;
//Varaibles para el conteo de dias del riego y fumigado
String riego;
String fumigado;
//Varaibles para la recepcion de temperatura y humedad del invernadero
float t = 0;
float h = 0;
//Varaibles para la recepcion de humedad del suelo
float hum_porcentaje = 0;
//Variable booleana para el control del paro de emergencia
bool flag = false; 
//Funciones
void mover_motores_paso();
void ContarPulsos();
void pantalla();
int ObtenerFrecuecia();
float medicion_sf(int volumen);
float nivel_tanque(int litros);
void paro_de_emergencia();
void printLocalTime();
void func_riego();
void func_fumigado();
//Se inicializa los sensores DHT11
DHT dht1(DHTPIN1, DHTTYPE1);
DHT dht3(DHTPIN3, DHTTYPE3);
DHT dht4(DHTPIN4, DHTTYPE4);

void setup(void) {
  pinMode(enable_dvr, OUTPUT);
  digitalWrite(enable_dvr, HIGH);
  pinMode(bmb_riego, OUTPUT);
  digitalWrite(bmb_riego, HIGH);
  pinMode(bmb_fumigado, OUTPUT);
  digitalWrite(bmb_fumigado, HIGH);
  //AAAAAAAAAAAAAAAAAAAAA
  rtc.setTime(11, 55, 0,17, 5, 2023);
  //AAAAAAAAAAAAAAAAAAAAA
  dato_riego.begin("riego", false);
  dato_fumigado.begin("fumigado", false);
  pinMode(btn_ON_riego, INPUT_PULLUP);
  pinMode(btn_ON_fumigado, INPUT_PULLUP);
  pinMode(btn_OFF, INPUT_PULLUP);
  pinMode(DIR, OUTPUT);
  pinMode(STEP, OUTPUT);
  dht1.begin();
  dht3.begin();
  dht4.begin();
  pinMode(FLJ1, INPUT);
  pinMode(FLJ2, INPUT); 
  attachInterrupt(FLJ1,ContarPulsos,RISING);//(Interrupción sensor de flujo 1,función,Flanco de subida)
  attachInterrupt(FLJ2,ContarPulsos,RISING);//(Interrupción sensor de flujo 2,función,Flanco de subida)
  t0=millis();
  attachInterrupt(btn_OFF,paro_de_emergencia,RISING);//(Interrupción paro de emergencia,función,Flanco de subida)
  tft.init();
  tft.setRotation(1);
  //Comando para leer los datos almacenados
  dato_riego.getString("riego", riego);
  dato_fumigado.getString("fumigado", fumigado);
}

void loop() {
  //leemos los valores de temperetaura y humedad del invernadero
  float h3 = dht3.readHumidity();     //DHT 1
  float t3 = dht3.readTemperature();
  float h4 = dht4.readHumidity();     //DHT 2
  float t4 = dht4.readTemperature();
  h = h4;                //humedad promedio
  t = t4;              //temperatura promedio    
  //leemos el valor de los sensores de humedad del suelo
  int hum_suelo1 = analogRead(HUM1);
  int hum_suelo2 = analogRead(HUM2);
  //float hum_suelo3 = analogRead(HUM3);
  int hum = (hum_suelo1 + hum_suelo2)/2;
  hum_porcentaje = map(hum, 4095, 2031, 0, 100);
  //recibimos el nivel de tanque para el tanque 1 y 2
  volumen_tq_riego_2 = volumen_tq_riego - volumen_riego;
  porcentaje_tq_riego = (volumen_tq_riego_2*100)/volumen_tq_riego;
  riego = dato_riego.getString("riego", riego);
  pantalla();
  delay(1500);
  //RIEGO AUTOMATICO
  int hora_24 = rtc.getHour(true);
  if (hora_24 == 19 || hora_24 == 18 || hora_24 == 17){
    if(t > 15 && t < 18){
      if(hum > 0 && hum < 60){
        digitalWrite(bmb_riego, LOW);
        delay(1000);
        digitalWrite(enable_dvr, LOW);
        mover_motores_paso();
        float frecuencia_riego = ObtenerFrecuecia(); //obtenemos la frecuencia de los pulsos en Hz
        float caudal_L_m_riego = frecuencia_riego/factor_conversion; //calculamos el caudal en L/m
        dt = millis()-t0; //calculamos la variación de tiempo
        t0 = millis();
        volumen_riego = volumen_riego-(caudal_L_m_riego/60)*(dt/1000); // volumen(L)=caudal(L/s)*tiempo(s)
        volumen_tq_riego_2 = volumen_tq_riego - volumen_riego;
        porcentaje_tq_riego = (volumen_tq_riego_2*100)/volumen_tq_riego;
        volumen_tq_riego = volumen_tq_riego_2;
        digitalWrite(enable_dvr, HIGH);
        digitalWrite(bmb_riego, HIGH);
        fumigado = rtc.getTime("%d/%m/%Y %H:%M");
        dato_fumigado.putString("riego", riego);
      }
    }
  }
}
//--------Función fumigado---------------------------------
//--------------Función riego----------------------------
//---------Función paro de emergencia--------------------
void paro_de_emergencia(){
  digitalWrite(bmb_fumigado, HIGH);
  digitalWrite(bmb_riego, HIGH);
  digitalWrite(enable_dvr, HIGH);
}
//---Función para el temporizador------------------------
void temporizador(){
}
//---Función para mostrar los datos en el HMI------------
void pantalla(){
  // Se configura el fondo de la pantalla en color negro
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0, 2);
  // Se configura el texto del "titulo" en color amarillo
  tft.setTextColor(TFT_YELLOW); tft.setTextFont(4);
  tft.println(" Sistema semiautomatico de");
  tft.println("        Riego y Fumigacion");
  tft.println();
  // Se imprimen los datos de los sensoresy los valores almacenados en la memoria del ESP32
  tft.setTextFont(2);
  tft.setTextColor(TFT_BLUE); tft.print("Temp. del invernaero = "); 
  tft.setTextColor(TFT_ORANGE); tft.print(t); tft.println(" °C");  // Valor de temperatura
  tft.setTextColor(TFT_BLUE); tft.print("Humedad del suelo: "); 
  tft.setTextColor(TFT_ORANGE); tft.println(hum_porcentaje);  // Valor de humedad
  tft.setTextColor(TFT_BLUE); tft.print("Nivel tanque 1 = "); 
  tft.setTextColor(TFT_ORANGE); tft.print(porcentaje_tq_riego); tft.println(" %");  // Nivel del tanque 1
  tft.setTextColor(TFT_BLUE); tft.print("Nivel tanque 2 = "); 
  tft.setTextColor(TFT_ORANGE); tft.print(porcentaje_tq_fumigado); tft.println(" %");  // Nivel del tanque 2
  tft.setTextColor(TFT_BLUE); tft.print("Ultimo riego el "); 
  tft.setTextColor(TFT_ORANGE); tft.println(riego);  // Ultimo riego (fecha)
  tft.setTextColor(TFT_BLUE); tft.print("Ultimo fumigado el "); 
  tft.setTextColor(TFT_ORANGE); tft.println(fumigado);  // Ultimo fumigado (fecha)
  tft.setTextFont(2); tft.println();
}
//---Función para mover los motores----------------------
void mover_motores_paso(){
  digitalWrite(DIR, HIGH);
  for(i = 0; i<steps_rev; i++)
  {
    digitalWrite(STEP, HIGH);
    delayMicroseconds(1000);
    digitalWrite(STEP, LOW);
    delayMicroseconds(1000);
  }
  digitalWrite(DIR, LOW);
  for(j = 0; j<steps_rev; j++)
  {
    digitalWrite(STEP, HIGH);
    delayMicroseconds(1000);
    digitalWrite(STEP, LOW);
    delayMicroseconds(1000);
  }
}
//---Función que se ejecuta en interrupción---------------
void ContarPulsos (){ 
  NumPulsos++;  //incrementamos la variable de pulsos
} 
//---Función para obtener frecuencia de los pulsos--------
int ObtenerFrecuecia(){
  int frecuencia;
  NumPulsos = 0;   //Ponemos a 0 el número de pulsos
  interrupts();    //Habilitamos las interrupciones
  delay(1000);   //muestra de 1 segundo
  noInterrupts(); //Deshabilitamos  las interrupciones
  frecuencia=NumPulsos; //Hz(pulsos por segundo)
  return frecuencia;
}
