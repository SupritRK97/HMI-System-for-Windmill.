// Pin definitions
#define BUZZER_PIN 8

const int ky024AnalogPin = A0;
const int ky024DigitalPin = 2;

const int flameAnalogPin = A1;
const int flameDigitalPin = 3;

const int photoresistorPin = A2;

#define DHT11_PIN 10

#define MQ4_PIN A4

// Include libraries
#include <DFRobot_DHT11.h>
#include <MQUnifiedsensor.h>

// Sensor objects
DFRobot_DHT11 DHT;
MQUnifiedsensor MQ4("Arduino UNO", 5, 10, MQ4_PIN, "MQ-4");

// Variables for flame sensor
int flameStatus = 0;
int flameLevel = 0;

// Threshold for buzzer trigger on temperature
const float TEMP_THRESHOLD = 35.0;    // Temperature threshold in °C

void setup() {
  Serial.begin(9600);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ky024DigitalPin, INPUT);
  pinMode(flameDigitalPin, INPUT);

  // Set up MQ4 sensor parameters
  MQ4.setRegressionMethod(1);
  MQ4.setA(30000000);
  MQ4.setB(-8.308);
  MQ4.init();

  Serial.print("Calibrating MQ4 sensor...");
  float calcR0 = 0;
  for (int i = 1; i <= 10; i++) {
    MQ4.update();
    calcR0 += MQ4.calibrate(4.4);
    Serial.print(".");
    delay(100);
  }
  MQ4.setR0(calcR0 / 10);
  Serial.println(" done.");

  if (isinf(calcR0)) {
    Serial.println("Warning: MQ4 R0 is infinite! Check wiring.");
    while (1);
  }
  if (calcR0 == 0) {
    Serial.println("Warning: MQ4 R0 is zero! Check wiring.");
    while (1);
  }

  MQ4.serialDebug(false);
}

void loop() {
  int incidentDetected = 0;

  // KY-024 Magnetic sensor
  int ky024DigitalValue = digitalRead(ky024DigitalPin);
  int ky024AnalogValue = 0;
  if (ky024DigitalValue == HIGH) {
    ky024AnalogValue = analogRead(ky024AnalogPin);
  }
  if (ky024AnalogValue > 50) {
    incidentDetected = 1;
  }

  // Flame sensor
  flameStatus = digitalRead(flameDigitalPin);
  flameLevel = analogRead(flameAnalogPin);
  int reversedFlameLevel = 1023- flameLevel;
  if (flameStatus == HIGH || reversedFlameLevel > 500) {
    incidentDetected = 1;
  }

  // Photoresistor sensor (inverted)
  int photoValue = analogRead(photoresistorPin);
  int invertedPhotoValue = 1023 - photoValue;
  if (invertedPhotoValue > 600) {
    incidentDetected = 1;
  }

  // DHT11 sensor readings
  DHT.read(DHT11_PIN);
  float temperature = DHT.temperature;

  if (temperature > TEMP_THRESHOLD) {
    incidentDetected = 1;
  }

  // MQ-4 smoke sensor
  MQ4.update();
  float smokePPM = MQ4.readSensor();
  if (smokePPM > 1000) {
    incidentDetected = 1;
  }

  // Buzzer control
  if (incidentDetected) {
    tone(BUZZER_PIN, 1000);
  } else {
    noTone(BUZZER_PIN);
  }

  // Print sensor data in one line (no humidity)
  Serial.print("Magnetic: "); Serial.print(ky024AnalogValue);
  Serial.print(" | FlameDetected: "); Serial.print(flameStatus == LOW ? "NO" : "YES");
  Serial.print(" | FlameLvl: "); Serial.print(reversedFlameLevel);
  Serial.print(" | LightLvl: "); Serial.print(invertedPhotoValue);
  Serial.print(" | Temp: "); Serial.print(temperature);
  Serial.print(" C | SmokePPM: "); Serial.print(smokePPM);
  Serial.println();

  delay(2000);
}

