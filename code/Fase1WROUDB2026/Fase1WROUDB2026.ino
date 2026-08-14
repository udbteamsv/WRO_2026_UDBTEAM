#include <Servo.h>

// --- Variables Sensores Ultrasónicos ---
const int echoI = 22; const int trigI = 23;
const int echoD = 24; const int trigD = 25;
const int echoF = 26; const int trigF = 27;

long duracionR, distanciaR;
long duracionL, distanciaL;
long duracionF, distanciaF;

// Límite de tiempo para pulseIn (30000 microsegundos = ~5 metros máximo)
const unsigned long TIMEOUT_US = 30000; 

// --- Variables L298N ---
const int ENA = 2; // Pin PWM
const int IN1 = 4; // Dirección 1
const int IN2 = 3; // Dirección 2

// --- Variables Servo y Control PD ---
Servo direccionServo;
const int pinServo = 9;
int centroServo = 90; // Centro teórico

// Constantes PID (Deberás ajustar estos valores probando en la pista)
float Kp = 2; 
float Kd = 0; 
int errorAnterior = 0;

// Variables de Estado de Giro
unsigned long tiempoUltimoGiro = 0;
const unsigned long TIEMPO_BLOQUEO_GIRO = 1500; // ms ignorando paredes después de girar

void setup() {
  Serial.begin(9600);
  
  // Configuración Servo
  direccionServo.attach(pinServo);
  direccionServo.write(centroServo);

  // Configuración Sensores
  pinMode(trigI, OUTPUT); pinMode(echoI, INPUT);
  pinMode(trigD, OUTPUT); pinMode(echoD, INPUT);
  pinMode(trigF, OUTPUT); pinMode(echoF, INPUT);

  // Configuración Motor
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  Serial.println("Iniciando WRO 2026...");
  delay(2000); // Dar tiempo antes de arrancar
}

void loop() {
  Ultrasonico();

  // 1. Evaluar si acabamos de terminar un giro (Periodo de gracia)
  // Si estamos en este periodo, solo avanzamos recto para salir de la diagonal.
  if (millis() - tiempoUltimoGiro < TIEMPO_BLOQUEO_GIRO) {
    direccionServo.write(centroServo);
    Avan(110);
    return; // Salta el resto del loop hasta que pase el tiempo
  }

  // 2. Lógica de Detección de Intersección (Giros de 90 grados)
  // Añadimos una validación de la distancia Frontal para evitar falsos positivos
  if ((distanciaR + distanciaL) > 130 && distanciaF < 80) {
    
    if (distanciaR > distanciaL) {
      direccionServo.write(70); // Girar Derecha //LIM 40
      Serial.println("GIRO DERECHA");
    } else {
      direccionServo.write(120); // Girar Izquierda //LIM 150
      Serial.println("GIRO IZQUIERDA");
    }
    
    Avan(110);
    delay(1500); // Tiempo que dura físicamente el giro (AJUSTAR SEGÚN TU CARRO)
    
    // Registrar el momento en que terminó el giro para activar el bloqueo temporal
    tiempoUltimoGiro = millis();
    
  } 
  // 3. Control PD para Centrado en el Carril
  else {
    Avan(110); // Motor siempre encendido a velocidad constante
    
    // Calcular el Error: Si L es mayor que R, estamos muy a la derecha (Error positivo)
    int error = distanciaL - distanciaR;
    
    // Término Proporcional
    float P = error;
    // Término Derivativo
    float D = error - errorAnterior;
    
    // Ecuación PD
    int ajuste = (Kp * P) + (Kd * D);
    int nuevoAngulo = centroServo + ajuste; // Restamos/Sumamos según la orientación de tu servo
    
    // Restringir el ángulo para no forzar el servo
    if (nuevoAngulo > 120) nuevoAngulo = 120;
    if (nuevoAngulo < 70) nuevoAngulo = 70;
    
    direccionServo.write(nuevoAngulo);
    
    // Guardar el error para la siguiente iteración
    errorAnterior = error;
  }
}

// --- Funciones de Movimiento ---
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

// --- Función Sensores Ultrasónicos con Timeout ---
void Ultrasonico() {
  // Lado Derecho
  digitalWrite(trigD, LOW); delayMicroseconds(2);
  digitalWrite(trigD, HIGH); delayMicroseconds(10);
  digitalWrite(trigD, LOW);
  duracionR = pulseIn(echoD, HIGH, TIMEOUT_US);
  distanciaR = (duracionR == 0) ? 40 : duracionR * 0.034 / 2; // Si da 0 (timeout), fingimos 40
  if(distanciaR >= 150) distanciaR = 150; // Limitar ruido

  // Lado Izquierdo
  digitalWrite(trigI, LOW); delayMicroseconds(2);
  digitalWrite(trigI, HIGH); delayMicroseconds(10);
  digitalWrite(trigI, LOW);
  duracionL = pulseIn(echoI, HIGH, TIMEOUT_US);
  distanciaL = (duracionL == 0) ? 40 : duracionL * 0.034 / 2;
  if(distanciaL >= 150) distanciaL = 150; // Limitar ruido

  // Frente
  digitalWrite(trigF, LOW); delayMicroseconds(2);
  digitalWrite(trigF, HIGH); delayMicroseconds(10);
  digitalWrite(trigF, LOW);
  duracionF = pulseIn(echoF, HIGH, TIMEOUT_US);
  distanciaF = (duracionF == 0) ? 200 : duracionF * 0.034 / 2;

  Serial.print("Izquierda: ");
  Serial.print(distanciaL);
  Serial.print(" cm  |  Frente: ");
  Serial.print(distanciaF);
  Serial.print(" cm  |  Derecha: ");
  Serial.print(distanciaR);
  Serial.println(" cm");

}