// --- Variables Sensores Ultrasónicos ---
const int echoD = 24;
const int trigD = 25;

const int echoI = 22;
const int trigI = 23;

const int echoF = A4;
const int trigF = A5; 

long duracionD, distanciaD;
long duracionI, distanciaI;
long duracionF, distanciaF;

// --- Variables L298N
const int ENA = 5; // Pin PWM
const int IN1 = 3; // Dirección 1
const int IN2 = 4; // Dirección 2

int fast = 0;

// --- Variables Servo ---
const int pinServo = 9;
int anguloAnterior = 90; // Variable global para recordar el último giro (Inicia en Centro: 90)

void setup() {
  Serial.begin(9600);
  
  // Pines Servo
  pinMode(pinServo, OUTPUT);
  
  // Pines Sensores Ultrasónicos
  pinMode(trigD, OUTPUT);
  pinMode(echoD, INPUT);
  pinMode(trigI, OUTPUT);
  pinMode(echoI, INPUT);
  pinMode(trigF, OUTPUT);
  pinMode(echoF, INPUT);
  
  // Pines L298N para un motor
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  Serial.println("Listo. Ingresa un angulo entre 0 y 180:");
  
  // Inicializar físicamente el servo en el centro al encender
  moverServo(40);
}

void loop() 
{
  Ultrasonico();
  //Avan(255);
  /*
  moverServo(40); //D
  delay(900);
  moverServo(100); //C-D
  delay(900);
  moverServo(150);//I
  delay(900);
  moverServo(90);//C-I
  delay(900);
  */

}

// --- Funciones de Movimiento
void Avan(int velocidad) 
{
  analogWrite(ENA, velocidad);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void retro(int velocidad) 
{
  analogWrite(ENA, velocidad);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
}

void parar() 
{
  analogWrite(ENA, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}

// --- Función Servo ---
void moverServo(int angulo) 
{
  int pulsoMicrosegundos = 1000 + (angulo * 1000L / 180L);
  
  for(int i = 0; i < 200; i++) 
  {
    digitalWrite(pinServo, HIGH);
    delayMicroseconds(pulsoMicrosegundos);
    digitalWrite(pinServo, LOW);
    delayMicroseconds(20000 - pulsoMicrosegundos); 
  }
}

// --- Función Sensores Ultrasónicos ---
void Ultrasonico() 
{
  // --- Lado Derecho ---
  digitalWrite(trigD, LOW);
  delayMicroseconds(2);
  digitalWrite(trigD, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigD, LOW);
  duracionD = pulseIn(echoD, HIGH);
  distanciaD = duracionD * 0.034 / 2;

  // --- Lado Izquierdo ---
  digitalWrite(trigI, LOW);
  delayMicroseconds(2);
  digitalWrite(trigI, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigI, LOW);
  duracionI = pulseIn(echoI, HIGH); 
  distanciaI = duracionI * 0.034 / 2; 

  // --- Frente ---
  digitalWrite(trigF, LOW);
  delayMicroseconds(2);
  digitalWrite(trigF, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigF, LOW);
  duracionF = pulseIn(echoF, HIGH);
  distanciaF = duracionF * 0.034 / 2;

  Serial.print("Izquierda: ");
  Serial.print(distanciaI);
  Serial.print(" cm  |  Frente: ");
  Serial.print(distanciaF);
  Serial.print(" cm  |  Derecha: ");
  Serial.print(distanciaD);
  Serial.println(" cm");

  // --- Lógica de Detección ---
  
  if ((distanciaD + distanciaI) <= 97) 
  {  
    // Avanzar Recto
    Avan(255);

    // Asegurar que el servo regrese al centro mientras avanza recto
    if (anguloAnterior != 90) 
    {
      if (anguloAnterior == 40) // Si venía de la derecha (Lim inf 40 o 0)
      {
        moverServo(100);
      }
      else if (anguloAnterior == 150) // Si venía de la izquierda (150)
      {
        moverServo(90);
      }
      anguloAnterior = 90; // Actualizamos el estado interno al Centro
    }
  }
  else 
  {
    if ((distanciaD + distanciaI) > 130) 
    {
      if(distanciaD > distanciaI)
      {
        moverServo(40);
        delay(3000);
        anguloAnterior = 40; // Actualizamos estado a D
      }
      else if(distanciaD < distanciaI)
      {
        moverServo(150);
        delay(3000);
        anguloAnterior = 150; // Actualizamos estado a I (Límite inferior)
      }
    }
    else // Rango intermedio (entre 97 y 120), reemplaza el "if ()" vacío
    {
      parar();
      /*
      // Lógica de centrado cuando no necesita esquivar agresivamente
      if (anguloAnterior != 40) 
      {
        if (anguloAnterior <= 30) 
        {
          moverServo(110); // 0 (o 30) a Centro
        }
        else if (anguloAnterior >= 160) 
        {
          moverServo(90);  // 160 a Centro
        }
        anguloAnterior = 40; // Actualizamos el estado interno al Centro
      }
      */
    }
  }
}