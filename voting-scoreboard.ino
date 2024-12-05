#include <FastLED.h>
#include <DFRobotDFPlayerMini.h>

#define SCOREBOARD_LED_PIN 6
#define CHASER_LED_PIN 7

#define NUM_SCOREBOARD_LEDS 174
#define NUM_CHASER_LEDS 50

#define NUM_BUTTONS 6
#define BUTTON_INC_LEFT  2
#define BUTTON_DEC_LEFT 3
#define BUTTON_INC_RIGHT 4
#define BUTTON_DEC_RIGHT 5

#define BUTTON_INC_LEFT_ALT 8
#define BUTTON_INC_RIGHT_ALT 9

#define BUTTON_TEST_SCORE 8  
#define BUTTON_TEST_SOUND 10
#define BUTTON_TEST_CHASE 9 

#define BORDER_START 175
#define BORDER_COUNT 50

//const bool digits[][7] PROGMEM = {
//    {false, true,  true,  true,  true,  true,  true},  // 0
//    {false, true,  false, false, false, false, true},  // 1
//    {true,  true,  true,  false, true,  true,  false}, // 2
//    {true,  true,  true,  false, false, true,  true},  // 3
//    {true,  true,  false, true,  false, false, true},  // 4
//    {true,  false, true,  true,  false, true,  true},  // 5
//    {true,  false, true,  true,  true,  true,  true},  // 6
//    {false, true,  true,  false, false, false, true},  // 7
//    {true,  true,  true,  true,  true,  true,  true},  // 8
//    {true,  true,  true,  true,  false, true,  true}   // 9
//};

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

CRGB _scoreboardLEDs[NUM_SCOREBOARD_LEDS];
CRGB _chaserLEDs[NUM_CHASER_LEDS];

DFRobotDFPlayerMini _dfPlayer;

int _globalBrightness = 255;

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

const SoundEffect _soundEffects[] = {
    {001, 3000}, {002, 1500}, {003, 2500}, {004, 1800}, {005, 1800}
};
const int _numSoundEffects = sizeof(_soundEffects) / sizeof(SoundEffect);
int _currentEffectIndex = 0;

const int _songs[] = {
  201, 202, 203, 204, 205, 006, 207, 208, 209, 210,
  211, 212, 213, 214, 205, 216, 217, 218, 219, 220
};
const int _numSongs = sizeof(_songs) / sizeof(int);

typedef void (*AnimationFunction)();
void animation1();
void animation2();
void animation3();
void animation4();

AnimationFunction _animations[] = {animation1, animation2, animation3, animation4};
const int _numAnimations = sizeof(_animations) / sizeof(AnimationFunction);
int _currentAnimationIndex = 0;

unsigned long _rainbowDelayStartTime;
bool _rainbowDelayActive = false;

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);

  if (!_dfPlayer.begin(Serial2)) {
    Serial.println("DFPlayer initialization failed!");
    while (true);
  }

  _dfPlayer.setTimeOut(500); //Set serial communictaion time out 500ms
  delay(100);

  _dfPlayer.volume(15);
  delay(100);

    //----Set different EQ----
  _dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
  delay(100);

  
  Serial.println("DFPlayer initialized!");

  FastLED.addLeds<WS2812, SCOREBOARD_LED_PIN, GRB>(_scoreboardLEDs, NUM_SCOREBOARD_LEDS);
  FastLED.addLeds<WS2812, CHASER_LED_PIN, GRB>(_chaserLEDs, NUM_CHASER_LEDS);

  FastLED.clear();
  FastLED.setBrightness(64); //For just testing on the bench
  //FastLED.setBrightness(128); //For just testing on the bench
  //FastLED.setBrightness(255); //For just testing on the bench
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
  
  randomSeed(analogRead(0));

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


  Serial.println("_numSoundEffects: ");
  Serial.print(_numSoundEffects);
  Serial.print(" sizeof(_soundEffects): ");
  Serial.println(sizeof(_soundEffects));
  Serial.print(" sizeof(SoundEffect): ");
  Serial.println(sizeof(SoundEffect));

  displayScore(_scoreLeft, 0, CRGB::Green);
  displayScore(_scoreRight, 3, CRGB::Red);

  Serial.print(F("Files found DFPlayer SD: "));
  Serial.println(_dfPlayer.readFileCounts());
  
  Serial.println("---- setup() method end ----");

}

//#################################################################
//void loop() {
//  for (int i = 1; i <= 6; i++) {
//    Serial.print("Playing file: ");
//    Serial.println(i);
//
//    //This always seems to playt the same file over and over.
//    //_dfPlayer.play(i);
//
//    //internet said to try this way instead.
//    _dfPlayer.playFolder(1,i);
//
//    Serial.print("DFPlayer CurrentFileNumber: ");
//    Serial.print(_dfPlayer.readCurrentFileNumber());
//    Serial.print("/");
//    Serial.println(_dfPlayer.readFileCounts());
//
//    delay(10000); // Wait for the song to play
//
//    if (_dfPlayer.available()) {
//      printDetail(_dfPlayer.readType(), _dfPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
//    }
//  }
//}

void loop() {
  scoreboardLoop();

  switch (_systemState) {
    case NORMAL:
      marqueeChaseEffect();
      break;

    case PLAYING_EFFECT:
      if (millis() - _effectStartTime >= _soundEffects[_currentEffectIndex].duration) {
        playRandomSongNonBlocking();
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

void handleButtonPress(int buttonIndex) {
  switch (buttonIndex) {
    case 0:  // Left increment
    case 4:  // Left increment (alternate)
      _scoreLeft = min(_scoreLeft + 1, 999);
      displayScore_Left(_scoreLeft);
      initiateSoundEffect();
      break;

    case 2:  // Right increment
    case 5:  // Right increment (alternate)
      _scoreRight = min(_scoreRight + 1, 999);
      displayScore_Right(_scoreRight);
      initiateSoundEffect();
      break;

    case 1: // Left decrement
      _scoreLeft = max(_scoreLeft - 1, 0);
      displayScore_Left(_scoreLeft);
      break;

    case 3: // Right decrement
      _scoreRight = max(_scoreRight - 1, 0);
      displayScore_Right(_scoreRight);
      break;
  }
}

void initiateSoundEffect() {
  if (_systemState == NORMAL) {

    Serial.print(" _currentEffectIndex: ");
    Serial.print(_currentEffectIndex);

    _currentEffectIndex = _currentEffectIndex % _numSoundEffects;
    
    Serial.print(" _soundEffects[_currentEffectIndex].id: ");
    Serial.println(_soundEffects[_currentEffectIndex].id);
    
    _dfPlayer.playFolder(1,_soundEffects[_currentEffectIndex].id);
    _effectStartTime = millis();
    _systemState = PLAYING_EFFECT;
    _currentEffectIndex++;
  }
}

void playRandomSongNonBlocking() {
  int songIndex = random(0, _numSongs);
  Serial.print("playRandomSongNonBlocking -- songIndex: ");
  Serial.print(songIndex);

  Serial.print(" of ");
  Serial.print(_numSongs);
  Serial.print(" songs -- ");

  Serial.print(" having ID:  ");
  Serial.println(_songs[songIndex]);
  _dfPlayer.playFolder(1,_songs[songIndex]);
  Serial.println("(play command issued and returned)");
  delay(100);
  _dfPlayer.loopFolder(1);
  Serial.println("(loop folder command issued and returned)");
  
  _currentAnimationIndex = random(0, _numAnimations);
  Serial.print("New _currentAnimationIndex:  ");
  Serial.println(_currentAnimationIndex);

  _animationStartTime = millis();
}

void startRandomAnimationEffect() {
  _currentAnimationIndex = random(0, _numAnimations);
  _animationStartTime = millis();
  _systemState = PLAYING_ANIMATION;
}

void animation1() {
  static uint16_t offset = 0;
  for (int i = 0; i < NUM_CHASER_LEDS; i++) {
    _chaserLEDs[i] = CHSV((i * 10 + offset) % 255, 255, 255);
  }
  offset += 5;
}

void animation2() {
  static byte heat[NUM_CHASER_LEDS];
  for (int i = 0; i < NUM_CHASER_LEDS; i++) {
    heat[i] = qsub8(heat[i], random8(0, ((55 * 10) / NUM_CHASER_LEDS) + 2));
  }
  for (int k = NUM_CHASER_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
  for (int j = 0; j < NUM_CHASER_LEDS; j++) {
    _chaserLEDs[j] = HeatColor(heat[j]);
  }
}

void animation3() {
  for (int i = 0; i < NUM_CHASER_LEDS; i++) {
    _chaserLEDs[i] = ((i % 3) == 0) ? CRGB::Red : ((i % 3) == 1) ? CRGB::White : CRGB::Blue;
  }
}

void animation4() {
  static int laserPos = 0;
  //FastLED.clear();
  clearBorder();
  for (int i = 0; i < 5; i++) {
    _chaserLEDs[(laserPos + i * 10) % NUM_CHASER_LEDS] = CHSV(random8(), 255, 255);
  }
  laserPos++;
}

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


//###################################################################################
//play specific mp3 in SD:/15/004.mp3; Folder Name(1~99); File Name(1~255)
//example: myDFPlayer.playFolder(1, 3);

//Otherwise, here are the problems with using the other methods:
//- 1. putting files in the root and calling play(fileNumber) only allows indexing based on the CREATED DATE!  Useless for us.
//- 2. Putting files in /mp3 and calling playMp3Folder(fileNumber) does not allow calling specific file names. Only sequential?
//--------------------------------------------------------------------------------
// routine to play a specific song

void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

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
