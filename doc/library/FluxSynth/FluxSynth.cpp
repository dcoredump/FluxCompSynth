/*===========================================================================*\

    FluxSynth - Arduino library for the M.D. Fluxamasynth shield.

    Implements an interface for the Atmel Dream 2195 synth chip.
    Based on the FlexamySynth library, but liberated from dependencies on
    any particular I/O library, by delegating the actual byte output to
    an assignable 'sendByte' function of your own.
    
    All documented RPN, NRPN, and SysEx parameter controls are supported.
    Standard controls are supported through a generic controlChange method,
    and control id macros, instead of a slur of member methods.

    This code is released to the public domain as-is, with no warranties.
    You may use it and change it as you wish, but keep my name on it.
    
    Love Nystrom, December 2012.
    
  CHANGE LOG:
    
    Dec 2012  LN  First version.
    Jan 2013  LN  I/O independent. User supplies IO actual function.

\*===========================================================================*/

#include "FluxSynth.h"

// Return biased control byte

byte _ccByte( int8_t Value )
{
    return MIDIDATA( CTV_CENTER + Value );
}

// Return biased control word

word _ccWord( int16_t Value )
{
    #ifndef NO_DATA_MASKING
    return ((Value + CTV_CENTER14) & CTV_MASK14);
    #else
    return CTV_CENTER14 + Value;
    #endif
}

//-----------------------------------------------------------------------------
//  FluxSynth
//-----------------------------------------------------------------------------

bool nullSend( byte B ) // Dummy output fuction does nothing
{ 
    return false; 
}

FluxSynth::FluxSynth(void)
{
    sendByte = nullSend;
    _runStat = 0;         // Midi running status
    _effects = EF_ALL;    // EF_REVERB, EF_SURROUND, EF_EQ_4BAND (+Chorus)
}

void FluxSynth::begin() 
{
    // nothing
}

void FluxSynth::writePort( byte b )
{
    sendByte( b );
}

void FluxSynth::writePort( byte *buf, word cnt )
{
    for( word i=0; i < cnt; i++ ) writePort( buf[ i ]);
}

void FluxSynth::writeMidiCmd( byte Cmd )
{
    if (Cmd != _runStat)    // MIDI running status changed
    {
        _runStat = Cmd;
        writePort( Cmd );   // Write new command byte
    }                       // else we're done!
}

//-----------------------------------------------------------------------------
// ME_SYSEX 0xF0 // {F0 id dd..dd F7} System Exclusive Message
//-----------------------------------------------------------------------------

void FluxSynth::sendParameterData( byte *data, word length ) 
{
    // Send a Parameter Control sysex message.
    // All but two of the sysex blocks that Atmel Dream 2195 respond to
    // use a common Roland GS compatible header, so it's hard coded here.
    // All invokations must skip the header, and the tailing sysex end (0xF7)

    byte head[6] =   // (Field comments assume GS packet compatibility)
    {
        ME_SYSEX,    // F0h
        SXID_ROLAND, // Roland id (41h)
        0x00,        // Device nr
        SXM_GS,      // Model id (GS)
        0x12,        // Command id (DT1 - Data One Way) 
        0x40         // Parameter address MSB
        // 'data' array begin with two address bytes
        // followed by parameter data
    };
    byte tail[2] = 
    { 
        0x00,       // Checksum (ignored by 2195; transmit for compatibilty)
        ME_EOX      // End of exclusive
    };
    _runStat = ME_SYSEX;
    
    #ifdef USE_SYSEX_CHKSUM // Calculate the Roland checksum
    byte i, chkSum = 0x40; // Parameter address MSB is first chksum data
    for( i=0; i < length; ++i ) chkSum += Data[ i ];
    chkSum &= 0x7F; // Remainder of 'chkSum % 128'
    chkSum ^= 0x7F; // Invert the lower 7 bits
    tail[0] = chkSum;
    #endif
    
    writePort( head, 6 );
    writePort( data, length );
    writePort( tail, 2 );
}

/*
void FluxSynth::sendParameterDataEx( byte moduleNr, byte *Data, word Length )
{
    byte head[6] =   // (Field comments assume GS packet compatibility)
    {
        ME_SYSEX,    // F0h
        SXID_ROLAND, // Roland id (41h)
        moduleNr,    // Device nr
        SXM_GS,      // Model id (GS)
        0x12,        // Command id (DT1 - Data One Way) 
        0x40         // Parameter address MSB
        // 'data' array begin with two address bytes
        // followed by parameter data
    };
    // Calculate the Roland checksum
    byte i, chkSum = 0x40; // Parameter address MSB is first chksum data
    for( i=0; i < Length; ++i ) chkSum += Data[ i ];
    chkSum &= 0x7F; // Remainder of 'chkSum % 128'
    chkSum ^= 0x7F; // Invert the lower 7 bits
    byte tail[2] = 
    { 
        chkSum,     // Checksum (ignored by 2195; transmit for compatibilty)
        ME_EOX      // End of exclusive
    };
    _runStat = ME_SYSEX;
    writePort( head, 6 );
    writePort( Data, Length );
    writePort( tail, 2 );
}

void FluxSynth::sendPartParameterEx( 
    byte moduleNr, byte groupNr, byte Part, byte ParmNr, byte CtrlVal ) 
{
    byte sxdata[3] = { groupNr | MIDICHAN( Part ), ParmNr, CtrlVal };
    sendParameterDataEx( moduleNr, sxdata,3 );
}
*/

//# Send patch part parameter

void FluxSynth::_sendPartParameter( byte Part, byte ParmNr, byte CtrlVal ) 
{
    byte sxdata[3] = { 0x10 | MIDICHAN( Part ), ParmNr, CtrlVal };
    sendParameterData( sxdata,3 );
}

//# Send mod control parameter (patch part param 0x2p group)
    
void FluxSynth::_sendModParameter( byte Channel, byte ParmNr, byte CtrlVal ) 
{
    byte sxdata[3] = { 0x20 | MIDICHAN( Channel ), ParmNr, CtrlVal };
    sendParameterData( sxdata,3 );
}

void FluxSynth::_sendDreamControl( byte FuncNr, byte Value ) 
{
    NRPN_Control( 0x00 ,0x37, FuncNr, Value );
}

//-----------------------------------------------------------------------------
// ME_NOTEON 0x90 // {9x kk vv}
//-----------------------------------------------------------------------------

void FluxSynth::noteOn( byte Channel, byte Key, byte Velocity ) 
{
    byte data[2] = { MIDIDATA( Key ), MIDIDATA( Velocity )};
    writeMidiCmd(_MIDICOMM( ME_NOTEON, Channel ));
    writePort( data, 2 );
}

//-----------------------------------------------------------------------------
// ME_NOTEOFF 0x80 // {8x kk vv}
//-----------------------------------------------------------------------------

void FluxSynth::noteOff( byte Channel, byte Key ) 
{
    byte data[2] = { MIDIDATA( Key ), VEL_MEZZOPIANO };
    writeMidiCmd(_MIDICOMM( ME_NOTEOFF, Channel ));
    writePort( data, 2 );
}

//-----------------------------------------------------------------------------
// ME_KEYTOUCH 0xA0 // {Ax kk vv} Key aftertouch
// Separate aftertouch valus for each key. ( My old DX7 had this :)
// Not supported by 2195, but included for the sake of completeness.
//-----------------------------------------------------------------------------

void FluxSynth::polyAftertouch( byte Channel, byte Key, byte Value ) 
{
    byte data[2] = { MIDIDATA( Key ), MIDIDATA( Value )};
    writeMidiCmd(_MIDICOMM( ME_POLYTOUCH, Channel ));
    writePort( data, 2 );
}

//-----------------------------------------------------------------------------
// ME_CONTROL 0xB0 // {Bx cc vv}
// Standard control change. Use the CT_nnn constants to identify which.
//-----------------------------------------------------------------------------

void FluxSynth::controlChange( byte Channel, byte CtrlNr, byte Value ) 
{
    byte data[2] = { MIDIDATA( CtrlNr ), MIDIDATA( Value ) };
    writeMidiCmd(_MIDICOMM( ME_CONTROL, Channel ));
    writePort( data, 2 );
}

// Set 14bit continuous controller, e.g ModWheel, Expression, Volume, Pan
// (Not sure if 14bit continuous controllers are supported by 2195)
// Gave this it's own name instead of overloading controlChange.

void FluxSynth::setControlValue( byte Channel, byte CtrlNr, word Value ) 
{
    byte mididata[4] =    // [Bx] ch hh cl ll
    {
        CtrlNr, CTV_HIGH( Value ), 
        CtrlNr + CT_LSB_DIFF, CTV_LOW( Value )
    };
    writeMidiCmd(_MIDICOMM( ME_CONTROL, Channel )); 
    writePort( mididata, 4 );
}

//-----------------------------------------------------------------------------
// ME_PROGCHANGE 0xC0 // {Cx pp}
// Use the patch id's from PgmChange.h, or roll your own.
//-----------------------------------------------------------------------------

void FluxSynth::programChange( byte Channel, byte Patch )
{
    writeMidiCmd(_MIDICOMM( ME_PROGCHANGE, Channel ));
    writePort( MIDIDATA( Patch ));
}

void FluxSynth::programChange( byte Channel, byte Bank, byte Patch )
{
    setPatchBank( Channel, Bank );
    programChange( Channel, Patch );
}

// 2195 supports patch banks 0 (GM), and 127 (MT-32).

void FluxSynth::setPatchBank( byte Channel, byte Bank )
{
    controlChange( Channel, CT_BANKSELECT, Bank );
}

void FluxSynth::setPatchBank( byte Bank )
{
    for( byte ch=0; ch < 16; ++ch )
        if (ch != 9) controlChange( ch, CT_BANKSELECT, Bank );
}

//-----------------------------------------------------------------------------
// ME_CHANTOUCH 0xD0 // {Dx vv} Channel aftertouch
// One common aftertouch value for all keys.
//-----------------------------------------------------------------------------

void FluxSynth::channelAftertouch( byte Channel, byte Value )
{
    writeMidiCmd(_MIDICOMM( ME_CHANTOUCH, Channel ));
    writePort( MIDIDATA(Value) );
}

//-----------------------------------------------------------------------------
// ME_PITCHBEND 0xE0 // {Ex ll hh} 14bit; ll/hh // 'bend' is a value from 0 to 0x3FFF
//-----------------------------------------------------------------------------

void FluxSynth::pitchBend( byte Channel, word Bend ) 
{
    byte data[2] = { CTV_LOW( Bend ), CTV_HIGH( Bend ) };
    writeMidiCmd(_MIDICOMM( ME_PITCHBEND, Channel ));
    writePort( data, 2 );
}

void FluxSynth::setBendRange( byte Channel, byte Semitones ) 
{
    RPN_Control( Channel, 0,0, Semitones ); // BnH 65H 00H 64H 00H 06H vv
}

//-----------------------------------------------------------------------------
// Registered / Non-Registered Parameter Numbers
//-----------------------------------------------------------------------------

void FluxSynth::RPN_Control( byte Channel, byte rpnHi, byte rpnLo, byte Value ) 
{
    byte data[6] = // [Bx] 65 hi 64 lo 06 vh
    {
        CT_REG_MSB, MIDIDATA( rpnHi ), CT_REG_LSB, MIDIDATA( rpnLo ),
        CT_DATAENTRY, MIDIDATA( Value )
    };
    writeMidiCmd(_MIDICOMM( ME_CONTROL, Channel ));
    writePort( data, 6 );
}

void FluxSynth::NRPN_Control( byte Channel, byte nrpnHi, byte nrpnLo, byte Value ) 
{
    byte data[6] = // [Bx] 63 hi 62 lo 06 val
    {
        CT_NONREG_MSB, MIDIDATA( nrpnHi ), CT_NONREG_LSB, MIDIDATA( nrpnLo ),
        CT_DATAENTRY, MIDIDATA( Value )
    };
    writeMidiCmd(_MIDICOMM( ME_CONTROL, Channel ));
    writePort( data, 6 );
}

void FluxSynth::dataEntry( byte Channel, byte Data ) // Provide data to RPN and NRPN
{
    controlChange( Channel, CT_DATAENTRY, Data );
}

#ifdef HAVE_14B_CONTROLLER // 14bit RPN / NRPN seems supported

void FluxSynth::RPN_ControlW( byte Channel, byte rpnHi, byte rpnLo, word Value ) 
{
    byte data[8] = // [Bx] 65 hi 64 lo 06 vh 26 vl
    {
        CT_REG_MSB, rpnHi, CT_REG_LSB, rpnLo, 
        CT_DATAENTRY, CTV_HIGH( Value ), CT_DATAENT_LSB, CTV_LOW( Value )
    };
    writeMidiCmd(_MIDICOMM( ME_CONTROL, Channel ));
    writePort( data, 8 );
}

void FluxSynth::NRPN_ControlW( byte Channel, byte nrpnHi, byte nrpnLo, word Value ) 
{
    byte data[8] = // [Bx] 63 hi 62 lo 06 vh 26 vl
    {
        CT_NONREG_MSB, nrpnHi, CT_NONREG_LSB, nrpnLo, 
        CT_DATAENTRY, CTV_HIGH( Value ), CT_DATAENT_LSB, CTV_LOW( Value )
    };
    writeMidiCmd(_MIDICOMM( ME_CONTROL, Channel ));
    writePort( data, 8 );
}

void FluxSynth::dataEntryW( byte Channel, word Data ) // Provide data to RPN and NRPN
{
    setControlValue( Channel, CT_DATAENTRY, Data );
}
#endif

//-----------------------------------------------------------------------------
// Channel control
//-----------------------------------------------------------------------------

void FluxSynth::setChannelVolume( byte Channel, byte Level ) 
{
    controlChange( Channel, CT_VOLUME, Level );
}

// Turn off all oscillators

void FluxSynth::allNotesOff( byte Channel ) 
{
    controlChange( Channel, CT_ALLNOTESOFF, CTV_SWITCHON );
}

// Part channel assign
// Allows to assign several parts to a single MIDI channel or mute a part.
    
void FluxSynth::setPartChannel( byte Part, byte Channel ) 
{
    _sendPartParameter( Part, 2, Channel );
}

// Part mode -- Drums or Sound
// Allows to setup several drum channels.
    
void FluxSynth::setPartMode( byte Part, boolean Drums ) 
{
    _sendPartParameter( Part, 0x15, Drums ? 1 : 0 );
}

// Set reserved voices. Table must contain 16 bytes with voice counts.

void FluxSynth::setVoiceReserve( byte *Table ) 
{
    byte head[8] = { ME_SYSEX, 0x41, 0x00, 0x42, 0x12, 0x40, 0x01, 0x10 };
    byte tail[2] = { 0x00, ME_EOX };
    _runStat = ME_SYSEX;
    writePort( head, 8 );
    writePort( Table, 16 );
    writePort( tail, 2 );
}

// Velocity curve
    
void FluxSynth::setVelocitySlope( byte Channel, byte Slope ) 
{
    _sendPartParameter( Channel, 0x1A, Slope );
}

void FluxSynth::setVelocityOffset( byte Channel, byte Offset ) 
{
    _sendPartParameter( Channel, 0x1B, Offset );
}

// Custom controls 1 and 2 assignment

void FluxSynth::assignCC1Controller( byte Channel, byte CtrlNr ) 
{
    if (CtrlNr <= 0x5F) _sendPartParameter( Channel, 0x1F, CtrlNr ); // def 0x10 - CT_GEN_1
}

void FluxSynth::assignCC2Controller( byte Channel, byte CtrlNr ) 
{
    if (CtrlNr <= 0x5F) _sendPartParameter( Channel, 0x20, CtrlNr ); // def 0x11 - CT_GEN_2
}

//-----------------------------------------------------------------------------
// Tuning
//-----------------------------------------------------------------------------

void FluxSynth::setTranspose( byte Channel, byte SemiCtrl ) 
{
    RPN_Control( Channel, 0,2, SemiCtrl );
}

// Tuning +-100 cent, step 100/64 cent
void FluxSynth::setTuning( byte Channel, byte CentValue )
{
    RPN_Control( Channel, 0,1, CentValue );
}

#ifdef HAVE_14B_CONTROLLER
// Tuning +-100 cent, step 100/8192 cent (not sure if supported by 2195)

int8_t tuningCentsW( uint16_t ctrlVal ) 
{
    return int8_t( ((int16_t(ctrlVal) - CTV_CENTER14) * 100) / 8192 );
}
    
uint16_t tuningCtrlValueW( int8_t Cents ) 
{
    return uint16_t( CTV_CENTER14 + ((int16_t(Cents) * 8192) / 100) );
}

void FluxSynth::setFineTuning( byte Channel, word CentValue ) 
{
    RPN_ControlW( Channel, 0,1, CentValue );
}
#endif

// Modify scale tuning for channel
// Tuning table must contain 12 bytes with semitone tuning:
// 00..40..7F --> -63cent..0..+63cent

void FluxSynth::setScaleTuning( byte Channel, byte* TuningTable ) 
{
    byte head[8] = 
    { 
        ME_SYSEX, 0x41, 0x00, 0x42, 0x12, 0x40, 0x10|(Channel & 0x0F), 0x15 
    };
    byte tail[2] = 
    { 
        0x00, ME_EOX 
    };
    _runStat = ME_SYSEX;
    writePort( head, 8 );
    writePort( TuningTable, 12 );
    writePort( tail, 2 );
}

//-----------------------------------------------------------------------------
// Drum specials
//-----------------------------------------------------------------------------

void FluxSynth::setDrumPitch( byte Channel, byte DrumNr, byte Semitone ) 
{
    NRPN_Control( Channel, 0x18, DrumNr, Semitone );
}

void FluxSynth::setDrumMix( 
    byte Channel, byte DrumNr, byte Level, byte Pan, byte Reverb, byte Chorus 
    ) 
{
    DrumNr &= 0x7F;
    if (Level < 0x80) NRPN_Control( Channel, 0x1A, DrumNr, Level );
    if (Pan < 0x80)   NRPN_Control( Channel, 0x1C, DrumNr, Pan );
    if (Reverb < 0x80) NRPN_Control( Channel, 0x1D, DrumNr, Reverb );
    if (Chorus < 0x80) NRPN_Control( Channel, 0x1E, DrumNr, Chorus );
}

//-----------------------------------------------------------------------------
// Master control
//-----------------------------------------------------------------------------

void FluxSynth::midiReset() 
{
    writePort( ME_RESET );
}

void FluxSynth::GM_Reset() // GM - General MIDI reset
{
    byte command[6] = { ME_SYSEX, 0x7E, 0x7F, 0x09, 0x01, ME_EOX };
    writePort( command, 6 );
}

void FluxSynth::GS_Reset() // GS - Reset GS settings
{
    byte sxdata[3] = { 0, 0x7F, 0 };
    sendParameterData( sxdata, 3 );
}

// Master volume / pan

void FluxSynth::setMasterVolume( byte Level ) // (GM) 0..127, Default 127 (0x7F)
{
    byte command[8] = // F0H 7FH 7FH 04H 01H 00H LL F7H
    {
        ME_SYSEX, SXID_REALTIME, SX_ALLDEVS, 0x04, 0x01, 0x00, 
        MIDIDATA( Level ), ME_EOX 
    };
    writePort( command, 8 );
}

void FluxSynth::GS_MasterVolume( byte Level ) // [GS] 0..127, Default 127 (0x7F)
{
    byte sxdata[3] = { 0,4, MIDIDATA( Level )}; // F0 41 00 42 12 40 00 04 vv xx F7
    sendParameterData( sxdata, 3 );
}

void FluxSynth::GS_MasterPan( byte Pan ) 
{
    byte sxdata[3] = { 0,6, MIDIDATA( Pan )}; // F0 41 00 42 12 40 00 06 vv xx F7
    sendParameterData( sxdata, 3 );
}

// Master tuning

void FluxSynth::setMasterTranspose( byte Semitone )
{
    byte sxdata[3] = { 0,5, MIDIDATA( Semitone )};
    sendParameterData( sxdata, 3 );
}

void FluxSynth::setMasterTuning( word Value )
{
    // Master tune, -100.0 to +100.0 cents, in steps of 0.1 cent.
    //
    // Roland GS: Value = 0018h - 0400h - 07E8h <--> -1000 - 0 - +1000
    // Nibblized data should be used (always four bytes).
    // Default 0400h - Nibbelized (MSB first): 00h 04h 00h 00h.
    
    byte hi = CTV_HIGH( Value );
    byte lo = CTV_LOW( Value );
    byte sxdata[6] = { 0,0, HI_NIB(hi), LO_NIB(hi), HI_NIB(lo), LO_NIB(lo) };
    sendParameterData( sxdata, 6 );
}

void FluxSynth::setEzMasterTuning( short DeciCent ) // +-100.0 cent in steps of 0.1 cent
{
    #ifdef FOOLPROOF
    if (abs( DeciCent ) <= 1000)
    #endif
    setMasterTuning( word( 0x400 + DeciCent )); 
}

//-----------------------------------------------------------------------------
// Effects
//-----------------------------------------------------------------------------

// Channel reverb send

void FluxSynth::setReverbSend( byte Channel, byte Level ) 
{
    controlChange( Channel, CT_REVERB, Level ); // 5B - Effect 1 depth
}

// Global reverb parameters

void FluxSynth::setReverbLevel( byte MasterLevel )
{
    byte sxdata[3] = { 0x01, 0x33, MIDIDATA( MasterLevel )};
    sendParameterData( sxdata, 3 );
}

void FluxSynth::setReverb( 
    byte Program, byte Time, byte Feedback, byte Character )
{
    setReverbProgram( Program );
    setReverbTime( Time );
    if (Program & 0x07 > 5) setReverbFeedback( Feedback );
    setReverbCharacter( Character );
}

void FluxSynth::setReverbProgram( byte Program ) 
{
    // Reverb programs
    // 0: Room1   1: Room2    2: Room3 
    // 3: Hall1   4: Hall2    5: Plate
    // 6: Delay   7: Pan delay
    byte pgdata[3] = { 0x01, 0x30, Program & 0x07 };
    sendParameterData( pgdata, 3 );
}

void FluxSynth::setReverbTime( byte Time ) 
{
    if (Time < 0x80) 
    {
        byte tmdata[3] = { 0x01, 0x34, Time }; //F0H 41H 00H 42H 12H 40H 01H 34H vv xx F7H
        sendParameterData( tmdata, 3 );
    }
}

void FluxSynth::setReverbFeedback( byte Feedback ) 
{
    if (Feedback < 0x80) 
    {
        byte fbdata[3] = { 0x01, 0x35, Feedback }; //F0H 41H 00H 42H 12H 40H 01H 35H vv xx F7H
        sendParameterData( fbdata, 3 );
    }
}

void FluxSynth::setReverbCharacter( byte Character ) 
{
    if (Character < 8) 
    {
        byte crdata[3] = { 0x01, 0x31, Character }; //F0H 41H 00H 42H 12H 40H 01H 31H vv xx F7H
        sendParameterData( crdata, 3 );
    }
}

// Channel chorus send

void FluxSynth::setChorusSend( byte Channel, byte Level ) 
{
    controlChange( Channel, CT_CHORUS, Level ); // 5D - Chorus depth
}

// Global chorus parameters

void FluxSynth::setChorusLevel( byte MasterLevel )  // Master chorus return level
{
    byte sxdata[3] = { 0x01, 0x3A, MIDIDATA( MasterLevel )};
    sendParameterData( sxdata, 3 );
}

void FluxSynth::setChorus( 
    byte Program, byte Delay, byte Feedback, byte Rate, byte Depth ) // [GS]
{
    setChorusProgram( Program);
    setChorusDelay( Delay );
    setChorusFeedback( Feedback );
    setChorusRate( Rate );
    setChorusDepth( Depth );
}

void FluxSynth::setChorusProgram( byte Program )
{
    // Chorus programs
    // 0: Chorus1   1: Chorus2  2: Chorus3      3: Chorus4
    // 4: Feedback  5: Flanger  6: Short delay  7: FB delay
    byte pgdata[3] = { 0x01, 0x38, Program & 0x07 };
    sendParameterData( pgdata, 3 );
}
void FluxSynth::setChorusDelay( byte Delay )
{
    if (Delay < 0x80) 
    {
        byte ddata[3] = { 0x01, 0x3C, Delay }; // F0H 41H 00H 42H 12H 40H 01H 3CH vv xx F7H
        sendParameterData( ddata,3 );
    }
}
void FluxSynth::setChorusFeedback( byte Feedback )
{
    if (Feedback < 0x80) 
    {
        byte fdata[3] = { 0x01, 0x3B, Feedback }; //F0H 41H 00H 42H 12H 40H 01H 3BH vv xx F7H
        sendParameterData( fdata,3 );
    }
}
void FluxSynth::setChorusRate( byte Rate ) 
{
    if (Rate < 0x80) 
    {
        // BUGBUGBUG // This causes infinite echo time or other REVERB malfunctions!!
        byte rdata[3] = { 0x01, 0x3D, Rate }; // F0H 41H 00H 42H 12H 40H 01H 3DH vv xx F7H
        sendParameterData( rdata,3 );
    }
}
void FluxSynth::setChorusDepth( byte Depth )
{
    if (Depth < 0x80) 
    {
        byte edata[3] = { 0x01, 0x3E, Depth }; // F0H 41H 00H 42H 12H 40H 01H 3EH vv xx F7H
        sendParameterData( edata,3 );
    }
}

//-----------------------------------------------------------------------------
// Modulators
//-----------------------------------------------------------------------------

// Vibrato

void FluxSynth::setVibrato( byte Channel, byte Rate, byte Depth, byte Delay ) 
{
    NRPN_Control( Channel, 1, 0x08, Rate );
    NRPN_Control( Channel, 1, 0x09, Depth );
    NRPN_Control( Channel, 1, 0x0A, Delay );
}

void FluxSynth::setVibratoRate( byte Channel, byte RateVal ) 
{
    NRPN_Control( Channel, 1, 0x08, RateVal );
}

void FluxSynth::setVibratoDepth( byte Channel, byte DepthVal ) 
{
    NRPN_Control( Channel, 1, 0x09, DepthVal );
}
    
void FluxSynth::setVibratoDelay( byte Channel, byte DelayVal ) 
{
    NRPN_Control( Channel, 1, 0x0A, DelayVal );
}

// Time Variant Filter

void FluxSynth::setTvFilter( byte Channel, byte CutoffFreq, byte Resonance ) 
{
    NRPN_Control( Channel, 1, 0x20, CutoffFreq );
    NRPN_Control( Channel, 1, 0x21, Resonance );
}

void FluxSynth::setTvfCutoff( byte Channel, byte CutoffFreq ) 
{
    NRPN_Control( Channel, 1, 0x20, CutoffFreq );
}
    
void FluxSynth::setTvfResonance( byte Channel, byte Resonance ) 
{
    NRPN_Control( Channel, 1, 0x21, Resonance );
}

// Envelope
    
void FluxSynth::setEnvelope( 
    byte Channel, byte Attack, byte Decay, byte Release ) 
{
    NRPN_Control( Channel, 1, 0x63, Attack );
    NRPN_Control( Channel, 1, 0x64, Decay );
    NRPN_Control( Channel, 1, 0x66, Release );
}

void FluxSynth::setEnvAttack( byte Channel, byte Attack ) 
{
    NRPN_Control( Channel, 1, 0x63, Attack );
}

void FluxSynth::setEnvDecay( byte Channel, byte Decay ) 
{
    NRPN_Control( Channel, 1, 0x64, Decay );
}

void FluxSynth::setEnvRelease( byte Channel, byte Release ) 
{
    NRPN_Control( Channel, 1, 0x66, Release );
}

// LFO

void FluxSynth::setLfoRate( byte CtrlVal ) 
{
    _sendModParameter( 0, 0x03, CtrlVal ); // Rate is common to all channe
}

#ifndef NO_MOD_CONTROLLERS // Modulator controllers

void FluxSynth::setModPitchDepth( byte Controller, byte Channel, byte SemiCtrl ) 
{
    _sendModParameter( Channel, Controller+0, SemiCtrl ); // +-24 --> 28..40..58
}

void FluxSynth::setModTvfCutoff( byte Controller, byte Channel, byte CtrlVal ) 
{
    _sendModParameter( Channel, Controller+1, CtrlVal ); // 0..40..7F
}

void FluxSynth::setModAmplitude( byte Controller, byte Channel, byte CtrlVal ) 
{
    _sendModParameter( Channel, Controller+2, CtrlVal ); // 0..40..7F
}

void FluxSynth::setModLfoPitchDepth( byte Controller, byte Channel, byte CtrlVal ) 
{
    _sendModParameter( Channel, Controller+4, CtrlVal ); // 0..40..7F
}

void FluxSynth::setModLfoTvfDepth( byte Controller, byte Channel, byte CtrlVal ) 
{
    _sendModParameter( Channel, Controller+5, CtrlVal ); // 0..40..7F
}

void FluxSynth::setModLfoTvaDepth( byte Controller, byte Channel, byte CtrlVal ) 
{
    _sendModParameter( Channel, Controller+6, CtrlVal ); // 0..40..7F
}

#endif //ndef NO_MOD_CONTROLLERS
//-----------------------------------------------------------------------------
// Special Dream functions
//-----------------------------------------------------------------------------

//+ Master volume

void FluxSynth::setOutputLevel( byte Level ) 
{
    _sendDreamControl( 7, Level );
}

//+ Enable effects

void FluxSynth::enableEffects( byte Flags ) 
{
    _sendDreamControl( 0x5F, Flags );
}

void FluxSynth::restartEffects() 
{
    enableEffects( EF_RESET );
    _effects = EF_ALL;
    delay( 50 );
}

void FluxSynth::enableReverb( boolean On ) 
{
    if (On) _effects |= EF_REVERB; 
    else    _effects &= byte( ~EF_REVERB );
    enableEffects( _effects );
}
    
void FluxSynth::enableSurround( boolean On ) 
{
    if (On) _effects |= EF_SURROUND; 
    else    _effects &= byte( ~EF_SURROUND );
    enableEffects( _effects );
}

void FluxSynth::setEqualizerMode( byte revMode ) // off / 2 band / 4 band
{
    _effects &= byte( ~EF_EQ_4BAND );
    _effects |= revMode;
    enableEffects( _effects );
}

//+ Clipping 

void FluxSynth::setClippingMode( byte Mode ) // soft / hard
{
    _sendDreamControl( 0x13, Mode );
}

//+ Equalizer

void FluxSynth::setEqualizer( byte BandNr, byte Freq, byte Gain )
{
    if (BandNr < 4) 
    {
        _sendDreamControl( 0x08 + BandNr, Freq );
        _sendDreamControl( BandNr, Gain );
    }
}

void FluxSynth::setEqGain( byte BandNr, byte Gain ) 
{
    if (BandNr < 4) _sendDreamControl( BandNr, Gain );
}
    
void FluxSynth::setEqFrequency( byte BandNr, byte Freq ) 
{
    if (BandNr < 4) _sendDreamControl( 8+BandNr, Freq );
}

#define EQFM_BASS   4700    // Freq limit of the bass filter
#define EQFM_MID    4200    // Freq limit of the mid filters
#define EQFM_TREBLE 18750   // Freq limit of the treble filter
#define EQCTL_MAX   0x7F    // Max ctrl value (midi data limit)

byte _EqBassCtrl( word Hz )
{
    if (Hz > EQFM_BASS) Hz = EQFM_BASS;
    return byte( Hz * EQCTL_MAX / EQFM_BASS );
}
word _EqBassHz( byte ctrlVal )
{
    ctrlVal &= EQCTL_MAX;
    return word( ctrlVal ) * EQFM_BASS / EQCTL_MAX;
}

byte _EqMidCtrl( word Hz )
{
    if (Hz > EQFM_MID) Hz = EQFM_MID;
    return byte( Hz * EQCTL_MAX / EQFM_MID );
}
word _EqMidHz( byte ctrlVal )
{
    ctrlVal &= EQCTL_MAX;
    return word( ctrlVal ) * EQFM_MID / EQCTL_MAX;
}

byte _EqTrebleCtrl( word Hz )
{
    if (Hz > EQFM_TREBLE) Hz = EQFM_TREBLE;
    return byte( Hz * EQCTL_MAX / EQFM_TREBLE );
}
word _EqTrebleHz( byte ctrlVal )
{
    ctrlVal &= EQCTL_MAX;
    return word( ctrlVal ) * EQFM_TREBLE / EQCTL_MAX;
}

//+ Surround 

void FluxSynth::setSurroundVolume( byte Level ) 
{
    _sendDreamControl( 0x20, Level );
}

void FluxSynth::setSurroundDelay( byte Time ) 
{
    _sendDreamControl( 0x2C, Time );
}

void FluxSynth::surroundMonoIn( boolean Mono ) 
{
    _sendDreamControl( 0x2D, Mono ? 0x7F : 0 );
}

//+ Routing
    
void FluxSynth::postprocGeneralMidi( boolean On ) 
{
    _sendDreamControl( 0x18, On ? 0x7F : 0 );
}

void FluxSynth::postprocReverbChorus( boolean On ) 
{
    _sendDreamControl( 0x1A, On ? 0x7F : 0 );
}

//+ GM

void FluxSynth::GM_ReverbSend( byte Level ) 
{
    _sendDreamControl( 0x15, Level );
}
void FluxSynth::GM_ReverbSend( byte Channel, byte Level ) 
{
    NRPN_Control( Channel ,0x37, 0x15, Level );
}

void FluxSynth::GM_ChorusSend( byte Level ) 
{
    _sendDreamControl( 0x16, Level );
}
void FluxSynth::GM_ChorusSend( byte Channel, byte Level ) 
{
    NRPN_Control( Channel ,0x37, 0x16, Level );
}

void FluxSynth::GM_Volume( byte Level ) 
{
    _sendDreamControl( 0x22, Level );
}
void FluxSynth::GM_Volume( byte Channel, byte Level ) 
{
    NRPN_Control( Channel ,0x37, 0x22, Level );
}

void FluxSynth::GM_Pan( byte Pan ) 
{
    _sendDreamControl( 0x23, Pan ); // same as GM sysex 40 00 06
}
void FluxSynth::GM_Pan( byte Channel, byte Pan ) 
{
    NRPN_Control( Channel ,0x37, 0x23, Pan );
}
    
//+ Misc
    
void FluxSynth::runSelfTest() 
{
    _sendDreamControl( 0x51, 0x23 );
}

void FluxSynth::setSysExModuleId( byte Id ) 
{
    _sendDreamControl( 0x57, Id );
}

// EOF