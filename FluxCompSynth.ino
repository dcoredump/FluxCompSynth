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
#include <RotaryEncoderDir.h> /* https://github.com/dcoredump/RotaryEncoderDir.git */
#include <Bounce2.h> /* https://github.com/thomasfredericks/Bounce2 */
#include "FluxVoiceNames.h" // Voice names in PROGMEM

#define LED_PIN 13

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

#define POT1_PIN A0
#define POT2_PIN A1
#define POT3_PIN A2
#define POT4_PIN A3

struct SynthGlobal
{
  int8_t volume;  // negative means OFF
  uint8_t reverb_type;
  uint8_t reverb_level;
  uint8_t chorus_type;
  uint8_t chorus_level;
  uint8_t clipping;
} synth_config;

struct SynthVoice
{
  uint8_t patch;
  int8_t volume;  // negative means OFF
  int8_t pan;     // 0=middle
  int8_t transpose;
  uint8_t reverb_send;
  uint8_t chorus_send;
  uint8_t bend_range;
} synth_voice_config[15]; // Channel 10 has a special handling for Drums

struct SynthDrums
{
  uint8_t drumkit;
  int8_t volume;  // negative means OFF
  int8_t pan;     // 0=middle
  uint8_t reverb_send;
  uint8_t chorus_send;
} synth_drums_config;

//**************************************************************************
// GLOBALS

// for  LCD-Modul QC2204A LCD2004 I2C-Controller
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_CHARS, LCD_LINES);

// Outgoing serial port
NewSoftSerial midiport(255, FLUXAMA_MIDI_IN_PIN); // 255 = OFF

// Synth
FluxSynth synth;

// Encoder
RotaryEncoderDir Encoder1(ENCODER1_PIN_A, ENCODER1_PIN_B);
RotaryEncoderDir Encoder2(ENCODER2_PIN_A, ENCODER2_PIN_B);

// Buttons (debouncer)
Bounce Button1 = Bounce(ENCODER1_BUTTON_PIN, DEBOUNCE_INTERVAL_MS);
Bounce Button2 = Bounce(ENCODER2_BUTTON_PIN, DEBOUNCE_INTERVAL_MS);

// div
char itoa_buf[11];

//
//**************************************************************************
// MAIN FUNCTIONS

void setup(void)
{
  Serial.begin(115200);

  lcd.init();
  lcd.blink_off();
  lcd.cursor_off();
  lcd.backlight();
  lcd.noAutoscroll();
  lcd.clear();
  lcd.display();

//------------------------<TESTCODE>
  uint8_t voice;
  uint8_t bank;
  char b[16];

  for (bank = 0; bank <= 1; bank++)
  {
    for (voice = 0; voice <= 127; voice++)
    {
      show(1, 0, itoa(bank, itoa_buf, 10));
      show(1, 2, itoa(voice, itoa_buf, 10));
      voiceName(b, bank, voice);
      show(1, 4, b);
    }
  }
//------------------------</TESTCODE>

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
  digitalWrite(LED_PIN, LOW);

  //lcd.clear();
}

void loop(void)
{
  static int8_t v;

  // do the update stuff
  Encoder1.tick();
  Encoder2.tick();
  Button1.update();
  Button2.update();

  // Encoder1 handling
  int8_t dir1 = Encoder1.hasChanged();
  if (dir1)
  {
    encoder_move(dir1, -8, 8, long(&v));
    show(2, 0, itoa(dir1, itoa_buf, 10));
    show(2, 2, itoa(v, itoa_buf, 10));
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

void show(uint8_t x, uint8_t y, char *string)
{
  lcd.setCursor(x, y);
  lcd.print(string);

  Serial.print(x, DEC);
  Serial.print(" / ");
  Serial.print(y, DEC);
  Serial.print(": ");
  Serial.println(string);
}

void encoder_move(int8_t dir, int8_t min, int8_t max, long *value)
{
  if (value + dir < min)
    value = min;
  else if (value + dir > max)
    value = max;
  else
    value += dir;

  return (value);
}

