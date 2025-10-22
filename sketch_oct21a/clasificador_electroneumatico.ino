#include <Wire.h>
#include <Adafruit_VL53L0X.h>

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

const int MOTOR_PIN = 12;
const int VALVE_PIN_SMALL = 13;
const int VALVE_PIN_MEDIUM = 14;
const int VALVE_PIN_LARGE = 15;

const int DISTANCIA_BASE_CINTA = 200;
const int UMBRAL_PEQUENA = 30;
const int UMBRAL_MEDIANA = 60;
const int UMBRAL_GRANDE = 90;

const unsigned long TIEMPO_VALVULA_ABIERTA_MS = 1500;
const unsigned long INTERVALO_LECTURA_SENSOR_MS = 300;

const int SENSOR_STATUS_OK = 0;

enum SystemState {
  STATE_ESPERANDO_CAJA,
  STATE_VALVULA_ACTIVA
};

SystemState currentState = STATE_ESPERANDO_CAJA;

unsigned long timerValvula = 0;
unsigned long timerLecturaSensor = 0;

void setup() {
  Serial.begin(115200);
  
  Wire.begin(); 

  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(VALVE_PIN_SMALL, OUTPUT);
  pinMode(VALVE_PIN_MEDIUM, OUTPUT);
  pinMode(VALVE_PIN_LARGE, OUTPUT);

  cerrarTodasLasValvulas();
  digitalWrite(MOTOR_PIN, HIGH);

  if (!lox.begin()) {
    Serial.println("Error al iniciar el VL53L0X. Verifique conexiones.");
    while (1);
  }

  Serial.println("Sistema clasificador listo: cinta en marcha.");
}

void loop() {
  if (currentState == STATE_ESPERANDO_CAJA) {
    handleEstadoEsperando();
  } 
  else if (currentState == STATE_VALVULA_ACTIVA) {
    handleEstadoValvula();
  }
}

void handleEstadoEsperando() {
  if (millis() - timerLecturaSensor < INTERVALO_LECTURA_SENSOR_MS) {
    return;
  }
  timerLecturaSensor = millis();

  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);

  if (measure.RangeStatus <= SENSOR_STATUS_OK) {
    int dist = measure.RangeMilliMeter;
    int altura = DISTANCIA_BASE_CINTA - dist;
    
    if (altura < 0) altura = 0; 
    
    Serial.print("Altura medida: ");
    Serial.print(altura);
    Serial.println(" mm");

    int zona = classifyBox(altura);

    if (zona > 0) {
      activarValvula(zona);
    }
    
  } else {
    Serial.println("... esperando caja");
  }
}

void handleEstadoValvula() {
  if (millis() > timerValvula) {
    cerrarTodasLasValvulas();
  }
}

int classifyBox(int altura) {
  if (altura > 0 && altura <= UMBRAL_PEQUENA) return 1;
  else if (altura > UMBRAL_PEQUENA && altura <= UMBRAL_MEDIANA) return 2;
  else if (altura > UMBRAL_MEDIANA && altura <= UMBRAL_GRANDE) return 3;
  else return 0;
}

void activarValvula(int zone) {
  Serial.print("¡Caja detectada! Zona: ");
  Serial.print(zone);
  Serial.println(". Activando válvula.");

  switch (zone) {
    case 1: digitalWrite(VALVE_PIN_SMALL, HIGH); break;
    case 2: digitalWrite(VALVE_PIN_MEDIUM, HIGH); break;
    case 3: digitalWrite(VALVE_PIN_LARGE, HIGH); break;
  }
  
  timerValvula = millis() + TIEMPO_VALVULA_ABIERTA_MS;
  
  currentState = STATE_VALVULA_ACTIVA;
}

void cerrarTodasLasValvulas() {
  digitalWrite(VALVE_PIN_SMALL, LOW);
  digitalWrite(VALVE_PIN_MEDIUM, LOW);
  digitalWrite(VALVE_PIN_LARGE, LOW);

  if (currentState == STATE_VALVULA_ACTIVA) {
    Serial.println("Válvulas cerradas. Volviendo a modo espera.");
    currentState = STATE_ESPERANDO_CAJA;
  }
}