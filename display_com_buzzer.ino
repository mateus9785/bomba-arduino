/* ********************************************************************** 
 * 4-Digit Countdown Timer (01:00 to 00:00)
 * Using one 74HC595 shift register for segments
 * and another 74HC595 shift register for digit selection
 ********************************************************************* */

/* ***************************************************
 *                Global Constants                   *
 *************************************************** */
 
// Common pins for both shift registers
const int dataPin  = 12;   // DS pin (serial data)
const int latchPin = 11;   // STCP pin (storage register clock)
const int clockPin = 9;    // SHCP pin (shift register clock)
const int buzzerPin = 10;  // Digital pin for the buzzer

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
// Segment values for each digit (0-F)
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

// Array of digit values to use with second shift register
byte digitControl[] = { DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4 };

byte displayDigits[] = { 0, 0, 0, 0 }; // Seconds ones, Seconds tens, Minutes ones, Minutes tens

unsigned long displayRefreshTime = 0; // tracks display refresh timing
unsigned long timerUpdateTime = 0;    // tracks timer update interval
int currentSecond = 0;                // current second (0-59)
int currentMinute = 1;                // current minute (starting at 1)
int currentDigit = 0;                 // tracks which digit is currently being displayed
bool timerRunning = true;             // flag to control if timer is running
bool timerFinished = false;           // flag to indicate when timer reaches 00:00

// Variables for blinking segment G when timer finishes
bool blinkState = true;
unsigned long lastBlinkTime = 0;
const int blinkInterval = 200; // blink every 500ms

// Buzzer variables
const int buzzerDuration = 50; // Duration of buzzer sound in ms
const int buzzerFrequency = 1000; // Base frequency of buzzer sound in Hz

// Beep control variables
unsigned long lastBeepTime = 0;    // Tracks when the last beep occurred
int beepInterval = 1000;           // Default interval between beeps (ms)
const int minBeepInterval = 100;   // Minimum interval between beeps (ms)

// Alarm variables
unsigned long alarmStartTime = 0;   // When the alarm started
const unsigned long alarmDuration = 30000; // 30 seconds in milliseconds
bool alarmActive = false;           // Flag to track if alarm is sounding

 
/* ***************************************************
 *           Global Adjustable Variables             *
 *************************************************** */
int digitDisplayTime = 2;             // time to display each digit in ms
int brightness = 90;                  // valid range of 0-100, 100=brightest
int displayRefreshInterval = 5;       // refresh display every 5ms
int secondUpdateInterval = 1000;      // update counter every 1000ms (1 second)
 
/* ***************************************************
 *                   Void Setup                      *
 *************************************************** */
void setup() {
    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
    pinMode(buzzerPin, OUTPUT); // Initialize buzzer pin as output
    
    // Initialize timing references
    timerUpdateTime = millis();
    displayRefreshTime = millis();
    
    // Initialize display with 01:00
    currentMinute = 1;
    currentSecond = 0;
    UpdateDisplay();
}

/* ***************************************************
 *                   Functions                       *
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

void PlayBuzzerSound() {
    // Calculate remaining time in seconds
    int remainingTime = currentMinute * 60 + currentSecond;
    
    // Adjust buzzer frequency based on remaining time (higher frequency as time decreases)
    int frequency = buzzerFrequency + (60 - remainingTime) * 20; // Increase by 20Hz for each second passed
    
    // Play a short beep
    tone(buzzerPin, frequency, buzzerDuration);
}

// New function to check and play beeps based on remaining time
void CheckAndPlayBeeps() {
    if (!timerRunning || timerFinished || alarmActive) {
        return;
    }
    
    // Calculate remaining time in seconds
    int remainingTime = currentMinute * 60 + currentSecond;
    
    // Adjust beep interval based on remaining time 
    // Start increasing frequency when less than 30 seconds remain
    if (remainingTime <= 30) {
        // Linear decrease in interval: from 1000ms at 30s to minBeepInterval at 0s
        beepInterval = minBeepInterval + ((remainingTime * (1000 - minBeepInterval)) / 30);
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

void CountSecondTimer() {
    if (!timerRunning || (timerFinished && !alarmActive)) {
        return;
    }
    
    // Check if alarm needs to be turned off
    unsigned long currentMillis = millis();
    if (alarmActive && (currentMillis - alarmStartTime >= alarmDuration)) {
        alarmActive = false;
        noTone(buzzerPin);
    }
    
    // If alarm is active, don't update the timer
    if (alarmActive) {
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
    
    
    // Check if timer has finished - if so, show blinking segment G
    if (timerFinished) {
        // Check if it's time to toggle the blink state
        unsigned long currentTime = millis();
        if (currentTime - lastBlinkTime >= blinkInterval) {
            lastBlinkTime = currentTime;
            blinkState = !blinkState; // Toggle blink state
        }
        
        // If in "on" state of blinking, show only segment G
        if (blinkState) {
            byte segmentValue = SEG_G; // Only turn on segment G
            
            // Display segment G on the current digit
            digitalWrite(latchPin, LOW);
            shiftOut(dataPin, clockPin, MSBFIRST, digitControl[currentDigit]);
            shiftOut(dataPin, clockPin, MSBFIRST, segmentValue);
            digitalWrite(latchPin, HIGH);
        }
        
        // Move to next digit
        currentDigit = (currentDigit + 1) % 4;
        
        // Leave the digit on for the display time
        delay(digitDisplayTime);
        
        return; // Exit function early
    }
    
    // Normal display for countdown (when timer hasn't finished)
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
 *                   Void Loop                       *
 *************************************************** */
void loop() {
    // Only refresh the display at specific intervals
    unsigned long currentMillis = millis();
    if (currentMillis - displayRefreshTime >= displayRefreshInterval) {
        displayRefreshTime = currentMillis;
        
        // Display the digits
        DisplayAllDigits();
        
        /* *************************************
         *         Control Brightness          *
         * *********************************** */
        delayMicroseconds(1638*((100-brightness)/10));  // brightness control
    }
    
    /* *************************************
     *        Update Timer Logic           *
     * *********************************** */
    CountSecondTimer();
    
    // Add the beep check
    CheckAndPlayBeeps();
}
