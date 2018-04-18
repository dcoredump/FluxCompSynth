//
// Flexama LCD synth
//
// (c)2018 H. Wirtz <dcoredump@googlemail.com>
//

#include <FlexamySynth.h>
#include <PgmChange.h>
#include <Encoder.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "FlexaPgm.h"

// Belegung für 16x4 LCD-Modul QC2204A LCD2004 I2C-Controller
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Synth
FlexamySynth synth;

extern char _voice;

void setup()
{
  Serial.begin(9600);

  uint16_t voice;
  char tBuffer[40];

  for (voice = 0; voice <= 255; voice++)
  {
    Serial.print(voice, DEC);
    Serial.print(" ");
    Serial.println(strcpy_P(tBuffer, (char*)pgm_read_word(&(voice_name[voice]))));
  }

  Serial.println("Done.");
  while (1);

  lcd.begin();
  lcd.setCursor(4, 1);
  lcd.print(F("Flexama LCD synth"));

  synth.begin();
  synth.midiReset();
  synth.GS_Reset();
}

void loop()
{

}

void init_synth(byte Chan, byte Patch, byte Vol, byte Rev, byte Chor, byte Bend)
{
  synth.programChange( Chan, Patch );
  synth.setChannelVolume( Chan, Vol );
  synth.setReverbSend( Chan, Rev );
  synth.setChorusSend( Chan, Chor );
  synth.setBendRange( Chan, Bend );
}

