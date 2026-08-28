#include <Servo.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

byte a = 0;
byte a_anterior = 255;

// Filtro para los ultrasonicos | 0=apagado | 1= encendido |
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

// Constantes PID 
double Kp = 3; //funciona: 3 y 2.5
double Kd = 1.5; //funciona: 1.5 
int errorAnterior = 0;

// variables para los estados
int estadoRobot = 0; // 0 = PD/Recto, 1 = Girando, 2 = Enfriamiento
unsigned long tiempoInicioGiro = 0;
bool arranque = true;
unsigned long tiempoUltimoGiro = 0;
const unsigned long TIEMPO_BLOQUEO_GIRO = 750; 
const unsigned long TIEMPO_DURACION_GIRO = 2500; 

int contador = 0;
int giro = 0;
bool contactivado = 1;
unsigned long tiempocontador;


unsigned long tiempoFinVuelta = 0;
bool terminandoVuelta = false;

void setup() {
  Serial.begin(9600);
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

for(int h = 0; h <= buffersize*2; h++){
  Ultrasonico();
}
  Serial.println("Iniciando WRO 2026...");
  delay(2000);
}

void loop() {
  
  // 1. Actualización de Pantalla
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
    Serial.println("Vuelta completada! Iniciando 6 segundos extra...");
  }

  if (terminandoVuelta && (millis() - tiempoFinVuelta >= 6000)) { //6000
    parar();
    Serial.println("Tiempo final agotado. Carro detenido permanentemente.");
    while(1); 
  }
  
  if(contactivado == 0 && (millis()-tiempocontador) >= 8000){
   contactivado = 1; 
   
  }
  // 3. Lectura de sensores
  Ultrasonico();
 /* if (distanciaR<5)
  {
    parar();
    direccionServo.write(65);
    retro(110);
    delay(1000);
    parar();
    direccionServo.write(centroServo);
  }
  else if(distanciaL<5)
  {
    parar();
    direccionServo.write(125);
    retro(110);
    delay(1000);
    parar();
    direccionServo.write(centroServo);
  }*/
  //4. estados posibles
  switch (estadoRobot) {
    
    
    // ESTADO 0: Avance y PD
  
    case 0: 

      // ¿Detectamos una esquina?
      if (((distanciaR + distanciaL) > 95 && ((distanciaF > 50 && distanciaF < 60) || (distanciaF > 30 && distanciaF < 40))) && contactivado == 1) {//distanciaF <= 45
        
        arranque = false;

        // Decidir dirección
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
        
        // Cambiamos de estado e iniciamos el cronómetro de giro
        estadoRobot = 1; 
        tiempoInicioGiro = millis();
          contador++;
          a++;
          contactivado = 0;
          tiempocontador = millis();
        /*if(contactivado == 1){
         
        }*/
        
      } 
      // Si no hay esquina, hacemos Control PD Normal
      else {

        if (arranque == true)
        {
          Avan(110); 
          direccionServo.write(centroServo);
        }

        else {
          if (distanciaF >= 60 && distanciaF < 90)
        {
          Avan(110); 
          direccionServo.write(centroServo);
        }
        else
        {
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

    
    // ESTADO 1: GIRANDO 
    
    case 1:
      Avan(110);
      // El servo ya está posicionado en 65 o 125 por el  case 0.

      if ((millis() - tiempoInicioGiro >= TIEMPO_DURACION_GIRO) || distanciaF > 170) {
        Serial.println("FIN DEL GIRO. Entrando a enfriamiento.");
        delay(500);
        if(distanciaF > 30 && distanciaF < 40){
          delay(250);
        }
        Ultrasonico();
        estadoRobot = 2; // periodo de cool
        tiempoUltimoGiro = millis();
      }
      break;

    
    // ESTADO 2: avanzar al frente y regresar a PD
    
    case 2:
      direccionServo.write(centroServo); // Enderezar llantas
      Avan(110);
      
      if (millis() - tiempoUltimoGiro >= TIEMPO_BLOQUEO_GIRO) {
        Serial.println("Avanzar y regresar al PD");
        estadoRobot = 0; // Regresamos al estado normal
        errorAnterior = 0; // Reseteamos el error
      }
      break;
  }
}

//Funciones de Movimiento
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

//Función sensores ultrasónicos con timeout
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
  /*
  if(FiltroUS == 1){
    sumdF -= dFbuffer[puntero];
    sumdF += distanciaF;
    dFbuffer[puntero] = distanciaF;
    distanciaF = sumdF/buffersize;
  }
  */

    
  Serial.print("Izquierda: "); Serial.print(distanciaL);
  Serial.print(" cm  |  Frente: "); Serial.print(distanciaF);
  Serial.print(" cm  |  Derecha: "); Serial.print(distanciaR);
  Serial.println(" cm");
  

  if(FiltroUS == 1){
    puntero ++;
    puntero %= buffersize;
  }
}