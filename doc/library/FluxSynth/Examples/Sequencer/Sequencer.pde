/*
    A simple MIDI sequencer example for the Modern Device Fluxamasynth shield.
    This code is released to the public domain, with no warranties.
    Author - Love Nystrom.
*/

#include <inttypes.h>
#include <avr/pgmspace.h>

#include <FluxSynth.h>
#include <PgmChange.h>

#define LED_PIN  13 // Visual feedback LED
#define _DBG

FluxSynth synt;

#define HIGH_BYTE(w)    (w >> 8)
#define prog_word       prog_uint16_t

// Simple 'sounding key' table with 12bit countdown duration.
// Limits each key to play on only one channel, no layering.
// However, the SAM2195 patches can be setup with layering.
// 12bit time makes for max 4 second note duration or pause.
//
// The table size is 254 byte, a compromise between ram use,
// ease of handling, and functionality. A playing que would
// be better on ram and functionality, at the expense of more
// complicated handling. This example is meant to be clear,
// so I kept the algorithm simple.

word keyTbl[ 127 ]; // 12 bit time (ms) and 4bit channel

#define KEY_TIME(w)  (w & 0x0FFF)
#define KEY_CHAN(w)  (w & 0xF000)
#define SHR_CHAN(w)  (w >> 12)
#define SHL_CHAN(b)  (word(b) << 12)

void startNote( byte chan, byte key, byte vel, word duration )
{
  if (keyTbl[ key ]) // if key already sounding, turn it off
    synt.noteOff( SHR_CHAN( keyTbl[ key ]), key );

  synt.noteOn( chan, key, vel );
  keyTbl[ key ] = SHL_CHAN( chan ) | KEY_TIME( duration );
  
  digitalWrite( LED_PIN, HIGH );
}

long msPrev = 0;
word msElapsed = 0;

void elapseNote( byte key ) 
{
  word chancode, time;
  if (keyTbl[ key ]) 
  {
    chancode = KEY_CHAN( keyTbl[ key ]);
    time = KEY_TIME( keyTbl[ key ]);
    if (time <= msElapsed)
    {
      synt.noteOff( SHR_CHAN(chancode), key );
      keyTbl[ key ] = 0;
      digitalWrite( LED_PIN, LOW );
    } 
    else 
    {
      time -= msElapsed;
      keyTbl[ key ] = chancode | time;
    }
  }
}

#define BASS_CH     1
#define STRING_CH   2
#define DRUM_CH     9

#define OCT_2   OCTAVE*2
#define OCT_3   OCTAVE*3
#define OCT_4   OCTAVE*4
#define OCT_5   OCTAVE*5

// Simple 3 word play que event, 2 12bit delta times (start/duration)

#define MK_EVENT( chan,key,vel,dton,dtoff ) \
  word( SHL_CHAN(chan)|KEY_TIME(dton) ), \
  word( (word(vel) << 8) | key ), \
  word( SHL_CHAN(chan)|KEY_TIME(dtoff) )

// Very simmple bass sequence with unrolled pattern loops, 192 byte.
// It would, of course, be better on ram if we could define patterns
// and combine and repeat them arbitrarily.
// That's left for you to do, I won't steal all your fun :)

#define PULSE    200
#define BLEN     (PULSE-10)
#define BVEL     VEL_FORTISSIMO

prog_word const sequence[] = 
{
  // 4 reps pattern 1
  MK_EVENT( BASS_CH, NOTE_E +OCT_2, BVEL, PULSE, BLEN ),  // bass 1
  MK_EVENT( BASS_CH, NOTE_E +OCT_2, BVEL, PULSE, BLEN ),  // bass
  MK_EVENT( BASS_CH, NOTE_G +OCT_2, BVEL, PULSE, BLEN ),  // bass
  MK_EVENT( BASS_CH, NOTE_B +OCT_2, BVEL, PULSE, BLEN ),  // bass
  
  MK_EVENT( BASS_CH, NOTE_E +OCT_2, BVEL, PULSE, BLEN ),  // bass 2
  MK_EVENT( BASS_CH, NOTE_E +OCT_2, BVEL, PULSE, BLEN ),  // bass
  MK_EVENT( BASS_CH, NOTE_G +OCT_2, BVEL, PULSE, BLEN ),  // bass
  MK_EVENT( BASS_CH, NOTE_B +OCT_2, BVEL, PULSE, BLEN ),  // bass

  MK_EVENT( BASS_CH, NOTE_E +OCT_2, BVEL, PULSE, BLEN ),  // bass 3
  MK_EVENT( BASS_CH, NOTE_E +OCT_2, BVEL, PULSE, BLEN ),  // bass
  MK_EVENT( BASS_CH, NOTE_G +OCT_2, BVEL, PULSE, BLEN ),  // bass
  MK_EVENT( BASS_CH, NOTE_B +OCT_2, BVEL, PULSE, BLEN ),  // bass

  MK_EVENT( BASS_CH, NOTE_E +OCT_2, BVEL, PULSE, BLEN ),  // bass 4
  MK_EVENT( BASS_CH, NOTE_E +OCT_2, BVEL, PULSE, BLEN ),  // bass
  MK_EVENT( BASS_CH, NOTE_G +OCT_2, BVEL, PULSE, BLEN ),  // bass
  MK_EVENT( BASS_CH, NOTE_B +OCT_2, BVEL, PULSE, BLEN ),  // bass

  // 4 reps pattern 2
  MK_EVENT( BASS_CH, NOTE_Fs +OCT_2, BVEL, PULSE, BLEN ),  // bass 5
  MK_EVENT( BASS_CH, NOTE_Fs +OCT_2, BVEL, PULSE, BLEN ),  // bass
  MK_EVENT( BASS_CH, NOTE_A  +OCT_2, BVEL, PULSE, BLEN ),  // bass
  MK_EVENT( BASS_CH, NOTE_Cs +OCT_3, BVEL, PULSE, BLEN ),  // bass
  
  MK_EVENT( BASS_CH, NOTE_Fs +OCT_2, BVEL, PULSE, BLEN ),  // bass 6
  MK_EVENT( BASS_CH, NOTE_Fs +OCT_2, BVEL, PULSE, BLEN ),  // bass
  MK_EVENT( BASS_CH, NOTE_A  +OCT_2, BVEL, PULSE, BLEN ),  // bass
  MK_EVENT( BASS_CH, NOTE_Cs +OCT_3, BVEL, PULSE, BLEN ),  // bass
  
  MK_EVENT( BASS_CH, NOTE_Fs +OCT_2, BVEL, PULSE, BLEN ),  // bass 7
  MK_EVENT( BASS_CH, NOTE_Fs +OCT_2, BVEL, PULSE, BLEN ),  // bass
  MK_EVENT( BASS_CH, NOTE_A  +OCT_2, BVEL, PULSE, BLEN ),  // bass
  MK_EVENT( BASS_CH, NOTE_Cs +OCT_3, BVEL, PULSE, BLEN ),  // bass
  
  MK_EVENT( BASS_CH, NOTE_Fs +OCT_2, BVEL, PULSE, BLEN ),  // bass 8
  MK_EVENT( BASS_CH, NOTE_Fs +OCT_2, BVEL, PULSE, BLEN ),  // bass
  MK_EVENT( BASS_CH, NOTE_A  +OCT_2, BVEL, PULSE, BLEN ),  // bass
  MK_EVENT( BASS_CH, NOTE_Cs +OCT_3, BVEL, PULSE, BLEN )   // bass
};
prog_word const seq_nevents = sizeof(sequence) / 6;
prog_word const seq_len = sizeof(sequence) / 2;

word seq_index;

// The sequence is in flash mem (read-only).
// We need to count down the delta time for note on
// so the next event is copied to ram.

word next_event[3];

void get_next_event() 
{
  for( byte i=0; i < 3; i++ )
    next_event[i] = sequence[ seq_index+i ];
}

byte mixLevel[2];
byte revLevel, revSend[2];
byte revProg, revTime, revFeedbk, revAlgo;
byte chorLevel, chorSend[2];
byte chorProg, chorDelay, chorFeedbk, chorRate, chorDepth;

// Program setup

bool sendMidiByte( byte B ) { // Output routine for FluxSynth.
  Serial.write( B );
  return true;
  }
    
void setup()
{
  seq_index = 0;
  get_next_event();
  for( byte i=0; i < 127; i++ ) keyTbl[i] = 0;
  
  Serial.begin( 31250 ); // MIDI baudrate
  synt.sendByte = sendMidiByte;
  synt.begin();
  synt.GS_Reset(); // Reset to GS mode
  delay( 500 );
  
  synt.setPatchBank( BANK_GM ); // Bank select for all channels except 9 (drums)
  synt.programChange( BASS_CH, GM_SynthBass1 ); // Synth bass
  synt.programChange( STRING_CH, GM_StringEnsemble1 ); // String ensemble
  synt.programChange( DRUM_CH, DRUMSET_STD ); // Standars drum set

  mixLevel[0] = 112;
  mixLevel[1] = 56;
  
  synt.setChannelVolume( BASS_CH, mixLevel[0] );
  synt.setChannelVolume( STRING_CH, mixLevel[1] );
  synt.setChannelVolume( DRUM_CH, 64 );

  synt.setDrumMix( DRUM_CH, DR_Kick, 72, CTV_CENTER, 32, 0 );
  synt.setDrumMix( DRUM_CH, DR_Snare, 64, CTV_CENTER+16, 48, 0 );
  synt.setDrumPitch( DRUM_CH, DR_Snare, CTV_CENTER-2 ); // Deeper snare tone
  synt.setDrumMix( DRUM_CH, DR_ClosedHiHat, 48, CTV_CENTER-16, 32, 0 );
  synt.setDrumMix( DRUM_CH, DR_PedalHiHat, 48, CTV_CENTER-16, 32, 0 );
  synt.setDrumMix( DRUM_CH, DR_OpenHiHat, 48, CTV_CENTER-16, 32, 0 );

  // Effects and post processing
  
  revProg   = REV_HALL1;
  revTime   = 112;  // 0-127
  revFeedbk = 32;   // Only for REV_DELAY and REV_PANDELAY
  revAlgo   = 4;    // Rev algorithm 0-7
  revLevel  = 96;
  revSend[0] = 64;
  revSend[1] = 88;
  
  synt.setReverb( revProg, revTime, revFeedbk, revAlgo ); // Master reverb setting
  synt.setReverbLevel( revLevel ); // Master reverb return level
  synt.setReverbSend( BASS_CH, revSend[0] );
  synt.setReverbSend( STRING_CH, revSend[1] );

  chorProg   = CHO_FLANGER;
  chorDelay  = 86;
  chorFeedbk = 112; 
  chorRate   = 1;
  chorDepth  = 3;
  chorLevel  = 64;
  chorSend[0] = 80;
  chorSend[1] = 40;
  
  synt.setChorus( chorProg, chorDelay, chorFeedbk, chorRate, chorDepth ); // Master chorus setting
  synt.setChorusLevel( chorLevel ); // Master chorus return level
  synt.setChorusSend( BASS_CH, chorSend[0] );
  synt.setChorusSend( STRING_CH, chorSend[1] );

  synt.setClippingMode( SOFT_CLIP );

  synt.setEqualizer( EQ_BASS, freqctrlBass( 220 ), ccByte( +32 ));
  synt.setEqualizer( EQ_LOWMID, freqctrlMid( 880 ), ccByte( 0 ));
  synt.setEqualizer( EQ_HIGHMID, freqctrlMid( 2200 ), ccByte( 0 ));
  synt.setEqualizer( EQ_TREBLE, freqctrlTreble( 8800 ), ccByte( +24 ));
  synt.setSurroundVolume( 72 );
  synt.setSurroundDelay( 96 );
  synt.surroundMonoIn( false );
  synt.postprocGeneralMidi( true );
  synt.postprocReverbChorus( true );
  
  msPrev = millis();
  Serial.begin( 115200 );
}

// Rudimentary serial value reader

#define comAvail  Serial.available()

int readNr()
{
  int val = -1;
  boolean haveDigit = false;
  byte b;
  while (comAvail) { // skip non-digits
    b = Serial.read();
    //delay(1); // agh
    if (b >= '0' && b <= '9') {
      haveDigit = true;
      break;
    }
  }
  if (haveDigit) {
    val = b - '0';
    while (comAvail)  {
      b = Serial.read();
      //delay(1); // agh
      if (b < '0' || b > '9') break; // alas, there's no 'put_back'
      val *= 10;
      val += (b - '0');
    }
  }
  return val;
}

// Debugging output

#ifdef _DBG
#define ECHO1(cap,n1) Serial.print( cap ); Serial.println( n1,DEC )
#define ECHO3(cap,n1,n2,n3)  Serial.print( cap ); \
  Serial.print( n1,DEC ); Serial.print(' '); \
  Serial.print( n2,DEC ); Serial.print(' '); \
  Serial.println( n3,DEC )
#define ECHO4(cap,n1,n2,n3,n4)  Serial.print( cap ); \
  Serial.print( n1,DEC ); Serial.print(' '); \
  Serial.print( n2,DEC ); Serial.print(' '); \
  Serial.print( n3,DEC ); Serial.print(' '); \
  Serial.println( n4,DEC )
#define ECHO5(cap,n1,n2,n3,n4,n5)  Serial.print( cap ); \
  Serial.print( n1,DEC ); Serial.print(' '); \
  Serial.print( n2,DEC ); Serial.print(' '); \
  Serial.print( n3,DEC ); Serial.print(' '); \
  Serial.print( n4,DEC ); Serial.print(' '); \
  Serial.println( n5,DEC )
#else
#define ECHO1(cap,n1)
#define ECHO3(cap,n1,n2,n3)
#define ECHO4(cap,n1,n2,n3,n4)
#define ECHO5(cap,n1,n2,n3,n4,n5)
#endif

// Program loop

void loop()
{
  word chancode, deltatime;
    
  long now = millis();
  msElapsed = word( now - msPrev );

  // See if sounding notes delta time have expired for note off

  for( byte i=0; i < 127; i++ ) elapseNote( i );

  // See if delta time for note on has expired for next event

  deltatime = KEY_TIME( next_event[0] );
  if (deltatime <= msElapsed) 
  {
    startNote (
      SHR_CHAN( next_event[ 0 ]),   // channel
      MIDIDATA( next_event[ 1 ]),   // key
      HIGH_BYTE( next_event[ 1 ]),  // velocity
      KEY_TIME( next_event[ 2 ])    // duration
      );
    if ((seq_index += 3) >= seq_len) seq_index = 0; // repeat
    get_next_event();
  }
  else // Count down the delta time for next note on
  {
    deltatime -= msElapsed;
    chancode = KEY_CHAN( next_event[ 0 ]);
    next_event[ 0 ] = chancode | deltatime;
  }

  // Check for parameter changes from PC host

  if (comAvail) 
  {
    switch( Serial.read() ) 
    {
      case 'R': // Reverb parameters
        // R <0..7> <0..127> <0..127> <0..7>
        if (comAvail) revProg = readNr();
        if (comAvail) revTime = readNr();
        if (comAvail) revFeedbk = readNr();
        if (comAvail) revAlgo = readNr();
        ECHO4( "R ", revProg, revTime, revFeedbk, revAlgo );
        synt.setReverb( revProg, revTime, revFeedbk, revAlgo );
        break;

      case 'r': // Reverb level
        // r <0..127>
        if (comAvail) revLevel = readNr();
        ECHO1( "r ", revLevel );
        synt.setReverbLevel( revLevel );
        break;

      case 'C': // Chorus parameters
        // C <0..7> <0..127> <0..127> <0..127> <0..127>
        if (comAvail) chorProg = readNr();
        if (comAvail) chorDelay = readNr();
        if (comAvail) chorFeedbk = readNr();
        if (comAvail) chorRate = readNr();
        if (comAvail) chorDepth = readNr();
        ECHO5( "C ", chorProg, chorDelay, chorFeedbk, chorRate, chorDepth );
        synt.setChorus( chorProg, chorDelay, chorFeedbk, chorRate, chorDepth );
        break;

      case 'c': // Chorus level
        // c <0..127>
        if (comAvail) chorLevel = readNr();
        ECHO1( "c ", chorLevel );
        synt.setChorusLevel( chorLevel );
        break;

      case 'B': // Bass channel settings
        // B <0..127> <0..127> <0..127>
        if (comAvail) mixLevel[0] = readNr();
        if (comAvail) revSend[0] = readNr();
        if (comAvail) chorSend[0] = readNr();
        ECHO3( "B ", mixLevel[0], revSend[0], chorSend[0] );
        synt.setChannelVolume( BASS_CH, mixLevel[0] );
        synt.setReverbSend( BASS_CH, revSend[0] );
        synt.setChorusSend( BASS_CH, chorSend[0] );
        break;

      case 'S': // String channel settings
        // S <0..127> <0..127> <0..127>
        if (comAvail) mixLevel[1] = readNr();
        if (comAvail) revSend[1] = readNr();
        if (comAvail) chorSend[1] = readNr();
        ECHO3( "S ",mixLevel[1], revSend[1], chorSend[1] );
        synt.setChannelVolume( STRING_CH, mixLevel[1] );
        synt.setReverbSend( STRING_CH, revSend[1] );
        synt.setChorusSend( STRING_CH, chorSend[1] );
        break;
        
      //case 'T': setPulse( msPulse + 10 ); break;
      //case 't': setPulse( msPulse - 10 ); break;
    }
  }

  // Do something more if you need to

  msPrev = now;
}
