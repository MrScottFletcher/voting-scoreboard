#include <FastLED.h>
#include <DFPlayerMini_Fast.h> //Soooo much better than DFRobotDFPlayerMini.h
#include <EEPROM.h>

#define SCOREBOARD_LED_PIN 6
#define CHASER_LED_PIN 7

#define NUM_SCOREBOARD_LEDS 174
#define NUM_CHASER_LEDS 50

#define NUM_BUTTONS 6
#define BUTTON_INC_LEFT  2
#define BUTTON_DEC_LEFT 3
#define BUTTON_INC_RIGHT 4
#define BUTTON_DEC_RIGHT 5

#define BUTTON_INC_LEFT_ALT 11
#define BUTTON_INC_RIGHT_ALT 12

#define RESET_BUTTON_PIN 13 // Pin for the reset button

#define NEXT_SONG_BUTTON_PIN 9
#define PREV_SONG_BUTTON_PIN 10


#define BORDER_START 175
#define BORDER_COUNT 50

#define EEPROM_SCORE_LEFT_ADDR 0
#define EEPROM_SCORE_RIGHT_ADDR 4

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

//=======================================
// Button pins
const int _buttonPins[NUM_BUTTONS + 2] = {
  BUTTON_INC_LEFT,
  BUTTON_DEC_LEFT,
  BUTTON_INC_RIGHT,
  BUTTON_DEC_RIGHT,
  BUTTON_INC_LEFT_ALT,  // New button
  BUTTON_INC_RIGHT_ALT  // New button
};


bool _buttonStates[NUM_BUTTONS] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
bool _lastButtonStates[NUM_BUTTONS] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
unsigned long _lastDebounceTimes[NUM_BUTTONS] = {0, 0, 0, 0, 0, 0};
const unsigned long _debounceDelay = 50;

unsigned long _lastResetPressTime = 0;
const unsigned long _resetDebounceDelay = 200;

unsigned long _lastButtonPressTime = 0;
const unsigned long _buttonDebounceDelay = 200;

CRGB _scoreboardLEDs[NUM_SCOREBOARD_LEDS];
CRGB _chaserLEDs[NUM_CHASER_LEDS];

DFPlayerMini_Fast _dfPlayer_SFX;
DFPlayerMini_Fast _dfPlayer_Music;

int _globalBrightness = 255;

//Initialize the score, but will be loaded from EPROM later
int _scoreLeft = 0;
int _scoreRight = 0;

int _segmentLEDCount = 4;
int _digitLEDCount = 29; //includes the one 'spare'

enum SystemState {
  NORMAL,
  PLAYING_EFFECT,
  PLAYING_ANIMATION
};

SystemState _systemState = NORMAL;

unsigned long _effectStartTime = 0;
unsigned long _animationStartTime = 0;

unsigned long _lastChaseUpdate = 0; // Tracks the last time the chaser was updated
unsigned long _chaseDelay = 500; // Tracks the last time the chaser was updated

struct SoundEffect {
  int id;
  int duration;
};

const SoundEffect _bruceQuotes[] = {
    {001, 5000}, {002, 3000}, {003, 3000}, {004, 3500}
};
const int _numBruceQuotes = sizeof(_bruceQuotes) / sizeof(SoundEffect);
int _currentEffectIndex = 0;

const int _soundEffects[] = {
  200, 201, 202
};
const int _numSoundEffects = sizeof(_soundEffects) / sizeof(int);

typedef void (*AnimationFunction)();
void rainbowEffect();
void twinkleEffect();
void meteorEffect();
void confettiEffect();
void runningLightsEffect();
void sparkleEffect();
void waveEffect();

//No thanks.
//void spaceLaserEffect();
//void patrioticEffect();
//void fireEffect();

AnimationFunction _animations[] = {rainbowEffect, twinkleEffect, meteorEffect, confettiEffect, runningLightsEffect, sparkleEffect, waveEffect};
const int _numAnimations = sizeof(_animations) / sizeof(AnimationFunction);
int _currentAnimationIndex = 0;

unsigned long _rainbowDelayStartTime;
bool _rainbowDelayActive = false;

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);
  Serial3.begin(9600);

  if (!_dfPlayer_SFX.begin(Serial2)) {
    Serial.println("SFX DFPlayer initialization failed!");
    while (true);
  }

  if (!_dfPlayer_Music.begin(Serial3)) {
    Serial.println("Music DFPlayer initialization failed!");
    while (true);
  }

  _dfPlayer_SFX.setTimeout(500); //Set serial communictaion time out 500ms
  _dfPlayer_Music.setTimeout(500); //Set serial communictaion time out 500ms
  delay(100);

  _dfPlayer_SFX.volume(15);
  _dfPlayer_Music.volume(15);
  delay(100);

    //----Set different EQ----
  _dfPlayer_SFX.EQSelect(0);
  _dfPlayer_Music.EQSelect(0);
  delay(100);

  Serial.println("DFPlayer(s) initialized!");

  //Play the startup sound while the LEDs are testing
  Serial.println("Playing startup sound on SFX player...");
  _dfPlayer_SFX.playFolder(2,1);
  Serial.println("Playing startup sound on Music player...");
  _dfPlayer_Music.playFolder(2,1);
  delay(300);

  FastLED.addLeds<WS2812, SCOREBOARD_LED_PIN, GRB>(_scoreboardLEDs, NUM_SCOREBOARD_LEDS);
  FastLED.addLeds<WS2812, CHASER_LED_PIN, GRB>(_chaserLEDs, NUM_CHASER_LEDS);

  FastLED.clear();
  //FastLED.setBrightness(64); //For just testing on the bench
  //FastLED.setBrightness(128); //For just testing on the bench
  FastLED.setBrightness(255); //For just testing on the bench
  FastLED.show();

//Do NOT loop.  The last two are copies
//  for (int i = 0; i < NUM_BUTTONS; i++) {
//    pinMode(_buttonPins[i], INPUT_PULLUP);
//  }

  pinMode(BUTTON_INC_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_DEC_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_INC_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_DEC_RIGHT, INPUT_PULLUP);

  pinMode(BUTTON_INC_LEFT_ALT, INPUT_PULLUP);
  pinMode(BUTTON_INC_RIGHT_ALT, INPUT_PULLUP);

  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  // Initialize next and previous song buttons
  pinMode(NEXT_SONG_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PREV_SONG_BUTTON_PIN, INPUT_PULLUP);

  randomSeed(analogRead(0));

  // Initialize reset button pin

  // Load scores from EEPROM
  Serial.println("Loading scores from EPROM...");
  loadScoresFromEEPROM();

  Serial.println("Before Setting score...");
  Serial.print("_scoreLeft: ");
  Serial.print(_scoreLeft);
  Serial.print(" _scoreRight: ");
  Serial.println(_scoreRight);

  displayScore(_scoreLeft, 0, CRGB::Green);
  displayScore(_scoreRight, 3, CRGB::Red);

  Serial.println("AFTER setting score...");
  Serial.print("_scoreLeft: ");
  Serial.print(_scoreLeft);
  Serial.print(" _scoreRight: ");
  Serial.println(_scoreRight);

  Serial.println("setup() complete!");
  Serial.println("Staring multiDigit TestLoop()...");
  multiDigitTestLoop(6);
  Serial.println("multiDigitTestLoop() complete!");

  Serial.println("AFTER Test Loop...");
  Serial.print("_scoreLeft: ");
  Serial.print(_scoreLeft);
  Serial.print(" _scoreRight: ");
  Serial.println(_scoreRight);

  Serial.println("_numBruceQuotes: ");
  Serial.print(_numBruceQuotes);
  Serial.print(" sizeof(_bruceQuotes): ");
  Serial.println(sizeof(_bruceQuotes));
  Serial.print(" sizeof(SoundEffect): ");
  Serial.println(sizeof(SoundEffect));

  displayScore(_scoreLeft, 0, CRGB::Green);
  displayScore(_scoreRight, 3, CRGB::Red);

  Serial.print(F("Files found DFPlayer SFX Folder 1: "));
  Serial.println(_dfPlayer_SFX.numTracksInFolder(1));

  Serial.print(F("Files found DFPlayer Music Folder 1: "));
  Serial.println(_dfPlayer_Music.numTracksInFolder(1));

  Serial.println("---- setup() method end ----");

  //Future - check the "music" switch to see if music is enabled.
  //Start the music
  _dfPlayer_Music.repeatFolder(1);
  Serial.println("(loop folder command issued and returned)");
}

//##############################################################

void loop() {
  scoreboardLoop();

  unsigned long currentTime = millis();

  //--------------------
  // Check if reset button is pressed
  if (digitalRead(RESET_BUTTON_PIN) == LOW) {
    if (currentTime - _lastResetPressTime > _resetDebounceDelay) {
      resetScores();
      _lastResetPressTime = currentTime;
    }
  }
  //--------------------
// Check Next Song button
  if (digitalRead(NEXT_SONG_BUTTON_PIN) == LOW) {
    if (currentTime - _lastButtonPressTime > _buttonDebounceDelay) {
      playNextSong();
      _lastButtonPressTime = currentTime;
    }
  }

  // Check Previous Song button
  if (digitalRead(PREV_SONG_BUTTON_PIN) == LOW) {
    if (currentTime - _lastButtonPressTime > _buttonDebounceDelay) {
      playPreviousSong();
      _lastButtonPressTime = currentTime;
    }
  }
  //--------------------
  switch (_systemState) {
    case NORMAL:
      marqueeChaseEffect();
      break;

    case PLAYING_EFFECT:
      if (millis() - _effectStartTime >= _bruceQuotes[_currentEffectIndex].duration) {
        playRandom_SFX_NonBlocking();
        _systemState = PLAYING_ANIMATION;
      }
      break;

    case PLAYING_ANIMATION:
      if (millis() - _animationStartTime < 5000) {
        _animations[_currentAnimationIndex]();
      } else {
        _systemState = NORMAL;
      }
      break;
  }
  FastLED.show();
}

//#################################################################
void scoreboardLoop() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    int reading = digitalRead(_buttonPins[i]);

    if (reading != _lastButtonStates[i]) {
      _lastDebounceTimes[i] = millis();
    }

    if ((millis() - _lastDebounceTimes[i]) > _debounceDelay && reading != _buttonStates[i]) {
      _buttonStates[i] = reading;
      if (_buttonStates[i] == LOW) {
        handleButtonPress(i);
      }
    }

    _lastButtonStates[i] = reading;
  }
}

//==========================================================================
//      BUTTONS
//==========================================================================

void handleButtonPress(int buttonIndex) {
  switch (buttonIndex) {
    case 0:  // Left increment
    case 4:  // Left increment (alternate)
      _scoreLeft = min(_scoreLeft + 1, 999);
      saveScoresToEEPROM(); // Save updated score
      displayScore_Left(_scoreLeft);
      initiateSoundEffect();
      break;

    case 2:  // Right increment
    case 5:  // Right increment (alternate)
      _scoreRight = min(_scoreRight + 1, 999);
      saveScoresToEEPROM(); // Save updated score
      displayScore_Right(_scoreRight);
      initiateSoundEffect();
      break;

    case 1: // Left decrement
      _scoreLeft = max(_scoreLeft - 1, 0);
      saveScoresToEEPROM(); // Save updated score
      displayScore_Left(_scoreLeft);
      break;

    case 3: // Right decrement
      _scoreRight = max(_scoreRight - 1, 0);
      saveScoresToEEPROM(); // Save updated score
      displayScore_Right(_scoreRight);
      break;
  }
}

//==========================================================================
//      SOUNDS 
//==========================================================================
void initiateSoundEffect() {
  if (_systemState == NORMAL) {

    Serial.println("Pausing Music");
    _dfPlayer_Music.pause();
    delay(200);

    Serial.println("Pausing current sound");
    _dfPlayer_SFX.pause();
    delay(200);

    _currentEffectIndex++;

    _currentEffectIndex = _currentEffectIndex % _numBruceQuotes;
    
    Serial.print("NEW _currentEffectIndex: ");
    Serial.print(_currentEffectIndex);

    Serial.print(" ----- _bruceQuotes[_currentEffectIndex].id: ");
    Serial.print(_bruceQuotes[_currentEffectIndex].id);

    Serial.print(" having duration: ");
    Serial.println(_bruceQuotes[_currentEffectIndex].duration);

     _dfPlayer_SFX.playFolder(1,_bruceQuotes[_currentEffectIndex].id);
    _effectStartTime = millis();
    _systemState = PLAYING_EFFECT;
  }
}

void playRandom_SFX_NonBlocking() {
  Serial.println("Pausing current sound");
  _dfPlayer_SFX.pause();
  delay(100);
  Serial.println("-- SFX Paused");

  Serial.println("Pausing Music");
  _dfPlayer_Music.pause();
  delay(100);
  Serial.println("-- Music Paused");

  int soundEffectIndex = random(0, _numSoundEffects);

  _dfPlayer_SFX.playFolder(1,_soundEffects[soundEffectIndex]);
  Serial.println("(play command issued and returned)");
  delay(100);
  
  _currentAnimationIndex = random(0, _numAnimations);
  Serial.print("New _currentAnimationIndex:  ");
  Serial.println(_currentAnimationIndex);

  Serial.println("Resuming Music");
  _dfPlayer_Music.resume();
  delay(100);
  Serial.println("-- Music Resumed.");

  _animationStartTime = millis();
}

void playNextSong() {
  _dfPlayer_Music.playNext(); // Play the next song
  Serial.println("Playing the next song.");
}

void playPreviousSong() {
  _dfPlayer_Music.playPrevious(); // Play the previous song
  Serial.println("Playing the previous song.");
}
//==========================================================================
//      ANIMATIONS 
//==========================================================================

void startRandomAnimationEffect() {
  _currentAnimationIndex = random(0, _numAnimations);
  _animationStartTime = millis();
  _systemState = PLAYING_ANIMATION;
}

void rainbowEffect() {
  static uint8_t hue = 0;                // Tracks the starting hue
  static unsigned long lastUpdate = 0;  // Tracks the last update time
  unsigned long currentTime = millis();

  if (currentTime - lastUpdate >= 3) { // Adjust delay for speed
    lastUpdate = currentTime;
    fill_rainbow(_chaserLEDs, NUM_CHASER_LEDS, hue, 7); // Fill with rainbow colors
    //hue++; // Increment hue for the next update
    hue = hue+2; // Increment hue MORE for the next update
  }
}

void fireEffect() {
  static unsigned long lastUpdate = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastUpdate >= 50) { // Adjust delay for flicker speed
    lastUpdate = currentTime;

    for (int i = 0; i < NUM_CHASER_LEDS; i++) {
      uint8_t heat = random(100, 255); // Random intensity for "fire" flicker
      _chaserLEDs[i] = CHSV(heat, 255, heat); // Red-orange palette
    }
  }
}

void patrioticEffect() {
  static int position = 0;             // Tracks the position in the chase
  static unsigned long lastUpdate = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastUpdate >= 100) { // Adjust delay for chase speed
    lastUpdate = currentTime;

    for (int i = 0; i < NUM_CHASER_LEDS; i++) {
      if ((i + position) % 3 == 0) {
        _chaserLEDs[i] = CRGB::Red;
      } else if ((i + position) % 3 == 1) {
        _chaserLEDs[i] = CRGB::White;
      } else {
        _chaserLEDs[i] = CRGB::Blue;
      }
    }
    position++; // Move the chase forward
  }
}
void spaceLaserEffect() {
  static int position = 0;             // Tracks the beam position
  static unsigned long lastUpdate = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastUpdate >= 50) { // Adjust delay for speed
    lastUpdate = currentTime;

    // Clear LEDs before applying lasers
    for (int i = 0; i < NUM_CHASER_LEDS; i++) {
      _chaserLEDs[i] = CRGB::Black;
    }

    // Add two lasers with overlapping positions
    _chaserLEDs[position % NUM_CHASER_LEDS] = CRGB::Green;
    _chaserLEDs[(position + NUM_CHASER_LEDS / 3) % NUM_CHASER_LEDS] = CRGB::Blue;

    position++; // Move the lasers forward
  }
}

void twinkleEffect() {
  static unsigned long lastUpdate = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastUpdate >= 30) { // Adjust delay for twinkle speed
    lastUpdate = currentTime;

    for (int i = 0; i < NUM_CHASER_LEDS; i++) {
      if (random(10) < 2) { // Randomly decide if this LED should twinkle
        _chaserLEDs[i] = CRGB::White; // Bright twinkle
      } else {
        _chaserLEDs[i].fadeToBlackBy(30); // Gradually fade
      }
    }
  }
}

void colorWipeEffect(CRGB color) {
  static int position = 0;
  static unsigned long lastUpdate = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastUpdate >= 50) { // Adjust delay for wipe speed
    lastUpdate = currentTime;

    if (position < NUM_CHASER_LEDS) {
      _chaserLEDs[position] = color;
      position++;
    } else {
      position = 0; // Reset for the next cycle
    }
  }
}

void meteorEffect() {
  static int position = 0;
  static unsigned long lastUpdate = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastUpdate >= 20) { // Adjust delay for meteor speed
    lastUpdate = currentTime;

    // Dim all LEDs to create a trail effect
    for (int i = 0; i < NUM_CHASER_LEDS; i++) {
      _chaserLEDs[i].fadeToBlackBy(50);
    }

    // Add the meteor
    for (int i = 0; i < 5; i++) { // Adjust meteor length
      int index = (position - i + NUM_CHASER_LEDS) % NUM_CHASER_LEDS;
      _chaserLEDs[index] = CRGB::White;
    }

    position = (position + 1) % NUM_CHASER_LEDS; // Move the meteor
  }
}

void confettiEffect() {
  static unsigned long lastUpdate = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastUpdate >= 10) { // Adjust delay for confetti speed
    lastUpdate = currentTime;

    // Fade all LEDs slightly
    for (int i = 0; i < NUM_CHASER_LEDS; i++) {
      _chaserLEDs[i].fadeToBlackBy(5);
    }

    // Add a random spark
    int pos = random(NUM_CHASER_LEDS);
    _chaserLEDs[pos] = CHSV(random8(), 200, 255); // Random hue and brightness
  }
}

void runningLightsEffect() {
  static int position = 0;
  static unsigned long lastUpdate = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastUpdate >= 40) { // Adjust delay for speed
    lastUpdate = currentTime;

    for (int i = 0; i < NUM_CHASER_LEDS; i++) {
      float intensity = sin((i + position) * 0.3) * 127 + 128; // Sine wave
      _chaserLEDs[i] = CRGB(intensity, 0, 255 - intensity); // Purple wave
    }

    position++; // Move the wave
  }
}

void sparkleEffect() {
  static unsigned long lastUpdate = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastUpdate >= 10) { // Adjust delay for sparkle speed
    lastUpdate = currentTime;

    // Fade all LEDs slightly
    for (int i = 0; i < NUM_CHASER_LEDS; i++) {
      _chaserLEDs[i].fadeToBlackBy(20);
    }

    // Add a random sparkle
    if (random(10) < 3) { // Random chance for a sparkle
      int pos = random(NUM_CHASER_LEDS);
      _chaserLEDs[pos] = CRGB::White;
    }
  }
}

void waveEffect() {
  static uint8_t hue = 0; // Starting hue
  static unsigned long lastUpdate = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastUpdate >= 30) { // Adjust delay for wave speed
    lastUpdate = currentTime;

    for (int i = 0; i < NUM_CHASER_LEDS; i++) {
      _chaserLEDs[i] = CHSV(hue + (i * 10), 255, 255); // Create a gradient
    }
    hue++; // Shift the gradient
  }
}

//===========================================================================

void marqueeChaseEffect() {
  static uint8_t chasePosition = 0;
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 500) {
    lastUpdate = millis();
    for (int i = 0; i < NUM_CHASER_LEDS; i++) {
      _chaserLEDs[i] = ((i + chasePosition) % 3 == 0) ? CRGB::White : CRGB::Black;
    }
    chasePosition = (chasePosition + 1) % NUM_CHASER_LEDS;
  }
}

//==========================================================================
//      SCORES 
//==========================================================================

void displayScore_Left(int score) {
  displayScore(score, 0, CRGB::Green);
}

void displayScore_Right(int score) {
  displayScore(score, 3, CRGB::Red);
}

void displayScore(int score, int startDigit, CRGB color) {
  int hundreds = score / 100;
  int tens = (score / 10) % 10;
  int units = score % 10;
//
  Serial.print("----- hundreds: ");
  Serial.print(" startDigit: ");
  Serial.print(startDigit);
  Serial.print("_scoreLeft: ");
  Serial.print(_scoreLeft);
  Serial.print(" _scoreRight: ");
  Serial.println(_scoreRight);
  displayDigit(hundreds, startDigit, color);

  startDigit++;
//  Serial.print("----- tens: ");
//  Serial.print(" startDigit: ");
//  Serial.print(startDigit);
//  Serial.print("_scoreLeft: ");
//  Serial.print(_scoreLeft);
//  Serial.print(" _scoreRight: ");
//  Serial.println(_scoreRight);
  displayDigit(tens, startDigit, color);

  startDigit++;
//  Serial.print("----- ones: ");
//  Serial.print(" startDigit: ");
//  Serial.print(startDigit);
//  Serial.print("_scoreLeft: ");
//  Serial.print(_scoreLeft);
//  Serial.print(" _scoreRight: ");
//  Serial.println(_scoreRight);
  displayDigit(units, startDigit, color);
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

//==========================================================================
//      EEPROM AND STORAGE
//==========================================================================

void resetScores() {
  // Reset scores to zero
  _scoreLeft = 0;
  _scoreRight = 0;

  // Save reset scores to EEPROM
  saveScoresToEEPROM();

  // Update the display
  displayScore_Left(_scoreLeft);
  displayScore_Right(_scoreRight);

  Serial.println("Scores have been reset to zero.");
}


void saveScoresToEEPROM() {
  EEPROM.put(EEPROM_SCORE_LEFT_ADDR, _scoreLeft);
  EEPROM.put(EEPROM_SCORE_RIGHT_ADDR, _scoreRight);
  Serial.println("Scores saved to EEPROM:");
  Serial.print("Left Score: ");
  Serial.println(_scoreLeft);
  Serial.print("Right Score: ");
  Serial.println(_scoreRight);
}

void loadScoresFromEEPROM() {
  EEPROM.get(EEPROM_SCORE_LEFT_ADDR, _scoreLeft);
  EEPROM.get(EEPROM_SCORE_RIGHT_ADDR, _scoreRight);

  // Add safety checks to prevent invalid values
  if (_scoreLeft < 0 || _scoreLeft > 999) _scoreLeft = 0;
  if (_scoreRight < 0 || _scoreRight > 999) _scoreRight = 0;

  Serial.println("Scores loaded from EEPROM:");
  Serial.print("Left Score: ");
  Serial.println(_scoreLeft);
  Serial.print("Right Score: ");
  Serial.println(_scoreRight);
}


//###################################################################################
//play specific mp3 in SD:/15/004.mp3; Folder Name(1~99); File Name(1~255)
//example: myDFPlayer.playFolder(1, 3);

//Otherwise, here are the problems with using the other methods:
//- 1. putting files in the root and calling play(fileNumber) only allows indexing based on the CREATED DATE!  Useless for us.
//- 2. Putting files in /mp3 and calling playMp3Folder(fileNumber) does not allow calling specific file names. Only sequential?
//--------------------------------------------------------------------------------
//################################################################

// Function to clear the border LEDs
void clearBorder() {
  setBorderColor(CRGB::Black);
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
