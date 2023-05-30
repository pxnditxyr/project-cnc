#include <ESP32Time.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <DHT.h>

ESP32Time rtc;

// DEFINICION DE PINES
//Modulo rele
#define PIN_BOMBA_RIEGO 32
#define PIN_BOMBA_FUMIGADO 33

//Sensores DHT
#define DHTPIN1 12
#define DHTTYPE1 DHT11

//Sensores de humedad del suelo
#define PIN_HUMEDAD_EX_IZQ 36       //A0
#define PIN_HUMEDAD_EX_DER 36       //A0

//Sensores de flujo
#define PIN_FLUJO 34

//Driver motor paso a paso DVR825
#define PIN_MOTOR_HABILITAR 21
#define PIN_MOTOR_DIR 1
#define PIN_MOTOR_PASOS 22

const int REVOLUCIONES_MOTOR = 2500;

// volatile int numero_pulsos_

void establecerPines () {
  // MOTOR
  pinMode( PIN_MOTOR_HABILITAR, OUTPUT );
  digitalWrite( PIN_MOTOR_HABILITAR, HIGH );
  // bombas
  pinMode( PIN_BOMBA_RIEGO, OUTPUT );
  digitalWrite( PIN_BOMBA_RIEGO, HIGH );
  pinMode( PIN_BOMBA_FUMIGADO, OUTPUT );
  digitalWrite( PIN_BOMBA_FUMIGADO, HIGH );
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

void setup () {
}

void loop () {

}
