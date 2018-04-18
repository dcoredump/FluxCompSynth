/* FluxamasynthDrumPatternPlayer.pde
 * A drum machine for the Modern Device Fluxamasynth Shield
 * ModernDevice.com
 *
 * By Michel Gutlich 26-2-2011 
 * A sensor signal on analog input 0 gives some tempo dynamics
 * Contact at iching@xs4al.nl
 *
 * Updated May 2012, Love Nystrom
 * Sensor commented out in favor of fixed tempo, &c.
 */

#include <FluxSynth.h>
#include <PgmChange.h>

FluxSynth synth; // create a synth object

// Our drum pattern
// Every array cell is the velocity of the note played 
// Tick         1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
byte bd  [] = {127,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 90,  0}; //Bassdrum
byte sn  [] = {  0,  0,  0,  0,127,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}; //Snare
byte hho [] = {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,127,  0}; //Hihat Open
byte hhc [] = {127, 40, 80, 40,127, 40, 80, 40,127, 40, 80, 40,127,  0,  0}; //Hihat Close
byte hhp [] = {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,127}; //Hihat Pedal

byte patternSize = 16;  // A pattern of max 16 ticks 
    //(actualy 15, because we loop and 16 is tick no.1.. you dig it ?)
byte tickNo;            // Our tick number variable

// Some basic settings

byte channel = DRUM_CHAN;    // MIDI channel number
byte tempo   = 120;          // Initial tempo

// setup

bool sendMidiByte( byte B ) { // Output routine for FluxSynth.
  Serial.write( B );
  return true;
  }
    
void setup()
{
  Serial.begin( 31250 ); // MIDI baudrate
  synth.sendByte = sendMidiByte;
  synth.begin();
  synth.midiReset();        // Do a complete MIDI reset

  // setReverb( Program, Time, Feedback, Character )
  synth.setReverb( REV_PLATE, 127, 100, 4 ); // A Plate Reverb with max time
  synth.setReverbLevel( 127 ); // Maximum effect level
  synth.setMasterVolume( 127 );	// Max. master volume

  synth.setReverbSend( channel, 120 );
  synth.setChannelVolume( channel, 127 ); // max. channel volume
}

// subs

void bassDrm( byte vel )
{
  synth.noteOn( channel, DR_Kick, vel );    // play a note
  synth.noteOff( channel, DR_Kick );        // send note off
}

void snareDrm( byte vel )
{
  synth.noteOn( channel, DR_Snare, vel );   // play a note
  synth.noteOff( channel, DR_Snare );       // send note off
}

void hihatClose( byte vel )
{
  synth.noteOn( channel, DR_ClosedHiHat, vel );  // play a note
  synth.noteOff( channel, DR_ClosedHiHat );      // send note off 
}

void hihatPedal( byte vel )
{
  synth.noteOn( channel, DR_PedalHiHat, vel );  // play a note
  synth.noteOff( channel, DR_PedalHiHat );      // send note off   
}

void hihatOpen( byte vel )
{
  synth.noteOn( channel, DR_OpenHiHat, vel );  // play a note
  synth.noteOff( channel, DR_OpenHiHat );      // send note off 
}

void tick()
{
  delay( tempo );
}

// loop

void loop() 
{
  for( byte tickNo=0; tickNo < patternSize; tickNo++ )
  {
    //tempo = analogRead( A0 ); // For live tempo dynamics :-)
    bassDrm( bd[ tickNo ]);
    snareDrm( sn[ tickNo ]);
    hihatClose( hhc[ tickNo ]);
    hihatOpen( hho[ tickNo ]);
    hihatPedal( hhp[ tickNo ]);
    tick();    
  }
} 

