#include <Keypad.h>
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <Wire.h>
#include <hd44780.h> // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h>
hd44780_I2Cexp lcd;
const byte ROWS = 4; // four rows
const byte COLS = 4; // three columns
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
byte rowPins[ROWS] = {9, 8, 7, 6}; // connect to the row pinouts of the

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
void setup()
{
    Serial.begin(115200);
    Serial.print(F("Enter track number then enter action"));
    Serial.println(F(" # = ENTER"));
    Serial.println(F(" * = Play immediate"));
    Serial.println(F(" A = Add to sequence list"));
    Serial.println(F(" B = Play sequence"));
    Serial.println(F("To add track 18 to the list, \"18#A#\""));
    Serial.println(F("To play track 3 immediately, \"3#*#\""));
    Serial.println(F(" C = STOP sequence"));
    Serial.println(F("\n\n"));
    mp3ss.begin(9600);
    myDFPlayer.begin(mp3ss);
    myDFPlayer.volume(20);
    myDFPlayer.play(3);
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
    if (mode == 0) // check for new key
    {
        key = keypad.getKey();
        if (key)
        {
            // Serial.print(F(" key code = "));
            // Serial.print(key);
            getEntry(key);
        }
        if (newEntry)
        {
            Serial.print(F(" keybuffer = "));
            Serial.println(keyBuffer);
            Serial.print("\nnew number entered = ");
            Serial.println(keyBuffer);
            // ***************
            int intKey = atoi(keyBuffer);
            //myDFPlayer.play(intKey);
            // ####################################################
            lcd.setCursor(16, currentDisplayLine);
            lcd.print(" ");                        // overwrite old data
            lcd.setCursor(16, currentDisplayLine); // reset cursor
            lcd.print(intKey);
            currentDisplayLine++;
            if (currentDisplayLine > 3)
            {
                currentDisplayLine = 1;
            }
            // ###################################################
            newEntry = false;

            mode = 2;
        }
        if (playList)
        {
            playTheList();
        }
    }
    else if (mode == 2) // is it new track
    {
        // if it is a digit (number) it has to be a track
        if (isdigit(keyBuffer[0]))
        {
            track = atoi(keyBuffer);
            Serial.print(F("new track "));
            Serial.println(track);
            mode = 0;
        }
        else
        {
            mode = 3;
        }
    }
    else if (mode == 3) // play immediate
    {
        if (keyBuffer[0] == '*')
        {
            myDFPlayer.play(track);
            Serial.print(F("Playing "));
            Serial.print(track);
            Serial.println(F(" immediate"));
            mode = 0;
        }
        else
        {
            mode = 4;
        }
    }
    if (mode == 4) // initialize or add to list
    {
        if (keyBuffer[0] == 'A')
        {
            Serial.print(F("Adding "));
            Serial.print(track);
            Serial.print(F(" to list"));
            trackList[trackIndex] = track;
            trackIndex++;
            // keep to the 3 positions
            if (trackIndex >= 3)
            {
                trackIndex = 0;
            }
            // how many tracks? up to 3.
            if (trackIndex > numTracks)
            {
                numTracks = trackIndex;
            }
            Serial.print(" ");
            Serial.print(trackList[2]);
            Serial.print(" ");
            Serial.print(trackList[1]);
            Serial.print(" ");
            Serial.print(trackList[0]);
            mode = 0;
        }
        else
        {
            mode = 5;
        }
    }
    else if (mode == 5)
    {
        if (keyBuffer[0] == 'B') // play list
        {
            playList = true;
            Serial.println(F("PLAY the listed tracks"));
            // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
            lastBusyPinState = 0; // set up to restart the sequence.
            // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
            mode = 0;
        }
        else
        {
            mode = 6;
        }
    }
    else if (mode == 6)
    {
        if (keyBuffer[0] == 'C') // stop playing list
        {
            Serial.println(F("STOP playing tracks"));
            playList = false;
            mode = 0;
        }
        else
        {
            mode = 0;
        }
    }
    newEntry = false;
}
void playTheList()
{
    static unsigned long timer = 0;
    unsigned long interval = 100;
    if (millis() - timer >= interval)
    {
        timer = millis();
        static byte playIndex = 0;
        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        bool busyPinState = digitalRead(busyPin); // read the busy pin
        if (busyPinState != lastBusyPinState)     // has it changed?
        {
            if (busyPinState == 1) // has it gone from low to high?, meaning the track finished
            {
                Serial.print("play index = ");
                Serial.println(playIndex);
                myDFPlayer.play(trackList[playIndex]);
                playIndex++;        // next track
                if (playIndex >= 3) // last track?
                {
                    playIndex = 0;      // reset list
                    keyBuffer[0] = 'C'; // set up for stop mode
                    mode = 6;           // call stop mode
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
    static byte keyBufferIndex = 0;
    if (key == 'D') // delete last key entry
    {
        if (keyBufferIndex > 0)
        {
            Serial.print(F("\t\t"));
            Serial.print(keyBuffer[keyBufferIndex - 1]);
            Serial.println(F(" deleted"));
            keyBufferIndex--;
            return;
        }
    }
    if (entryStarted == false)
    {
        keyBufferIndex = 0;
        entryStarted = true;
        keyBuffer[keyBufferIndex] = key;
        // Serial.println(keyBuffer);
        newEntry = false;
        keyBufferIndex++;
    }
    else if (entryStarted == true && key != '#')
    {
        keyBuffer[keyBufferIndex] = key;
        // Serial.println(keyBuffer);
        keyBufferIndex++;
    }
    else if (key == '#')
    {
        keyBuffer[keyBufferIndex] = '\0';
        entryStarted = false;
        newEntry = true;
        // Serial.println(keyBuffer);
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