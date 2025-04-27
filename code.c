#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Stepper.h>
#include <DHT.h>

// --- Stepper Motor Setup ---
const int stepsPerRevolution = 2048;
Stepper myStepper(stepsPerRevolution, 8, 9, 10, 11);

// --- DHT11 Sensor Setup ---
#define DHT_PIN 2
DHT dht(DHT_PIN, DHT11);

// --- Sensor & Control Pins ---
#define LIGHT_SENSOR A1
#define BUTTON_CLOCKWISE 4
#define BUTTON_COUNTERCLOCKWISE 5

// --- LCD Setup ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- Variables ---
bool lastDayState = true;
bool manualClockwiseActive = false;
bool manualCounterClockwiseActive = false;
bool manualActionDone = false;
bool autoActionDone = true;

bool lastButtonClockwiseState = HIGH;
bool lastButtonCounterClockwiseState = HIGH;

// --- Motor Movement Duration Settings ---
const unsigned long spinDuration = 40000; // Changed to 20 seconds
const int motorSpeed = 10;                 // Increased from 15 to 30 RPM
const int stepsPerMove = 20;                // Optionally increased steps per move for faster visual spinning

unsigned long motorStartTime = 0;
int motorDirection = 0; // 1 = CW, -1 = CCW, 0 = idle

// --- LCD State Variables ---
String lastLine1 = "";
String lastLine2 = "";

void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(BUTTON_CLOCKWISE, INPUT_PULLUP);
  pinMode(BUTTON_COUNTERCLOCKWISE, INPUT_PULLUP);

  myStepper.setSpeed(motorSpeed);

  dht.begin();
}

void loop() {
  // --- Read Temperature ---
  float tempC = dht.readTemperature();

  if (isnan(tempC)) {
    Serial.println("Failed to read from DHT sensor!");
    lcdUpdate("Sensor Error", " ");
    delay(1000);
    return;
  }

  // --- Read Sensors ---
  int lightVal = analogRead(LIGHT_SENSOR);
  bool isDay = lightVal > 100;

  bool buttonClockwise = digitalRead(BUTTON_CLOCKWISE);
  bool buttonCounterClockwise = digitalRead(BUTTON_COUNTERCLOCKWISE);

  // --- Handle Button Toggles ---
  if (buttonClockwise == LOW && lastButtonClockwiseState == HIGH) {
    manualClockwiseActive = !manualClockwiseActive;
    manualCounterClockwiseActive = false;
    manualActionDone = false;
    motorDirection = 0;
    Serial.println("Manual Clockwise Button Pressed (Now CCW after reverse)");
    delay(200);
  }

  if (buttonCounterClockwise == LOW && lastButtonCounterClockwiseState == HIGH) {
    manualCounterClockwiseActive = !manualCounterClockwiseActive;
    manualClockwiseActive = false;
    manualActionDone = false;
    motorDirection = 0;
    Serial.println("Manual CounterClockwise Button Pressed (Now CW after reverse)");
    delay(200);
  }

  lastButtonClockwiseState = buttonClockwise;
  lastButtonCounterClockwiseState = buttonCounterClockwise;

  // --- LCD Display Management ---
  String line1 = "Temp: " + String(tempC) + " C";
  String line2 = "";

  if (manualClockwiseActive) {
    line2 = "Manual Close (CCW)"; // reversed!
  } else if (manualCounterClockwiseActive) {
    line2 = "Manual Open (CW)"; // reversed!
  } else {
    if (isDay) line2 = "Light: Day";
    else line2 = "Light: Night";
  }

  lcdUpdate(line1, line2);

  // --- Decide What Action to Take ---
  if (manualClockwiseActive) {
    if (!manualActionDone && motorDirection == 0) {
      Serial.println("Manual Close - Counter-Clockwise Spin (reversed)");
      motorStartTime = millis();
      motorDirection = -1; // Reverse: Button that was CW now CCW
    }
  } else if (manualCounterClockwiseActive) {
    if (!manualActionDone && motorDirection == 0) {
      Serial.println("Manual Open - Clockwise Spin (reversed)");
      motorStartTime = millis();
      motorDirection = 1; // Reverse: Button that was CCW now CW
    }
  } else {
    if (isDay != lastDayState) {
      Serial.println(isDay ? "Auto: Day Detected (Spin CCW)" : "Auto: Night Detected (Spin CW)");
      autoActionDone = false;
      motorStartTime = millis();
      motorDirection = isDay ? -1 : 1; // Reverse: Day spins CCW, Night spins CW
      lastDayState = isDay;
    }
  }

  // --- Motor Spin Handler ---
  if (motorDirection != 0) {
    if (millis() - motorStartTime <= spinDuration) {
      myStepper.step(stepsPerMove * motorDirection);
    } else {
      motorDirection = 0;
      if (manualClockwiseActive || manualCounterClockwiseActive) {
        manualActionDone = true;
      } else {
        autoActionDone = true;
      }
    }
  }

  delay(20);
}

// --- LCD Update Function ---
void lcdUpdate(String line1, String line2) {
  if (line1 != lastLine1 || line2 != lastLine2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);

    lastLine1 = line1;
    lastLine2 = line2;
  }
}

