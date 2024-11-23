#include <Adafruit_NeoPixel.h>
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>

#define LED_PIN 6
#define NUM_LEDS 268     // 168 for digits + 100 for border
#define BUTTON_INC_LEFT 2
#define BUTTON_DEC_LEFT 3
#define BUTTON_INC_RIGHT 4
#define BUTTON_DEC_RIGHT 5

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
SoftwareSerial mySerial(10, 11); // RX, TX for DFPlayer
DFRobotDFPlayerMini dfPlayer;

int scoreLeft = 0;
int scoreRight = 0;
int leftSfxIndex = 1;
int rightSfxIndex = 6;
bool pulsing = false;  // Track if the border is pulsing
unsigned long pulseStartTime = 0;

// Define the segments for the 7-segment display (same as before)
bool digits[10][7] = {
  {true, true, true, true, true, true, false},    // 0
  {false, true, true, false, false, false, false}, // 1
  {true, true, false, true, true, false, true},   // 2
  {true, true, true, true, false, false, true},   // 3
  {false, true, true, false, false, true, true},  // 4
  {true, false, true, true, false, true, true},   // 5
  {true, false, true, true, true, true, true},    // 6
  {true, true, true, false, false, false, false}, // 7
  {true, true, true, true, true, true, true},     // 8
  {true, true, true, true, false, true, true}     // 9
};

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

void setup() {
  strip.begin();
  strip.show();
  
  pinMode(BUTTON_INC_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_DEC_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_INC_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_DEC_RIGHT, INPUT_PULLUP);

  mySerial.begin(9600);
  dfPlayer.begin(mySerial);
  dfPlayer.volume(20);

  displayScore(scoreLeft, 0);
  displayScore(scoreRight, 3);
}

void loop() {
  // Handle left score increment and play sound
  if (digitalRead(BUTTON_INC_LEFT) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      scoreLeft = min(scoreLeft + 1, 999);
      displayScore(scoreLeft, 0);
      playLeftSfx();  // Play sound for left side increment
      startPulse();   // Start the border pulsing
      lastDebounceTime = millis();
    }
  }

  // Handle left score decrement
  if (digitalRead(BUTTON_DEC_LEFT) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      scoreLeft = max(scoreLeft - 1, 0);
      displayScore(scoreLeft, 0);
      lastDebounceTime = millis();
    }
  }

  // Handle right score increment and play sound
  if (digitalRead(BUTTON_INC_RIGHT) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      scoreRight = min(scoreRight + 1, 999);
      displayScore(scoreRight, 3);
      playRightSfx(); // Play sound for right side increment
      startPulse();   // Start the border pulsing
      lastDebounceTime = millis();
    }
  }

  // Handle right score decrement
  if (digitalRead(BUTTON_DEC_RIGHT) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      scoreRight = max(scoreRight - 1, 0);
      displayScore(scoreRight, 3);
      lastDebounceTime = millis();
    }
  }

  // Handle border pulsing effect
  if (pulsing) {
    updatePulse();
  }
}

// Function to start the pulsing effect
void startPulse() {
  pulsing = true;
  pulseStartTime = millis();
}

// Function to update the pulsing effect
void updatePulse() {
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - pulseStartTime;
  int pulsePhase = (elapsed / 125) % 2;  // 125 ms on/off for 4 pulses in 1 second
  
  if (elapsed >= 1000) {
    pulsing = false;  // Stop pulsing after 1 second
    clearBorder();
  } else {
    if (pulsePhase == 0) {
      setBorderColor(strip.Color(0, 255, 0)); // Border on (green color)
    } else {
      setBorderColor(strip.Color(0, 0, 0));   // Border off
    }
    strip.show();
  }
}

// Function to set the border color
void setBorderColor(uint32_t color) {
  for (int i = 168; i < 268; i++) {  // Border LEDs are from 168 to 267
    strip.setPixelColor(i, color);
  }
}

// Function to clear the border
void clearBorder() {
  setBorderColor(strip.Color(0, 0, 0));
  strip.show();
}

// Function to play left side sound effects in sequence
void playLeftSfx() {
  dfPlayer.play(leftSfxIndex);
  leftSfxIndex++;
  if (leftSfxIndex > 5) leftSfxIndex = 1;
}

// Function to play right side sound effects in sequence
void playRightSfx() {
  dfPlayer.play(rightSfxIndex);
  rightSfxIndex++;
  if (rightSfxIndex > 10) rightSfxIndex = 6;
}

// Display functions (same as before)
void displayScore(int score, int startDigit) {
  int hundreds = score / 100;
  int tens = (score / 10) % 10;
  int units = score % 10;
  
  displayDigit(hundreds, startDigit);
  displayDigit(tens, startDigit + 1);
  displayDigit(units, startDigit + 2);
}

void displayDigit(int digit, int position) {
  int segmentStart = position * 28;
  
  for (int segment = 0; segment < 7; segment++) {
    setSegment(segmentStart + (segment * 4), digits[digit][segment]);
  }
  strip.show();
}

void setSegment(int startLED, bool on) {
  for (int i = 0; i < 4; i++) {
    strip.setPixelColor(startLED + i, on ? strip.Color(255, 0, 0) : strip.Color(0, 0, 0));
  }
}
