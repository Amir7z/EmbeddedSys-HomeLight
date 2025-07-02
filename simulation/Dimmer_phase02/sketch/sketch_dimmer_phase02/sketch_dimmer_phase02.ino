/*
  AC Light Dimmer with Auto/Manual Modes
  - Automatic mode uses an LDR to adjust brightness based on ambient light.
  - Manual mode uses a potentiometer for user control.
  - Automatically reverts to Auto mode after a timeout.
*/

// === Pins ===
#define TRIAC_PULSE_PIN   7   // Output for TRIAC driver
#define ZCD_PIN           2   // Input from Zero-Crossing Detector (Interrupt 0)
#define POTENTIOMETER_PIN A0  // Input for manual control
#define LDR_PIN           A1  // Input for LDR sensor

// === Dimmer Constants ===
const int AC_FREQUENCY = 50;
const int AC_HALF_CYCLE_MICROS = 1000000 / (2 * AC_FREQUENCY);
const int MIN_DIM_DELAY = 1500;  // For maximum brightness
const int MAX_DIM_DELAY = AC_HALF_CYCLE_MICROS - 2000; // For minimum brightness
// A flag to enable/disable detailed debug messages. Set to true for more logging.
const bool DETAILED_DEBUG = true;

// === Mode Control Constants ===
const long MANUAL_MODE_TIMEOUT = 10000; // 10 seconds timeout
const int POT_CHANGE_THRESHOLD = 10;    // Minimum change to detect pot movement

// === Global Variables ===
volatile int dimmingDelay = MIN_DIM_DELAY;
bool isManualMode = false;
long lastManualAdjustmentTime = 0;
int lastPotValue = 0;// initialize currectly in setup
int lastLDRValue = 0;

// === Interrupt Service Routine ===
void zeroCrossInterrupt() {
  delayMicroseconds(dimmingDelay);
  digitalWrite(TRIAC_PULSE_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIAC_PULSE_PIN, LOW);
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("----------------------------------"));
  Serial.println(F("Dimmer Setup Start... "));
  Serial.println(F("----------------------------------"));
  
  pinMode(TRIAC_PULSE_PIN, OUTPUT);
  digitalWrite(TRIAC_PULSE_PIN, LOW);
  pinMode(ZCD_PIN, INPUT_PULLUP);
  pinMode(POTENTIOMETER_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);

  // Attach the interrupt for ZCD
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), zeroCrossInterrupt, RISING);

  // Read initial pot value
  lastPotValue = analogRead(POTENTIOMETER_PIN);
  lastLDRValue = analogRead(LDR_PIN);
   if (DETAILED_DEBUG) {
    Serial.println("");
    Serial.println(F("--- Configuration ---"));
    Serial.print(F("AC Frequency: ")); Serial.print(AC_FREQUENCY); Serial.println(F(" Hz"));
    Serial.print(F("PPOTENTIOMETER initial value : ")); Serial.println(lastPotValue);
    Serial.print(F("LDR initial value : ")); Serial.println(lastLDRValue);
    Serial.println(F("--- End of Configuration ---"));
    Serial.println("");
  }
  
  Serial.println(F("SETUP: Dimmer Setup Complete."));
  Serial.println(F("----------------------------------"));
}

void loop() {
  // 1. Check for manual adjustment
  int currentPotValue = analogRead(POTENTIOMETER_PIN);
  if (abs(currentPotValue - lastPotValue) > POT_CHANGE_THRESHOLD) {
    if (!isManualMode) {
      Serial.println("Switching to MANUAL mode.");
    }
    isManualMode = true;
    lastManualAdjustmentTime = millis(); // Reset the timeout timer
    lastPotValue = currentPotValue;
    if (DETAILED_DEBUG) {
    Serial.print(F("PPOTENTIOMETER value: ")); Serial.println(lastPotValue);
    Serial.print(F("Current time: "));Serial.println(lastManualAdjustmentTime);
    Serial.print(F("At "));Serial.print(MANUAL_MODE_TIMEOUT+lastManualAdjustmentTime);
    Serial.println(F(",system will revert to Automatic mode."));
    }
  }

  // 2. Check for timeout to revert to auto mode
  if (isManualMode && (millis() - lastManualAdjustmentTime > MANUAL_MODE_TIMEOUT)) {
    Serial.println("Timeout! Reverting to AUTOMATIC mode.");
     if (DETAILED_DEBUG) {
      Serial.print(F("Current time: "));Serial.println(millis());
    }
    isManualMode = false;
  }

  // 3. Determine the control value based on the current mode
  int controlValue;
  if (isManualMode) {
    controlValue = lastPotValue;
  } else {
    // In automatic mode, use the LDR value
    int ldrValue = analogRead(LDR_PIN);
    if(DETAILED_DEBUG && ldrValue != lastLDRValue){
       Serial.print(F("LDR new value : ")); Serial.println(ldrValue);
       lastLDRValue=ldrValue;
    }
    // Inverse mapping: less ambient light (lower ldrValue) -> brighter lamp (higher controlValue)
    // You need to calibrate the 100 and 900  based on your LDR and room lighting
    controlValue = map(ldrValue, 10, 975, 1023, 0); 
  }
  
  // 4. Calculate the final dimming delay
  // The 'constrain' function ensures the value stays within a valid range
  // so if you don't calibrate  LDR and the map function ,maps the value to invalid range
  // the system will work
  controlValue = constrain(controlValue, 0, 1023);
  
  dimmingDelay = map(controlValue, 0, 1023, MAX_DIM_DELAY, MIN_DIM_DELAY);
  
  delay(50); // Small delay for loop stability
}