#include "HUSKYLENS.h"
#include "Wire.h"

HUSKYLENS huskyLens;
const int ID_VERDE = 1; 
const int ID_ROJO  = 2; 

//CONFIGURACIÓN DE PANTALLA 
const int CENTROX = 160; 
const int MARGENC = 30;   // Zona de centrado 
const int AREAMIN = 400;  
const int AREAC   = 7000; 

//sensor de color
#define S0 10
#define S1 11
#define S2 12
#define S3 13
#define sensorOut A0

int redPW = 0;
int greenPW = 0;
int bluePW = 0;

const int echoD = 24;
const int trigD = 25;

const int echoI = 22;
const int trigI = 23;

const int echoF = A4;
const int trigF = A5; 

long duracionD, distanciaD;
long duracionI, distanciaI;
long duracionF, distanciaF;

const int ENA = 5; // PWM
const int IN1 = 3; 
const int IN2 = 4; 

const int pinServo = 9;
int anguloActual   = 90; 
int anguloAnterior = 90; // Memoria de dirección 

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);

  // Frecuencia al 20%
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  pinMode(pinServo, OUTPUT);
  pinMode(trigD, OUTPUT); pinMode(echoD, INPUT);
  pinMode(trigI, OUTPUT); pinMode(echoI, INPUT);
  pinMode(trigF, OUTPUT); pinMode(echoF, INPUT);
  pinMode(ENA, OUTPUT);   pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);

  while (!huskyLens.begin(Wire)) {
    Serial.println(F("Falla al conectar HuskyLens"));
    delay(500);
  }
  huskyLens.writeAlgorithm(ALGORITHM_COLOR_RECOGNITION);

  // Posición inicial
  moverServo(90);
  parar();
  Serial.println(F("SISTEMA OK"));
}

void loop() {
  leerUltrasonicos();
  readRGB();

//sensor de color
  if (isOrange()) {
    Serial.println(F(" linea NARANJA-> Giro derecha"));
    parar();
    moverServo(40); 
    anguloAnterior = 40;
    delay(300);
    Avan(200);
    delay(1200); 
    centrarServoCalibrado();
    return; 
  } 
  else if (isBlue()) {
    Serial.println(F("linea AZUL-> Giro Izquierda"));
    parar();
    moverServo(150); 
    anguloAnterior = 150;
    delay(300);
    Avan(200);
    delay(1200);
    centrarServoCalibrado();
    return;
  }

  if (huskyLens.request()) {
    int maxArea = 0;
    int idCuboMasCercano = 0;
    int xCuboMasCercano  = 0;

    while (huskyLens.available()) {
      HUSKYLENSResult result = huskyLens.read();
      if (result.command == COMMAND_RETURN_BLOCK) {
        int areaActual = result.width * result.height;
        if (areaActual > maxArea && areaActual > AREAMIN) {
          maxArea          = areaActual;
          idCuboMasCercano = result.ID;
          xCuboMasCercano  = result.xCenter;
        }
      }
    }

    if (idCuboMasCercano != 0) {
      if (xCuboMasCercano < (CENTROX - MARGENC)) {
        Serial.println(F("Corregir a IZQUIERDA"));
        moverServo(150); 
        anguloAnterior = 150;
        Avan(180);
      } 
      else if (xCuboMasCercano > (CENTROX + MARGENC)) {
        Serial.println(F("Corregir a DERECHA"));
        moverServo(40); 
        anguloAnterior = 40;
        Avan(180);
      } 
      else {
        Serial.println(F("Alineado. Acercando con Ultrasónico"));
        centrarServoCalibrado();
        acercarConUltrasonico(idCuboMasCercano);
      }
      return; 
    }
  }

  if (distanciaF > 0 && distanciaF <= 30) {
    Serial.print(F("PRIO 3 [ULTRA] -> Distancia Frente: "));
    Serial.print(distanciaF);
    Serial.println(F(" cm"));

    if (distanciaF <= 12) {
      parar();
      delay(300);
    } else {
      Avan(150);
    }
  } 
  else {
    if ((distanciaD + distanciaI) <= 97) {
      Avan(255);
      centrarServoCalibrado();
    } 
    else if ((distanciaD + distanciaI) > 130) {
      if (distanciaD > distanciaI) {
        moverServo(40);
        anguloAnterior = 40;
        delay(3000);
      } else if (distanciaD < distanciaI) {
        moverServo(150);
        anguloAnterior = 150;
        delay(3000);
      }
    } 
    else {
      parar();
    }
  }

  delay(30);
}

void readRGB() {
  //Rojo
  digitalWrite(S2, LOW); digitalWrite(S3, LOW);
  redPW = pulseIn(sensorOut, LOW, 10000);

  //Verde
  digitalWrite(S2, HIGH); digitalWrite(S3, HIGH);
  greenPW = pulseIn(sensorOut, LOW, 10000);

  //Azul
  digitalWrite(S2, LOW); digitalWrite(S3, HIGH);
  bluePW = pulseIn(sensorOut, LOW, 10000);
}

bool isOrange() {
  return (redPW > 0 && redPW < 120 && greenPW < 180 && bluePW > redPW + 40);
}

bool isBlue() {
  return (bluePW > 0 && bluePW < redPW && bluePW < greenPW && bluePW < 150);
}

void acercarConUltrasonico(int colorID) {
  if (distanciaF > 12) {
    Avan(170);
  } else {
    parar();
    if (colorID == ID_VERDE) {
      Serial.println(F("Cubo VERDE -> Esquive por la DERECHA"));
      moverServo(40);
      anguloAnterior = 40;
      delay(300);
      Avan(200);
      delay(1500);
      centrarServoCalibrado();
    } else if (colorID == ID_ROJO) {
      Serial.println(F("Cubo ROJO -> Esquive por la IZQUIERDA"));
      moverServo(150);
      anguloAnterior = 150;
      delay(300);
      Avan(200);
      delay(1500);
      centrarServoCalibrado();
    }
    parar();
    delay(500);
  }
}

void centrarServoCalibrado() {
  if (anguloAnterior != 90) {
    if (anguloAnterior == 40) {
      moverServo(100);
    } else if (anguloAnterior == 150) {
      moverServo(90);
    }
    anguloAnterior = 90;
  }
}

void moverServo(int angulo) {
  if (angulo == anguloActual) return;
  
  int pulsoMicrosegundos = 1000 + (angulo * 1000L / 180L);
  for (int i = 0; i < 20; i++) {
    digitalWrite(pinServo, HIGH);
    delayMicroseconds(pulsoMicrosegundos);
    digitalWrite(pinServo, LOW);
    delayMicroseconds(20000 - pulsoMicrosegundos); 
  }
  anguloActual = angulo;
}

void leerUltrasonicos() {
  digitalWrite(trigD, LOW); delayMicroseconds(2);
  digitalWrite(trigD, HIGH); delayMicroseconds(10);
  digitalWrite(trigD, LOW);
  duracionD = pulseIn(echoD, HIGH, 25000);
  distanciaD = (duracionD == 0) ? 999 : (duracionD * 0.034 / 2);

  digitalWrite(trigI, LOW); delayMicroseconds(2);
  digitalWrite(trigI, HIGH); delayMicroseconds(10);
  digitalWrite(trigI, LOW);
  duracionI = pulseIn(echoI, HIGH, 25000);
  distanciaI = (duracionI == 0) ? 999 : (duracionI * 0.034 / 2);

  digitalWrite(trigF, LOW); delayMicroseconds(2);
  digitalWrite(trigF, HIGH); delayMicroseconds(10);
  digitalWrite(trigF, LOW);
  duracionF = pulseIn(echoF, HIGH, 25000);
  distanciaF = (duracionF == 0) ? 999 : (duracionF * 0.034 / 2);
}

void Avan(int velocidad) {
  analogWrite(ENA, velocidad);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void retro(int velocidad) {
  analogWrite(ENA, velocidad);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
}

void parar() {
  analogWrite(ENA, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}
