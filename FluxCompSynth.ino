//
// FluxCompSynth
//
// Flexama-Synth-Shield based Arduino synthesizer
//
// (c)2018 H. Wirtz <wirtz@parasitstudio.de>
//

#include "config.h"
#include <FluxSynth.h> /* https://sourceforge.net/projects/flexamysynth/files/ */
#include <SoftwareSerial.h>
#include <MIDI.h>
#include <PgmChange.h>
#include <Wire.h>
#include <LiquidCrystalPlus_I2C.h> /* https://github.com/dcoredump/LiquidCrystalPlus_I2C (https://github.com/marcoschwartz/LiquidCrystal_I2C) */
#include <RotaryEncoderDir.h> /* https://github.com/dcoredump/RotaryEncoderDir.git */
#include <Bounce2.h> /* https://github.com/thomasfredericks/Bounce2 */
#include <EEPROM.h>
#include "FluxVoiceNames.h" // Voice names in PROGMEM

#if !defined(__AVR_ATmega2560__)  // Arduino MEGA2560
#error Arduino-MEGA-2560 is needed!
#else
//#define EXTENDED_SETUP
#define FLUXAMA_MIDI_IN Serial1
#endif

#define LED_PIN 13

#define LCD_I2C_ADDRESS 0x3f
#define LCD_CHARS 20
#define LCD_LINES 4

#define FLUXAMA_MIDI_OUT_PIN 4
#define FLUXAMA_MIDI_IN_PIN 3

#define DEBOUNCE_INTERVAL_MS 5
#define ENCODER1_PIN_A 5
#define ENCODER1_PIN_B 6
#define ENCODER1_BUTTON_PIN 7
#define ENCODER2_PIN_A 8
#define ENCODER2_PIN_B 9
#define ENCODER2_BUTTON_PIN 10
#ifdef EXTENDED_SETUP
#define ENCODER3_PIN_A 11
#define ENCODER3_PIN_B 12
#define ENCODER3_BUTTON_PIN 13
#define ENCODER4_PIN_A 22
#define ENCODER4_PIN_B 23
#define ENCODER4_BUTTON_PIN 24
#define MAX_ENCODER 4
#else
#define MAX_ENCODER 2
#endif

#define POT1_PIN A0
#define POT2_PIN A1
#define POT3_PIN A2
#define POT4_PIN A3
#ifdef EXTENDED_SETUP
#define POT5_PIN A4
#define POT6_PIN A5
#define POT7_PIN A6
#define POT8_PIN A7
#define POT9_PIN A8
#define POT10_PIN A9
#define POT11_PIN A10
#define POT12_PIN A11
#define POT13_PIN A12
#define POT14_PIN A13
#define POT15_PIN A14
#define POT16_PIN A15
#endif

#define REFRESH_BUT1 0
#define REFRESH_BUT2 1
#ifdef EXTENDED_SETUP
#define REFRESH_BUT3 2
#endif
#define REFRESH_ENC1 3
#define REFRESH_ENC2 4
#ifdef EXTENDED_SETUP
#define REFRESH_ENC3 5
#endif
#define REFRESH_POT 6
#define REFRESH 7

struct SynthGlobal
{
  int8_t level = 50; // negative means OFF
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
#ifdef EXTENDED_SETUP
  uint8_t eq_bass = 0;
  uint8_t eq_lowmid = 0;
  uint8_t eq_highmid = 0;
  uint8_t eq_high = 0;
  uint8_t surround_postproc = 0; // Bit0: enable sourround, Bit1: enable surround+eq on GM, Bit2: enable surround+eq on reverb/chorus
#endif
} synth_config;

struct SynthVoice
{
  int8_t patch = -1;
  uint8_t bank = 0;
  int8_t volume = 64; // negative means OFF
  int8_t pan = 0;   // 0=middle
  int8_t transpose = 0;
  uint8_t reverb_send = 0;
  uint8_t chorus_send = 0;
  uint8_t bend_range = 12;
} synth_voice_config[16];

#ifdef EXTENDED_SETUP
struct SynthDrumMix
{
  int8_t note = 48; // C3
  uint8_t level = 127;
  int8_t pan = 0;
  uint8_t reverb_send = 0;
  uint8_t chorus_send = 0;
  uint8_t pitch = 64;
} synth_drummix_config[79]; // Note 27 (D#1)- 106( A#7)
#endif

//**************************************************************************
// GLOBALS

// for  LCD-Modul QC2204A LCD2004 I2C-Controller
LiquidCrystalPlus_I2C lcd(LCD_I2C_ADDRESS, LCD_CHARS, LCD_LINES);

// Fluxama serial port
SoftwareSerial fluxama(255, FLUXAMA_MIDI_OUT_PIN); // 255 = OFF

// MIDI-IN port
//SoftwareSerial midiport(FLUXAMA_MIDI_IN_PIN, 255); // 255 = OFF
MIDI_CREATE_INSTANCE(HardwareSerial, FLUXAMA_MIDI_IN, midi_in);

// Synth
FluxSynth synth;

// Encoder
RotaryEncoderDir Encoder1(ENCODER1_PIN_A, ENCODER1_PIN_B);
RotaryEncoderDir Encoder2(ENCODER2_PIN_A, ENCODER2_PIN_B);
#ifdef EXTENDED_SETUP
RotaryEncoderDir Encoder3(ENCODER3_PIN_A, ENCODER3_PIN_B);
RotaryEncoderDir Encoder4(ENCODER4_PIN_A, ENCODER4_PIN_B);
#endif

// Buttons (debouncer)
Bounce Button1 = Bounce(ENCODER1_BUTTON_PIN, DEBOUNCE_INTERVAL_MS);
Bounce Button2 = Bounce(ENCODER2_BUTTON_PIN, DEBOUNCE_INTERVAL_MS);
#ifdef EXTENDED_SETUP
Bounce Button3 = Bounce(ENCODER3_BUTTON_PIN, DEBOUNCE_INTERVAL_MS);
Bounce Button4 = Bounce(ENCODER4_BUTTON_PIN, DEBOUNCE_INTERVAL_MS);
#endif

// vars
int8_t voice = -1;
uint8_t channel = 0;
uint8_t bank = PATCH_BANK0;
uint8_t refresh = REFRESH;
const uint8_t max_storage = EEPROM.length() / (sizeof(SynthVoice) * 16 + sizeof(SynthGlobal));
//
//**************************************************************************
// MAIN FUNCTIONS

void setup(void)
{
#ifdef DEBUG
  Serial.begin(115200);
  Serial.println(F("Start"));
  //Serial.println(sizeof(SynthVoice)*16+sizeof(SynthGlobal));
#endif

  lcd.init();
  lcd.blink_off();
  lcd.cursor_off();
  lcd.backlight();
  lcd.noAutoscroll();
  lcd.clear();
  lcd.display();

  lcd.show(0, 0, 20, "FluxCompSynth");

  fluxama.begin(31250);
  synth.begin();
  synth.sendByte = sendMidiByte;
  synth.midiReset();
  synth.GS_Reset();
  synth.GM_Reset();
  synth.postprocGeneralMidi(false);  // Surround + EQ on GM
  synth.postprocReverbChorus(false); // Surround + EQ on Reverb and Chorus
  synth.surroundMonoIn(false);

  //midi_in.begin(MIDI_CHANNEL_OMNI);

  pinMode(ENCODER1_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENCODER2_BUTTON_PIN, INPUT_PULLUP);
  pinMode(POT1_PIN, INPUT_PULLUP);
  pinMode(POT2_PIN, INPUT_PULLUP);
  pinMode(POT3_PIN, INPUT_PULLUP);
  pinMode(POT4_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
#ifdef EXTENDED_SETUP
  pinMode(ENCODER3_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENCODER4_BUTTON_PIN, INPUT_PULLUP);
  pinMode(POT5_PIN, INPUT_PULLUP);
  pinMode(POT6_PIN, INPUT_PULLUP);
  pinMode(POT7_PIN, INPUT_PULLUP);
  pinMode(POT8_PIN, INPUT_PULLUP);
  pinMode(POT9_PIN, INPUT_PULLUP);
  pinMode(POT10_PIN, INPUT_PULLUP);
  pinMode(POT11_PIN, INPUT_PULLUP);
  pinMode(POT12_PIN, INPUT_PULLUP);
  pinMode(POT13_PIN, INPUT_PULLUP);
  pinMode(POT14_PIN, INPUT_PULLUP);
  pinMode(POT15_PIN, INPUT_PULLUP);
  pinMode(POT16_PIN, INPUT_PULLUP);
#endif
  digitalWrite(LED_PIN, LOW);

#ifdef INIT_STORAGE
  init_storage();
  for (uint8_t i = 0; i < 16; i++)
    store_setup(i);
#else
  restore_setup(0);
#endif

  //lcd.clear();
}

void loop(void)
{
  int8_t dir = 0;

  // Forward MIDI-IN to Fluxama
  //fluxama.write(midi_in.read());

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
      synth_voice_config[channel].patch = (bank << 7) | voice;
      setSynth(channel);
      store_voice_setup(0, channel);
    }
  }

  // Encoder1 handling
  dir = Encoder1.hasChanged();
  if (dir)
  {
    bitSet(refresh, REFRESH_ENC1);
    voice = uint8_t(encoder_move(dir, -1, 127, long(voice)));
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

#ifdef PLAY_TEST_CHORD
  boolean no = false;
  if (millis() % 2000 == 0)
  {
    if (no == false)
    {
      synth.noteOn( 1, 64, 100 );
      synth.noteOn( 1, 68, 100 );
      synth.noteOn( 1, 71, 100 );
      no = true;
    }
    else
    {
      synth.noteOff( 1, 64 );
      synth.noteOff( 1, 68 );
      synth.noteOff( 1, 71 );
      no = false;
    }
  }
#endif
}

//**************************************************************************
// FUNCTIONS

void show_ui(void)
{
  char voice_name[17];

  // Show-UI
  if (voice < 0)
  {
    lcd.show(1, 0, 16, "OFF");
    lcd.show(1, 18, 2, channel + 1);
  }
  else
  {
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
    lcd.show(1, 0, 16, voice_name);
    lcd.show(1, 18, 2, channel + 1);
  }

  refresh = 0;
}

// Output routine for FluxSynth.
bool sendMidiByte(byte B)
{
  fluxama.write(B);
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
  synth.setMasterVolume(synth_config.level);
  synth.GS_MasterVolume(synth_config.level);
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

  if (n > max_storage - 1 || channel > 15)
    return;

  // store voice configs
  EEPROM.put(n * (sizeof(synth_config) + sizeof(synth_voice_config)) + sizeof(synth_config) + channel * sizeof(synth_voice_config), synth_voice_config[channel]);
}

void store_setup(uint8_t n)
{
  uint8_t v;

  if (n > max_storage - 1)
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

  if (n > max_storage - 1)
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
  voice = synth_voice_config[channel].patch;
}

#ifdef INIT_STORAGE
void init_storage(void)
{
  lcd.show(1, 0, 20, "Init Storage");

  for (uint16_t i = 0 ; i < EEPROM.length() ; i++)
  {
    lcd.show(2, 0, 4, i);
    EEPROM.write(i, 0);
  }

  lcd.show(1, 0, 20, "");
  lcd.show(2, 0, 20, "");
}
#endif
