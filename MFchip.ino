#include <Wire.h>

#define I2C_HIGHDRIVER_ADRESS  (0x7A)
#define I2C_POWERMODE      0x01
#define I2C_FREQUENCY      0x02
#define I2C_PVOLTAGE       0x06
#define I2C_UPDATEVOLTAGE  0x0A

boolean bPumpState = false;
uint8_t nPumpVoltageByte = 0x1F; // Max amplitude (250Vpp default)
uint8_t nFrequencyByte = 0x40;   // Default 100Hz

float desiredFlowRate = 10.0;
unsigned long pumpOnInterval = 5000;
unsigned long pumpOffInterval = 5000;

unsigned long previousMillis = 0;
bool pumpRunning = false;

const int flowSensorPin = A0; // Connect your flow sensor here
const float conversionFactor = 0.1; // Adjust based on your sensor calibration

void Highdriver_init() {
  Wire.beginTransmission(I2C_HIGHDRIVER_ADRESS);
  Wire.write(I2C_POWERMODE);
  Wire.write(0x01); // enable
  Wire.write(nFrequencyByte);
  Wire.write(0x00); // sine wave
  Wire.write(0x00); // boost 800KHz
  Wire.write(0x00); // audio off
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.write(0x01); // update
  Wire.endTransmission();
}

void Highdriver_setvoltage(uint8_t amplitude) {
  float temp = amplitude * 31.0 / 250.0;
  uint8_t voltage_byte = constrain(temp, 0, 31);
  Wire.beginTransmission(I2C_HIGHDRIVER_ADRESS);
  Wire.write(I2C_PVOLTAGE);  
  Wire.write(0);
  Wire.write(0);
  Wire.write(0);
  Wire.write(bPumpState ? voltage_byte : 0);
  Wire.write(0x01);
  Wire.endTransmission(); 
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  Highdriver_init();
}

void loop() {
  handleSerialInput();
  managePump();
  sendFlowRate();
}

void handleSerialInput() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    if (input.startsWith("SET,")) {
      float flow;
      unsigned long onInterval, offInterval;
      if (sscanf(input.c_str(), "SET,%f,%lu,%lu", &flow, &onInterval, &offInterval) == 3) {
        desiredFlowRate = flow;
        pumpOnInterval = onInterval;
        pumpOffInterval = offInterval;
        Serial.println("ACK,Settings Updated");
      }
    }
  }
}

void managePump() {
  unsigned long currentMillis = millis();
  if (pumpRunning && (currentMillis - previousMillis >= pumpOnInterval)) {
    pumpRunning = false;
    bPumpState = false;
    Highdriver_setvoltage(nPumpVoltageByte);
    previousMillis = currentMillis;
  } else if (!pumpRunning && (currentMillis - previousMillis >= pumpOffInterval)) {
    pumpRunning = true;
    bPumpState = true;
    previousMillis = currentMillis;
  }

  if (pumpRunning) {
    float currentFlow = readFlowSensor();
    adjustAmplitude(currentFlow);
  }
}

float readFlowSensor() {
  int analogValue = analogRead(flowSensorPin);
  return analogValue * conversionFactor;
}

void adjustAmplitude(float currentFlow) {
  if (currentFlow > 0) {
    float newAmplitude = (desiredFlowRate / currentFlow) * 250.0;
    newAmplitude = constrain(newAmplitude, 0, 250);
    Highdriver_setvoltage((uint8_t)newAmplitude);
  } else {
    Highdriver_setvoltage(nPumpVoltageByte);
  }
}

void sendFlowRate() {
  static unsigned long lastSent = 0;
  unsigned long interval = 500; // send every 500ms
  if (millis() - lastSent > interval) {
    float flow = readFlowSensor();
    Serial.print("FLOW,");
    Serial.println(flow);
    lastSent = millis();
  }
}
