/*
  Phase 4: AC light Dimmer with Scene Control & PIR Motion Sensor
  - Priority: Manual Potentiometer > PIR Motion > Scene Control > Auto LDR Mode
  - PIR triggers an LDR-adjusted brightness level.
  - Motion mode has a 60s timeout after the last detected motion.
*/

#include <EEPROM.h>

// =================== Pin Definitions ===================
#define TRIAC_PULSE_PIN   7   // Output for TRIAC driver
#define ZCD_PIN           2   // Input from Zero-Crossing Detector (Interrupt 0)
#define POTENTIOMETER_PIN A0  // Input for manual control
#define LDR_PIN           A1  // Input for LDR sensor
#define PIR_PIN           8   // Input for PIR Motion Sensor
const int SCENE_PINS[] = {4, 5, 6}; // Input pins for scenes
#define FEEDBACK_LED_PIN  13  // Built-in LED for feedback

// =================== Constants ===================
const int AC_FREQUENCY = 50;
const int AC_HALF_CYCLE_MICROS = 1000000 / (2 * AC_FREQUENCY); // 10000 us for 50Hz
const int MIN_DIM_DELAY = 1500;  // Minimum firing delay for max brightness
const int MAX_DIM_DELAY = AC_HALF_CYCLE_MICROS - 2000; // Max firing delay for min brightness
const long MANUAL_MODE_TIMEOUT = 10000; // 10 seconds for manual mode timeout
const long SCENE_MODE_TIMEOUT = 30000;  // 30 seconds for scene mode timeout
const long MOTION_MODE_TIMEOUT = 60000; // 60 seconds for motion mode timeout
const long LONG_PRESS_DURATION = 3000;  // 3 seconds to be considered a long press
const int POT_CHANGE_THRESHOLD = 10;    // Min change in pot value to trigger manual mode
const bool DETAILED_DEBUG = true;       // Enable/disable detailed serial logs

// =================== Scene Configuration ===================
const int NUM_SCENES = 3;
const int EEPROM_SCENES_START_ADDR = 0; // Start address for storing scenes in EEPROM
int sceneControlValues[NUM_SCENES];     // Stores scene brightness values (0-1023)

unsigned long sceneButtonPressedTime[NUM_SCENES] = {0};
bool sceneActionTaken[NUM_SCENES] = {false}; // Flag to ensure action (save/recall) is taken only once per press

// =================== Global State Variables ===================
volatile int dimmingDelay = MAX_DIM_DELAY; // Start with the light off/dimmest
enum ControlMode { AUTO, MANUAL, SCENE, MOTION }; 
ControlMode currentMode = AUTO;
ControlMode previousMode = AUTO; // To store the mode before a scene was activated

int controlValue = 0;
int lastPotValue = 0;
unsigned long lastManualAdjustmentTime = 0;
unsigned long lastSceneActivationTime = 0;
unsigned long lastMotionTime = 0; // Timer for motion detection
int activeSceneIndex = -1;

// =================== Interrupt Service Routine ===================
void zeroCrossInterrupt() {
  delayMicroseconds(dimmingDelay);
  digitalWrite(TRIAC_PULSE_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIAC_PULSE_PIN, LOW);
}

// =================== Helper Functions ===================
// Provides visual feedback using the built-in LED
void giveFeedback(int blinks) {
  for (int i = 0; i < blinks; i++) {
    digitalWrite(FEEDBACK_LED_PIN, HIGH);
    delay(100);
    digitalWrite(FEEDBACK_LED_PIN, LOW);
    delay(100);
  }
}
// Converts ControlMode enum to a printable string for logging
const char* controlModeToString(ControlMode mode) {
  switch (mode) {
    case MANUAL: return "MANUAL";
    case SCENE:  return "SCENE";
    case MOTION: return "MOTION"; // NEW
    case AUTO:   return "AUTO (LDR)";
    default:     return "UNKNOWN";
  }
}
// Loads scene values from EEPROM at startup
void loadScenesFromEEPROM() {
  Serial.println(F("EEPROM: Loading scenes..."));
  for (int i = 0; i < NUM_SCENES; ++i) {
    int address = EEPROM_SCENES_START_ADDR + i * sizeof(int);
    EEPROM.get(address, sceneControlValues[i]);
    // Validate the loaded value. If it's invalid (uninitialized EEPROM returns 0xFFFF) or out of range, set a default.
    if (sceneControlValues[i] < 0 || sceneControlValues[i] > 1023) {
      if (DETAILED_DEBUG) {
        Serial.print(F("  -> Scene ")); Serial.print(i + 1);
        Serial.println(F(" is invalid. Setting default."));
      }
      // Set some reasonable default values
      if (i == 0) sceneControlValues[i] = 1023;// Full brightness
      else if (i == 1) sceneControlValues[i] = 512; // Medium brightness
      else sceneControlValues[i] = 100; // Low brightness
      EEPROM.put(address, sceneControlValues[i]);
    } else {
      if (DETAILED_DEBUG) {
        Serial.print(F("  -> Scene ")); Serial.print(i + 1);
        Serial.print(F(" loaded: ")); Serial.println(sceneControlValues[i]);
      }
    }
  }
}
// Saves a single scene's control value to EEPROM
void saveSceneToEEPROM(int sceneIndex, int value) {
  if (sceneIndex < 0 || sceneIndex >= NUM_SCENES) return;
  sceneControlValues[sceneIndex] = constrain(value, 0, 1023);
  int address = EEPROM_SCENES_START_ADDR + sceneIndex * sizeof(int);
  EEPROM.put(address, sceneControlValues[sceneIndex]);
  if (DETAILED_DEBUG) {
    Serial.print(F("EEPROM: Scene ")); Serial.print(sceneIndex + 1);
    Serial.print(F(" saved: ")); Serial.println(sceneControlValues[sceneIndex]);
  }
  giveFeedback(5);
}

// =================== Main Setup ===================
void setup() {
  Serial.begin(9600);
  Serial.println(F("\n----------------------------------"));
  Serial.println(F("Advanced AC Dimmer - Initializing..."));
  Serial.println(F("----------------------------------"));

  pinMode(TRIAC_PULSE_PIN, OUTPUT);
  digitalWrite(TRIAC_PULSE_PIN, LOW);
  pinMode(ZCD_PIN, INPUT_PULLUP);
  pinMode(POTENTIOMETER_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(PIR_PIN, INPUT); 
  pinMode(FEEDBACK_LED_PIN, OUTPUT);
  for (int i = 0; i < NUM_SCENES; i++) {
    pinMode(SCENE_PINS[i], INPUT_PULLUP);
  }

  lastPotValue = analogRead(POTENTIOMETER_PIN);
  if (DETAILED_DEBUG) {
    Serial.println(F("\n--- Configuration ---"));
    Serial.print(F("AC Frequency: ")); Serial.println(AC_FREQUENCY);
    Serial.print(F("Initial Potentiometer Value: ")); Serial.println(lastPotValue);
    Serial.println(F("--- End of Configuration ---\n"));
  }
  
  loadScenesFromEEPROM();

  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), zeroCrossInterrupt, RISING);

  Serial.println(F("\nSetup Complete. Initial Mode: AUTO (LDR)"));
  Serial.println(F("----------------------------------"));
}

// =================== Main Loop  ===================
void loop() {
  unsigned long currentTime = millis();
  bool motionDetected = (digitalRead(PIR_PIN) == HIGH);

  // 1. Handle Potentiometer Override (Highest Priority)
  int currentPotValue = analogRead(POTENTIOMETER_PIN);
  if (abs(currentPotValue - lastPotValue) > POT_CHANGE_THRESHOLD) {
    if (currentMode != MANUAL) {
      Serial.print(F("STATE_CHANGE: Potentiometer moved -> MANUAL mode. From: "));
      Serial.println(controlModeToString(currentMode));
    }
    currentMode = MANUAL;
    lastManualAdjustmentTime = currentTime;
    lastPotValue = currentPotValue;
    activeSceneIndex = -1; // Deactivate any active scene
  }
  // 2. Handle PIR Motion (Second Highest Priority)
  else if (motionDetected) {
     if (currentMode != MOTION) {
        Serial.print(F("STATE_CHANGE: Motion detected -> MOTION mode. From: "));
        Serial.println(controlModeToString(currentMode));
        previousMode = currentMode; // Save the mode we are interrupting
        currentMode = MOTION;
     }
     lastMotionTime = currentTime; // Keep resetting the timer as long as motion is detected
  }
  // 3. Handle Scene Buttons (Third Priority)
  else { //  check for scenes if no motion is detected and pot is not being turned
      for (int i = 0; i < NUM_SCENES; i++) {
        if (digitalRead(SCENE_PINS[i]) == LOW) { // Button is pressed
            if (sceneButtonPressedTime[i] == 0) { // First moment the button is detected as pressed
                sceneButtonPressedTime[i] = currentTime;
                sceneActionTaken[i] = false;
                if(DETAILED_DEBUG) { Serial.print(F("EVENT: Button " )); Serial.print(i+1); Serial.println(F(" pressed."));}
            }
            // Check for long press
            if (!sceneActionTaken[i] && (currentTime - sceneButtonPressedTime[i] > LONG_PRESS_DURATION)) {
                Serial.print(F("ACTION: Long press on button ")); Serial.print(i + 1); Serial.println(F(". Saving scene."));
                saveSceneToEEPROM(i, controlValue);
                sceneActionTaken[i] = true; // Mark that we've acted on this press
            }
        } else { // Button is not pressed (released)
            if (sceneButtonPressedTime[i] != 0) { // It was previously pressed
                if (!sceneActionTaken[i]) { // It was a short press
                    Serial.print(F("ACTION: Short press on button ")); Serial.print(i + 1); Serial.println(F(". Recalling scene."));
                    if (currentMode != SCENE) {
                        previousMode = currentMode;
                    }
                    currentMode = SCENE;
                    activeSceneIndex = i;
                    lastSceneActivationTime = currentTime;
                    giveFeedback(i + 1);
                }
                sceneButtonPressedTime[i] = 0; // Reset the timer for this button
            }
        }
    }
  }

 

  // 4. Handle Timeouts
  if (currentMode == MANUAL && (currentTime - lastManualAdjustmentTime > MANUAL_MODE_TIMEOUT)) {
    Serial.println(F("TIMEOUT: Manual mode expired. Reverting to AUTO."));
    currentMode = AUTO;
  }
  if (currentMode == SCENE && (currentTime - lastSceneActivationTime > SCENE_MODE_TIMEOUT)) {
    Serial.print(F("TIMEOUT: Scene mode expired. Reverting to previous: "));
    Serial.println(controlModeToString(previousMode));
    currentMode = previousMode;
    activeSceneIndex = -1;
  }
  // Timeout for motion mode
  if (currentMode == MOTION && !motionDetected && (currentTime - lastMotionTime > MOTION_MODE_TIMEOUT)) {
      Serial.print(F("TIMEOUT: Motion mode expired. Reverting to previous: "));
      Serial.println(controlModeToString(previousMode));
      currentMode = previousMode;
  }


  // 5. Determine final control value based on the current mode
  switch (currentMode) {
    case MANUAL:
      controlValue = lastPotValue;
      break;
    case SCENE:
      if (activeSceneIndex != -1) {
        controlValue = sceneControlValues[activeSceneIndex];
      }
      break;
    case MOTION: // Use LDR to set brightness on motion, so don't break this case for 
    // using Fall-through 
    // To set MAX brightness, simply un-comment controlValue = 1023; and break;
    // controlValue = 1023;
    //break;
    case AUTO:
      int ldrValueAuto = analogRead(LDR_PIN);
      controlValue = map(ldrValueAuto, 10, 975, 1023, 0); // Inverting map
      break;
  }

  // 6. Calculate and safely update the global dimmingDelay
   // constrain() ensures the value is within the 0-1023 range before mapping.
  int newDimmingDelay = map(constrain(controlValue, 0, 1023), 0, 1023, MAX_DIM_DELAY, MIN_DIM_DELAY);  

  // Disable interrupts temporarily to safely update the volatile variable
  noInterrupts();
  dimmingDelay = newDimmingDelay;
  interrupts();

  delay(20); // Small delay to prevent the loop from running too fast 
}