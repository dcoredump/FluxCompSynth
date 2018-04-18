/* BendingNotes.pde
 * Demonstrates note bending on the Modern Device Fluxamasynth Shield
 * ModernDevice.com
 */

#include <FluxSynth.h>

FluxSynth synth;

int i = CTV_CENTER14;   // middle of pitch bend range

bool sendMidiByte( byte B ) { // Output routine for FluxSynth.
  Serial.write( B );
  return true;
  }
    
void setup()
{
  Serial.begin( 31250 ); // MIDI baudrate
  synth.sendByte = sendMidiByte;
  synth.begin();
  synth.GS_Reset();

  synth.programChange( 0, GM_Pad4_Choir );
  synth.pitchBendRange( 0, 4 );  // four semitones pitch bend range

  synth.noteOn( 0, NOTE_D + OCTAVE*5, 127 );
}

void loop()
{
  while( i < CTV_MAX14 )   // loop to upper bound of pitch bend range
  {
    synth.pitchBend( 0, i );
    i += 16;
    delay( 10 );
  }

  while( i > 0 )        // loop to lower bound of pitch bend range
  {
    synth.pitchBend( 0, i );
    i -= 16;
    delay( 10 );
  }
}
