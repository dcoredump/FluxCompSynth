/*
    Use the Fluxamasynth shield to mimick a known movie company jingle,
    then generate a movie soundtrack thingy for a while :)
    
    This code is released to the public domain, with no warranties.
    Author - Love Nystrom.
*/

#include <FluxSynth.h>
#include <PgmChange.h>

FluxSynth syn;

long msStart;

#define OCT3    OCTAVE*3
#define OCT4    OCTAVE*4

void initChan( byte Chan, byte Patch, byte Vol, byte Rev, byte Chor, byte Bend )
{
  syn.programChange( Chan, Patch );
  syn.setChannelVolume( Chan, Vol );
  syn.setReverbSend( Chan, Rev );
  syn.setChorusSend( Chan, Chor );
  syn.setBendRange( Chan, Bend );
}

#define Note_1  NOTE_E + OCT3
#define Note_2  NOTE_B + OCT3
#define Note_3  NOTE_E + OCT4
#define Note_4  NOTE_B + OCT4
#define InitMasterVol  100

bool sendMidiByte( byte B ) { // Output routine for FluxSynth.
  Serial.write( B );
  return true;
  }
    
void setup()
{
  Serial.begin( 31250 ); // MIDI baudrate
  syn.sendByte = sendMidiByte;
  syn.begin();
  syn.midiReset();
  syn.GS_Reset();
  delay( 500 );
    
  syn.setPatchBank( BANK_GM ); // Bank select for all channels except 9 (drums)
  initChan( 0, GM_SynthBass2,      126, 126, 124, 12 );
  initChan( 1, GM_StringEnsemble1, 120, 126, 124, 12 );
  initChan( 2, GM_ChurchOrgan,     110, 126, 110, 12 );
  initChan( 3, GM_Pad3_Polysynth,  100, 126, 124, 12 );
  initChan( 4, GM_ChoirAahs,       100, 126, 124, 12 );

  syn.setReverb( REV_PANDELAY, 127, 127, 5 );
  syn.setChorus( CHO_FLANGER, 100, 100, 2, 127 );
  syn.setReverbLevel( 126 );
  syn.setChorusLevel( 126 );
  delay( 10 );

  byte vol = InitMasterVol;
  syn.setMasterVolume( vol );
    
  // Up sweep notes
  syn.noteOn( 0, Note_1, VEL_FORTISSIMO );
  syn.noteOn( 1, Note_1, VEL_FORTISSIMO );
  syn.noteOn( 2, Note_2, VEL_FORTISSIMO );
  // Down sweep notes
  syn.noteOn( 3, Note_3, VEL_FORTISSIMO );
  syn.noteOn( 4, Note_4, VEL_FORTISSIMO );

  int bend = 0x2000;
  while( bend > 0 )
  {
    syn.pitchBend( 0, CTV_CENTER14 - bend );
    syn.pitchBend( 1, CTV_CENTER14 - bend );
    syn.pitchBend( 2, CTV_CENTER14 - bend );
    syn.pitchBend( 3, CTV_CENTER14-1 + bend );
    syn.pitchBend( 4, CTV_CENTER14-1 + bend );
    bend -= 64;
    delay( 30 );
  }
  
  while( ++vol < 127 ) {
    syn.setMasterVolume( vol );
    delay( 15 );
  }
  delay( 4000 );
  while( vol-- ) {
    syn.setMasterVolume( vol );
    delay( 10 );
  }
  syn.noteOff( 0, Note_1 );
  syn.noteOff( 1, Note_1 );
  syn.noteOff( 2, Note_2 );
  syn.noteOff( 3, Note_3 );
  syn.noteOff( 4, Note_4 );

  syn.setMasterVolume( InitMasterVol );
  randomSeed( analogRead( A1 ));
  msStart = millis();
}

#define NoteE  NOTE_E + OCTAVE*2

byte vol = InitMasterVol;
byte minorScale[15] = { 0,2,3,5,7,8,10,12 };
byte Count = 0;
byte NoteX, Len;

void loop()
{
  if (msStart) 
  {
    if (!Count) {
      NoteX = OCT3 + NOTE_E + minorScale[ random( 8 )];
      syn.noteOn( 3, NoteX, VEL_FORTISSIMO + random( 16 ));
      //syn.noteOn( 1, NoteX, VEL_MEZZOPIANO + random( 8 ));
      Len = 8 + 8*random( 2 );
    }

    syn.noteOn( 0, NoteE, VEL_FORTE + random( 32 )); delay( 150 );
    syn.noteOff( 0, NoteE ); delay( 50 );

    if (++Count == Len) {
      syn.noteOff( 3, NoteX );
      //syn.noteOff( 1, NoteX );
      Count = 0;
    }
    if ((millis() - msStart) > 120000) {
      syn.midiReset();
      msStart = 0;
    }
  }
}

