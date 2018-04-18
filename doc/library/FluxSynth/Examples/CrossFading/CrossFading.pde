/* CrossFading.pde
 * Demonstrates cross fading on the Modern Device Fluxamasynth Shield
 * ModernDevice.com
 *
 * Updated Jun 2012, Love Nystrom
 */

#include <FluxSynth.h>
#include <PgmChange.h>

FluxSynth      synth;
//NewSoftSerial  port( 255, 4 );

byte tone1 = NOTE_E + OCTAVE*4;
byte tone2 = NOTE_Cs + OCTAVE*5;

byte vol1 = 64;          // Volumes must be inverses of each other
byte vol2 = 127 - vol1;  // or they'll get out of sync

byte i = 1;              // Step size
byte dir = 1;            // Direction to step in: 1 = up, -1 = down

bool sendMidiByte( byte B ) { // Output routine for FluxSynth.
  //port.print( B );
  Serial.write( B );
  return true;
  }
    
void setup()
{
  Serial.begin( 31250 ); // MIDI baudrate
  synth.sendByte = sendMidiByte;
  synth.begin();
  synth.GM_Reset();
  
  synth.programChange( 0, GM_Pad2_Warm ); // give our two channels different voices
  synth.programChange( 1, GM_Pad3_Polysynth );

  synth.setChannelVolume( 0, vol1 );    // set their volumes
  synth.setChannelVolume( 1, vol2 );

  synth.noteOn( 0, tone1, 127 );        // turn on a note for each channel
  synth.noteOn( 1, tone2, 127 );
}

void loop() 
{
  while (vol1 > i && vol1 < (127 - i))  // loops while in valid volume range
  {
    delay( 20 );
    vol1 += i * dir;                     // vol1 increments by i in direction dir
    vol2 = 127 - vol1;                  // vol2 is always the inverse
    synth.setChannelVolume( 0, vol1 );  // update volumes
    synth.setChannelVolume( 1, vol2 );
  }
  dir *= -1;                            // once volumes reach bounds, reverse dir
  vol1 += i * dir;                      // increment once in new direction
  byte vol2 = 127 - vol1;                // to ensure volumes are back in range
}
