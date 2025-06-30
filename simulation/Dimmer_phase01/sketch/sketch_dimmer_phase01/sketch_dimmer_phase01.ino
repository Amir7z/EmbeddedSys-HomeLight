/*
  Simple AC Light Dimmer
  Connects a Zero-Crossing Detector (ZCD) and a TRIAC driver to an Arduino uno.
  A potentiometer  controls the dimming level.
*/
// === Pins ===
#define TRIAC_PULSE_PIN 7    // Output Pin connected to TRIAC (via optocoupler driver)
#define ZCD_PIN 2            // Input Pin connected to Zero Cross Detection circuit output (must be an interrupt pin  -> Interrupt 0)
#define POTENTIOMETER_PIN A0 // Input Pin connected to Potentiometer for manual brightness adjustment

// === Dimmer Constants ===
const int AC_FREQUENCY = 50; // AC power frequency in Hertz
const int AC_HALF_CYCLE_MICROS = 1000000 / (2 * AC_FREQUENCY); // Automatic half-cycle calculation
const int MIN_DIM_DELAY = 1500;      // Minimum delay (in microseconds) -> Maximum brightness
const int MAX_DIM_DELAY = AC_HALF_CYCLE_MICROS - 2000; // Maximum delay (in microseconds) -> Minimum brightness
// A flag to enable/disable detailed debug messages. Set to true for more logging.
const bool DETAILED_DEBUG = true;

// A volatile variable to be shared between the ISR and the main loop
volatile bool zeroCrossed = false;
// Global variables for sensor readings:
int potValueRaw = 0;
int lastpotValueRaw = 0;

// Function to be called by the interrupt
void zeroCrossInterrupt() {
  zeroCrossed = true;
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("----------------------------------"));
  Serial.println(F("Dimmer Setup Start... "));
  Serial.println(F("----------------------------------"));

  pinMode(POTENTIOMETER_PIN, INPUT);
  pinMode(ZCD_PIN, INPUT_PULLUP); 
  pinMode(TRIAC_PULSE_PIN, OUTPUT);
  digitalWrite(TRIAC_PULSE_PIN, LOW);

  // Attach an interrupt to the ZCD pin
  // It will call zeroCrossInterrupt() every time the pin has a RISING  edge
  attachInterrupt(digitalPinToInterrupt(ZCD_PIN), zeroCrossInterrupt, RISING );

  if (DETAILED_DEBUG) {
    Serial.println(F("--- Configuration ---"));
    Serial.print(F("AC Frequency: ")); Serial.print(AC_FREQUENCY); Serial.println(F(" Hz"));
    Serial.println(F("--- End Configuration ---"));
  }

  Serial.print(F("SETUP: Dimmer Setup Complete."));
  Serial.println(F("----------------------------------"));
}

void loop() {
  
  if (zeroCrossed) {
    // Reset the flag for the next cycle
    zeroCrossed = false;

    // Read the potentiometer value (0 to 1023)
    potValueRaw = analogRead(POTENTIOMETER_PIN);
    if(potValueRaw != lastpotValueRaw){
    Serial.print(F(" | New potentiometer value : ")); Serial.println(potValueRaw);
    lastpotValueRaw = potValueRaw;
    }
    // Map the potentiometer value to a delay time in microseconds.
    int dimmingDelay = map(potValueRaw, 0, 1023, MAX_DIM_DELAY, MIN_DIM_DELAY);

    // Wait for the calculated delay
    delayMicroseconds(dimmingDelay);

    // Send a short pulse to fire the TRIAC
    digitalWrite(TRIAC_PULSE_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIAC_PULSE_PIN, LOW);
  }
}