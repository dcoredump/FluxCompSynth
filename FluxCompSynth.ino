//
// FluxCompSynth
//
// Flexama-Synth-Shield based Arduino synthesizer
//
// (c)2018 H. Wirtz <wirtz@parasitstudio.de>
//
// (For UI: https://github.com/ondryaso/lcdui/blob/master/src/test.cpp)

#include <FluxSynth.h> /* https://sourceforge.net/projects/flexamysynth/files/ */
#include <NewSoftSerial.h>
#include <PgmChange.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h> /* https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library */
#define ENCODER_DO_NOT_USE_INTERRUPTS // Avoid problems with NewSoftwareSerial
#include <Encoder.h> /* https://github.com/PaulStoffregen/Encoder */
#include "FluxVoiceNames.h"

#define LCD_I2C_ADDRESS 0x27
#define LCD_CHARS 20
#define LCD_LINES 4

#define FLUXAMA_MIDI_IN_PIN 4

#define ENCODER1_PIN_A 5
#define ENCODER1_PIN_B 6
#define ENCODER1_BUTTON_PIN 10
#define ENCODER2_PIN_A 7
#define ENCODER2_PIN_B 8
#define ENCODER2_BUTTON_PIN 11

#define POT1_PIN A0
#define POT2_PIN A1
#define POT3_PIN A2
#define POT4_PIN A3

//**************************************************************************
// GLOBALS

// for  LCD-Modul QC2204A LCD2004 I2C-Controller
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_CHARS, LCD_LINES);

// Outgoing serial port
NewSoftSerial midiport(255, FLUXAMA_MIDI_IN_PIN);

// Synth
FluxSynth synth;

// Encoder
Encoder Encoder1(ENCODER1_PIN_A,ENCODER1_PIN_B);
Encoder Encoder2(ENCODER2_PIN_A,ENCODER2_PIN_B);

//**************************************************************************
// MAIN FUNCTIONS

void setup()
{
  Serial.begin(115200);
  lcd.begin();
  
  lcd.blink_off();
  lcd.cursor_off();
  lcd.backlight();
  lcd.noAutoscroll();
  lcd.clear();
  lcd.display();
  
  /*G
    uint8_t voice;
    uint8_t bank;
    char b[16];

    for (bank = 0; bank <= 1; bank++)
    {
    for (voice = 0; voice <= 127; voice++)
    {
      Serial.print(bank, DEC);
      Serial.print(":");
      Serial.print(voice, DEC);
      Serial.print(" ");
      voiceName(b, bank, voice);
      Serial.println(b);
    }
    }
  */

  show(1, 1, "FluxCompSynth");

  midiport.begin(31250);
  synth.begin();
  synth.sendByte = sendMidiByte;
  synth.midiReset();
  synth.GS_Reset();

  pinMode(ENCODER1_BUTTON_PIN,INPUT_PULLUP);
  pinMode(ENCODER2_BUTTON_PIN,INPUT_PULLUP);
  pinMode(POT1_PIN,INPUT);
  pinMode(POT2_PIN,INPUT);
  pinMode(POT3_PIN,INPUT);
  pinMode(POT4_PIN,INPUT);

  lcd.clear();
}

void loop()
{

}

//**************************************************************************
// FUNCTIONS

// Output routine for FluxSynth.
bool sendMidiByte(byte B)
{ 
  midiport.write(B);
  return true;
}

void initSynth(byte Chan, byte Patch, byte Vol, byte Rev, byte Chor, byte Bend)
{
  synth.programChange( Chan, Patch );
  synth.setChannelVolume( Chan, Vol );
  synth.setReverbSend( Chan, Rev );
  synth.setChorusSend( Chan, Chor );
  synth.setBendRange( Chan, Bend );
}

void voiceName(char *buffer, uint8_t bank, uint8_t program)
{
  strcpy_P(buffer, (char*)pgm_read_word(&(_voice_name[bank * 128 + program])));
}

void show(uint8_t x, uint8_t y, char* string)
{
  lcd.setCursor(x, y);
  lcd.print(string);

  Serial.print(x, DEC);
  Serial.print("/");
  Serial.print(y, DEC);
  Serial.print(":");
  Serial.println(string);
}
