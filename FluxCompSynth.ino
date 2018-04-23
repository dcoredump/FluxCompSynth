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

#define DEBUG 1

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
  int8_t volume = 100; // negative means OFF
  uint8_t reverb_type = REV_ROOM1;
  uint8_t reverb_level = REV_DEFLEVEL;
  uint8_t chorus_type = CHO_CHORUS1;
  uint8_t chorus_level = 0;
  uint8_t clipping = SOFT_CLIP;
} synth_config;

struct SynthVoice
{
  uint8_t patch = 0;
  int8_t volume = 64; // negative means OFF
  int8_t pan = 0;   // 0=middle
  int8_t transpose = 0;
  uint8_t reverb_send = 0;
  uint8_t chorus_send = 0;
  uint8_t bend_range = 12;
} synth_voice_config[16];

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

// vars
uint8_t voice = 0;
uint8_t channel = 1;
uint8_t bank = PATCH_BANK0;

// Temp buffers
//char _buf10[11];

//
//**************************************************************************
// MAIN FUNCTIONS

void setup(void)
{
#ifdef DEBUG
  Serial.begin(115200);
#endif

  lcd.init();
  lcd.blink_off();
  lcd.cursor_off();
  lcd.backlight();
  lcd.noAutoscroll();
  lcd.clear();
  lcd.display();

  show_string(0, 0, 20, "FluxCompSynth");

  /*  //------------------------<TESTCODE>
    uint8_t voice;
    uint8_t bank;
    char b[16];

    for (bank = 0; bank <= 1; bank++)
    {
      for (voice = 0; voice <= 127; voice++)
      {
        show(1, 0, 1, itoa(bank, _buf10, 10), true);
        show(1, 2, 3, itoa(voice, _buf10, 10), true);
        voiceName(b, bank, voice);
        show(1, 6, 16, b, false);
      }
    }
    //------------------------</TESTCODE>
  */
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
  int8_t dir1=0;
  int8_t dir2=0;
  int8_t but1=0;
  int8_t but2=0;
  
  // do the update stuff
  Encoder1.tick();
  Encoder2.tick();
  if (but1=Button1.update())
  {
    if (Button1.fell())
    {
      synth_voice_config[channel].patch = voice;
      initSynth(channel, bank, voice, 64, 0, 0, 0);
      Serial.println("Storing...");
    }
  }
  Button2.update();

  // Encoder1 handling
  dir1 = Encoder1.hasChanged();
  if (dir1)
  {
    voice = uint8_t(encoder_move(dir1, 0, 127, long(voice)));
  }

  // Encoder2 handling
  dir2 = Encoder2.hasChanged();
  if (dir2)
  {
    channel = uint8_t(encoder_move(dir2, 1, 16, long(channel)));
    voice = synth_voice_config[channel].patch;
  }

  // show UI
  if (dir1 || dir2 || but1 || but2)
    show_ui();

  /*
    boolean no = false;
    if (millis() % 2000 == 0)
    {
      if (no == false)
      {
        synth.noteOn( 1, 64, 100 );
        no = true;
      }
      else
      {
        synth.noteOff( 1, 64 );
        no = false;
      }
    }
  */
}

//**************************************************************************
// FUNCTIONS

void show_ui(void)
{
  char voice_name[17];

  // Show-UI
  if (channel == 9)
  {
    synth_voice_config[channel].patch = uint8_t(pgm_read_byte(&_drum_prog_map[voice % 5]));
    strcpy_P(voice_name, (char*)pgm_read_word(&(_drum_name[voice % 5])));
  }
  else
  {
    voiceName(voice_name, bank, voice);
  }
  show_string(1, 0, 16, voice_name);
  show_num(1, 18, 2, channel);
}

// Output routine for FluxSynth.
bool sendMidiByte(byte B)
{
  midiport.write(B);
  return true;
}

void initSynth(byte Bank, byte Chan, byte Patch, byte Vol, byte Rev, byte Chor, byte Bend)
{
  synth.programChange( Chan, Bank, Patch );
  synth.setChannelVolume( Chan, Vol );
  synth.setReverbSend( Chan, Rev );
  synth.setChorusSend( Chan, Chor );
  synth.setBendRange( Chan, Bend );
}

void voiceName(char *buffer, uint8_t bank, uint8_t program)
{
  strcpy_P(buffer, (char*)pgm_read_word(&(_voice_name[bank * 128 + program])));
}

void show_string(uint8_t pos_y, uint8_t pos_x, uint8_t field_size, char *str)
{
  show(pos_y, pos_x, field_size, str, false, false);
}

void show_num(uint8_t pos_y, uint8_t pos_x, uint8_t field_size, long num)
{
  char _buf10[11];

  show(pos_y, pos_x, field_size, itoa(num, _buf10, 10), true, true);
}

void show(uint8_t pos_y, uint8_t pos_x, uint8_t field_size, char *str, bool justify_right, bool fill_zero)
{
  char tmp[LCD_CHARS + 1];
  char *s = tmp;
  uint8_t l = strlen(str);

  if (fill_zero == true)
    memset(tmp, '0', field_size);
  else
    memset(tmp, 0x20, field_size); // blank
  tmp[field_size] = '\0';

  if (l > field_size)
    l = field_size;

  if (justify_right == true)
    s += field_size - l;

  strncpy(s, str, l);

  lcd.setCursor(pos_x, pos_y);
  lcd.print(tmp);

#ifdef DEBUG
  Serial.print(pos_y, DEC);
  Serial.print(F("/"));
  Serial.print(pos_x, DEC);
  Serial.print(F(": ["));
  Serial.print(tmp);
  Serial.println(F("]"));
#endif
}

long encoder_move(int8_t dir, int16_t min, int16_t max, long value)
{
  if (value + dir < min)
    value = min;
  else if (value + dir > max)
    value = max;
  else
    value += dir;

  return (value);
}

