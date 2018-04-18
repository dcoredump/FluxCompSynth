/* PlayAllVoices
 * Plays all voices with a random algorithmic tune
 * on the Modern Device Fluxamasynth Shield.
 */

#include <FluxSynth.h>
#include <PgmChange.h>

FluxSynth synth;

byte channel = 0;
byte bank = BANK_GM;
byte voice = 0;
byte j = 0;

byte minorScale[15] = {0,2,3,5,7,8,10, 12,14,15,17,19,20,22, 24 }; // 2 octaves
byte majorScale[15] = {0,2,4,5,7,9,11, 12,14,16,17,19,21,23, 24 }; // 2 octaves

bool sendMidiByte( byte B ) { // Output routine for FluxSynth.
  Serial.write( B );
  return true;
  }
    
void setup()
{
  Serial.begin( 31250 ); // MIDI baudrate
  synth.sendByte = sendMidiByte;
  synth.begin();
  synth.midiReset();
  
  synth.setPatchBank( bank );
  synth.programChange( DRUM_CHAN, DRUMSET_BRUSH );
  synth.programChange( channel, GM_GrandPiano );
  synth.setBendRange( channel, 12 ); // Bend a whole octave
}

void loop()
{
  synth.programChange( channel, voice );

  for( int i=0; i < 8; i++ )
  {
    byte note = majorScale[ random( 15 )] + OCTAVE*4;

    synth.noteOn( channel, note, 127 );
    delay( 110 );

    synth.noteOff( channel, note );
    delay( 10 );
  }

  if (++voice > 127) {
    voice = 0;
    if (bank == BANK_GM) bank = BANK_MT32; 
    else bank = BANK_GM;
    synth.setPatchBank( bank );
  }
}
