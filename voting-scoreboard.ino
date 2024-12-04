//#include <Adafruit_NeoPixel.h>
#include <FastLED.h>
#include <DFRobotDFPlayerMini.h>
//#include <SoftwareSerial.h>

#define SCOREBOARD_LED_PIN 6
#define CHASER_LED_PIN 7

#define NUM_SCOREBOARD_LEDS 174     // 174 for digits + 100 for border
#define NUM_CHASER_LEDS 50     // 174 for digits + 100 for border

#define NUM_BUTTONS 6
#define BUTTON_INC_LEFT  2
#define BUTTON_DEC_LEFT 3
#define BUTTON_INC_RIGHT 4
#define BUTTON_DEC_RIGHT 5

#define BUTTON_INC_LEFT_ALT 8  // New button for left score increment
#define BUTTON_INC_RIGHT_ALT 9 // New button for right score increment

#define BUTTON_TEST_SCORE 8  
#define BUTTON_TEST_SOUND 10
#define BUTTON_TEST_CHASE 9 


#define BORDER_START 175    // Starting LED index for the border
#define BORDER_COUNT 50    // Number of LEDs in the border

//=======================================
// Button pins
const int buttonPins[NUM_BUTTONS + 2] = {
  BUTTON_INC_LEFT,
  BUTTON_DEC_LEFT,
  BUTTON_INC_RIGHT,
  BUTTON_DEC_RIGHT,
  BUTTON_INC_LEFT_ALT,  // New button
  BUTTON_INC_RIGHT_ALT  // New button
};


// Button states
bool _buttonStates[NUM_BUTTONS] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH}; // Current state

bool lastButtonStates[NUM_BUTTONS] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
unsigned long _lastDebounceTimes[NUM_BUTTONS] = {0, 0, 0, 0, 0, 0};

// Debounce delay
const unsigned long _debounceDelay = 50;

//=======================================

CRGB _scoreboardLEDs[NUM_SCOREBOARD_LEDS]; // Array to hold LED data
CRGB _chaserLEDs[NUM_CHASER_LEDS]; // Array to hold LED data

//SoftwareSerial _myDFPlayerSerial(18, 11); // RX, TX for _dfPlayer
DFRobotDFPlayerMini _dfPlayer;

int _globalBrightness = 255;

int _scoreLeft = 0;
int _scoreRight = 0;
int _leftSfxIndex = 0;
int _rightSfxIndex = 5;

bool _pulsing = false;        // Is the border _pulsing?
unsigned long _pulseStart = 0; // Start time of the pulse effect

int _segmentLEDCount = 4;
int _digitLEDCount = 29; //includes the one 'spare'

unsigned long _lastChaseUpdate = 0; // Tracks the last time the chaser was updated
unsigned long _chaseDelay = 500; // Tracks the last time the chaser was updated

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
//===============================================

bool _showRainbow = false;       // Flag to indicate the rainbow effect is active
unsigned long _rainbowStartTime; // Timestamp when the rainbow effect starts
const unsigned long _rainbowDuration = 5000; // Rainbow effect duration (2 seconds)
bool _rainbowDelayActive = false;      // Flag to indicate if the delay is active
unsigned long _rainbowDelayStartTime;  // Start time for the 1-second delay

//===============================================
//  // AUDIO TEST LOOP
//void setup() {
//  Serial.begin(9600);    // Debugging via USB
//  Serial2.begin(9600);   // Start Serial2 for DFPlayer communication
//
//  if (!_dfPlayer.begin(Serial2)) { // Use Serial2 instead of SoftwareSerial
//    Serial.println("DFPlayer initialization failed!");
//    while (true); // Stop execution if DFPlayer fails to initialize
//  }
//
//  _dfPlayer.volume(15); // Set volume level (0-30)
//  Serial.println("DFPlayer initialized successfully!");
//}

//void loop() {
//  // Example: Play track 1
//  Serial.println("Playing Track 1!");
//  _dfPlayer.play(1);
//  delay(5000); // Wait for the track to play
//}
//===============================================

void setup() {
  
  Serial.begin(9600);    // Debugging via USB
  Serial2.begin(9600);   // Start Serial2 for DFPlayer communication

  if (!_dfPlayer.begin(Serial2)) { // Use Serial2 instead of SoftwareSerial
    Serial.println("DFPlayer initialization failed!");
    while (true); // Stop execution if DFPlayer fails to initialize
  }

  _dfPlayer.volume(15); // Set volume level (0-30)
  Serial.println("DFPlayer initialized successfully!");Serial.println("setup() starting...");

  FastLED.addLeds<WS2812, SCOREBOARD_LED_PIN, GRB>(_scoreboardLEDs, NUM_SCOREBOARD_LEDS);
  FastLED.addLeds<WS2812, CHASER_LED_PIN, GRB>(_chaserLEDs, NUM_CHASER_LEDS);
  
  FastLED.clear(); // Initialize all LEDs to off
  FastLED.setBrightness(64); //For just testing on the bench
  //FastLED.setBrightness(128); //For just testing on the bench
  //FastLED.setBrightness(255); //For just testing on the bench
  FastLED.show();
  
  pinMode(BUTTON_INC_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_DEC_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_INC_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_DEC_RIGHT, INPUT_PULLUP);

  pinMode(BUTTON_INC_LEFT_ALT, INPUT_PULLUP);
  pinMode(BUTTON_INC_RIGHT_ALT, INPUT_PULLUP);

  displayScore(_scoreLeft, 0, CRGB::Green);
  displayScore(_scoreRight, 3, CRGB::Red);
  Serial.println("setup() complete!");
  Serial.println("Staring multiDigit TestLoop()...");
  multiDigitTestLoop(6);
  Serial.println("multiDigitTestLoop() complete!");
}

//========================================
void loop() {
  // Handle scoreboard logic
    scoreboardLoop();

  // Handle the rainbow delay logic
  if (_rainbowDelayActive) {
    unsigned long currentTime = millis();
    Serial.print("Checking rainbow delay. Current time: ");
    Serial.print(currentTime);
    Serial.print(", Start time: ");
    Serial.println(_rainbowDelayStartTime);

    if (currentTime - _rainbowDelayStartTime >= 1000) {
      Serial.println("Rainbow delay completed.");
      _rainbowDelayActive = false; // Stop the delay
      startRainbowEffect();       // Trigger the rainbow effect
    }
  }

  // If no rainbow effect is active, handle marquee lights
  if (!_showRainbow) {
    marqueeChaseEffect();
  } else {
    updateRainbowEffect();
  }

  FastLED.show();
}
//========================================

void marqueeChaseEffect() {
  static uint8_t chasePosition = 0;      // Tracks the current position in the pattern
  static unsigned long lastUpdate = 0;  // Tracks the last update time

  unsigned long currentTime = millis();

  // Check if enough time has passed to update the marquee
  if (currentTime - lastUpdate >= _chaseDelay) {
    lastUpdate = currentTime; // Update the last update time

    // Clear only the chaser LEDs
    //setAllChaserLEDColor(CRGB::Black);

    //As recommended by ChatGPT
    // Clear all chaser LEDs explicitly
//    for (int i = 0; i < NUM_CHASER_LEDS; i++) {
//      _chaserLEDs[i] = CRGB::Black; // Reset to "off"
//    }
    
    // Loop through all chaser LEDs and assign them to a "chasing group"
    for (int i = 0; i < NUM_CHASER_LEDS; i++) {
      if ((i + chasePosition) % 3 == 0) {
        _chaserLEDs[i] = CRGB::White; // Set the "on" LEDs
      }
      else{
        _chaserLEDs[i] = CRGB::Black; // Set the "off" LEDs
      }
    }

    // Update only the chaser LEDs
    //FastLED.show();

    // Move the position forward for the chase effect
    chasePosition = (chasePosition + 1) % 3;
  }
}


void setAllChaserLEDColor(CRGB color){
  for (int i = 0; i < NUM_CHASER_LEDS; i++) {
    _chaserLEDs[i] = color;
  }
}

void updateRainbowEffect() {
  unsigned long _currentTime = millis();

  // Apply the rainbow effect
  static uint8_t _hue = 0; // Starting hue
  //fill_rainbow(_scoreboardLEDs, NUM_SCOREBOARD_LEDS, _hue, 7); // Adjust "7" for rainbow spread
  fill_rainbow(_chaserLEDs, NUM_CHASER_LEDS, _hue, 7);         // Adjust for chaser LEDs

  //FastLED.show();

  // Increment the hue for the rainbow animation
  _hue += 5;

  // Check if the effect duration has elapsed
  if (_currentTime - _rainbowStartTime >= _rainbowDuration) {
    _showRainbow = false; // Stop the rainbow effect
    //clearScoreboard();   // Reset scoreboard LEDs
    //clearChaser();       // Reset chaser LEDs
  }
}

void clearScoreboard() {
  for (int i = 0; i < NUM_SCOREBOARD_LEDS; i++) {
    _scoreboardLEDs[i] = CRGB::Black;
  }
  //FastLED.show();
}

void clearChaser() {
  for (int i = 0; i < NUM_CHASER_LEDS; i++) {
    _chaserLEDs[i] = CRGB::Black;
  }
  //FastLED.show();
}


//=========================================

void scoreboardLoop(){
  displayScore(_scoreLeft, 0, CRGB::Green);
  displayScore(_scoreRight, 3, CRGB::Red);
  for (int i = 0; i < NUM_BUTTONS; i++) {
    int reading = digitalRead(buttonPins[i]);
    
    if (reading != lastButtonStates[i]) {
      _lastDebounceTimes[i] = millis();
    }
    
    if ((millis() - _lastDebounceTimes[i]) > _debounceDelay && reading != _buttonStates[i]) {
      _buttonStates[i] = reading;
      if (_buttonStates[i] == LOW) {
        handleButtonPress(i);
      }
    }
    
    // Save the current reading for next iteration
    lastButtonStates[i] = reading;
  }
}

void handleButtonPress(int buttonIndex) {
  switch (buttonIndex) {
    case 0:  // Left increment
    case 4:  // Left increment (alternate)
      _scoreLeft = min(_scoreLeft + 1, 999);
      displayScore_Left(_scoreLeft);
      playLeftSfx();
      startRainbowDelay(); // Start the 1-second delay
      break;

    case 2:  // Right increment
    case 5:  // Right increment (alternate)
      _scoreRight = min(_scoreRight + 1, 999);
      displayScore_Right(_scoreRight);
      playRightSfx();
      startRainbowDelay(); // Start the 1-second delay
      break;

    case 1:  // Left decrement
      _scoreLeft = max(_scoreLeft - 1, 0);
      displayScore_Left(_scoreLeft);
      break;

    case 3:  // Right decrement
      _scoreRight = max(_scoreRight - 1, 0);
      displayScore_Right(_scoreRight);
      break;
  }
}


void displayScore_Right(int score){
  displayScore(score, 3, CRGB::Red);
}

void displayScore_Left(int score){
  displayScore(score, 0, CRGB::Green);
}

// Display functions (same as before)
void displayScore(int score, int startDigit, CRGB color) {
  int hundreds = score / 100;
  int tens = (score / 10) % 10;
  int units = score % 10;
  
  displayDigit(hundreds, startDigit, color);
  displayDigit(tens, startDigit + 1, color);
  displayDigit(units, startDigit + 2, color);
  
  //cli();
  //FastLED.show(); // Update the LEDs to reflect the changes
  //sei();
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
    }

  //Let the caller do it
  //FastLED.show(); // Update the LEDs to reflect the changes
}

void setSegment(int startLED, bool on, CRGB color) {
  for (int i = 0; i < _segmentLEDCount; i++) {
    _scoreboardLEDs[startLED + 1 + i] = on ? color : CRGB::Black; // Use red for "on" and black for "off"
  }
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
  //FastLED.show();
}

// Function to play left side sound effects in sequence
void playLeftSfx() {
  Serial.print("Play Left SFX index:");
  Serial.println(_leftSfxIndex);
  _dfPlayer.play(_leftSfxIndex);
  Serial.print("Set next Left SFX index");
  _leftSfxIndex++;
  if (_leftSfxIndex > 4) _leftSfxIndex = 1;
}

// Function to play right side sound effects in sequence
void playRightSfx() {
  Serial.print("Play Right SFX index:");
  Serial.println(_rightSfxIndex);
  _dfPlayer.play(_rightSfxIndex);
  Serial.print("Set next Right SFX index");
  _rightSfxIndex++;
  if (_rightSfxIndex > 9) _rightSfxIndex = 5;
}

//==================================================================================================================
void startRainbowDelay() {
  _rainbowDelayActive = true;
  _rainbowDelayStartTime = millis(); // Record the current time
  Serial.print("Rainbow delay started. Start time: ");
  Serial.println(_rainbowDelayStartTime);
}

void startRainbowEffect() {
  _showRainbow = true;
  _rainbowStartTime = millis(); // Record the start time
}

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
        delay(5);                      // Wait 1 second before changing
      }
    }
    
      for(int currentDigit = 0; currentDigit < numDigits; currentDigit++){
        for(int i=0; i<10;i++){
          //--reset on every digit
            displayDigit(i,currentDigit, CRGB::Green); //zero shows in the first digit
            FastLED.show();
            delay(10);                      // Wait 1 second before changing
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
