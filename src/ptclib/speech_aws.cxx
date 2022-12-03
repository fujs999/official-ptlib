/*
 * speech_aws.cxx
 *
 * AWS SDK interface wrapper for speech
 *
 * Portable Windows Library
 *
 * Copyright (c) 2021 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): Robert Jongbloed.
 */

#include <ptlib.h>

#if P_AWS_SDK

#include <ptclib/speech.h>
#include <ptclib/pwavfile.h>
#include <ptclib/random.h>
#include <ptclib/url.h>
#include <ptclib/cypher.h>
#include <ptclib/http.h>
#include <ptclib/pjson.h>
#include <ptclib/aws_sdk.h>

#define USE_IMPORT_EXPORT
#include <aws/polly/PollyClient.h>
#include <aws/text-to-speech/PCMOutputDriver.h>
#include <aws/text-to-speech/TextToSpeechManager.h>
#include <aws/transcribe/TranscribeServiceClient.h>
#include <aws/transcribe/model/CreateVocabularyRequest.h>
#include <aws/transcribe/model/DeleteVocabularyRequest.h>


#define PTraceModule() "AWS-Speech"

#ifdef _MSC_VER
  #pragma comment(lib, P_AWS_SDK_LIBDIR(polly))
  #pragma comment(lib, P_AWS_SDK_LIBDIR(text-to-speech))
  #pragma comment(lib, P_AWS_SDK_LIBDIR(transcribe))
#endif


 ////////////////////////////////////////////////////////////
// Text to speech using AWS

class PTextToSpeech_AWS : public PTextToSpeech, PAwsClient<Aws::Polly::PollyClient>
{
  PCLASSINFO(PTextToSpeech_AWS, PTextToSpeech);

  std::shared_ptr<Aws::TextToSpeech::TextToSpeechManager> m_manager;
  Aws::TextToSpeech::CapabilityInfo m_capability;

  struct WAVOutputDriver : Aws::TextToSpeech::PCMOutputDriver, PObject
  {
    PWAVFile m_wavFile;

    virtual bool WriteBufferToDevice(const unsigned char* data, size_t size)
    {
      return m_wavFile.Write(data, size);
    }
    virtual Aws::Vector<Aws::TextToSpeech::DeviceInfo> EnumerateDevices() const
    {
      Aws::TextToSpeech::DeviceInfo deviceInfo;
      deviceInfo.deviceName = "*.wav";
      return Aws::Vector<Aws::TextToSpeech::DeviceInfo>(1, deviceInfo);
    }
    virtual void SetActiveDevice(const Aws::TextToSpeech::DeviceInfo & deviceInfo, const Aws::TextToSpeech::CapabilityInfo & capability)
    {
      if (m_wavFile.Open(deviceInfo.deviceName.c_str(), PFile::WriteOnly) &&
          m_wavFile.SetSampleRate(capability.sampleRate) &&
          m_wavFile.SetChannels(capability.channels) &&
          m_wavFile.SetSampleSize(capability.sampleWidthBits)) {
        PTRACE(4, "Opened WAV file " << m_wavFile.GetName());
      }
    }
    virtual const char* GetName() const
    {
      return m_wavFile.GetName().c_str();
    }
  };

  struct OutputDriverFactory : Aws::TextToSpeech::PCMOutputDriverFactory
  {
    virtual Aws::Vector<std::shared_ptr<Aws::TextToSpeech::PCMOutputDriver>> LoadDrivers() const
    {
      Aws::Vector<std::shared_ptr<Aws::TextToSpeech::PCMOutputDriver>> drivers;
      drivers.push_back(std::make_shared<WAVOutputDriver>());
      return drivers;
    }
  };

public:
  PTextToSpeech_AWS()
  {
    m_capability.sampleRate = 8000;
    m_capability.channels = 1;
    m_capability.sampleWidthBits = 16;
  }


  ~PTextToSpeech_AWS()
  {
    Close();
  }


  virtual bool SetOptions(const PStringOptions & options)
  {
    return PAwsClient<Aws::Polly::PollyClient>::SetOptions(options) && PTextToSpeech::SetOptions(options);
  }


  virtual PStringArray GetVoiceList()
  {
    //Enumerate the available voices 
    auto mgr = m_manager;
    if (!mgr)
      mgr = Aws::TextToSpeech::TextToSpeechManager::Create(GetClient());
    Aws::Vector<std::pair<Aws::String, Aws::String>> awsVoices = mgr->ListAvailableVoices();

    PStringArray voiceList(awsVoices.size());
    for (size_t i = 0; i < awsVoices.size(); ++i)
      voiceList[i] = PSTRSTRM(awsVoices[i].first << ':' << awsVoices[i].second);
    PTRACE(3, "Voices: " << setfill(',') << voiceList);
    return voiceList;
  }


  virtual bool InternalSetVoice(const PString & name, const PString & /*language*/)
  {
    if (!m_manager)
      return false;

    m_manager->SetActiveVoice(name.c_str());
    return true;
  }


  virtual bool SetSampleRate(unsigned rate)
  {
    m_capability.sampleRate = rate;
    return true;
  }


  virtual unsigned GetSampleRate() const
  {
    return m_capability.sampleRate;
  }


  virtual bool SetChannels(unsigned channels)
  {
    m_capability.channels = channels;
    return true;
  }


  virtual unsigned GetChannels() const
  {
    return m_capability.channels;
  }


  bool SetVolume(unsigned)
  {
    return false;
  }


  unsigned GetVolume() const
  {
    return 100;
  }


  bool OpenFile(const PFilePath & fn)
  {
    m_manager = Aws::TextToSpeech::TextToSpeechManager::Create(GetClient(), std::make_shared<OutputDriverFactory>());
    std::shared_ptr<WAVOutputDriver> driver = std::make_shared<WAVOutputDriver>();
    PTRACE_CONTEXT_ID_TO(*driver);
    Aws::TextToSpeech::DeviceInfo deviceInfo;
    deviceInfo.deviceName = fn.c_str();
    m_manager->SetActiveDevice(driver, deviceInfo, m_capability);
    if (m_voiceName.empty())
      SetVoice(PString::Empty());
    else
      InternalSetVoice(m_voiceName, m_voiceLanguage);
    return true;
  }


  bool OpenChannel(PChannel * /*channel*/)
  {
    Close();
    return false;
  }


  bool IsOpen() const
  {
    return !!m_manager;
  }


  bool Close()
  {
    m_manager.reset();
    return true;
  }


  bool Speak(const PString & text, TextType /*hint*/)
  {
    bool success = false;
    PSyncPoint done;
    Aws::TextToSpeech::SendTextCompletedHandler handler =
          [&success, &done]
          (const char*, const Aws::Polly::Model::SynthesizeSpeechOutcome&, bool doneGood)
          {
            success = doneGood;
            done.Signal();
          };
    m_manager->SendTextToOutputDevice(text.c_str(), handler);
    done.Wait();
    return success;
  }
};


PFACTORY_CREATE(PFactory<PTextToSpeech>, PTextToSpeech_AWS, P_TEXT_TO_SPEECH_AWS, false);


////////////////////////////////////////////////////////////
// Speech recognition using Microsoft's Speech API (SAPI)

class PSpeechRecognition_AWS : public PSpeechRecognition, PAwsClient<Aws::TranscribeService::TranscribeServiceClient>
{
  PCLASSINFO(PSpeechRecognition_AWS, PSpeechRecognition);

  unsigned m_sampleRate;
  Aws::TranscribeService::Model::LanguageCode m_language;
  PStringSet m_vocabularies;

  PWebSocket m_webSocket;
  PThread  * m_eventThread;
  PDECLARE_MUTEX(m_openMutex);
  PDECLARE_MUTEX(m_eventMutex);

  #pragma pack(1)
  struct Prelude
  {
    PUInt32b m_totalLen;
    PUInt32b m_headerLen;
    PUInt32b m_preludeCRC;
  };
  #pragma pack()

  bool WriteEvent(const PBYTEArray & contents, const PStringOptions & headers)
  {
    size_t headerLen = 0;
    for (PStringOptions::const_iterator it = headers.begin(); it != headers.end(); ++it)
      headerLen += 1 + it->first.length() + 1 + 2 + it->second.length();

    PBYTEArray message(sizeof(Prelude) + headerLen + contents.size() + sizeof(PUInt32b));
    BYTE * ptr = message.GetPointer();

    Prelude & prelude = *reinterpret_cast<Prelude *>(ptr);
    ptr += sizeof(Prelude);

    prelude.m_headerLen = headerLen;
    prelude.m_totalLen = message.size();
    prelude.m_preludeCRC = PCRC32::Calculate(&prelude, sizeof(Prelude) - sizeof(prelude.m_preludeCRC));

    for (PStringOptions::const_iterator it = headers.begin(); it != headers.end(); ++it) {
      PString key = it->first;
      PString value = it->second;
      *ptr++ = key.length();
      memcpy(ptr, key.c_str(), key.length());
      ptr += key.length();
      *ptr++ = 7;  // String
      *(PUInt16b *)ptr = value.length();
      ptr += 2;
      memcpy(ptr, value.c_str(), value.length());
      ptr += value.length();
    }

    memcpy(ptr, contents, contents.size());
    ptr += contents.size();

    *(PUInt32b *)ptr = PCRC32::Calculate(message, message.size() - sizeof(PUInt32b));

    return m_webSocket.Write(message, message.size());
  }

  bool WriteAudioEvent(const int16_t * samples, size_t sampleCount)
  {
    PStringOptions headers;
    headers.Set(":message-type", "event");
    headers.Set(":event-type", "AudioEvent");
    headers.Set(":content-type", "application/octet-stream");

    return WriteEvent(PBYTEArray((const BYTE *)samples, sampleCount*sizeof(uint16_t), false), headers);
  }

  bool ReadEvent(PBYTEArray & contents, PStringOptions & headers)
  {
    PBYTEArray message;
    if (!m_webSocket.ReadMessage(message))
      return false;

    if (message.size() < sizeof(Prelude)) {
      PTRACE(2, "Received message ridiculously small");
      return false;
    }

    Prelude prelude = message.GetAs<Prelude>();
    if (message.size() < prelude.m_totalLen) {
      PTRACE(2, "Received message too small");
      return false;
    }

    uint32_t calculatedCRC = PCRC32::Calculate(message, sizeof(Prelude) - sizeof(prelude.m_preludeCRC));
    uint32_t providedCRC = prelude.m_preludeCRC;
    if (calculatedCRC != providedCRC) {
      PTRACE(2, "Received message header CRC failed: " << calculatedCRC << "!=" << providedCRC);
      return false;
    }

    size_t crcPos = message.size() - sizeof(uint32_t);
    calculatedCRC = PCRC32::Calculate(message, crcPos);
    providedCRC = message.GetAs<PUInt32b>(crcPos);
    if (calculatedCRC != providedCRC) {
      PTRACE(2, "Received message CRC failed: " << calculatedCRC << "!=" << providedCRC);
      return false;
    }

    const BYTE * ptr = message.GetPointer() + sizeof(prelude);
    const BYTE * endHeaders = ptr + prelude.m_headerLen;
    while (ptr < endHeaders) {
      PString key(reinterpret_cast<const char *>(ptr)+1, *ptr);
      ptr += key.length() + 1;
      switch (*ptr++) {
        default:
          PTRACE(2, "Received message with invalid header");
          return false;
        case 0:
          headers.SetBoolean(key, true);
          break;
        case 1:
          headers.SetBoolean(key, false);
          break;
        case 2:
          headers.SetInteger(key, *ptr++);
          break;
        case 3:
          headers.SetInteger(key, *reinterpret_cast<const PUInt16b *>(ptr));
          ptr += sizeof(PUInt16b);
          break;
        case 4:
          headers.SetInteger(key, *reinterpret_cast<const PUInt32b *>(ptr));
          ptr += sizeof(PUInt32b);
          break;
        case 5:
          headers.SetVar(key, *reinterpret_cast<const PUInt64b *>(ptr));
          ptr += sizeof(PUInt64b);
          break;
        case 6:
        case 7:
        case 8:
          uint16_t len = *reinterpret_cast<const PUInt16b *>(ptr);
          headers.SetString(key, PString(reinterpret_cast<const char *>(ptr)+sizeof(PUInt16b), len));
          ptr += len + sizeof(PUInt16b);
          break;
      }
    }

    contents = PBYTEArray(ptr, message.size() - sizeof(prelude) - prelude.m_headerLen - sizeof(uint32_t));
    return true;
  }

  void ReadEvents()
  {
    PTRACE(4, "Reading events thread started");
    std::set<Transcript> sentTranscripts;
    while (m_webSocket.IsOpen()) {
      PBYTEArray data;
      PStringOptions headers;
      if (ReadEvent(data, headers)) {
        PTRACE(5, "Read " << data.size() << " bytes, headers:\n" << headers);

        PJSON json(PJSON::e_Null);
        if (headers.Get(":content-type") == PMIMEInfo::ApplicationJSON())
          json.FromString(data);

        PString messageType = headers.Get(":message-type");
        if (messageType == "event") {
          PString type = headers.Get(":event-type");
          if (type == "TranscriptEvent" &&
              json.IsType(PJSON::e_Object) &&
              json.GetObject().IsType("Transcript", PJSON::e_Object) &&
              json.GetObject().GetObject("Transcript").IsType("Results", PJSON::e_Array)) {
            PJSON::Array const & results = json.GetObject().GetObject("Transcript").GetArray("Results");
            PTRACE_IF(4, !results.empty(), "Received " << type << ":\n" << json.AsString(0, 2));
            for (size_t idxResult = 0; idxResult < results.size(); ++idxResult) {
              if (results.IsType(idxResult, PJSON::e_Object)) {
                PJSON::Object const & result = results.GetObject(idxResult);
                if (result.IsType("Alternatives", PJSON::e_Array)) {
                  PJSON::Array const & alternatives = result.GetArray("Alternatives");
                  for (size_t idxAlternative = 0; idxAlternative < alternatives.size(); ++idxAlternative) {
                    if (alternatives.GetObject(idxAlternative).IsType("Items", PJSON::e_Array)) {
                      PJSON::Array const & items = alternatives.GetObject(idxAlternative).GetArray("Items");
                      for (size_t idxItem = 0; idxItem < items.size(); ++idxItem) {
                        if (items.IsType(idxItem, PJSON::e_Object)) {
                          PJSON::Object const & item = items.GetObject(idxItem);
                          if (item.GetString("Type") == "pronunciation") {
                            Transcript transcript(false,
                                                  PTimeInterval::Seconds((double)item.GetNumber("StartTime")),
                                                  item.GetString("Content"),
                                                  (float)item.GetNumber("Confidence"));
                            if (!transcript.m_content.empty() && sentTranscripts.find(transcript) == sentTranscripts.end()) {
                              sentTranscripts.insert(transcript);
                              PTRACE(4, "Sending notification of " << transcript);
                              m_notifier(*this, transcript);
                            }
                          }
                        }
                      }
                    }
                  }
                  if (!alternatives.empty() && !result.GetBoolean("IsPartial")) {
                    Transcript transcript(true,
                                          PTimeInterval::Seconds((double)result.GetNumber("StartTime")),
                                          alternatives.GetObject(0).GetString("Transcript"),
                                          1);
                    PTRACE(4, "Sending notification of " << transcript);
                    m_notifier(*this, transcript);
                  }
                }
              }
            }
          }
          else {
            PTRACE(4, "Received " << type << ":\n" << json.AsString(0, 2));
          }
        }
#if PTRACING
        else if (PTrace::CanTrace(2) && messageType == "exception") {
          ostream & log = PTRACE_BEGIN(2);
          log << "Received exception " << headers.Get(":exception-type", "Unknown") << ", details:";
          if (json.IsType(PJSON::e_Null))
            log << '\n' << data;
          else {
            PString msg;
            if (json.IsType(PJSON::e_Object))
              msg = json.GetObject().GetString("Message");
            else if (json.IsType(PJSON::e_String))
              msg = json.GetString();
            if (msg.empty())
              log << '\n' << json.AsString(0, 2);
            else
              log << (msg.Find('\n') != P_MAX_INDEX ? '\n' : ' ') << msg;
          }
          log << PTrace::End;
        }
#endif // PTRACING
      }
    }
    PTRACE(4, "Reading events thread ended");
  }


  void DeleteVocabulary(const PString & name)
  {
    if (name.empty() || !m_vocabularies.Contains(name))
      return;

    GetClient()->DeleteVocabulary(Aws::TranscribeService::Model::DeleteVocabularyRequest()
                                    .WithVocabularyName(name.c_str()));
    m_vocabularies -= name;
  }


public:
  PSpeechRecognition_AWS()
    : m_sampleRate(8000)
    , m_language(Aws::TranscribeService::Model::LanguageCode::en_US)
    , m_eventThread(NULL)
  { }


  ~PSpeechRecognition_AWS()
  {
    Close();

    while (!m_vocabularies.empty())
      DeleteVocabulary(*m_vocabularies.begin());
  }


  virtual bool SetOptions(const PStringOptions & options)
  {
    m_webSocket.SetProxies(options);
    return PAwsClient<Aws::TranscribeService::TranscribeServiceClient>::SetOptions(options) && PSpeechRecognition::SetOptions(options);
  }


  virtual bool SetSampleRate(unsigned rate)
  {
    PWaitAndSignal lock(m_openMutex);
    if (IsOpen())
      return false;

    m_sampleRate = rate;
    return true;
  }


  virtual unsigned GetSampleRate() const
  {
    return m_sampleRate;
  }


  virtual bool SetChannels(unsigned channels)
  {
    return channels == 1;
  }


  virtual unsigned GetChannels() const
  {
    return 1;
  }


  virtual bool SetLanguage(const PString & language)
  {
    PWaitAndSignal lock(m_openMutex);
    if (IsOpen())
      return false;

    auto code = Aws::TranscribeService::Model::LanguageCodeMapper::GetLanguageCodeForName(language.c_str());
    if (code ==  Aws::TranscribeService::Model::LanguageCode::NOT_SET)
      return false;

    m_language = code;
    return true;
  }


  virtual PString GetLanguage() const
  {
    return Aws::TranscribeService::Model::LanguageCodeMapper::GetNameForLanguageCode(m_language);
  }


  virtual bool SetVocabulary(const PString & name, const PStringArray & words)
  {
    DeleteVocabulary(name);

    if (words.empty())
      return true;

    Aws::Vector<Aws::String> phrases(words.size());
    for (size_t i = 0; i < words.size(); ++i)
      phrases[i] = words[i].c_str();

    auto outcome = GetClient()->CreateVocabulary(Aws::TranscribeService::Model::CreateVocabularyRequest()
                                                  .WithVocabularyName(name.c_str())
                                                  .WithLanguageCode(m_language)
                                                  .WithPhrases(phrases));
    if (outcome.IsSuccess()) {
      m_vocabularies += name;
      return true;
    }

    PTRACE(2, "Could not create vocubulary: " << outcome.GetResult().GetFailureReason());
    return false;
  }


  static void AddSignedHeader(PURL & url, PStringStream & canonical, const char * key, const PString & value, bool first = false)
  {
    url.SetQueryVar(key, value);
    if (!first)
      canonical << '&';
    canonical << key << '=' << PURL::TranslateString(value, PURL::QueryTranslation);
  }

  virtual bool Open(const Notifier & notifier, const PString & vocabulary)
  {
    PWaitAndSignal lock(m_openMutex);
    Close();

    PTRACE(4, "Opening speech recognition.");

    m_notifier = notifier;

    HeaderMap headers;
    headers["language-code"] = GetLanguage();
    headers["media-encoding"] = "pcm";
    headers["sample-rate"] = GetSampleRate();

    if (!vocabulary.empty())
      headers["vocabulary-name"] = vocabulary;

    PURL url = SignURL("wss://transcribestreaming:8443/stream-transcription-websocket", "transcribe", headers);

    if (!m_webSocket.Connect(url))
      return false;

    m_webSocket.SetBinaryMode();
    m_eventThread = new PThreadObj<PSpeechRecognition_AWS>(*this, &PSpeechRecognition_AWS::ReadEvents, false, "AWS-SR");
    PTRACE(3, "Opened speech recognition.");
    return true;
  }


  virtual bool IsOpen() const
  {
    return m_webSocket.IsOpen();
  }


  virtual bool Close()
  {
    PWaitAndSignal lock(m_openMutex);
    if (!IsOpen())
      return false;

    PTRACE(3, "Closing speech recognition.");
    m_webSocket.Close();
    PThread::WaitAndDelete(m_eventThread);

    return true;
  }


  virtual bool Listen(const PFilePath & fn)
  {
    if (!PAssert(!fn.empty(), PInvalidParameter))
      return false;

    PWaitAndSignal lock(m_openMutex);
    if (!m_webSocket.IsOpen())
      return false;

    PWAVFile wavFile;
    if (!wavFile.Open(fn, PFile::ReadOnly))
      return false;

    vector<int16_t> buffer(4096);
    while (wavFile.Read(buffer.data(), buffer.size()*sizeof(int16_t))) {
      if (!WriteAudioEvent(buffer.data(), wavFile.GetLastReadCount()/sizeof(int16_t)))
        return false;
    }

    WriteAudioEvent(NULL, 0); // Empty event ends the stream
    PTRACE(4, "Written " << fn);
    return wavFile.GetErrorCode() == PChannel::NoError;
  }


  virtual bool Listen(const int16_t * samples, size_t sampleCount)
  {
    PWaitAndSignal lock(m_openMutex);
    return m_webSocket.IsOpen() && WriteAudioEvent(samples, sampleCount);
  }
};

PFACTORY_CREATE(PFactory<PSpeechRecognition>, PSpeechRecognition_AWS, P_SPEECH_RECOGNITION_AWS, false);


#endif // P_AWS_SDK
