/*
 * speech.h
 *
 * Text To Speech and Speech Recognition classes
 *
 * Portable Windows Library
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 */

#ifndef PTLIB_SPEECH_H
#define PTLIB_SPEECH_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>

#include <ptlib/pfactory.h>


#if P_TEXT_TO_SPEECH

class PTextToSpeech : public PObject
{
    PCLASSINFO(PTextToSpeech, PObject);
  public:
    static PTextToSpeech * Create(const PString & name = PString::Empty())
    {
      return PFactory<PTextToSpeech>::CreateInstance(name.empty() ? PFactory<PTextToSpeech>::GetKeyList().front() : name);
    }

    virtual PStringArray GetVoiceList() = 0;
    virtual bool SetVoice(const PString & voice) = 0;

    virtual bool SetSampleRate(unsigned rate) = 0;
    virtual unsigned GetSampleRate() const = 0;

    virtual bool SetChannels(unsigned channels) = 0;
    virtual unsigned GetChannels() const = 0;

    virtual bool SetVolume(unsigned volume) = 0;
    virtual unsigned GetVolume() const = 0;

    virtual bool OpenFile(const PFilePath & fn) = 0;
    virtual bool OpenChannel(PChannel * chanel) = 0;
    virtual bool IsOpen() const = 0;
    virtual bool Close() = 0;

    P_DECLARE_STREAMABLE_ENUM(TextType,
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
    );

    virtual bool Speak(const PString & text, TextType hint = Default) = 0;
};

PFACTORY_LOAD(PTextToSpeech_WAV);
#if P_SAPI
  PFACTORY_LOAD(PTextToSpeech_SAPI);
#endif

#if P_FESTIVAL
  PFACTORY_LOAD(PTextToSpeech_Festival);
#endif

#if P_AWS_SDK
  PFACTORY_LOAD(PTextToSpeech_AWS);
#endif

#endif // P_TEXT_TO_SPEECH

#if P_SPEECH_RECOGNITION

class PSpeechRecognition : public PObject
{
    PCLASSINFO(PSpeechRecognition, PObject);
  public:
    static PSpeechRecognition * Create(const PString & name = PString::Empty())
    {
      return PFactory<PSpeechRecognition>::CreateInstance(name.empty() ? PFactory<PSpeechRecognition>::GetKeyList().front() : name);
    }

    virtual bool SetSampleRate(unsigned rate) = 0;
    virtual unsigned GetSampleRate() const = 0;

    virtual bool SetChannels(unsigned channels) = 0;
    virtual unsigned GetChannels() const = 0;

    virtual bool SetLanguage(const PString & language) = 0;
    virtual PString GetLanguage() const = 0;

    struct Transcript : PObject
    {
      bool          m_final;
      PTimeInterval m_when;
      PString       m_content;

      Transcript(bool final, const PTimeInterval & when, const PString & content);
      virtual Comparison Compare(const PObject & other) const;
      virtual void PrintOn(ostream & strm) const;
    };
    typedef PNotifierTemplate<Transcript> Notifier;
    #define PDECLARE_SpeechRecognitionNotifier(cls, fn) PDECLARE_NOTIFIER2(PSpeechRecognition, cls, fn, PSpeechRecognition::Transcript)
    virtual void SetNotifier(const Notifier & notifier) { m_notifier = notifier; }
    const Notifier & GetNotifier() const { return m_notifier; }

    virtual bool Open(const Notifier & notifier, const PStringArray & words) = 0;
    virtual bool IsOpen() const = 0;
    virtual bool Close() = 0;

    virtual bool Listen(const PFilePath & fn) = 0;
    virtual bool Listen(const short * samples, size_t sampleCount) = 0;

  protected:
    Notifier m_notifier;
};

#if P_SAPI
  PFACTORY_LOAD(PSpeechRecognition_SAPI);
#endif
#if P_AWS_SDK
  PFACTORY_LOAD(PSpeechRecognition_AWS);
#endif


#endif // P_SPEECH_RECOGNITION

#endif // PTLIB_SPEECH_H


// End Of File ///////////////////////////////////////////////////////////////
