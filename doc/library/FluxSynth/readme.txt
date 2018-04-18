INTRODUCTION

    FluxSynth - Arduino library for the M.D. Fluxamasynth shield.

    Implements an interface for the Atmel/Dream SAM2195 synth chip.
    Based on the FlexamySynth library, but liberated from dependencies on
    any particular I/O library, by delegating the actual byte output to
    an assignable 'sendByte' function of your own.
    
    All documented RPN, NRPN, and SysEx parameter controls are supported.
    With few exceptions, standard controls are supported through a generic
    'controlChange' method and control id macros, instead of a slur of member
    methods.

    Love Nystrom 05/2012.
    
    CHANGE LOG:
    
    May 2012  LN  Class name changed to FluxSynth to disambiguate.
       -"-    LN  pitchBend changed to take compliant midi 14bit control value.
       -"-    LN  Corrected setReverb to set feedback only for revtype 6,7.
       -"-    LN  Enhanced setReverb to set all GS reverb parameters.
       -"-    LN  Enhanced setChorus to set all GS chorus parameters.
       -"-    LN  Added so many functions I'm reluctant to enumerate them..

    Acknowldegents:
    Paul Badger of Modern Devices (Original Fluxamasynth_NSS lib)
    R.McGinnis (Original PgmChange header)
    
    -------------------------------------------------------------------------

INSTALLATION

    (1) Unpack the FlexamySynh.rar archive to your <Arduino>/libraries folder.
    
    (2) Enable versioning support in the library:

        If You use Arduino older than 1.0, move the provided 'Arduino.h' file
        to your <Arduino>/hardware/arduino/cores/arduino directory.
      
        - OR -
        
        If You use Arduino 1.0 or newer, add the following entry to your 
        <Arduino>/hardware/arduino/cores/arduino/Arduino.h file:
            #define _ARDUINO_VER  0x0100
        and delete the provided stub version Arduino.h.
        
    (3) Optionally, if you use Arduino 1.0 or newer and already have
        the NewSoftSerial library installed, you may delete the provided
        version, and use your pre-existing one.

LICENSE

    FlexamySynth is released under the NO NONSENSE OPEN SOURCE LICENSE,
    which is intended to protect developers from 3rd part profiteering.
    
[ NO NONSENSE OPEN SOURCE LICENSE ]

    (aka. No Profiteering Open Source License)

Copyright (c) B.O Love Nystrom 2012
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

DISCLAIMER:

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

(
    Footnote 2012:
    This license is open source, though not ratified by the OSI,
    because OSI will not acknowledge a license that forbids 3rd parties
    to profiteer on other people's work without compensation. 
    Caveat Emptor.
)
[END]
