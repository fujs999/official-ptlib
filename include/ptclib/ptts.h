/*
 * ptts.h
 *
 * Text To Speech classes
 *
 * Portable Tools Library
 *
 * Copyright (c) 2002 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Tools Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 */

#ifndef PTLIB_PTTS_H
#define PTLIB_PTTS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>

#if P_TTS

#include <ptlib/pfactory.h>


class PTextToSpeech : public PObject
{
  PCLASSINFO(PTextToSpeech, PObject);
  public:
    enum TextType {
      Default,
      Literal,
      Digits,
      Number,
      Currency,
      Time,
      Date,
      DateAndTime,
      Phone,
      IPAddress,
      Duration,
      Spell
    };

    virtual PStringArray GetVoiceList() = 0;
    virtual bool SetVoice(const PString & voice) = 0;

    virtual bool SetSampleRate(unsigned rate) = 0;
    virtual unsigned GetSampleRate() = 0;

    virtual bool SetChannels(unsigned channels) = 0;
    virtual unsigned GetChannels() = 0;

    virtual bool SetVolume(unsigned volume) = 0;
    virtual unsigned GetVolume() = 0;

    virtual bool OpenFile(const PFilePath & fn) = 0;
    virtual bool OpenChannel(PChannel * chanel) = 0;
    virtual bool IsOpen() = 0;

    virtual bool Close() = 0;
    virtual bool Speak(const PString & text, TextType hint = Default) = 0;
};

#if P_SAPI
  PFACTORY_LOAD(PTextToSpeech_SAPI);
#endif

#if P_PIPECHAN
  PFACTORY_LOAD(PTextToSpeech_Festival);
#endif


#endif // P_TTS

#endif // PTLIB_PTTS_H


// End Of File ///////////////////////////////////////////////////////////////
