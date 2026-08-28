#include <Servo.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HUSKYLENS.h"

//PANTALLA OLED
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

byte a = 0;
byte a_anterior = 255;

//CÁMARA HUSKYLENS
HUSKYLENS huskyLens;
const int ID_VERDE = 2; 
const int ID_ROJO  = 1; 

const int CENTROX = 160; 
const int AREAMIN = 600;  // Área mínima para considerar un cubo válido
float Kp_Camara = 1;   // Constante proporcional para centrar el cubo suavemente
bool e=true;
//FILTRO DE ULTRASÓNICOS 
const bool FiltroUS = 1;
const int buffersize = 7;
int puntero = 0;

// Variables sensores ultrasónicos 
const int echoI = 22; const int trigI = 23;
const int echoD = 24; const int trigD = 25;
const int echoF = 26; const int trigF = 27;

double duracionR, distanciaR;
double duracionL, distanciaL;
double duracionF, distanciaF;

// Buffers 
double dRbuffer[buffersize]={};
double dLbuffer[buffersize]={};
double dFbuffer[buffersize]={};
double sumdR;
double sumdL;
double sumdF;

const unsigned long TIMEOUT_US = 30000; 

// Variables L298N 
const int ENA = 2; 
const int IN1 = 4; 
const int IN2 = 3; 

// Variables servo y control PD 
Servo direccionServo;
const int pinServo = 9;
int centroServo = 95; 

// Constantes PID de Muros
double Kp = 3;   // funciona: 2.5
double Kd = 1.5; // funciona: 1.5 
int errorAnterior = 0;

// variables de estado 
// 0=PD Muros, 1=Girando Esquina, 2=Enfriamiento Esquina, 3=Acercando a Cubo, 4=Esquivando
int estadoRobot = 0; 
unsigned long tiempoInicioGiro = 0;
bool arranque = false;
unsigned long tiempoUltimoGiro = 0;

const unsigned long TIEMPO_BLOQUEO_GIRO = 750; 
const unsigned long TIEMPO_DURACION_GIRO = 2500; 

// variables 
const double distanciaEsquive = 15.0; // Distancia para iniciar maniobras
int idCuboObjetivo = 0;
unsigned long tiempoInicioEsquive = 0;

int contador = 0;
int giro = 0;
bool contactivado = 1;
unsigned long tiempocontador;

// Variables finales
unsigned long tiempoFinVuelta = 0;
bool terminandoVuelta = false;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  Serial.println("OLED intialized");
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 

  display.display();
  display.clearDisplay();
  display.display();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  
  direccionServo.attach(pinServo);
  direccionServo.write(centroServo);

  pinMode(trigI, OUTPUT); pinMode(echoI, INPUT);
  pinMode(trigD, OUTPUT); pinMode(echoD, INPUT);
  pinMode(trigF, OUTPUT); pinMode(echoF, INPUT);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  // Inicialización de la HuskyLens
  while (!huskyLens.begin(Wire)) {
    Serial.println(F("Falla al conectar HuskyLens... Reintentando"));
    delay(500);
  }
  huskyLens.writeAlgorithm(ALGORITHM_COLOR_RECOGNITION);

  for(int h = 0; h <= buffersize*2; h++){
    Ultrasonico();
  }
  Serial.println("Iniciando WRO 2026 - Fase Integrada...");
  delay(2000);
}

void loop() {
  
  // 1. Actualización de pantalla
  if (a != a_anterior) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("v: ");
    display.println(a);
    display.display();
    a_anterior = a;
  }
   
  // 2. Lógica de parada final
  if (contador >= 12 && !terminandoVuelta) {
    terminandoVuelta = true;
    tiempoFinVuelta = millis(); 
    Serial.println("Vuelta completada + 6 seg");
  }

  if (terminandoVuelta && (millis() - tiempoFinVuelta >= 6000)) { 
    parar();
    Serial.println("Tiempo final agotado. Carro detenido permanentemente.");
    while(1); 
  }
  
  if(contactivado == 0 && (millis()-tiempocontador) >= 8000){
   contactivado = 1; 
  }
  
  // 3. Lectura de sensores constante
  Ultrasonico();


  // Lectura de la camara Huskylens
 
  int idCuboDetectado = 0;
  int xCuboDetectado  = 0;
  
  if (huskyLens.request()) {
    int maxArea = 0;
    while (huskyLens.available()) {
      HUSKYLENSResult result = huskyLens.read();
      if (result.command == COMMAND_RETURN_BLOCK) {
        int areaActual = result.width * result.height;
        if (areaActual > maxArea && areaActual > AREAMIN) {
          maxArea = areaActual;
          idCuboDetectado = result.ID;
          xCuboDetectado  = result.xCenter;
         // Serial.println(areaActual);
        }
      }
    }
  }

  // --- 4. estados 
  switch (estadoRobot) {
        // estado 0: Avance y PD
=
    case 0: 
      // 1ro. Prioridad esquinas
      if (((distanciaR + distanciaL) > 95 && ((distanciaF > 50 && distanciaF < 60) || (distanciaF > 30 && distanciaF < 40))) && contactivado == 1) {
        arranque = false;
        if ((distanciaR > distanciaL && giro == 0) || giro == 1) {
          giro = 1;
          direccionServo.write(65); // Derecha
          Serial.println("INICIANDO GIRO DERECHA");
        } 
        else if ((distanciaL > distanciaR && giro == 0) || giro == 2) {
          giro = 2;
          direccionServo.write(125); // Izquierda
          Serial.println("INICIANDO GIRO IZQUIERDA");
        }
        
        estadoRobot = 1; 
        tiempoInicioGiro = millis();
        contador++;
        a++;
        contactivado = 0;
        tiempocontador = millis();
        e=false;

      } 
      // 2do. Prioridad cámara: si se detecta un cubo cambia el estado
      else if (idCuboDetectado != 0 && e == true) {
        idCuboObjetivo = idCuboDetectado;
        estadoRobot = 3; // Pasamos al estado de Acercamiento
        Serial.println("Cubo Detectado -> Cambiando a Estado 3 (Acercamiento)");
      }
      // 3ro. Si no hay nada, hacemos control PD normal
      else {
        if (arranque == true) {
          Avan(110); 
          direccionServo.write(centroServo);
        } else {
          if (distanciaF >= 60 && distanciaF < 90) {
            Avan(110); 
            direccionServo.write(centroServo);
          } else {
            Avan(110); 
            int error = distanciaL - distanciaR;
            float P = error;
            float D = error - errorAnterior;
            
            int ajuste = (Kp * P) + (Kd * D);
            int nuevoAngulo = centroServo + ajuste; 
            
            if (nuevoAngulo > 115) nuevoAngulo = 115;
            if (nuevoAngulo < 75) nuevoAngulo = 75;
            
            direccionServo.write(nuevoAngulo);
            errorAnterior = error;
          }
        }
      }
      break;


    // Estado 1: Giro en esquina

    case 1:
      Avan(110);
      if ((millis() - tiempoInicioGiro >= TIEMPO_DURACION_GIRO) || distanciaF > 170) {
        Serial.println("FIN DEL GIRO. Entrando a enfriamiento.");
      
        delay(500); 
        if(distanciaF > 30 && distanciaF < 40){ delay(250); }
        Ultrasonico();
        estadoRobot = 2; 
        tiempoUltimoGiro = millis();
      }
      break;

    
    // Estado 2: avanzar al frente 
   
    case 2:
      direccionServo.write(centroServo); 
      Avan(110);
      if (millis() - tiempoUltimoGiro >= TIEMPO_BLOQUEO_GIRO) {
        Serial.println("avanzar y regresar al PD");
        estadoRobot = 0; 
        errorAnterior = 0; 
        e=true;
      }
      break;

 
    // Estado 3: acercarse al cubo.

    case 3:
      Avan(110);
      
      //iniciamos el esquive 
      if (distanciaF <= distanciaEsquive) {
        Serial.println("Distancia alcanzada. Iniciando ESQUIVE (Estado 4)");
        estadoRobot = 4;
        tiempoInicioEsquive = millis();
      } 
      // pd para central el cubo utilizando la cámara
      else if (idCuboDetectado != 0) {
        // Cálculo del error: CENTRO (160) - Posición del Cubo
        // Si el cubo está en 100 (Izquierda), el error es +60.
        int errorCamara = CENTROX - xCuboDetectado; 
        
        int ajusteCamara = errorCamara * Kp_Camara;
        int nuevoAngulo = centroServo + ajusteCamara;
        
        // limitar giros bruscos
        if (nuevoAngulo > 115) nuevoAngulo = 115;
        if (nuevoAngulo < 75) nuevoAngulo = 75;
        
        direccionServo.write(nuevoAngulo);
      } 
      // al perder el cubo volver a PD
      else {
        Serial.println("Cubo perdido. Volviendo a Estado 0");
        estadoRobot = 0;
        errorAnterior = 0;
      }
      break;


    // Estado 4: Maniobra de esquive de cubos

    case 4:
      Avan(110);
      unsigned long tiempoEsquivando = millis() - tiempoInicioEsquive;
      
      if (idCuboObjetivo == ID_VERDE) {
        // Rojo -> Pasar por la Izquierda (Giro agresivo Izquierda, luego Derecha)
        if (tiempoEsquivando < 1300) {
          direccionServo.write(125); // Giro Fuerte Izquierda
        } 
        else if (tiempoEsquivando < 2300) {
          direccionServo.write(70);
          delay(200);
          direccionServo.write(centroServo);  // recto
        } 
        else if (tiempoEsquivando < 3300) {
          direccionServo.write(70);  // Contravolante para enderezar
        }
        else {
          estadoRobot = 0;
          errorAnterior = 0; // Para que el PD de muros inicie
          Serial.println("Fin de Esquive VERDE. Volviendo al PD.");
        }
      } 
      else if (idCuboObjetivo == ID_ROJO) {
        // Verde -> Pasar por la Derecha (Giro agresivo Derecha, luego Izquierda)
        if (tiempoEsquivando < 1300) {
          direccionServo.write(65);  // Giro fuerte a la derecha
        } 
        else if (tiempoEsquivando < 2300) {
          direccionServo.write(120);
          delay(200);
          direccionServo.write(centroServo);  // recto
        }
        else if (tiempoEsquivando < 3300) {
          direccionServo.write(120); // enderezar
        } 
        else {
          estadoRobot = 0;
          errorAnterior = 0; // Ayuda al PD de muros iniciar 
          Serial.println("Fin de Esquive ROJO. Volviendo al PD.");
        }
      }
      break;
  }
}

// Funciones de movimiento
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

// Función de sensores ultrasónicos 
void Ultrasonico() {
  // Lado Derecho
  digitalWrite(trigD, LOW); delayMicroseconds(2);
  digitalWrite(trigD, HIGH); delayMicroseconds(10);
  digitalWrite(trigD, LOW);
  duracionR = pulseIn(echoD, HIGH, TIMEOUT_US);
  distanciaR = (duracionR == 0) ? 155 : duracionR * 0.034 / 2; 
  
  if(FiltroUS == 1){
    sumdR -= dRbuffer[puntero];
    sumdR += distanciaR;
    dRbuffer[puntero] = distanciaR;
    distanciaR = sumdR/buffersize;
  }
  if(distanciaR >= 150) distanciaR = 150; 

  // Lado Izquierdo
  digitalWrite(trigI, LOW); delayMicroseconds(2);
  digitalWrite(trigI, HIGH); delayMicroseconds(10);
  digitalWrite(trigI, LOW);
  duracionL = pulseIn(echoI, HIGH, TIMEOUT_US);
  distanciaL = (duracionL == 0) ? 155 : duracionL * 0.034 / 2;

  if(FiltroUS == 1){
    sumdL -= dLbuffer[puntero];
    sumdL += distanciaL;
    dLbuffer[puntero] = distanciaL;
    distanciaL = sumdL/buffersize;
  }
  if(distanciaL >= 150) distanciaL = 150; 

  // Frente
  digitalWrite(trigF, LOW); delayMicroseconds(2);
  digitalWrite(trigF, HIGH); delayMicroseconds(10);
  digitalWrite(trigF, LOW);
  duracionF = pulseIn(echoF, HIGH, TIMEOUT_US);
  distanciaF = (duracionF == 0) ? 90 : duracionF * 0.034 / 2;

  // Comentado para no saturar 
  // Serial.print("Izquierda: "); Serial.print(distanciaL);
  // Serial.print(" cm  |  Frente: "); Serial.print(distanciaF);
  // Serial.print(" cm  |  Derecha: "); Serial.print(distanciaR);
  // Serial.println(" cm");

  if(FiltroUS == 1){
    puntero ++;
    puntero %= buffersize;
  }
}