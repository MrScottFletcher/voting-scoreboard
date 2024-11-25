//#include <Adafruit_NeoPixel.h>
#include <FastLED.h>
#include <DFRobotDFPlayerMini.h>
#include <SoftwareSerial.h>

#define SCOREBOARD_LED_PIN 6
#define CHASER_LED_PIN 7

#define NUM_SCOREBOARD_LEDS 174     // 174 for digits + 100 for border
#define NUM_CHASER_LEDS 100     // 174 for digits + 100 for border

#define NUM_BUTTONS 4
#define BUTTON_INC_LEFT  2
#define BUTTON_DEC_LEFT 3
#define BUTTON_INC_RIGHT 4
#define BUTTON_DEC_RIGHT 5

#define BORDER_START 175    // Starting LED index for the border
#define BORDER_COUNT 100    // Number of LEDs in the border

//=======================================
// Button pins
const int buttonPins[NUM_BUTTONS] = {
  BUTTON_INC_LEFT,
  BUTTON_DEC_LEFT,
  BUTTON_INC_RIGHT,
  BUTTON_DEC_RIGHT
};

// Button states
bool _buttonStates[NUM_BUTTONS] = {HIGH, HIGH, HIGH, HIGH}; // Current state
bool lastButtonStates[NUM_BUTTONS] = {HIGH, HIGH, HIGH, HIGH}; // Last known state
unsigned long _lastDebounceTimes[NUM_BUTTONS] = {0, 0, 0, 0};

// Debounce delay
unsigned long _lastDebounceTime = 0;
const unsigned long _debounceDelay = 50;

//=======================================

CRGB _scoreboardLEDs[NUM_SCOREBOARD_LEDS]; // Array to hold LED data
CRGB _chaserLEDs[NUM_CHASER_LEDS]; // Array to hold LED data

SoftwareSerial _myDFPlayerSerial(10, 11); // RX, TX for DFPlayer
DFRobotDFPlayerMini dfPlayer;

int _globalBrightness = 255;

int _scoreLeft = 0;
int _scoreRight = 0;
int leftSfxIndex = 1;
int rightSfxIndex = 6;

bool pulsing = false;        // Is the border pulsing?
unsigned long pulseStart = 0; // Start time of the pulse effect

int _segmentLEDCount = 4;
int _digitLEDCount = 29; //includes the one 'spare'

unsigned long _lastChaseUpdate = 0; // Tracks the last time the chaser was updated
unsigned long _chaseDelay = 100; // Tracks the last time the chaser was updated

const bool digits[][7] PROGMEM = {
{false,true,true,true,true,true,true}, // 0
{false,true,false,false,false,false,true}, // 1
{true,true,true,false,true,true,false}, // 2
{true,true,true,false,false,true,true}, // 3
{true,true,false,true,false,false,true}, // 4
{true,false,true,true,false,true,true}, // 5
{true,false,false,true,true,true,true}, // 6
{false,true,true,false,false,false,true}, // 7
{true,true,true,true,true,true,true}, // 8
{true,true,true,true,false,true,true}, // 9
};



void setup() {
  Serial.begin(9600);
  
  _myDFPlayerSerial.begin(9600);
  Serial.println("Test sketch uploaded successfully!");

  FastLED.addLeds<WS2812, SCOREBOARD_LED_PIN, GRB>(_scoreboardLEDs, NUM_SCOREBOARD_LEDS);
  FastLED.addLeds<WS2812, CHASER_LED_PIN, GRB>(_chaserLEDs, NUM_CHASER_LEDS);
  
  FastLED.clear(); // Initialize all LEDs to off
  FastLED.setBrightness(64); //For just testing on the bench
  FastLED.show();
  
  pinMode(BUTTON_INC_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_DEC_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_INC_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_DEC_RIGHT, INPUT_PULLUP);
  
  dfPlayer.begin(_myDFPlayerSerial);
  dfPlayer.volume(20);

  displayScore(_scoreLeft, 0, CRGB::Green);
  displayScore(_scoreRight, 3, CRGB::Red);
}

//========================================
void loop() {
  
  //scoreboardLoop();
  //singleDigitTestLoop(1);
  multiDigitTestLoop(6 );
  //firstDigitTestLoop();
  
  chasingLightsLoop(); 

  
}
//========================================

void chasingLightsLoop(){
  updateChasingLights(CRGB::Blue); 
}

// Function to create a chasing lights effect
void updateChasingLights(CRGB color) {
  static int _chasePosition = 0; // Tracks the current position of the chase
  unsigned long currentTime = millis();

  // Check if enough time has passed to update the chaser
  if (currentTime - _lastChaseUpdate >= _chaseDelay) {
      _lastChaseUpdate = currentTime; // Record the current time
      
    // Clear all LEDs
    setAllChaserLEDColor(CRGB::Black);
  
    // Turn on the current LED and a few trailing LEDs for the "chase" effect
    for (int i = 0; i < 3; i++) { // Adjust the range for longer or shorter trails
      int index = (_chasePosition - i + NUM_CHASER_LEDS) % NUM_CHASER_LEDS; // Wrap around using modulo
      _chaserLEDs[index] = color;
    }
  
    FastLED.show();
  
    // Move to the next position
    _chasePosition = (_chasePosition + 1) % NUM_CHASER_LEDS; // Wrap around when reaching the end
  }
}

void setAllChaserLEDColor(CRGB color){
  for (int i = 0; i < NUM_CHASER_LEDS; i++) {
    _chaserLEDs[i] = color;
  }
}

//=========================================

void scoreboardLoop(){
  displayScore(_scoreLeft, 0, CRGB::Green);
  displayScore(_scoreRight, 0, CRGB::Red);
  for (int i = 0; i < NUM_BUTTONS; i++) {
    int reading = digitalRead(buttonPins[i]);

    // Debounce logic
    if (reading != lastButtonStates[i]) {
      _lastDebounceTimes[i] = millis();
    }

    if ((millis() - _lastDebounceTimes[i]) > _debounceDelay) {
      // If the button state has changed
      if (reading != _buttonStates[i]) {
        _buttonStates[i] = reading;

        // Only act on button press (LOW)
        if (_buttonStates[i] == LOW) {
          handleButtonPress(i);
        }
      }
    }

    // Save the current reading for next iteration
    lastButtonStates[i] = reading;
  }
      
    // Handle border pulsing effect
    if (pulsing) {
      updatePulse();
    }

}

// Handle button press events
void handleButtonPress(int buttonIndex) {
  switch (buttonIndex) {
    case 0: // Left increment
      _scoreLeft = min(_scoreLeft + 1, 999);
      displayScore_Left(_scoreLeft);
      playLeftSfx();
      startPulse();
      break;

    case 1: // Left decrement
      _scoreLeft = max(_scoreLeft - 1, 0);
      displayScore_Left(_scoreLeft);
      break;

    case 2: // Right increment
      _scoreRight = min(_scoreRight + 1, 999);
      displayScore_Right(_scoreRight);
      playRightSfx();
      startPulse();
      break;

    case 3: // Right decrement
      _scoreRight = max(_scoreRight - 1, 0);
      displayScore_Right(_scoreRight);
      break;

    default:
      break;
  }
}
void displayScore_Right(int score){
  displayScore(score, 3, CRGB::Red);
}

void displayScore_Left(int score){
  displayScore(score, 3, CRGB::Green);
}

void displayDigit(int numberVal, int numberPosition, CRGB color) {
    int digitStartLED = numberPosition * (_digitLEDCount);

    for (int segment = 0; segment < 7; segment++) {
      int segmentStart = digitStartLED + (segment * _segmentLEDCount);
      if(segment >3) {
        segmentStart = segmentStart+1; // skip the spare in each digit
      }
      bool segmentState = pgm_read_byte(&digits[numberVal][segment]);
      setSegment(segmentStart, segmentState, color);
      FastLED.show();
    }

  FastLED.show(); // Update the LEDs to reflect the changes
}

void setSegment(int startLED, bool on, CRGB color) {
  for (int i = 0; i < _segmentLEDCount; i++) {
    _scoreboardLEDs[startLED + 1 + i] = on ? color : CRGB::Black; // Use red for "on" and black for "off"
  }
}

// Function to start the pulsing effect
void startPulse() {
  pulsing = true;
  pulseStart = millis();
}

// Function to update the pulsing effect
void updatePulse() {
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - pulseStart;

  // Calculate phase (on/off every 125ms for 4 cycles in 1 second)
  int phase = (elapsed / 125) % 2;

  if (elapsed >= 1000) {
    pulsing = false; // Stop pulsing after 1 second
    clearBorder();
    return;
  }

  // Set border LEDs to on or off based on the phase
  setBorderColor(phase == 0 ? CRGB::Green : CRGB::Black);
  FastLED.show();
}

// Function to set the border color
void setBorderColor(CRGB color) {
  for (int i = BORDER_START; i < BORDER_START + BORDER_COUNT; i++) {
    _scoreboardLEDs[i] = color;
  }
}

// Function to clear the border LEDs
void clearBorder() {
  setBorderColor(CRGB::Black);
  FastLED.show();
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
void displayScore(int score, int startDigit, CRGB color) {
  int hundreds = score / 100;
  int tens = (score / 10) % 10;
  int units = score % 10;
  
  displayDigit(hundreds, startDigit, color);
  displayDigit(tens, startDigit + 1, color);
  displayDigit(units, startDigit + 2, color);
}



//==================================================================================================================
//==================================================================================================================
int serialPrintf(const char *format, ...) {
  char buffer[128]; // Adjust size if needed
  va_list args;
  va_start(args, format);
  int n = vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  Serial.print(buffer);
  return n;
}

void firstDigitTestLoop()
{
    //Test each segment
    //Turn all on White
    FastLED.clear();
    fill_solid(_scoreboardLEDs, NUM_SCOREBOARD_LEDS, CRGB::White); 
    FastLED.show();
    
    delay(1000);

    //blank for a second
    FastLED.clear();
    FastLED.show();
    delay(1000);

    //Test each segment
    for (int segment = 0; segment < 7; segment++) {
      int segmentStart = segment * _segmentLEDCount;
      if(segment >3) {
        segmentStart = segmentStart+1;// = segmentStart + 1; //skip the spare in each digit
      }
      setSegment(segmentStart, true, CRGB::Red);
      //setSegment(segmentStart + (segment * _segmentLEDCount), digits[digit][segment]);
      FastLED.show();
      delay(200);                      // Wait 1 second before changing
    }
    
    for(int i=0; i<10;i++){
    //--reset on every digit
      displayDigit(i,0, CRGB::Red); //zero shows in the first digit
      FastLED.show();
      delay(300);                      // Wait 1 second before changing
    }
}

void multiDigitTestLoop(int numDigits)
{
    //Test each segment
    //Turn all on White
    FastLED.clear();
    fill_solid(_scoreboardLEDs, NUM_SCOREBOARD_LEDS, CRGB::White); 
    FastLED.show();
    
    delay(500);

    //blank for a second
    FastLED.clear();
    FastLED.show();
    delay(500);

    //Test each segment
    for(int currentDigit = 0; currentDigit < numDigits; currentDigit++){
      int digitStartLED = currentDigit * 29;
      
      for (int segment = 0; segment < 7; segment++) {
        int segmentStart = digitStartLED + (segment * _segmentLEDCount);
        if(segment >3) {
          segmentStart = segmentStart+1;// = segmentStart + 1; //skip the spare in each digit
        }
        setSegment(segmentStart, true, CRGB::Red);
        //setSegment(segmentStart + (segment * _segmentLEDCount), digits[digit][segment]);
        FastLED.show();
        delay(30);                      // Wait 1 second before changing
      }
    }
    
      for(int currentDigit = 0; currentDigit < numDigits; currentDigit++){
        for(int i=0; i<10;i++){
          //--reset on every digit
            displayDigit(i,currentDigit, CRGB::Green); //zero shows in the first digit
            FastLED.show();
            delay(100);                      // Wait 1 second before changing
          }
         displayDigit(8,currentDigit, CRGB::Blue);  //set to 8 to illuminate all segments
      }
}

void singleDigitTestLoop(int digitPosition)
{
    //Test each segment
    //Turn all on White
    FastLED.clear();
    fill_solid(_scoreboardLEDs, NUM_SCOREBOARD_LEDS, CRGB::White); 
    FastLED.show();
    
    delay(1000);

    //blank for a second
    FastLED.clear();
    FastLED.show();
    delay(1000);

    //Test each segment
      int digitStartLED = digitPosition * _digitLEDCount;
      
      for (int segment = 0; segment < 7; segment++) {
        int segmentStart = digitStartLED + (segment * _segmentLEDCount);
        if(segment >3) {
          segmentStart = segmentStart+1;// = segmentStart + 1; //skip the spare in each digit
        }
        setSegment(segmentStart, true, CRGB::Red);
        FastLED.show();
        delay(75);                      // Wait 1 second before changing
      }
    
      for(int i=0; i<10;i++){
        //--reset on every digit
          displayDigit(i,digitPosition, CRGB::Red); //zero shows in the first digit
          FastLED.show();
          delay(1500);                      // Wait 1 second before changing
        }
}
