#include <DFRobotDFPlayerMini.h>

// Create a DFPlayer instance
DFRobotDFPlayerMini myDFPlayer;

void setup() {
  // Start Serial for debugging (via USB)
  Serial.begin(9600);
  Serial.println("Initializing DFPlayer...");

  // Start Serial2 for DFPlayer communication
  Serial2.begin(9600);

  // Initialize the DFPlayer Mini
  if (!myDFPlayer.begin(Serial2)) {
    Serial.println("DFPlayer initialization failed!");
    Serial.println("1. Check wiring.");
    Serial.println("2. Ensure SD card is inserted.");
    while (true); // Halt execution if initialization fails
  }

  Serial.println("DFPlayer initialized successfully.");

  // Set initial volume (0-30)
  myDFPlayer.volume(20);
}

void loop() {
  // Play the first track
  Serial.println("Playing track 1...");
  myDFPlayer.play(1);
  delay(120000); // Wait 5 seconds before playing again

  // Play the second track
  Serial.println("Playing track 2...");
  myDFPlayer.play(2);
  delay(120000); // Wait 5 seconds before restarting
}


//####################################################################
////Works on Uno
//#include <SoftwareSerial.h>
//#include <DFRobotDFPlayerMini.h>
//
//// Define SoftwareSerial pins for DFPlayer communication
//SoftwareSerial mySoftwareSerial(2, 3); // RX, TX
//DFRobotDFPlayerMini myDFPlayer;
//
//void setup() {
//  // Start Serial for debugging
//  Serial.begin(9600);
//  Serial.println("Initializing DFPlayer...");
//
//  // Start SoftwareSerial for DFPlayer
//  mySoftwareSerial.begin(9600);
//
//  // Initialize the DFPlayer
//  if (!myDFPlayer.begin(mySoftwareSerial)) {
//    Serial.println("DFPlayer initialization failed!");
//    Serial.println("1. Check wiring.");
//    Serial.println("2. Ensure SD card is inserted.");
//    while (true); // Halt execution if initialization fails
//  }
//
//  Serial.println("DFPlayer initialized successfully.");
//
//  // Set initial volume (0-30)
//  myDFPlayer.volume(12);
//}
//
//void loop() {
//  // Play the first track
//  Serial.println("Playing track 1...");
//  myDFPlayer.play(1);
//  delay(25000); // Wait 5 seconds before playing again
//
//  // Play the second track
//  Serial.println("Playing track 2...");
//  myDFPlayer.play(2);
//  delay(25000); // Wait 5 seconds before restarting
//}
