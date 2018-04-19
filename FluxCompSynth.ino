//
// FluxCompSynth
//
// Flexama-Synth-Shield based Arduino synthesizer
//
// (c)2018 H. Wirtz <wirtz@parasitstudio.de>
//

#include <FluxSynth.h> /* https://sourceforge.net/projects/flexamysynth/files/ */
#include <NewSoftSerial.h>
#include <PgmChange.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h> /* https://github.com/marcoschwartz/LiquidCrystal_I2C (https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library) */
//#define ENCODER_DO_NOT_USE_INTERRUPTS // Avoid problems with NewSoftwareSerial
#include <RotaryEncoder.h> /* https://github.com/mathertel/RotaryEncoder */
#include <Bounce2.h> /* https://github.com/thomasfredericks/Bounce2 */
#include "FluxVoiceNames.h"

#define LED_PIN 13

//#define LCD_I2C_ADDRESS 0x27
#define LCD_I2C_ADDRESS 0x3f
#define LCD_CHARS 20
#define LCD_LINES 4

#define FLUXAMA_MIDI_IN_PIN 4

#define MAX_ENCODER 2
#define ENCODER1_PIN_A 5
#define ENCODER1_PIN_B 6
#define ENCODER1_BUTTON_PIN 10
#define ENCODER2_PIN_A 7
#define ENCODER2_PIN_B 8
#define ENCODER2_BUTTON_PIN 11
#define DEBOUNCE_INTERVAL_MS 5
#define ROTARYSTEPS 2
#define ROTARYMIN 0
#define ROTARYMAX 16

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
RotaryEncoder Encoder1(ENCODER1_PIN_A, ENCODER1_PIN_B);
RotaryEncoder Encoder2(ENCODER2_PIN_A, ENCODER2_PIN_B);

// Buttons (debouncer)
Bounce Button1 = Bounce(ENCODER1_BUTTON_PIN, DEBOUNCE_INTERVAL_MS);
Bounce Button2 = Bounce(ENCODER2_BUTTON_PIN, DEBOUNCE_INTERVAL_MS);

//**************************************************************************
// MAIN FUNCTIONS

void setup()
{
  Serial.begin(115200);

  lcd.init();
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

  show(0, 0, "FluxCompSynth");

  midiport.begin(31250);
  synth.begin();
  synth.sendByte = sendMidiByte;
  synth.midiReset();
  synth.GS_Reset();

  pinMode(ENCODER1_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENCODER2_BUTTON_PIN, INPUT_PULLUP);
  pinMode(POT1_PIN, INPUT_PULLUP);
  pinMode(POT2_PIN, INPUT_PULLUP);
  pinMode(POT3_PIN, INPUT_PULLUP);
  pinMode(POT4_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW );

  //lcd.clear();
}

void loop()
{
  static int8_t enc_pos[MAX_ENCODER] = { 0, 0};
  int8_t new_pos;

  // do the update stuff
  Encoder1.tick();
  Encoder2.tick();
  Button1.update();
  Button2.update();

  // Encoder1 handling
  new_pos = Encoder1.getPosition() * ROTARYSTEPS;
  if(new_pos < ROTARYMIN)
  {
    Encoder1.setPosition(ROTARYMIN / ROTARYSTEPS);
    new_pos = ROTARYMIN;
  }
  else if (new_pos > ROTARYMAX)
  {
    Encoder1.setPosition(ROTARYMAX / ROTARYSTEPS);
    new_pos = ROTARYMAX;
  }
  if (enc_pos[0] != new_pos)
  {
    Serial.print(new_pos);
    Serial.println();
    enc_pos[0] = new_pos;
  }

  // Get the updated value :
  bool value = Button1.read();

  // Turn on or off the LED as determined by the state :
  if ( value == LOW ) {
    digitalWrite(LED_PIN, HIGH );
  }
  else {
    digitalWrite(LED_PIN, LOW );
  }

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
