#include <Keypad.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <Wire.h>
#include <hd44780.h> // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h>
hd44780_I2Cexp lcd;
int row = 16;
byte currentSelection = 1; // Tracks the current selection (1st, 2nd, 3rd)
const byte ROWS = 4;       // four rows
const byte COLS = 4;       // three columns
char keys[ROWS][COLS] =
    {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}};
const byte busyPin = 10;
const byte buttonPin = 8;
const byte pinLed = 5;
const byte ssRXPin = 11;
const byte ssTXPin = 12;
byte playIndex = 0;
byte rowPins[ROWS] = {9, 8, 7, 6}; // connect to the row pinouts of the
byte keyBufferIndex = 0;
byte colPins[COLS] = {5, 4, 3, 2}; // connect to the column pinouts of the

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
SoftwareSerial mp3ss(ssRXPin, ssTXPin); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
char key;
char keyBuffer[21];
bool newEntry = false;
byte track = 0;
byte trackList[3] = {0, 0, 0};
byte trackIndex = 0;
byte numTracks = 0;
byte mode = 0;
bool playList = false;
static bool lastBusyPinState = 0;
byte currentDisplayLine = 1;
#define MAX_SEQUENCE_LENGTH 10 // Maximum length of the sequence list

int sequenceList[MAX_SEQUENCE_LENGTH]; // Array to store the sequence list
int sequenceLength = 0;                // Current length of the sequence list
// Function to add a track to the sequence list
void addToSequenceList(int trackNumber)
{
  if (sequenceLength < MAX_SEQUENCE_LENGTH)
  {
    sequenceList[sequenceLength] = trackNumber;
    sequenceLength++;
    Serial.print("Track ");
    Serial.print(trackNumber);
    Serial.println(" added to sequence list");
  }
  else
  {
    Serial.println("Sequence list is full");
  }
}

// Function to play the sequence
void playSequence()
{
  for (int i = 0; i < sequenceLength; i++)
  {
    // myDFPlayer.playFolder(1, sequenceList[i]); // Assuming track numbers are in folder 1
    // delay(1000);                               // Adjust delay as needed
  }
}

// Function to stop the sequence
void stopSequence()
{
  playList = false;
  playIndex = 0;

  myDFPlayer.stop();
}
void skipSequence()
{
  Serial.print("play next = ");
  Serial.println(playIndex);
  myDFPlayer.stop();
  Serial.println( myDFPlayer.readState());
  delay(1000);
   Serial.println( myDFPlayer.readState());
  // playIndex++;
  Serial.print("play index number: ");
  Serial.println(playIndex);
  Serial.print("sequence index : ");
  Serial.println(sequenceLength);
  if (playIndex != sequenceLength) // last track?
  {
    myDFPlayer.play(sequenceList[playIndex]);
  }
}

void playTheList()
{

  static unsigned long timer = 0;
  unsigned long interval = 100;
  if (millis() - timer >= interval)
  {
    timer = millis();

    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    bool busyPinState = digitalRead(busyPin);                           // read the busy pin
    if (busyPinState != lastBusyPinState && playIndex < sequenceLength) // has it changed?
    {
      if (busyPinState == 1) // has it gone from low to high?, meaning the track finished
      {
        Serial.print("play number  = ");
        Serial.println(sequenceList[playIndex]);
        Serial.print("play index = ");
        Serial.println(playIndex);
        myDFPlayer.play(sequenceList[playIndex]);
        playIndex++;                     // next track
        if (playIndex >= sequenceLength) // last track?
        {
          sequenceLength = 0;
          playIndex = 0;      // reset list
          keyBuffer[0] = 'C'; // set up for stop mode
          mode = 6;           // call stop mode
          playList = false;
        }
      }
      lastBusyPinState = busyPinState; // remember the last busy state

      // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    }
  }
}

void getEntry(char key)
{
  static boolean entryStarted = false;
  if (key == 'C' && playList == true)
  {
    Serial.println(F(" stop the playing"));
    keyBufferIndex = 0;
    Serial.println(F(" skipping the track"));
    skipSequence();
    // playList = false;
  }
  // Increment current selection or wrap back to 1
  if (key == 'A')
  {
    currentSelection++;
    row = 16;
    if (currentSelection > 3)
    {
      currentSelection = 1;
    }
  }
  if (key == 'D')
  { // Delete last key entry
    if (keyBufferIndex > 0)
    {
      memset(keyBuffer, 0, sizeof(keyBuffer));
      keyBufferIndex = 0;
      row = 16;
      // Serial.print(F("\t\t"));
      Serial.print(sequenceList[sequenceLength]);
      Serial.println(F(" deleted track"));

      // Reset display
      // sequenceList[sequenceLength] = trackNumber;
      // sequenceLength--;
      if (currentSelection == 1)
      {
        lcd.setCursor(0, 1);
        lcd.print(" 1st Selection <___> ");
      }
      else if (currentSelection == 2)
      {
        lcd.setCursor(0, 2);
        lcd.print(" 2nd Selection <___> ");
      }
      else if (currentSelection == 3)
      {
        lcd.setCursor(0, 3);
        lcd.print(" 3rd Selection <___> ");
      }
      // return;
    }
  }
  if (key == '#')
  { // End of entry
    keyBuffer[keyBufferIndex] = '\0';
    entryStarted = false;
    newEntry = true;
  }
  else if (key == '*' || key == 'A' || key == 'B' || key == 'C')
  { // Action keys
    if (keyBufferIndex > 0)
    { // Check if a track number has been entered
      switch (key)
      {
      case '*': // Play immediate
        Serial.println(F(" play immediate"));
        myDFPlayer.play(atoi(keyBuffer)); // Assuming track numbers are in folder 1
        keyBufferIndex = 0;
        // Clear the buffer
        memset(keyBuffer, 0, sizeof(keyBuffer));
        break;
      case 'A': // Add to sequence list
        Serial.println(F(" add to list"));
        addToSequenceList(atoi(keyBuffer));
        // Clear the buffer
        memset(keyBuffer, 10, sizeof(keyBuffer));
        // keyBufferIndex = 0;
        break;
      case 'B': // Play sequence
        Serial.println(F(" playing the sequence"));
        playList = true;
        keyBufferIndex = 0;
        playSequence();
        break;
      case 'C': // STOP sequence
        Serial.println(F(" stop the playings"));
        keyBufferIndex = 0;
       // stopSequence();
        break;
      default:
        break;
      }
    }
  }

  else if (key != '#' && key != '*' && key != 'D')
  { // Continue entry
    if (keyBufferIndex < 9 && isDigit(key))
    {
      // Serial.println("entry1");
      if (row <= 18) // max length to go
      {
        // Serial.println("entry2");
        keyBuffer[keyBufferIndex] = key;
        keyBufferIndex++;
        // Update LCD screen with entered track number for current selection
        lcd.setCursor(row, currentSelection); // Adjust the cursor position as needed
        lcd.print(key);
        row++;
      }
    }
  }
}
float entryToFloat(char *entry)
{
  return (atof(entry));
}
int entryToInt(char *entry)
{
  return (atoi(entry));
}

void setup()
{
  Serial.begin(115200);
  Serial.print(F("Enter track number then enter action"));
  Serial.println(F(" # = ENTER"));
  Serial.println(F(" * = Play immediate"));
  Serial.println(F(" A = Add to sequence list"));
  Serial.println(F(" B = Play sequence"));
  Serial.println(F("To add track 18 to the list, \"18A\""));
  Serial.println(F("To play track 3 immediately, \"3*\""));
  Serial.println(F(" C = STOP sequence"));
  Serial.println(F("\n\n"));
  mp3ss.begin(9600);
  myDFPlayer.begin(mp3ss);
  myDFPlayer.volume(20);
  // myDFPlayer.play(3);
  lcd.begin(20, 4);
  lcd.clear(); // the only time that you should use clear
  lcd.print(" Halloween Sounds ");
  lcd.setCursor(0, 1);
  lcd.print(" 1st Selection <___> ");
  lcd.setCursor(0, 2);
  lcd.print(" 2nd Selection <___>");
  lcd.setCursor(0, 3);
  lcd.print(" 3rd Selection <___>");
}
void loop()
{
  if (playList)
  {
    playTheList();
  }
  key = keypad.getKey();
  if (key)
  {
    Serial.print(F(" key code = "));
    Serial.print(key);
    getEntry(key);
  }
}