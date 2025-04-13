/* ********************************************************************** 
 * Arduino Bomb Simulator
 * Combines 4-Digit Countdown Timer and Wire Cutting Detection
 ********************************************************************* */

/* ***************************************************
 *                Pin Definitions                    *
 *************************************************** */
// Pins for display (74HC595 shift registers)
const int dataPin  = 2;    // DS pin (serial data) - CHANGED from 12
const int latchPin = 3;    // STCP pin (storage register clock) - CHANGED from 11
const int clockPin = 4;    // SHCP pin (shift register clock) - CHANGED from 9
const int buzzerPin = 10;  // Digital pin for the buzzer
const int detonatorPin = 9; // Detonator pin

// Pins for button reading (74HC165 shift register)
#define SHIFT_LOAD_PIN 11  // SH/LD pin (pin 1)
#define SHIFT_CLK_PIN 12   // CLK pin (pin 2)
#define SHIFT_DATA_PIN 13  // QH pin (pin 9)

// LED pins
#define LED_GREEN_PIN 5    // D5
#define LED_ERROR1_PIN 6   // D6
#define LED_ERROR2_PIN 7   // D7
#define LED_ERROR3_PIN 8   // D8

/* ***************************************************
 *                Wire Definitions                    *
 *************************************************** */
#define WIRE_A_PIN 4   // Pino 11 - Preto (fio fino)
#define WIRE_B_PIN 5   // Pino 12 - Amarelo (fio grosso)
#define WIRE_C_PIN 6   // Pino 13 - Amarelo (fio fino)
#define WIRE_D_PIN 7   // Pino 14 - Laranja (fio fino)
#define WIRE_E_PIN 0   // Pino 3 - Roxo (fio fino)
#define WIRE_F_PIN 1   // Pino 4 - Vermelho (grosso)
#define WIRE_G_PIN 2   // Pino 5 - Roxo (fio grosso)
#define WIRE_H_PIN 3   // Pino 6 - Vermelho (fio fino)

// Wire names for debug output
const char* wireNames[] = {
  "Roxo (fino)",      // E - Pin 3
  "Vermelho (grosso)", // F - Pin 4
  "Roxo (grosso)",    // G - Pin 5
  "Vermelho (fino)",  // H - Pin 6
  "Preto (fino)",     // A - Pin 11
  "Amarelo (grosso)", // B - Pin 12
  "Amarelo (fino)",   // C - Pin 13
  "Laranja (fino)"    // D - Pin 14
};

// Wire state tracking (1 = connected, 0 = cut)
bool wireStates[8] = {true, true, true, true, true, true, true, true};
byte lastWireReading = 0xFF; // All wires initially connected (all bits 1)

/* ***************************************************
 *                Global Constants                   *
 *************************************************** */
// Segment mapping for first 74HC595 (values used to turn segments ON)
const byte SEG_A  = 0b00000001;  // Q0 (pin 15) -> Display pin 11
const byte SEG_B  = 0b00000010;  // Q1 (pin 1)  -> Display pin 7
const byte SEG_C  = 0b00000100;  // Q2 (pin 2)  -> Display pin 4
const byte SEG_D  = 0b00001000;  // Q3 (pin 3)  -> Display pin 2
const byte SEG_E  = 0b00010000;  // Q4 (pin 4)  -> Display pin 1
const byte SEG_F  = 0b00100000;  // Q5 (pin 5)  -> Display pin 10
const byte SEG_G  = 0b01000000;  // Q6 (pin 6)  -> Display pin 5
const byte SEG_DP = 0b10000000;  // Q7 (pin 7)  -> Display pin 3

// Digit mapping for second 74HC595 (values used to select digits)
const byte DIGIT_1 = 0b00000001;  // Q0 (pin 15) -> Display pin 12
const byte DIGIT_2 = 0b00000010;  // Q1 (pin 1)  -> Display pin 9
const byte DIGIT_3 = 0b00000100;  // Q2 (pin 2)  -> Display pin 8
const byte DIGIT_4 = 0b00001000;  // Q3 (pin 3)  -> Display pin 6
 
/* ***************************************************
 *                Global Variables                   *
 *************************************************** */
// Display variables
byte table[]= 
    {   
        SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,          // 0
        SEG_B | SEG_C,                                          // 1
        SEG_A | SEG_B | SEG_G | SEG_E | SEG_D,                  // 2
        SEG_A | SEG_B | SEG_G | SEG_C | SEG_D,                  // 3
        SEG_F | SEG_G | SEG_B | SEG_C,                          // 4
        SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,                  // 5
        SEG_A | SEG_F | SEG_G | SEG_C | SEG_D | SEG_E,          // 6
        SEG_A | SEG_B | SEG_C,                                  // 7
        SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,  // 8
        SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,          // 9
        SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,          // A
        SEG_F | SEG_E | SEG_G | SEG_C | SEG_D,                  // b
        SEG_A | SEG_F | SEG_E | SEG_D,                          // C
        SEG_B | SEG_C | SEG_G | SEG_E | SEG_D,                  // d
        SEG_A | SEG_F | SEG_G | SEG_E | SEG_D,                  // E
        SEG_A | SEG_F | SEG_G | SEG_E,                          // F
        0x00                                                    // blank
    };

byte digitControl[] = { DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4 };
byte displayDigits[] = { 0, 0, 0, 0 }; // Seconds ones, Seconds tens, Minutes ones, Minutes tens

unsigned long displayRefreshTime = 0; // tracks display refresh timing
unsigned long timerUpdateTime = 0;    // tracks timer update interval
int currentSecond = 0;                // current second (0-59)
int currentMinute = 30;               // current minute (starting at 30)
int currentDigit = 0;                 // tracks which digit is currently being displayed
bool timerRunning = true;             // flag to control if timer is running
bool timerFinished = false;           // flag to indicate when timer reaches 00:00

// Button reading variables
bool errorState = false;              // Replace errorCount with boolean flag
unsigned long errorLedStartTime = 0;  // Track when error LEDs were turned on
const unsigned long errorLedSolidDuration = 1000;  // 1 second of solid LEDs
bool successState = false;
int lastCutWireIndex = -1;  // New variable to track the last cut wire

// Buzzer variables
const int buzzerDuration = 50; // Duration of buzzer sound in ms
const int buzzerFrequency = 1000; // Base frequency of buzzer sound in Hz

// Beep control variables
unsigned long lastBeepTime = 0;    // Tracks when the last beep occurred
int beepInterval = 1000;           // Default interval between beeps (ms)
const int minBeepInterval = 100;   // Minimum interval between beeps (ms)

// Alarm variables
unsigned long alarmStartTime = 0;   // When the alarm started
const unsigned long alarmDuration = 10000; // 10 seconds in milliseconds
bool alarmActive = false;           // Flag to track if alarm is sounding

// Detonator variables
bool detonatorActive = false;           // Flag to track if detonator is activated
unsigned long detonatorStartTime = 0;    // When the detonator was activated
const unsigned long detonatorDuration = 5000; // 5 seconds in milliseconds
int detonatorActivationCount = 0;       // Counter for detonator activations

// LED control variables
int currentLedIndex = 0;    // Tracks which LED should be lit next
bool ledsActive = false;    // Flag to track if LEDs are currently active
unsigned long ledOffTime = 0; // When to turn off the active LED
const int ledPins[] = {LED_ERROR1_PIN, LED_ERROR2_PIN, LED_ERROR3_PIN};  // Use error LEDs for blinking
const int numLeds = 3;      // Number of LEDs
int ledOnDuration = 100;    // How long each LED stays on (ms)

// Error notification variables
bool errorSoundActive = false;
unsigned long errorSoundStartTime = 0;
const unsigned long errorSoundDuration = 2000; // 2 seconds of error sound
const int errorToneFrequency = 1000; // Error tone frequency (Hz)
 
int correctWireIndex = 7; // Index of the wire to cut (0-7)

/* ***************************************************
 *           Global Adjustable Variables             *
 *************************************************** */
int digitDisplayTime = 2;             // time to display each digit in ms
int brightness = 90;                  // valid range of 0-100, 100=brightest
int displayRefreshInterval = 5;       // refresh display every 5ms
int secondUpdateInterval = 1000;      // update counter every 1000ms (1 second)
int buttonReadInterval = 500;         // check wires every 500ms
unsigned long lastButtonReadTime = 0; // tracks when wires were last read

/* ***************************************************
 *              Display Functions                    *
 *************************************************** */
void UpdateDisplay() {
    // Update the display with current time (MM:SS)
    // Format: displayDigits[3] displayDigits[2] : displayDigits[1] displayDigits[0]
    
    // Calculate individual digits
    displayDigits[0] = currentSecond % 10;        // Seconds ones place
    displayDigits[1] = currentSecond / 10;        // Seconds tens place
    displayDigits[2] = currentMinute % 10;        // Minutes ones place
    displayDigits[3] = currentMinute / 10;        // Minutes tens place
}

void CountSecondTimer() {
    // Add check for success state at the beginning
    if (successState) {
        // Stop all sounds when in success state
        noTone(buzzerPin);
        return;
    }
    
    if (!timerRunning || (timerFinished && !alarmActive)) {
        return;
    }
    
    // Check if alarm needs to be turned off
    unsigned long currentMillis = millis();
    if (alarmActive && (currentMillis - alarmStartTime >= alarmDuration)) {
        alarmActive = false;
        noTone(buzzerPin);
    }
    
    // Check if detonator needs to be turned off
    if (detonatorActive && (currentMillis - detonatorStartTime >= detonatorDuration)) {
        detonatorActive = false;
        digitalWrite(detonatorPin, LOW);
    }
    
    // If alarm is active, don't update the timer but make LEDs blink in sequence
    if (alarmActive) {
        // Check if it's time for a new LED to activate
        if (currentMillis - lastBeepTime >= beepInterval) {
            lastBeepTime = currentMillis;
            ActivateNextLed();
        }
        return;
    }
    
    // Check if it's time to update the second counter
    if (currentMillis - timerUpdateTime >= secondUpdateInterval) {
        timerUpdateTime = currentMillis;  // Reset the timer reference
        
        // Decrement the second
        currentSecond--;
        
        // Check if we need to decrement minutes
        if (currentSecond < 0) {
            currentSecond = 59;
            currentMinute--;
            
            // Check if timer has finished
            if (currentMinute < 0) {
                currentMinute = 0;
                currentSecond = 0;
                timerFinished = true;
                alarmActive = true;
                alarmStartTime = currentMillis;
                
                // Only activate detonator if bomb has not been successfully defused
                // and it hasn't been activated before
                if (!successState && detonatorActivationCount == 0) {
                    detonatorActive = true;
                    detonatorStartTime = currentMillis;
                    digitalWrite(detonatorPin, HIGH);
                    detonatorActivationCount++; // Increment activation counter
                }
                
                // Start continuous alarm
                tone(buzzerPin, 2000);
            }
        }
        
        // Update the display
        UpdateDisplay();
    }
}

void DisplayAllDigits() {
    // Turn off all digits
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, 0);  // Digit control (all off)
    shiftOut(dataPin, clockPin, MSBFIRST, 0);  // Segment control (all off)
    digitalWrite(latchPin, HIGH);
    
    // Select and display current digit (rotating through all 4)
    // Reverse the order of digits: 3-currentDigit 
    byte segmentValue = table[displayDigits[3-currentDigit]];
    
    // Add decimal point to the first minute digit (position 1) to simulate colon
    if (currentDigit == 1) {
        segmentValue |= SEG_DP;
    }
    
    // Display the current digit
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, digitControl[currentDigit]);
    shiftOut(dataPin, clockPin, MSBFIRST, segmentValue);
    digitalWrite(latchPin, HIGH);
    
    // Leave the digit on for the display time
    delay(digitDisplayTime);
    
    // Move to next digit
    currentDigit = (currentDigit + 1) % 4;
}

/* ***************************************************
 *              Buzzer Functions                    *
 *************************************************** */
void PlayBuzzerSound() {
    // Calculate remaining time in seconds
    int remainingTime = currentMinute * 60 + currentSecond;
    
    // Only adjust frequency when less than 1 minute remains
    if (remainingTime <= 60) {
        // Adjust buzzer frequency based on remaining time (higher frequency as time decreases)
        int frequency = buzzerFrequency + (60 - remainingTime) * 20; // Increase by 20Hz for each second passed
        tone(buzzerPin, frequency, buzzerDuration);
    } else {
        // Use fixed frequency (1000 Hz) when more than 1 minute remains
        tone(buzzerPin, buzzerFrequency, buzzerDuration);
    }
    
    // Activate the next LED in sequence
    ActivateNextLed();
}

// Function to handle LED activation
void ActivateNextLed() {
    // Don't activate LEDs if we're in success state or have max errors
    if (successState || errorState) return;
    
    // Turn off all LEDs first (except green LED if success state)
    for (int i = 0; i < numLeds; i++) {
        digitalWrite(ledPins[i], LOW);
    }
    
    // Turn on the current LED
    digitalWrite(ledPins[currentLedIndex], HIGH);
    
    // Set timer to turn off the LED
    ledOffTime = millis() + ledOnDuration;
    ledsActive = true;
    
    // Move to the next LED for next activation
    currentLedIndex = (currentLedIndex + 1) % numLeds;
}

// Function to check and turn off LEDs after duration
void CheckLedStatus() {
    if (ledsActive && millis() >= ledOffTime) {
        // Don't turn off LEDs that should stay on due to errors
        if (errorState) {
            digitalWrite(LED_ERROR1_PIN, HIGH);
            digitalWrite(LED_ERROR2_PIN, HIGH);
            digitalWrite(LED_ERROR3_PIN, HIGH);
        }
        
        // Green LED should stay on if in success state
        if (!successState) {
            for (int i = 0; i < numLeds; i++) {
                digitalWrite(ledPins[i], LOW);
            }
        }
        ledsActive = false;
    }
}

/* ***************************************************
 *              Wire Reading Functions               *
 *************************************************** */
// Function to log detailed status of LEDs
void logLEDStatus() { 
  Serial.println("--- LED Status ---");
  Serial.print("GREEN LED: ");
  Serial.println(digitalRead(LED_GREEN_PIN) == HIGH ? "ON" : "OFF");
  Serial.print("ERROR1 LED: ");
  Serial.println(digitalRead(LED_ERROR1_PIN) == HIGH ? "ON" : "OFF");
  Serial.print("ERROR2 LED: ");
  Serial.println(digitalRead(LED_ERROR2_PIN) == HIGH ? "ON" : "OFF");
  Serial.print("ERROR3 LED: ");
  Serial.println(digitalRead(LED_ERROR3_PIN) == HIGH ? "ON" : "OFF");
  Serial.println("----------------");
}

// Renamed function to better reflect wire reading
byte readWires() {
  byte wireReading = 0;
  
  // Load the data from parallel inputs
  digitalWrite(SHIFT_LOAD_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(SHIFT_LOAD_PIN, HIGH);
  
  // Shift in the data
  for (int i = 0; i < 8; i++) {
    int bitValue = digitalRead(SHIFT_DATA_PIN);
    wireReading |= (bitValue << i);
    
    // Pulse the clock
    digitalWrite(SHIFT_CLK_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(SHIFT_CLK_PIN, LOW);
    delayMicroseconds(5);
  }
  
  return wireReading;
}

// Function to count uncut wires (wires that are still connected)
int countUncutWires() {
  int count = 0;
  for (int i = 0; i < 8; i++) {
    if (wireStates[i]) {
      count++;
    }
  }
  return count;
}

// Check if the wire is thick based on the wire index
bool isThickWire(int wireIndex) {
  // Thick wires: Vermelho (grosso) - F - Index 1
  //              Amarelo (grosso) - B - Index 5
  //              Roxo (grosso) - G - Index 2
  return wireIndex == 1 || wireIndex == 2 || wireIndex == 5;
}

// Count the number of thick wires that are still connected
int countUncutThickWires() {
  int count = 0;
  if (wireStates[1]) count++; // Vermelho (grosso) - F
  if (wireStates[2]) count++; // Roxo (grosso) - G
  if (wireStates[5]) count++; // Amarelo (grosso) - B
  return count;
}

// Function to determine which wire should be cut based on number of uncut wires
int getCorrectWireIndex() {
  int uncutWiresCount = countUncutWires();
  
  if(uncutWiresCount == 8){
    Serial.println("Entering case 8");
    return 7;  // Laranja (fino) - D - Pin 14
  }

  if(uncutWiresCount == 7){
    Serial.println("Entering case 7");
    // If the last cut wire was Amarelo or Laranja or Vermelho
    if (lastCutWireIndex == 5 || lastCutWireIndex == 6 || lastCutWireIndex == 7 || 
        lastCutWireIndex == 1 || lastCutWireIndex == 3) {
      // Cut the thin purple wire if there's another purple wire remaining
      if (wireStates[0] && wireStates[2]) { // If both purple wires are present
        return 0; // Cut thin purple wire (Roxo fino - E - Index 0)
      }
    } 
    
    // If the last cut wire was Preto or Roxo
    if (lastCutWireIndex == 4 || lastCutWireIndex == 0 || lastCutWireIndex == 2) {
      // Cut the thick yellow wire if it hasn't been cut yet
      if (wireStates[5]) { // If thick yellow wire exists
        return 5; // Cut thick yellow wire (Amarelo grosso - B - Index 5)
      }
    }
    
    // Otherwise (default case)
    // Cut the thin purple wire if it hasn't been cut yet, otherwise cut the thin orange wire
    if (wireStates[0]) {
      return 0; // Cut thin purple wire (Roxo fino - E - Index 0)
    } else {
      return 7; // Cut thin orange wire (Laranja fino - D - Index 7)
    }
  }

  if(uncutWiresCount == 6){
    Serial.println("Entering case 6");
    // Check if there's a thick purple wire and thin orange wire hasn't been cut
    if (wireStates[2] && wireStates[7]) { // Roxo (grosso) - G and Laranja (fino) - D
      return 7; // Cut thin orange wire
    }
    
    // Check if last cut was thick black or thick purple
    if (lastCutWireIndex == 2) { // Thick purple was cut last (Roxo grosso - G)
      // Cut thick light-colored wire with priority: orange > yellow > red
      // Note: We don't have a thick orange wire in the configuration
      if (wireStates[5]) return 5; // Amarelo (grosso) - B
      if (wireStates[1]) return 1; // Vermelho (grosso) - F
    }
    
    // Follow priority order
    if (wireStates[2]) return 2;       // Roxo (grosso) - G
    if (wireStates[5]) return 5;       // Amarelo (grosso) - B
    if (wireStates[4]) return 4;       // Preto (fino) - A
    if (wireStates[3]) return 3;       // Vermelho (fino) - H
    if (wireStates[7]) return 7;       // Laranja (fino) - D
    // No Laranja (grosso) in our configuration
    if (wireStates[1]) return 1;       // Vermelho (grosso) - F
    if (wireStates[6]) return 6;       // Amarelo (fino) - C
    // No Preto (grosso) in our configuration
    if (wireStates[0]) return 0;       // Roxo (fino) - E
  }

  if(uncutWiresCount == 5){
    Serial.println("Entering case 5");
      // Count colored wires (Amarelo, Laranja, Vermelho)
      int coloredWireCount = 0;
      int darkWireCount = 0;
      
      // Count Amarelo wires
      if (wireStates[5]) coloredWireCount++; // Amarelo (grosso)
      if (wireStates[6]) coloredWireCount++; // Amarelo (fino)
      
      // Count Laranja wires
      if (wireStates[7]) coloredWireCount++; // Laranja (fino)
      // No Laranja (grosso) in our configuration
      
      // Count Vermelho wires
      if (wireStates[1]) coloredWireCount++; // Vermelho (grosso)
      if (wireStates[3]) coloredWireCount++; // Vermelho (fino)
      
      // Count Preto and Roxo wires
      // No Preto (grosso) in our configuration
      if (wireStates[4]) darkWireCount++; // Preto (fino)
      if (wireStates[0]) darkWireCount++; // Roxo (fino)
      if (wireStates[2]) darkWireCount++; // Roxo (grosso)
      
      // Rule 1: More colored than dark wires
      if (coloredWireCount > darkWireCount && wireStates[6]) {
        return 6; // Cut Amarelo (fino) - C
      }
      
      // Rule 2: Odd number of thick wires
      int thickWiresCount = countUncutThickWires();
      if (thickWiresCount % 2 == 1 && wireStates[3]) {
        return 3; // Cut Vermelho (fino) - H
      }
      
      // Rule 3: Cut based on priority sequence
      // Preto Grosso < Roxo Fino < Laranja Grosso < Amarelo Fino < Vermelho Grosso 
      // < Laranja Fino < Amarelo Grosso < Vermelho Fino < Roxo Grosso < Preto Fino
      
      // No Preto (grosso) in our configuration
      if (wireStates[0]) return 0; // Roxo (fino) - E
      // No Laranja (grosso) in our configuration
      if (wireStates[6]) return 6; // Amarelo (fino) - C
      if (wireStates[1]) return 1; // Vermelho (grosso) - F
      if (wireStates[7]) return 7; // Laranja (fino) - D
      if (wireStates[5]) return 5; // Amarelo (grosso) - B
      if (wireStates[3]) return 3; // Vermelho (fino) - H
      if (wireStates[2]) return 2; // Roxo (grosso) - G
      if (wireStates[4]) return 4; // Preto (fino) - A
  }

  if(uncutWiresCount == 4){
    Serial.println("Entering case 4");
    // Check if last cut wire was Yellow (Amarelo)
    if (lastCutWireIndex == 5 || lastCutWireIndex == 6) { // Amarelo (grosso or fino)
      // Check if there are 2 or more thick wires left
      int thickWiresCount = countUncutThickWires();
      if (thickWiresCount >= 2 && wireStates[2]) {
        return 2; // Cut Roxo (grosso) - G if available
      }
      
      // Check for exactly one pair of wires of the same color
      bool hasRedPair = wireStates[1] && wireStates[3]; // Vermelho pair
      bool hasPurplePair = wireStates[0] && wireStates[2]; // Roxo pair
      bool hasYellowPair = wireStates[5] && wireStates[6]; // Amarelo pair
      
      // Count total pairs
      int pairCount = 0;
      if (hasRedPair) pairCount++;
      if (hasPurplePair) pairCount++;
      if (hasYellowPair) pairCount++;
      
      // If there's exactly one pair, cut the thin wire of that pair
      if (pairCount == 1) {
        if (hasRedPair) return 3;    // Cut Vermelho (fino) - H
        if (hasPurplePair) return 0; // Cut Roxo (fino) - E
        if (hasYellowPair) return 6; // Cut Amarelo (fino) - C
      }
    }
    
    // If last wire cut was black or purple
    else if (lastCutWireIndex == 0 || lastCutWireIndex == 2 || lastCutWireIndex == 4) {
      // Priority: Laranja Fino > Amarelo Grosso > Vermelho Fino > Laranja Grosso > Vermelho Grosso > Amarelo Fino
      if (wireStates[7]) return 7; // Laranja (fino) - D
      if (wireStates[5]) return 5; // Amarelo (grosso) - B
      if (wireStates[3]) return 3; // Vermelho (fino) - H
      // No Laranja (grosso) in our configuration
      if (wireStates[1]) return 1; // Vermelho (grosso) - F
      if (wireStates[6]) return 6; // Amarelo (fino) - C
    }
    
    // If last wire cut was orange, yellow or red
    else if (lastCutWireIndex == 1 || lastCutWireIndex == 3 || lastCutWireIndex == 5 || 
             lastCutWireIndex == 6 || lastCutWireIndex == 7) {
      // Priority: Roxo Grosso > Preto Fino > Preto Grosso > Roxo Fino
      if (wireStates[2]) return 2; // Roxo (grosso) - G
      if (wireStates[4]) return 4; // Preto (fino) - A
      // No Preto (grosso) in our configuration
      if (wireStates[0]) return 0; // Roxo (fino) - E
    }
    
    // Default rule - separate by color
    if (lastCutWireIndex == 0 || lastCutWireIndex == 2 || lastCutWireIndex == 4) {
      // If last cut was black or purple, cut in this order: Grosso Roxo > Grosso Preto > Fino Preto > Fino Roxo
      if (wireStates[2]) return 2; // Roxo (grosso) - G
      // No Preto (grosso) in our configuration
      if (wireStates[4]) return 4; // Preto (fino) - A
      if (wireStates[0]) return 0; // Roxo (fino) - E
    } else {
      // If last cut was orange, yellow or red, cut in this order:
      // Grosso Amarelo > Grosso Laranja > Grosso Vermelho > Fino Vermelho > Fino Laranja > Fino Amarelo
      if (wireStates[5]) return 5; // Amarelo (grosso) - B
      // No Laranja (grosso) in our configuration
      if (wireStates[1]) return 1; // Vermelho (grosso) - F
      if (wireStates[3]) return 3; // Vermelho (fino) - H
      if (wireStates[7]) return 7; // Laranja (fino) - D
      if (wireStates[6]) return 6; // Amarelo (fino) - C
    }
  }

  if(uncutWiresCount == 3){
    Serial.println("Entering case 3");
    // Check if the remaining wires are Yellow, Red, and any other color
    bool hasYellow = wireStates[5] || wireStates[6]; // Amarelo (grosso or fino)
    bool hasRed = wireStates[1] || wireStates[3];    // Vermelho (grosso or fino)
    
    // Count total remaining wires by color
    int yellowCount = (wireStates[5] ? 1 : 0) + (wireStates[6] ? 1 : 0);
    int redCount = (wireStates[1] ? 1 : 0) + (wireStates[3] ? 1 : 0);
    int purpleCount = (wireStates[0] ? 1 : 0) + (wireStates[2] ? 1 : 0);
    int blackCount = (wireStates[4] ? 1 : 0); // Only Preto (fino) exists
    int orangeCount = (wireStates[7] ? 1 : 0); // Only Laranja (fino) exists
    
    // If we have Yellow, Red, and exactly one other color
    if (hasYellow && hasRed && (yellowCount + redCount == 2) && (purpleCount + blackCount + orangeCount == 1)) {
      // Cut the other color
      if (purpleCount > 0) {
        if(wireStates[0]) return 0; // Cut Roxo (fino) - E
        else if (wireStates[2]) return 2; // Cut Roxo (grosso) - G
      }
      if (blackCount > 0 && wireStates[4]) {
        return 4; // Return Preto (fino)
      }
      if (orangeCount > 0 && wireStates[7]) {
        return 7; // Return Laranja (fino)
      }
    }
    
    // If last cut wire was Black or Purple
    if (lastCutWireIndex == 0 || lastCutWireIndex == 2 || lastCutWireIndex == 4) {
      // Count thin versus thick wires
      int thinWiresCount = 0;
      int thickWiresCount = 0;
      
      // Count thin wires
      if (wireStates[0]) thinWiresCount++; // Roxo (fino)
      if (wireStates[3]) thinWiresCount++; // Vermelho (fino)
      if (wireStates[4]) thinWiresCount++; // Preto (fino)
      if (wireStates[6]) thinWiresCount++; // Amarelo (fino)
      if (wireStates[7]) thinWiresCount++; // Laranja (fino)
      
      // Count thick wires
      if (wireStates[1]) thickWiresCount++; // Vermelho (grosso)
      if (wireStates[2]) thickWiresCount++; // Roxo (grosso)
      if (wireStates[5]) thickWiresCount++; // Amarelo (grosso)
      
      // If more thin wires than thick wires
      if (thinWiresCount > thickWiresCount && wireStates[7]) {
        return 7; // Cut Laranja (fino)
      }
    }
    
    // If last cut wire was thick
    if (isThickWire(lastCutWireIndex)) {
      // Cut in sequence: Vermelho Grosso > Preto Fino > Amarelo Grosso > Roxo Fino > Laranja Grosso > Vermelho Fino > Preto Grosso > Roxo Grosso > Amarelo Fino > Laranja Fino
      if (wireStates[1]) return 1; // Vermelho (grosso) - F
      if (wireStates[4]) return 4; // Preto (fino) - A
      if (wireStates[5]) return 5; // Amarelo (grosso) - B
      if (wireStates[0]) return 0; // Roxo (fino) - E
      // No Laranja (grosso) in our configuration
      if (wireStates[3]) return 3; // Vermelho (fino) - H
      // No Preto (grosso) in our configuration
      if (wireStates[2]) return 2; // Roxo (grosso) - G
      if (wireStates[6]) return 6; // Amarelo (fino) - C
      if (wireStates[7]) return 7; // Laranja (fino) - D
    }
    // If last cut wire was thin
    else {
      // Cut in sequence: Vermelho Grosso > Vermelho Fino > Roxo Fino > Roxo Gross > Preto Grosso > Preto Fino > Laranja Grosso > Laranja Fino > Amarelo Fino > Amarelo Grosso
      if (wireStates[1]) return 1; // Vermelho (grosso) - F
      if (wireStates[3]) return 3; // Vermelho (fino) - H
      if (wireStates[0]) return 0; // Roxo (fino) - E
      if (wireStates[2]) return 2; // Roxo (grosso) - G
      // No Preto (grosso) in our configuration
      if (wireStates[4]) return 4; // Preto (fino) - A
      // No Laranja (grosso) in our configuration
      if (wireStates[7]) return 7; // Laranja (fino) - D
      if (wireStates[6]) return 6; // Amarelo (fino) - C
      if (wireStates[5]) return 5; // Amarelo (grosso) - B
    }
  }

  if(uncutWiresCount == 2){
    Serial.println("Entering case 2");
    // Store the indexes of the remaining two wires
    int remainingWires[2];
    int wireCount = 0;
    
    for (int i = 0; i < 8; i++) {
      if (wireStates[i]) {
        remainingWires[wireCount++] = i;
        if (wireCount >= 2) break; // Found both wires
      }
    }
    
    bool wire1IsColoredWire = remainingWires[0] == 1 || remainingWires[0] == 3 || // Vermelho
                             remainingWires[0] == 5 || remainingWires[0] == 6 || // Amarelo
                             remainingWires[0] == 7; // Laranja
                             
    bool wire2IsColoredWire = remainingWires[1] == 1 || remainingWires[1] == 3 || // Vermelho
                             remainingWires[1] == 5 || remainingWires[1] == 6 || // Amarelo
                             remainingWires[1] == 7; // Laranja
                             
    bool wire1IsDarkWire = remainingWires[0] == 0 || remainingWires[0] == 2 || // Roxo
                          remainingWires[0] == 4; // Preto
                          
    bool wire2IsDarkWire = remainingWires[1] == 0 || remainingWires[1] == 2 || // Roxo
                          remainingWires[1] == 4; // Preto
    
    // Check if both are colored (yellow, red, orange)
    if (wire1IsColoredWire && wire2IsColoredWire) {
      // If one is thick and one is thin, cut the thin one
      bool wire1IsThick = isThickWire(remainingWires[0]);
      bool wire2IsThick = isThickWire(remainingWires[1]);
      
      if (wire1IsThick && !wire2IsThick) {
        return remainingWires[1]; // Cut the thin wire (wire2)
      }
      else if (!wire1IsThick && wire2IsThick) {
        return remainingWires[0]; // Cut the thin wire (wire1)
      }
    }
    
    // Check if both are dark (black or purple)
    if (wire1IsDarkWire && wire2IsDarkWire) {
      // If one is thick and one is thin, cut the thin one
      bool wire1IsThick = isThickWire(remainingWires[0]);
      bool wire2IsThick = isThickWire(remainingWires[1]);
      
      if (wire1IsThick && !wire2IsThick) {
        return remainingWires[1]; // Cut the thin wire (wire2)
      }
      else if (!wire1IsThick && wire2IsThick) {
        return remainingWires[0]; // Cut the thin wire (wire1)
      }
    }
    
    // If last cut wire was yellow, red, or orange
    if (lastCutWireIndex == 1 || lastCutWireIndex == 3 || // Vermelho
        lastCutWireIndex == 5 || lastCutWireIndex == 6 || // Amarelo
        lastCutWireIndex == 7) { // Laranja
      
      // Check for black wires
      bool hasBlackWire = wireStates[4]; // Preto fino
      // No Preto grosso in our configuration
      
      if (hasBlackWire) {
        return 4; // Cut the black wire
      }
      
      // Check for purple wires
      bool hasThinPurple = wireStates[0]; // Roxo fino
      bool hasThickPurple = wireStates[2]; // Roxo grosso
      
      if (hasThinPurple && !hasThickPurple) {
        return 0; // Cut thin purple
      }
      else if (!hasThinPurple && hasThickPurple) {
        return 2; // Cut thick purple
      }
    }
    
    // If last cut wire was black or purple
    if (lastCutWireIndex == 0 || lastCutWireIndex == 2 || // Roxo
        lastCutWireIndex == 4) { // Preto
      
      // Count colored wires
      int coloredWireCount = 0;
      if (wireStates[1]) coloredWireCount++; // Vermelho grosso
      if (wireStates[3]) coloredWireCount++; // Vermelho fino
      if (wireStates[5]) coloredWireCount++; // Amarelo grosso
      if (wireStates[6]) coloredWireCount++; // Amarelo fino
      if (wireStates[7]) coloredWireCount++; // Laranja fino
      
      // If only one colored wire left
      if (coloredWireCount == 1) {
        if (wireStates[1]) return 1; // Vermelho grosso (thick)
        if (wireStates[5]) return 5; // Amarelo grosso (thick)
        // No Laranja grosso in our configuration
        
        // If no thick colored wires, cut the thin colored wire
        if (wireStates[3]) return 3; // Vermelho fino
        if (wireStates[6]) return 6; // Amarelo fino
        if (wireStates[7]) return 7; // Laranja fino
      }
    }
    
    // Otherwise cut the thin wire following the sequence: Amarelo > Preto > Vermelho > Laranja > Roxo
    if (wireStates[6]) return 6; // Amarelo fino
    if (wireStates[4]) return 4; // Preto fino
    if (wireStates[3]) return 3; // Vermelho fino
    if (wireStates[7]) return 7; // Laranja fino
    if (wireStates[0]) return 0; // Roxo fino
    
    // Otherwise cut the thick wire following the sequence: Vermelho > Preto > Amarelo > Laranja > Roxo
    if (wireStates[1]) return 1; // Vermelho grosso
    // No Preto grosso in our configuration
    if (wireStates[5]) return 5; // Amarelo grosso
    // No Laranja grosso in our configuration
    if (wireStates[2]) return 2; // Roxo grosso
  }

  if(uncutWiresCount == 1) {
    Serial.println("Entering case 1");
    // If only one wire is left, cut it
    for (int i = 0; i < 8; i++) {
      if (wireStates[i]) {
        return i; // Cut the last remaining wire
      }
    }
  }

  return -1;
}

// Print the current state of all wires
void printWireStates() {
  Serial.println("\n--- Estado dos fios ---");
  for (int i = 0; i < 8; i++) {
    Serial.print(wireNames[i]);
    Serial.print(": ");
    Serial.println(wireStates[i] ? "CONECTADO" : "CORTADO");
  }
  
  // Print as an array
  Serial.print("Array de fios [");
  for (int i = 0; i < 8; i++) {
    Serial.print(wireStates[i] ? "1" : "0");
    if (i < 7) Serial.print(", ");
  }
  Serial.println("]");
}

// Replace processButtons with processWires
void processWires() {
  unsigned long currentMillis = millis();
  
  // Only check wires at specified interval
  if (currentMillis - lastButtonReadTime < buttonReadInterval) {
    return;
  }
  
  lastButtonReadTime = currentMillis;
  
  // Read wire states from 74HC165
  byte wireData = readWires();
  
  // Check for changes in wire state
  for (int i = 0; i < 8; i++) {
    bool previousState = wireStates[i];
    bool currentState = (wireData >> i) & 1;
    
    // Only process wire cutting if:
    // 1. The wire was previously connected (true/1)
    // 2. Current reading shows it as cut (false/0)
    // 3. We haven't already processed this wire being cut
    if (previousState == true && currentState == false) {     
      Serial.print("\n!!! FIO CORTADO: ");
      Serial.print(wireNames[i]);
      Serial.println(" !!!");
      
      // Update wire state to cut
      wireStates[i] = false;
      lastCutWireIndex = i;  // Track the last cut wire

      // Print updated wire state array
      printWireStates();
      
      // Check if correct wire was cut
      if (i == correctWireIndex || correctWireIndex <= -1) {
        Serial.println("INDEX: " + String(correctWireIndex));
        Serial.println("ACERTOU! Fio correto cortado.");
        // Only turn on success LED if this was the last wire and timer didn't reach zero
        if (countUncutWires() == 0 && !timerFinished) {
          digitalWrite(LED_GREEN_PIN, HIGH); // Turn on green LED
          successState = true;
          Serial.println("BOMBA DESARMADA COM SUCESSO!");
        }
      } else {
        Serial.println("INDEX: " + String(correctWireIndex));
        errorState = true;
        errorLedStartTime = millis();  // Start timing for solid LED duration
        Serial.println("ERRO! Fio errado cortado.");
        Serial.print("Deveria ter cortado: ");
        if(correctWireIndex == -1) {
          Serial.println("Não foi encontrado o fio correto.");
        } else {
          Serial.print(wireNames[correctWireIndex]);
        }
        updateErrorLEDs();
        reduceTimer(); // Reduce timer by 5 minutes

        // Activate error sound and LEDs
        errorSoundActive = true;
        errorSoundStartTime = millis();
        tone(buzzerPin, errorToneFrequency);
        digitalWrite(LED_ERROR1_PIN, HIGH);
        digitalWrite(LED_ERROR2_PIN, HIGH);
        digitalWrite(LED_ERROR3_PIN, HIGH);
      }

      // Get the correct wire that should have been cut
      correctWireIndex = getCorrectWireIndex();
    }
    // We don't update wireStates for already cut wires to prevent re-triggering
    // This completely ignores any changes to wires that have already been cut
  }
  
  // Save current reading for next comparison
  lastWireReading = wireData;

  // Check if error sound duration has elapsed
  if (errorSoundActive && millis() - errorSoundStartTime >= errorSoundDuration) {
    errorSoundActive = false;
    noTone(buzzerPin);
    digitalWrite(LED_ERROR1_PIN, LOW);
    digitalWrite(LED_ERROR2_PIN, LOW);
    digitalWrite(LED_ERROR3_PIN, LOW);
  }
}

// Helper function to update error LEDs based on error state
void updateErrorLEDs() {
  if (errorState) {
    digitalWrite(LED_ERROR1_PIN, HIGH);
    digitalWrite(LED_ERROR2_PIN, HIGH);
    digitalWrite(LED_ERROR3_PIN, HIGH);
  }
}

// New helper function to reduce the timer by 5 minutes
void reduceTimer() {
  currentMinute -= 5;
  
  // Handle case where timer would go below zero
  if (currentMinute < 0) {
    unsigned long currentMillis = millis();

    currentMinute = 0;
    currentSecond = 0;
    timerFinished = true;

    // Only activate detonator if it hasn't been activated before
    if (detonatorActivationCount == 0) {
        detonatorActive = true;
        detonatorStartTime = currentMillis;
        digitalWrite(detonatorPin, HIGH);
        detonatorActivationCount++; // Increment activation counter
    }
  }
  
  UpdateDisplay(); // Update display to show the new time
}

/* ***************************************************
 * Function to print the raw pin states of the 74HC165 shift register
 *************************************************** */
void print74HC165PinStates() {
  Serial.println("\n--- 74HC165 Raw Pin States ---");
  
  // Load the data from parallel inputs
  digitalWrite(SHIFT_LOAD_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(SHIFT_LOAD_PIN, HIGH);
  
  // Shift in and print each bit
  for (int i = 0; i < 8; i++) {
    int bitValue = digitalRead(SHIFT_DATA_PIN);
    
    // Print the pin state and corresponding wire
    Serial.print("Pin ");
    Serial.print(i);
    Serial.print(" (");
    Serial.print(wireNames[i]);
    Serial.print("): ");
    Serial.println(bitValue ? "HIGH (1) - Connected" : "LOW (0) - Cut");
    
    // Pulse the clock to get next bit
    digitalWrite(SHIFT_CLK_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(SHIFT_CLK_PIN, LOW);
    delayMicroseconds(5);
  }
  Serial.println("-----------------------------");
}

// New function to check and play beeps based on remaining time
void CheckAndPlayBeeps() {
  // Add check for success state
  if (successState || !timerRunning || timerFinished || alarmActive) {
      return;
  }
  
  // Calculate remaining time in seconds
  int remainingTime = currentMinute * 60 + currentSecond;
  
  // Adjust beep interval based on remaining time 
  // Only start increasing frequency when less than 1 minute remains
  if (remainingTime <= 60) {
      // Linear decrease in interval: from 1000ms at 60s to minBeepInterval at 0s
      beepInterval = minBeepInterval + ((remainingTime * (1000 - minBeepInterval)) / 60);
  } else {
      beepInterval = 1000; // Default: beep once per second
  }
  
  // Check if it's time for a beep
  unsigned long currentMillis = millis();
  if (currentMillis - lastBeepTime >= beepInterval) {
      lastBeepTime = currentMillis;
      PlayBuzzerSound();
  }
}

// Add this new function to manage error LED behavior
void manageErrorLeds() {
  // Turn off all error LEDs if in success state
  if (successState) {
      digitalWrite(LED_ERROR1_PIN, LOW);
      digitalWrite(LED_ERROR2_PIN, LOW);
      digitalWrite(LED_ERROR3_PIN, LOW);
      return;
  }
  
  if (!errorState) return;
  
  unsigned long currentTime = millis();
  
  // If within the 1-second solid period after error
  if (currentTime - errorLedStartTime < errorLedSolidDuration) {
    // Keep all LEDs on solid
    digitalWrite(LED_ERROR1_PIN, HIGH);
    digitalWrite(LED_ERROR2_PIN, HIGH);
    digitalWrite(LED_ERROR3_PIN, HIGH);
  } 
  // After solid period, blink with the buzzer timing
  else if (!timerFinished && !alarmActive) {
    // Only blink LEDs if we're not in timer finished/alarm state
    // Check if it's time for a beep/blink based on beepInterval
    if (currentTime - lastBeepTime >= beepInterval) {
      // Turn on LEDs briefly with beeps
      digitalWrite(LED_ERROR1_PIN, HIGH);
      digitalWrite(LED_ERROR2_PIN, HIGH);
      digitalWrite(LED_ERROR3_PIN, HIGH);
    } else if (currentTime - lastBeepTime >= buzzerDuration) {
      // Turn off LEDs between beeps
      digitalWrite(LED_ERROR1_PIN, LOW);
      digitalWrite(LED_ERROR2_PIN, LOW);
      digitalWrite(LED_ERROR3_PIN, LOW);
    }
  }
}

/* ***************************************************
 *                   Void Setup                      *
 *************************************************** */
void setup() {
    // Initialize serial communication
    Serial.begin(9600);
    Serial.println("Arduino Bomb Simulator starting...");
    
    // Setup display pins
    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
    pinMode(buzzerPin, OUTPUT); // Initialize buzzer pin as output
    pinMode(detonatorPin, OUTPUT); // Initialize detonator pin as output
    digitalWrite(detonatorPin, LOW); // Ensure detonator starts inactive
    
    // Setup wire reading pins
    pinMode(SHIFT_LOAD_PIN, OUTPUT);
    pinMode(SHIFT_CLK_PIN, OUTPUT);
    pinMode(SHIFT_DATA_PIN, INPUT);
    
    // Initialize shift register control pins
    digitalWrite(SHIFT_LOAD_PIN, HIGH);
    digitalWrite(SHIFT_CLK_PIN, LOW);
    
    // Set up LED pins
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_ERROR1_PIN, OUTPUT);
    pinMode(LED_ERROR2_PIN, OUTPUT);
    pinMode(LED_ERROR3_PIN, OUTPUT);
    
    // Initialize all LEDs to OFF
    digitalWrite(LED_GREEN_PIN, LOW);
    digitalWrite(LED_ERROR1_PIN, LOW);
    digitalWrite(LED_ERROR2_PIN, LOW);
    digitalWrite(LED_ERROR3_PIN, LOW);
    
    // Initialize timing references
    timerUpdateTime = millis();
    displayRefreshTime = millis();
    lastButtonReadTime = millis();
    
    // Initialize display with 30:00
    currentMinute = 30;
    currentSecond = 0;
    UpdateDisplay();
    
    // Initialize wire states to all connected
    for (int i = 0; i < 8; i++) {
        wireStates[i] = true;
    }
    
    // Print initial wire state
    Serial.println("Estado inicial dos fios:");
    printWireStates();
    
    // Print raw pin states coming into the 74HC165
    Serial.println("Estado dos pinos do 74HC165:");
    print74HC165PinStates();
    
    Serial.println("Setup complete. Running main loop...");
    logLEDStatus();
}

/* ***************************************************
 *                   Void Loop                       *
 *************************************************** */
void loop() {
    unsigned long currentMillis = millis();
    
    // Display refresh handling
    if (currentMillis - displayRefreshTime >= displayRefreshInterval) {
        displayRefreshTime = currentMillis;
        
        // Display the digits
        DisplayAllDigits();
        
        // Brightness control
        delayMicroseconds(1638*((100-brightness)/10));
    }
    
    // Timer countdown handling
    CountSecondTimer();
    
    // Wire processing
    processWires();
    
    // Check and play timer beeps
    CheckAndPlayBeeps();
    
    // Manage error LED behavior
    manageErrorLeds();
    
    // Check if LEDs need to be turned off
    CheckLedStatus();
    
    // Uncomment the line below to continuously print 74HC165 pin states
    // (Not recommended to leave enabled as it will slow down the display)
    // if (millis() - lastButtonReadTime >= 3000) print74HC165PinStates();
}