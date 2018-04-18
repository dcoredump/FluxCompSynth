/*------------------------------------------------------------------------*//**

    FluxSynth.h
    FluxSynth Arduino Library.
    Version 1
    B.O. Love Nystrom.
    
    Drive the Fluxamasynth SAM2195 shield.
    
    Implements an interface for the Atmel/Dream SAM2195 synth chip
    present on the Modern Device Fluxamasynth shield.
    
    The actual I/O function is defined by the application, e.g:
    \code
    FluxSynth  synth;
    
    bool sendMidiByte( byte B ) { // Output routine for FluxSynth.
        Serial.write( B );
        return true;
    }
    
    void setup() {
        Serial.begin( 31250 ); // TX0 at MIDI baudrate
        synth.sendByte = sendMidiByte;
        synth.begin();
        synth.midiReset();
        synth.GS_Reset();
        delay( 500 ); // SAM2195 settling time
    }
    \endcode
    
    All documented RPN, NRPN, and SysEx parameter controls are supported.
    In all, 100 methods are provided to control the versatile SAM2195,
    covering common MIDI Voice messages, Channel control, RPN-, NRPN-,
    and SysEx Patch Parameter controls, Modulators, Modulation controllers,
    Tuning, Advanced Drum settings, Reverb and Chorus effect control,
    Parametric Equalizer, 3D Surround effect, Clipping control,
    and Master Control.

    In addition, all standard MIDI controllers have symbolic identifiers
    for easy use with the \ref FluxSynth::controlChange "controlChange"
    method, and several hundred other symbolic identifiers have been defined
    for Patch and Bank names, Reverb and Chorus program names, Modulation
    controller id's, et c.. to make your project code <i>clearly legible</i>.
    
    This code is released to the public domain under the..
    
    NO NONSENSE OPEN SOURCE LICENSE

    Copyright (c) 2012, B.O.Love Nystrom.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification,
    are permitted provided that the following conditions are met:

    (*) Redistributions of source work must retain the above copyright notice,
        this list of conditions and the following disclaimer.

    (*) Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
       
    (*) Neither the name of the copywriter(s) nor the names of any contributors
        other than yourself may be used to endorse or promote products derived 
        from this work without specific prior written permission.

    (*) Except for Your own efforts, You may not demand payment for this work 
        without written permission from the original author(s).

    Explanation, 4th clause:

    You may f.ex. charge for Your own efforts if You include this work in derivative 
    works of Your own, or You may extract a reasonable charge for the distribution 
    media and mailing cost if You send a disk containing the work to a 3rd party, 
    but You may not charge for the work itself without written permission from 
    the copywriter(s). 

    *DISCLAIMER*

    THIS WORK IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.

    IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS WORK, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef _FluxSynth_h_inc_
#define _FluxSynth_h_inc_

#include <Arduino.h>
//#include "PgmChange.h"

#define NO_MOD_CONTROLLERS // Agh, there's a firmware bug in SAM2195..
// All 30 'modulation controller' sysex parameter controls are dysfunctional.

//=============================================================================
// Command byte macros
// Macros to handle MIDI command (status) bytes.
// 
// A MIDI command is indicated by the high bit of the data byte being set.
// The command byte is a composite of command and channel number. The channel
// number is the four least significant bits of the command byte.
// Since the high bit denotes a command, it follows that all data values
// must be restricted to 7 bits.

#define HI_NIB(b)       ((b) & 0xF0)    // Get high nibble.
#define LO_NIB(b)       ((b) & 0x0F)    // Get low nibble.

#define MAX_MDATA       0x7F            // Max possible MIDI data value.

#define MIDICMD(cmd)    HI_NIB(cmd)     // Get MIDI command nibble.
#define MIDICHAN(cmd)   LO_NIB(cmd)     // Get MIDI channel nibble.

#ifndef NO_DATA_MASKING
#define MIDIDATA(dat)   ((dat) & 0x7F)  // Mask MIDI 7-bit data.
#else   // Save a little code space and execution time at the expense of safety.
#define MIDIDATA(dat)   (dat)           // Unmasked MIDI 7-bit data. Careful with that axe!
#endif

#define MIDICOMM(cmd,chan)    byte(MIDICMD(cmd) | MIDICHAN(chan)) // Make a MIDI command byte, cmd=8#-F#,chan=0-F (hex).
#define _MIDICOMM(cmd,chan)   byte((cmd) | MIDICHAN(chan)) // Make a MIDI command byte ('cmd' not masked).
#define __MIDICOMM(cmd,chan)  byte((cmd) | (chan)) // Make a MIDI command byte ('cmd' and ''chan' not masked).

// MIDI Command byte constants
// Command bytes are defined as channel 1 messages (channel = 0).

#define ME_NOTEOFF      0x80  // {8x kk vv} Note OFF voice message.
#define ME_NOTEON       0x90  // {9x kk vv} Note ON voice message.
#define ME_POLYTOUCH    0xA0  // {Ax kk vv} Polyphonic key aftertouch. (Not supported by SAM2195).
#define ME_CONTROL      0xB0  // {Bx cc vv} Controller change.
#define ME_PROGCHANGE   0xC0  // {Cx pp}    Program change.
#define ME_CHANTOUCH    0xD0  // {Dx vv}    Channel aftertouch.
#define ME_PITCHBEND    0xE0  // {Ex ll hh} Pitchbend change (14bit; ll/hh).
#define ME_SYSTEM       0xF0  // {Fx} System Common / System RealTime messages.
                              // Only SYSEX (F0h/F7h) and RESET (FFh) are supported by SAM2195.

// System Exclusive constants
// Symbolic id's for some common sysex constants.

#define ME_SYSEX        0xF0  // {F0 id dd..dd F7} System Exclusive message.
#define ME_EOX          0xF7  // End of System Exclusive {F7}.
#define ME_RESET        0xFF  // Reset all receivers to power-up status.

// Some common sysex id's

#define SXID_ROLAND     0x41  // Roland sysex id (GS is a Roland standard).
#define SXID_REALTIME   0x7F  // RealTime Universal SysEx id.
#define SXID_NON_REALTM 0x7E  // Non-RealTime Universal SysEx id.

#define SXM_GS          0x42  // GS sysex model id.

#define SX_ALLDEVS      0x7F  // All devices (Special device id for sx rt/nrt packets).
#define SX_ALLCHANS     0x7F  // All channels (Special channel nr for sx rt/nrt packets).

// Note names (sharp enharmonics)
// Symbolic note names to hit the right note (and make your code clear).

#define NOTE_C    0
#define NOTE_Cs   1
#define NOTE_D    2
#define NOTE_Ds   3
#define NOTE_E    4
#define NOTE_F    5
#define NOTE_Fs   6
#define NOTE_G    7
#define NOTE_Gs   8
#define NOTE_A    9
#define NOTE_As   10
#define NOTE_B    11

#define OCTAVE    12  // An octave has 12 semitones.
#define TOP_NOTE  (NOTE_G + OCTAVE*10) // Top note in the MIDI note range (127).

// Velocity constants
// Symbolic velocity names corresponding to music score note strengths.

#define VEL_FORTEFORTIS 127   // Forte fortizzimo (bombastic)
#define VEL_FORTISSIMO  112	  // Fortizzimo (very strong)
#define VEL_FORTE       96	  // Forte (strong)
#define VEL_MEZZOFORTE  80	  // Mezzo forte (half-strong)
#define VEL_MEZZOPIANO  64	  // Mezzo piano (half-calm)
#define VEL_PIANO       48	  // Piano (soft)
#define VEL_PIANISSIMO  32	  // Pianizzimo (very soft)
#define VEL_PIANOPIANIS 16	  // Piano pianizzimo (extremely soft)
#define VEL_MEDIUM      VEL_MEZZOFORTE // Half-strong

//=============================================================================
// Controller Id's
// Symbols for the first data byte of ME_CONTROL messages.
// 
// Note: SAM2195 support for all of these have not been verified.
// SAM2195 may also support controls not yet defined here (e.g NATIVE, GM/GS).
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~---

//Basic controller Id's
// 7-bit controllers (and 7 MSB of 14-bit controllers).

#define CT_BANKSELECT   0x00  // Bank select, for synth with > 128 patches.
#define CT_WHEEL        0x01  // Modulation wheel.
#define CT_BREATHCTRL   0x02  // Breath controller.
#define CT_FOOTCTRL     0x04  // Foot controller (volume, wahwah &c).
#define CT_PORTATIME    0x05  // Portamento time
#define CT_DATAENTRY    0x06  // Data entry (for RPN and NRPN).
#define CT_VOLUME       0x07  // Channel volume.
#define CT_PAN          0x0A  // Channel panoration (L <-> R).
#define CT_EXPRESSION   0x0B  // Expression controller.
#define CT_GEN_1        0x10  // General purpose 1 continuous controller.
#define CT_GEN_2        0x11  // General purpose 2 continuous controller.
#define CT_GEN_3        0x12  // General purpose 3 continuous controller.
#define CT_GEN_4        0x13  // General purpose 4 continuous controller.

// LSB Control Id's
// ID's for setting the least significant 7 bits of 14 bit controllers.
// 
// Add CT_LSB_DIFF to the base id to get the LSB controller Id,
// or use the predefined constants below.

#define CT_LSB_DIFF     0x20  // Add CT_LSB_DIFF to the control id to get the LSB control Id.

#define CT_BANKSEL_LSB  0x20  // Bank select, for synth with > 128 patches.
#define CT_WHEEL_LSB    0x21  // Modulation wheel.
#define CT_BREATH_LSB   0x22  // Breath controller.
#define CT_FOOT_LSB     0x24  // Foot controller (volume,wahwah &c).
#define CT_PORTATIM_LSB 0x25  // Portamento time.
#define CT_DATAENT_LSB  0x26  // 14bit Data entry (for RPN and NRPN).
#define CT_VOLUME_LSB   0x27  // Channel volume.
#define CT_PAN_LSB      0x2A  // Channel panoration (L <-> R).
#define CT_EXPRESS_LSB  0x2B  // Expression controller.
#define CT_GEN1_LSB     0x30  // General purpose 1 continuous controller LSB.
#define CT_GEN2_LSB     0x31  // General purpose 2 continuous controller LSB.
#define CT_GEN3_LSB     0x32  // General purpose 3 continuous controller LSB.
#define CT_GEN4_LSB     0x33  // General purpose 4 continuous controller LSB.

// Switch Controllers
// Switch things on or off (vv = 7F:On or 00:Off).

#define CT_DAMPER       0x40  // 64 - Damper pedal (Sustain) On/Off.
#define CT_PORTAMENTO   0x41  // 65 - Portamento On/Off.
#define CT_SOSTENUTO    0x42  // 66 - Sostenuto pedal On/Off.
#define CT_SOFT         0x43  // 67 - Soft pedal On/Off.
#define CT_REVERBPROG   0x50  // 80 - Set reverb program, 0-7. (SAM, Global i.e CH0)
#define CT_CHORUSPROG   0x51  // 81 - Set chorus program, 0-7. (SAM, Global i.e CH0)
#define CT_GEN_SW3      0x52  // 82 - General purpose switch 3.
#define CT_GEN_SW4      0x53  // 83 - General purpose switch 4.

// Effect Send Controllers
// Controls effect send levels (vv = 0-127).

#define CT_REVERB       0x5B  // 91 - Reverb depth.
#define CT_TREMOLO      0x5C  // 92 - Tremolo depth.
#define CT_CHORUS       0x5D  // 93 - Chorus depth.
#define CT_DETUNE       0x5E  // 94 - Detune amount.
#define CT_PHASER       0x5F  // 95 - Phaser depth.

// RPN- and NRPN Controls
// Registered- and NonRegistered Parameter Number data entry controllers.

#define CT_DATAINC      0x60  // 96 - Value +1.
#define CT_DATADEC      0x61  // 97 - Value -1.
#define CT_NONREG_LSB   0x62  // 98 - NRPN (non-registered parameter number) LSB.
#define CT_NONREG_MSB   0x63  // 99 - NRPN MSB (high part of number).
#define CT_REG_LSB      0x64  // 100 - RPN (registered parameter number) LSB.
#define CT_REG_MSB      0x65  // 101 - RPN MSB (most significant byte).

// Channel Mode Messages
// Special controllers affecting the synth operation mode.

#define CT_ALLSOUNDOFF  0x78  // 120 - Silence all outputs.
#define CT_RESETCTRL    0x79  // 121 - Reset all controllers to defaults.
#define CT_LOCALCTRL    0x7A  // 122 - Local control On/Off (vv=$7F/$00).
#define CT_ALLNOTESOFF  0x7B  // 123 - Turn off all oscillators.
#define CT_OMNI_OFF     0x7C  // 124 - Omni mode Off (vv=0).
#define CT_OMNI_ON      0x7D  // 125 - Omni mode On (vv=0).
#define CT_MONO_ON      0x7E  // 126 - Mono mode On (Poly Off) (vv=Nr chans(Omni off) or 0(Omni on)).
#define CT_POLY_ON      0x7F  // 127 - Poly mode On (Mono Off) (vv=0).

//=============================================================================
// Control Data Values
// Constants and macros for control data.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Common values
// Symbolic constants for common control values.

#define CTV_SWITCHON     0x7F    // ON (3rd byte of midi switch controller)
#define CTV_SWITCHOFF    0x00    // OFF (3rd byte of midi switch controller)

#define CTV_CENTER       0x40    // Neutral (center) value for 7bit bipolar ctrl values
#define CTV_CENTER14     0x2000  // Neutral (center) value for 14bit bipolar ctrl values
#define CTV_MAX14        0x3FFF  // Maximum 14 bit control value
#define CTV_MASK14       0x3FFF  // Mask 14 bits of word value

#define CTV_NOPITCH14    CTV_CENTER14 // Center pitchbend value (14 bits)
#define CTV_NOPITCH      CTV_CENTER   // High 7 bits of center pitchbend value

// Data Value Macros
// Macros to handle 14 bit 'big' controller values, e.g pitch bend.

// Make a 14-bit control value from low/high bytes.
#define CTV_VAL14(lo,hi)    word(((hi & 0x7F) << 7)| (lo & 0x7F))

// Make a 14-bit control value, *no masking*.
#define _CTV_VAL14(lo,hi)   word((hi << 7)| lo )

// Extract low 7 bits of a 14-bit control value.
#define CTV_LOW(val)        byte(val & 0x7F)

// Extract high 7 bits of a 14-bit control value.
#define CTV_HIGH(val)       byte((val >> 7) & 0x7F)

// Extract high 7 bits of a 14-bit control value, *no masking*.
#define _CTV_HIGH(val)      byte(val >> 7)

//=============================================================================
// Control Value Conversion Functions
// Convert human comprehensible values to MIDI control values.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Bipolar Value Conversion
// Convert bipolar values to biased MIDI controls.
// 
// You can use these functions to convert signed bipolar values
// (+-63/+-8191) to standard biased MIDI 7bit/14bit control values.
// To avoid messing up MIDI control value compatibility by using signed
// control values, class methods expect standard (biased) control values
// for "bipolar" controllers, e.g. pitch bend, tuning, or transpose.

byte _ccByte( int8_t Value ); // Agh, Arduino 0.23 requires these functions to
word _ccWord( short Value );  // be globally unique over ALL libraries! Go figure..
    
// Equalizer Frequency Control Values.
// Convert corner frequency in Hz to corresponding SAM2195 control value.

byte _EqBassCtrl( word Hz );
byte _EqMidCtrl( word Hz );
byte _EqTrebleCtrl( word Hz );
    
// Equalizer Control back-convert
// Convert EQ control value back to Hz.
// (Can also tell you how/if your set frequency was quantized.)

word _EqBassHz( byte ctrlVal );
word _EqMidHz( byte ctrlVal );
word _EqTrebleHz( byte ctrlVal );
    
//+----------------------------------------------------------------------------
// FluxSynth -- Atmel/Dream SAM2195 Fluxamasynth interface class.
//+----------------------------------------------------------------------------

bool nullSend( byte B ); // Dummy output fuction does nothing

class FluxSynth {
protected:
    byte    _runStat;      // MIDI running status
    byte    _effects;      // Effect enable flags

    void _sendPartParameter( byte Part, byte ParmNr, byte CtrlVal );
    void _sendModParameter( byte Channel, byte ParmNr, byte CtrlVal );
    void _sendDreamControl( byte FuncNr, byte Value );

public:

    FluxSynth(void);

    // FluxSynth Common class methods
    // General subroutines used by other methods.

    bool (*sendByte)( byte B ); // Actual ouput function is up to you

    void begin();
    void writePort( byte B );
    void writePort( byte *Buf, word Count );
    void writeMidiCmd( byte Cmd );  
    void sendParameterData( byte *Data, word Length ); 

    // SAM2195 Channel control
    // These methods control a single MIDI channel.

    void noteOn( byte Channel, byte Key, byte Velocity );
    void noteOff( byte Channel, byte Key );
    void controlChange( byte Channel, byte CtrlNr, byte Value );
    void setControlValue( byte Channel, byte CtrlNr, word Value );
    void programChange( byte Channel, byte Patch );
    void programChange( byte Channel, byte Bank, byte Patch );
    void setPatchBank( byte Channel, byte Bank );
    void setPatchBank( byte Bank );
    void polyAftertouch( byte Channel, byte Key, byte Value );
    void channelAftertouch( byte Channel, byte Value );
    void pitchBend( byte Channel, word Bend );
    void setBendRange( byte Channel, byte Range );
    void RPN_Control( byte Channel, byte rpnHi, byte rpnLo, byte Data );
    void NRPN_Control( byte Channel, byte nrpnHi, byte nrpnLo, byte Data );
    void dataEntry( byte Channel, byte Data );

    void setChannelVolume( byte Channel, byte Level );
    void allNotesOff( byte Channel );

    void setPartChannel( byte Part, byte Channel );
    void setPartMode( byte Part, boolean Drums );
    void setVoiceReserve( byte *CountTable );
    void assignCC1Controller( byte Channel, byte CtrlNr );
    void assignCC2Controller( byte Channel, byte CtrlNr );

    // Reverb & Chorus Send 
    // Note: Reverb/Chorus effect parameters are global, further down.

    void setReverbSend( byte Channel, byte Level );
    void setChorusSend( byte Channel, byte Level );

    /*
    SAM2195 Tuning functions
    Synth master tuning and channel tune/detune.
    
    In MIDI, an entire device is tuned by either sending RPN#1
    (Channel Fine Tuning) to all MIDI channels being used, or by sending
    a System Exclusive Master Tune.

    RPN#1 allows tuning to be specified in steps of approximately 0.012 cents
    (to be precise, 100/8192 cent), and System Exclusive Master Tune allows
    tuning in steps of 0.1 cent. One cent is 1/100th of a semitone.
    The values of RPN#1 (Channel Fine Tuning) and System Exclusive Master Tune
    are added together to determine the actual pitch sounded by each Part.

    Frequently used tuning values for your reference:

    +~~~~~~~~~+~~~~~~~~+~~~~~~~~~~~~~~~~~~~~~~~+~~~~~~~~~~~~~~~~~~~~+
    | Hz @ A4 | cents  |         RPN#1         |  Sys.Ex. 40 00 00  |
    +~~~~~~~~~+~~~~~~~~+~~~~~~~~~~~~~~~~~~~~~~~+~~~~~~~~~~~~~~~~~~~~+
    |  445.0  | +19.56 | (+1603) 2643 4C:43 4C | (+196) 00 04 0C 04 |
    |  444.0  | +15.67 | (+1283) 2503 4A:03 4A | (+157) 00 04 09 0D |
    |  443.0  | +11.76 | (+ 964) 23C4 47:44 47 | (+118) 00 04 07 06 |
    |  442.0  | + 7.85 | (+ 643) 2283 45:03 45 | (+ 79) 00 04 04 0F |
    |  441.0  | + 3.93 | (+ 322) 2142 42:42 42 | (+ 39) 00 04 02 07 |
    |  440.0  |   0    | (  0  ) 2000 40:00 40 | (   0) 00 04 00 00 |
    |  439.0  | - 3.94 | (- 323) 1EBD 3D:3D 3D | (- 39) 00 03 0D 09 |
    |  438.0  | - 7.89 | (- 646) 1D7A 3A:7A 3A | (- 79) 00 03 0B 01 |
    +~~~~~~~~~+~~~~~~~~+~~~~~~~~~~~~~~~~~~~~~~~+~~~~~~~~~~~~~~~~~~~~+
    
    Note: SAM2195 doesn't support 14 bit RPN controls, so the tuning step of
    RPN#1 is just 100/64 (1.56) cent. SysEx master tune is better for tuning.
    That is, use setEzMasterTuning to tune the synth to other instruments.
    However, RPN#1 is also useful for various effects, e.g layering a
    detuned duplicate voice for a fatter sound, or whatever..
    */

    void setTranspose( byte Channel, byte SemiCtrl );
    void setTuning( byte Channel, byte CentsCtrl );
    void setScaleTuning( byte Channel, byte* TuningTable );

    // Master tuning (global, but declared here for the sake of doxygen grouping)
    
    void setMasterTranspose( byte Semitones );
    void setMasterTuning( word CtrlValue );
    void setEzMasterTuning( short DeciCent );

    // SAM2195 Advanced Drum Settings
    // Allow precise control of the drum synth mix and effects.

    void setDrumPitch( byte Channel, byte DrumNr, byte Semitone );
    void setDrumMix( 
        byte Channel, byte DrumNr, byte Level, byte Pan, byte Reverb, byte Chorus
        );

    // SAM2195 Modulators
    // These methods govern how the modulators affect the timbre of the currently
    // selected patch on one of the 16 MIDI channels.
    
    void setVelocitySlope( byte Channel, byte Slope );
    void setVelocityOffset( byte Channel, byte Offset );
    void setVibrato( byte Channel, byte Rate, byte Depth, byte Delay );
    void setVibratoRate( byte Channel, byte Rate );
    void setVibratoDepth( byte Channel, byte Depth );
    void setVibratoDelay( byte Channel, byte Delay );
    void setTvFilter( byte Channel, byte CutoffFreq, byte Resonance );
    void setTvfCutoff( byte Channel, byte CutoffFreq );
    void setTvfResonance( byte Channel, byte Resonance );
    void setEnvelope( byte Channel, byte Attack, byte Decay, byte Release );
    void setEnvAttack( byte Channel, byte Attack );
    void setEnvDecay( byte Channel, byte Decay );
    void setEnvRelease( byte Channel, byte Release );
    void setLfoRate( byte CtrlVal );

#ifndef NO_MOD_CONTROLLERS
    // SAM2195 Modulation Controllers
    // Setup five different modulation controllers.
    // These methods govern how the modulation controllers affect the modulators
    // in the currently selected patch on a channel.
    
    #define MOD_WHEEL   0x00  // Modulation wheel controller.
    #define MOD_BENDER  0x10  // Pitchbend controller.
    #define MOD_CAF     0x20  // [C]hannel [AF]tertouch control.
    #define MOD_CC1     0x40  // Programmable custom control 1. See assignCC1Controller
    #define MOD_CC2     0x50  // Programmable custom control 2. See assignCC2Controller
    
    void setModPitchDepth( byte Controller, byte Channel, byte SemiCtrl );
    void setModTvfCutoff( byte Controller, byte Channel, byte CtrlVal );
    void setModAmplitude( byte Controller, byte Channel, byte CtrlVal );
    void setModLfoPitchDepth( byte Controller, byte Channel, byte CtrlVal );
    void setModLfoTvfDepth( byte Controller, byte Channel, byte CtrlVal );
    void setModLfoTvaDepth( byte Controller, byte Channel, byte CtrlVal );

#endif //ndef NO_MOD_CONTROLLERS

    // SAM2195 Master Control
    // These methods control the whole synthesizer.

    void midiReset();
    void GM_Reset();
    void GS_Reset();
    void setMasterVolume( byte Level );
    void GS_MasterVolume( byte Level );
    void GS_MasterPan( byte Pan );

    // SAM2195 Reverb Effect Control
    // Reverb parameters (global effect unit, not per channel)

    void setReverbLevel( byte MasterLevel );
    void setReverb(
        byte Program, byte Time, byte Feedback, byte Character = 4 
        );

    #define REV_ROOM1       0  // Room 1 reverb program.
    #define REV_ROOM2       1  // Room 2 reverb program.
    #define REV_ROOM3       2  // Room 3 reverb program.
    #define REV_HALL1       3  // Hall 1 reverb program.
    #define REV_HALL2       4  // Hall 2 reverb program.
    #define REV_PLATE       5  // Plate reverb program.
    #define REV_DELAY       6  // Delay reverb program.
    #define REV_PANDELAY    7  // Panning delay reverb program.

    #define REV_DEFLEVEL  0x64 // Default reverb master level
    #define REV_DEFCHAR   0x04 // Default reverb character

    void setReverbProgram( byte Program );
    void setReverbTime( byte Time );
    void setReverbFeedback( byte Feedback );
    void setReverbCharacter( byte Character );

    // SAM2195 Chorus Effect Control
    // Chorus parameters (global effect unit, not per channel)

    void setChorusLevel( byte MasterLevel );
    void setChorus( 
        byte Program, byte Delay, byte Feedback, byte Rate, byte Depth
        );

    #define CHO_CHORUS1     0  // Chorus 1 program.
    #define CHO_CHORUS2     1  // Chorus 2 program.
    #define CHO_CHORUS3     2  // Chorus 3 program.
    #define CHO_CHORUS4     3  // Chorus 4 program.
    #define CHO_FEEDBACK    4  // Feedback chorus program.
    #define CHO_FLANGER     5  // Flanger program.
    #define CHO_SHORTDELAY  6  // Short delay chorus program.
    #define CHO_FBDELAY     7  // Feedback delay chorus program.

    void setChorusProgram( byte Program );
    void setChorusDelay( byte Delay );
    void setChorusFeedback( byte Feedback );
    void setChorusRate( byte Rate );
    void setChorusDepth( byte Depth );

    // Special DREAM Functions
    // Special SAM2195 NRPN 37**h controls.
    // These are 37**h controls that didn't fit in some other category.

    void enableEffects( byte Flags );

    #define EF_EQ_2BAND  0x02  // 2 band eq, polyphony -4 voice
    #define EF_EQ_4BAND  0x03  // 4 band eq, polyphony -8 voice
    #define EF_SURROUND  0x08  // Surround on, polyphony -2 voice
    #define EF_REVERB    0x20  // Reverb on, polyphony -13 voice
    #define EF_ALL       EF_REVERB| EF_SURROUND| EF_EQ_4BAND // Combine all EF flags.

    #define EF_RESET     0x45  // Reverb ON, Chorus ON, Surround ON, 4-Band Equalizer.

    void restartEffects();
    void enableReverb( boolean On );
    void enableSurround( boolean On );
    void setEqualizerMode( byte revMode );

    // Soft/hard clipping and sound Output level

    void setClippingMode( byte Mode );

    #define SOFT_CLIP   0x00  // Soft clipping mode
    #define HARD_CLIP   0x7F  // Hard clipping mode

    void setOutputLevel( byte Level ); 

    // SAM2195 Equalizer Control
    // Change settings for the parametric equalizer.
    //
    // The low-band filter frequency is 0-4700 Hz, the two mid-band
    // filters are 0-4200 Hz, and the high-band filter 0-18750 Hz.
    // (The actual lower frequency limit of each band is yet undetermined,
    // so it's uncertain what frequency control value 0 correspond to).

    #define EQ_BASS     0   // EQ Band 0: 0-4.7 kHz, +-12 dB [default 444Hz(0Ch) +6dB(60h)]
    #define EQ_LOWMID   1   // EQ Band 1: 0-4.2 kHz, +-12 dB [default 893Hz(1Bh) 0dB(40h)]
    #define EQ_HIGHMID  2   // EQ Band 2: 0-4.2 kHz, +-12 dB [default 3770kHz(72h) 0dB(40h)]
    #define EQ_TREBLE   3   // EQ Band 3: 0-18.75 kHz, +-12 dB [default 9449Hz(40h) +6dB(60h)]

    void setEqualizer( byte BandNr, byte Freq, byte Gain );
    void setEqFrequency( byte BandNr, byte Frequency );
    void setEqGain( byte BandNr, byte Gain );

    #define EQL_P_12dB  0x7F  // +12 dB EQ gain
    #define EQL_P_9dB   0x70  //  +9 dB EQ gain
    #define EQL_P_6dB   0x60  //  +6 dB EQ gain
    #define EQL_P_3dB   0x50  //  +3 dB EQ gain
    #define EQL_0dB     0x40  //   0 dB EQ gain
    #define EQL_M_3dB   0x30  //  -3 dB EQ gain
    #define EQL_M_6dB   0x20  //  -6 dB EQ gain
    #define EQL_M_9dB   0x10  //  -9 dB EQ gain
    #define EQL_M_12dB  0x00  // -12 dB EQ gain

    // SAM2195 3D Surround Control
    // Set volume, time, and input mode of the 3D surround unit.

    void setSurroundVolume( byte Level );
    void setSurroundDelay( byte Time );
    void surroundMonoIn( boolean Mono );

    // SAM2195 Post Processing Control
    // Signal routing (post processing on/off).
    // Post processing effects are the equalizer and 3D surround unit.

    void postprocGeneralMidi( boolean On );
    void postprocReverbChorus( boolean On );

    // SAM2195 Special GM Controls
    // General Midi controls (SAM2195 NRPN 0x37## controls)
    // 
    // Note: At the time of this writing, it is undetermined if the basic
    // GM controls in SAM2195 support individual channels or are global.
    // Both implementations are provided so you may find out.
    // Please report your findings to the lib distribution page.

    // Channel 0 (Midi cmd byte == 0xB0)
    
    void GM_ReverbSend( byte Level );
    void GM_ChorusSend( byte Level );
    void GM_Volume( byte Level );
    void GM_Pan( byte Pan );

    // Channel specific (Midi cmd byte == 0xB#)
    
    void GM_ReverbSend( byte Channel, byte Level );
    void GM_ChorusSend( byte Channel, byte Level );
    void GM_Volume( byte Channel, byte Level );
    void GM_Pan( byte Channel, byte Pan );

    // SAM2195 Miscellaneous
    // Rarely used methods

    void setSysExModuleId( byte Id );
    void runSelfTest();
};

#endif //ndef _FluxSynth_h_inc_
