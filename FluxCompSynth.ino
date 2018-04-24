//
// FluxCompSynth
//
// Flexama-Synth-Shield based Arduino synthesizer
//
// (c)2018 H. Wirtz <wirtz@parasitstudio.de>
//

#include <FluxSynth.h> /* https://sourceforge.net/projects/flexamysynth/files/ */
#include <SoftwareSerial.h>
#include <PgmChange.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h> /* https://github.com/marcoschwartz/LiquidCrystal_I2C (https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library) */
#include <RotaryEncoderDir.h> /* https://github.com/dcoredump/RotaryEncoderDir.git */
#include <Bounce2.h> /* https://github.com/thomasfredericks/Bounce2 */
#include <EEPROM.h>
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

#define REFRESH_BUT1 0
#define REFRESH_BUT2 1
#define REFRESH_ENC1 2
#define REFRESH_ENC2 3
#define REFRESH 7

struct SynthGlobal
{
  int8_t volume = 50; // negative means OFF
  int8_t pan = 0;
  int8_t transpose = 0;
  uint8_t reverb_level = REV_DEFLEVEL;
  int8_t reverb_program = REV_ROOM1; // negative means OFF, 0-7
  uint8_t reverb_time = 50;
  uint8_t reverb_feedback = 10;
  uint8_t reverb_character = 0;
  uint8_t chorus_level = 0;
  int8_t chorus_program = CHO_CHORUS1; // negative means OFF, 0-7
  uint8_t chorus_delay = 50;
  uint8_t chorus_feedback = 10;
  uint8_t chorus_rate = 10;
  uint8_t chorus_depth = 20;
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

#define MAX_STORAGE 8
//#define INIT_STORAGE 1

//**************************************************************************
// GLOBALS

// for  LCD-Modul QC2204A LCD2004 I2C-Controller
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_CHARS, LCD_LINES);

// Outgoing serial port
SoftwareSerial midiport(255, FLUXAMA_MIDI_IN_PIN); // 255 = OFF

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
uint8_t channel = 0;
uint8_t bank = PATCH_BANK0;
uint8_t refresh = REFRESH;

//
//**************************************************************************
// MAIN FUNCTIONS

void setup(void)
{
#ifdef DEBUG
  Serial.begin(115200);
  Serial.println(sizeof(SynthVoice)*16+sizeof(SynthGlobal));
#endif

  lcd.init();
  lcd.blink_off();
  lcd.cursor_off();
  lcd.backlight();
  lcd.noAutoscroll();
  lcd.clear();
  lcd.display();

  show_string(0, 0, 20, "FluxCompSynth");

  midiport.begin(31250);
  synth.begin();
  synth.sendByte = sendMidiByte;
  synth.midiReset();
  synth.GS_Reset();
  synth.GM_Reset();
  synth.postprocGeneralMidi(true);  // Surround + EQ on GM
  synth.postprocReverbChorus(true); // Surround + EQ on Reverb and Chorus
  synth.surroundMonoIn(false);
  
  pinMode(ENCODER1_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENCODER2_BUTTON_PIN, INPUT_PULLUP);
  pinMode(POT1_PIN, INPUT_PULLUP);
  pinMode(POT2_PIN, INPUT_PULLUP);
  pinMode(POT3_PIN, INPUT_PULLUP);
  pinMode(POT4_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

#ifdef INIT_STORAGE
  init_storage();
  for(uint8_t i=0;i<16;i++)
    store_setup(0,i);
#else
  restore_setup(0);
#endif

  //lcd.clear();
}

void loop(void)
{
  int8_t dir = 0;

  // do the update stuff
  Encoder1.tick();
  Encoder2.tick();

  // handle buttons
  if (Button1.update())
  {
    if (Button1.fell())
    {
      bitSet(refresh, REFRESH_BUT1);
    }
  }
  if (Button2.update())
  {
    if (Button2.fell())
    {
      bitSet(refresh, REFRESH_BUT2);
      synth_voice_config[channel].patch = (bank << 7)|voice;
      setSynth(channel);
      store_voice_setup(0, channel);
    }
  }

  // Encoder1 handling
  dir = Encoder1.hasChanged();
  if (dir)
  {
    bitSet(refresh, REFRESH_ENC1);
    voice = uint8_t(encoder_move(dir, 0, 127, long(voice)));
    synth_voice_config[channel].patch = voice;
  }

  // Encoder2 handling
  dir = Encoder2.hasChanged();
  if (dir)
  {
    bitSet(refresh, REFRESH_ENC2);
    channel = uint8_t(encoder_move(dir, 0, 15, long(channel)));
    voice = synth_voice_config[channel].patch;
  }

  // show UI
  if (refresh)
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
    voice = voice % 5;
    synth_voice_config[channel].patch = uint8_t(pgm_read_byte(&_drum_prog_map[voice]));
    strcpy_P(voice_name, (char*)pgm_read_word(&(_drum_name[voice])));
  }
  else
  {
    voiceName(voice_name, bank, synth_voice_config[channel].patch);
  }
  show_string(1, 0, 16, voice_name);
  show_num(1, 18, 2, channel + 1);

  refresh = 0;
}

// Output routine for FluxSynth.
bool sendMidiByte(byte B)
{
  midiport.write(B);
  return true;
}

void setSynth(uint8_t channel)
{
  synth.programChange(channel, synth_voice_config[channel].patch & 0x7f, synth_voice_config[channel].patch >> 7);
  synth.setChannelVolume(channel, synth_voice_config[channel].volume);
  synth.GM_Volume(channel, synth_voice_config[channel].volume);
  synth.GM_Pan(channel, synth_voice_config[channel].pan);
  synth.setReverbSend(channel, synth_voice_config[channel].reverb_send);
  synth.GM_ReverbSend(channel, synth_voice_config[channel].reverb_send);
  synth.setChorusSend(channel, synth_voice_config[channel].chorus_send);
  synth.GM_ChorusSend(channel, synth_voice_config[channel].chorus_send);
  synth.setBendRange(channel, synth_voice_config[channel].bend_range );
}

void setConfig(void)
{
  // Global settings
  synth.setMasterVolume(synth_config.volume);
  synth.GS_MasterVolume(synth_config.volume);
  synth.GS_MasterPan(synth_config.pan);
  synth.setClippingMode(synth_config.clipping);
  synth.setMasterTranspose(synth_config.transpose);

  // Reverb
  if (synth_config.reverb_program >= 0)
  {
    synth.enableReverb(true);
    synth.setReverb(synth_config.reverb_program, synth_config.reverb_time, synth_config.reverb_feedback, synth_config.reverb_character);
    synth.setReverbLevel(synth_config.reverb_level);
  }
  else
    synth.enableReverb(false);
  // Chorus
  if (synth_config.chorus_program >= 0)
  {
    synth.enableEffects(true);
    synth.setChorus(synth_config.chorus_program, synth_config.chorus_delay, synth_config.chorus_feedback, synth_config.chorus_rate, synth_config.chorus_depth);
    synth.setChorusLevel(synth_config.chorus_level);
  }
  else
    synth.enableEffects(false);
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

void store_voice_setup(uint8_t n, uint8_t channel)
{
  uint8_t v;

  if (n > MAX_STORAGE - 1 || channel > 15)
    return;

  // store voice configs
  EEPROM.put(n * (sizeof(synth_config) + sizeof(synth_voice_config)) + sizeof(synth_config) + channel * sizeof(synth_voice_config), synth_voice_config[channel]);
}

void store_setup(uint8_t n)
{
  uint8_t v;

  if (n > MAX_STORAGE - 1)
    return;

  // store global config
  EEPROM.put(n * (sizeof(synth_config) + sizeof(synth_voice_config)), synth_config);
  for (v = 0; v < 16; v++)
  {
    // store voice configs
    store_voice_setup(n, v);
  }
}
void restore_setup(uint8_t n)
{
  uint8_t v;

  if (n > MAX_STORAGE - 1)
    return;

  // restore global config
  EEPROM.get(n * (sizeof(synth_config) + sizeof(synth_voice_config)), synth_config);
  setConfig();
  for (v = 0; v < 16; v++)
  {
    // restore voice configs
    EEPROM.get(n * (sizeof(synth_config) + sizeof(synth_voice_config)) + sizeof(synth_config) + v * sizeof(synth_voice_config), synth_voice_config[v]);
  }
  synth.restartEffects();
}

#ifdef INIT_STORAGE
void init_storage(void)
{
  show_string(1, 0, 20, "Init Storage");

  for (uint16_t i = 0 ; i < EEPROM.length() ; i++)
  {
    show_num(2, 0, 4, i);
    EEPROM.write(i, 0);
  }

  show_string(1, 0, 20, "");
  show_string(2, 0, 20, "");
}
#endif
