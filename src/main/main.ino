#include <Arduino.h>
#include <Servo.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

// === CONFIG ===
#define SERVO_PIN       D0
#define NEO_DATA_PIN    12
#define NEO_PWR_PIN     11
#define SECONDARY_R     17
#define SECONDARY_G     16
#define SECONDARY_B     25

#define NEO_COUNT       1
#define FAILSAFE_PWM    1500
#define EEPROM_SIZE     1
#define EEPROM_MODE_ADDR 0  // EEPROM address to store mode
#define TORTURE_TEST_STEP_DURATION 10
#define SUPER_TORTURE_TEST_STEP_DURATION 1000

// === GLOBAL STATE ===
Adafruit_NeoPixel neo(NEO_COUNT, NEO_DATA_PIN, NEO_GRB + NEO_KHZ800);
Servo pwmServo;

uint8_t mode = 0; // 0 = full forward, 1 = full reverse, 2 = torture
bool secondaryBlinkState = false;
unsigned long blinkTimer = 0;
uint16_t tortureVal = 1000;

void setNeoColor(uint8_t r, uint8_t g, uint8_t b) {
  neo.setPixelColor(0, neo.Color(r, g, b));
  neo.show();
}

void setSecondaryColor(uint8_t r, uint8_t g, uint8_t b) {
  digitalWrite(SECONDARY_R, r > 0 ? HIGH : LOW);
  digitalWrite(SECONDARY_G, g > 0 ? HIGH : LOW);
  digitalWrite(SECONDARY_B, b > 0 ? HIGH : LOW);
}

void blinkNeoMode(uint8_t count) {
  for (uint8_t i = 0; i < count; i++) {
    setNeoColor(255, 255, 255);
    delay(200);
    setNeoColor(0, 0, 0);
    delay(200);
  }
}

void watchdogBlink() {
  if (millis() - blinkTimer > 500) {
    blinkTimer = millis();
    secondaryBlinkState = !secondaryBlinkState;
    setSecondaryColor(secondaryBlinkState ? 255 : 0,
                      secondaryBlinkState ? 255 : 0,
                      secondaryBlinkState ? 255 : 0);
  }
}

void setup() {
  pinMode(NEO_PWR_PIN, OUTPUT);
  digitalWrite(NEO_PWR_PIN, HIGH);  // Enable NeoPixel power

  pinMode(SECONDARY_R, OUTPUT);
  pinMode(SECONDARY_G, OUTPUT);
  pinMode(SECONDARY_B, OUTPUT);

  neo.begin();
  neo.show();

  pwmServo.attach(SERVO_PIN, 1000, 2000);
  pwmServo.writeMicroseconds(FAILSAFE_PWM);

  EEPROM.begin(EEPROM_SIZE);
  mode = EEPROM.read(EEPROM_MODE_ADDR);
  mode = (mode + 1) % 4;
  EEPROM.write(EEPROM_MODE_ADDR, mode);
  EEPROM.commit();

  blinkNeoMode(mode + 1);

  delay(3000);

  switch (mode) {
    case 0: // Full throttle forward
      pwmServo.writeMicroseconds(2000);
      setNeoColor(0, 255, 0); // Green
      break;

    case 1: // Full throttle reverse
      pwmServo.writeMicroseconds(1000);
      setNeoColor(255, 0, 0); // Red
      break;

    case 2: // Torture test init
      tortureVal = 1500;
      break;

   case 3: //Super torture test
     break;
  }
}

void loop() {
  if (mode == 2) {
        
    setNeoColor(
      tortureVal >= 1500 ? 0 : map(tortureVal, 1000, 1500, 255, 0),
      tortureVal <= 1500 ? 0 : map(tortureVal, 1500, 2000, 0, 255),
      0);

    pwmServo.writeMicroseconds(tortureVal);

    delay(TORTURE_TEST_STEP_DURATION);

    if (tortureVal == 2000) {
      tortureVal = 1000;
    } else {
      tortureVal++;
    }
  }

  if (mode == 3) {
    setNeoColor(255, 0, 0);
    pwmServo.writeMicroseconds(1000);
    delay(SUPER_TORTURE_TEST_STEP_DURATION);

    setNeoColor(0, 255, 0);
    pwmServo.writeMicroseconds(2000);
    delay(SUPER_TORTURE_TEST_STEP_DURATION);
  }

  watchdogBlink();
}
