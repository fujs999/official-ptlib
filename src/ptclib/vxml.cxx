/*
 * vxml.cxx
 *
 * VXML engine for pwlib library
 *
 * Copyright (C) 2002 Equivalence Pty. Ltd.
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

#ifdef __GNUC__
#pragma implementation "vxml.h"
#endif

#include <ptlib.h>

#define P_DISABLE_FACTORY_INSTANCES

#if P_VXML

#include <ptclib/vxml.h>

#include <ptlib/pprocess.h>
#include <ptlib/vconvert.h>
#include <ptclib/memfile.h>
#include <ptclib/random.h>
#include <ptclib/http.h>
#include <ptclib/mediafile.h>
#include <ptclib/simplescript.h>
#include <ptclib/SignLanguageAnalyser.h>


#define PTraceModule() "VXML"


static PConstString const ApplicationScope("application");
static PConstString const DocumentScope("document");
static PConstString const DialogScope("dialog");
static PConstString const PropertyScope("property");
static PConstString const SessionScope("session");

static PConstString const TimeoutProperty("timeout");
static PConstString const BargeInProperty("bargein");
static PConstString const CachingProperty("caching");
static PConstString const InputModesProperty("inputmodes");
static PConstString const InterDigitTimeoutProperty("interdigittimeout");
static PConstString const TermTimeoutProperty("termtimeout");
static PConstString const TermCharProperty("termchar");
static PConstString const CompleteTimeoutProperty("completetimeout");
static PConstString const IncompleteTimeoutProperty("incompletetimeout");

static PConstCaselessString const SafeKeyword("safe");

static PConstString const FormElement("form");
static PConstString const MenuElement("menu");
static PConstString const PromptElement("prompt");
static PConstString const FilledElement("filled");
static PConstString const NameAttribute("name");
static PConstString const IdAttribute("id");
static PConstString const ExprAttribute("expr");
static PConstString const SrcAttribute("src");
static PConstString const NextAttribute("next");
static PConstString const DtmfAttribute("dtmf");
static PConstString const VoiceAttribute("voice");
static PConstString const DestAttribute("dest");
static PConstString const DestExprAttribute("destexpr");
static PConstString const ErrorSemantic("error.semantic");
static PConstString const ErrorBadFetch("error.badfetch");

static PConstCaselessString const SRGS("application/srgs");


class PVXMLChannelPCM : public PVXMLChannel
{
  PCLASSINFO(PVXMLChannelPCM, PVXMLChannel);

  public:
    PVXMLChannelPCM();

    // overrides from PVXMLChannel
    virtual PString GetAudioFormat() const;
    virtual unsigned GetSampleRate() const;
    virtual bool SetSampleRate(unsigned rate);
    virtual unsigned GetChannels() const;
    virtual bool SetChannels(unsigned channels);
    virtual PBoolean WriteFrame(const void * buf, PINDEX len);
    virtual PBoolean ReadFrame(void * buffer, PINDEX amount);
    virtual PINDEX CreateSilenceFrame(void * buffer, PINDEX amount);
    virtual PBoolean IsSilenceFrame(const void * buf, PINDEX len) const;
    virtual void GetBeepData(PBYTEArray & data, unsigned ms);

  protected:
    unsigned m_sampleRate;
    unsigned m_channels;
};


class PVXMLChannelG7231 : public PVXMLChannel
{
  PCLASSINFO(PVXMLChannelG7231, PVXMLChannel);
  public:
    PVXMLChannelG7231();

    // overrides from PVXMLChannel
    virtual PString GetAudioFormat() const;
    virtual unsigned GetSampleRate() const;
    virtual bool SetSampleRate(unsigned rate);
    virtual unsigned GetChannels() const;
    virtual bool SetChannels(unsigned channels);
    virtual PBoolean WriteFrame(const void * buf, PINDEX len);
    virtual PBoolean ReadFrame(void * buffer, PINDEX amount);
    virtual PINDEX CreateSilenceFrame(void * buffer, PINDEX amount);
    virtual PBoolean IsSilenceFrame(const void * buf, PINDEX len) const;
};


class PVXMLChannelG729 : public PVXMLChannel
{
  PCLASSINFO(PVXMLChannelG729, PVXMLChannel);
  public:
    PVXMLChannelG729();

    // overrides from PVXMLChannel
    virtual PString GetAudioFormat() const;
    virtual unsigned GetSampleRate() const;
    virtual bool SetSampleRate(unsigned rate);
    virtual unsigned GetChannels() const;
    virtual bool SetChannels(unsigned channels);
    virtual PBoolean WriteFrame(const void * buf, PINDEX len);
    virtual PBoolean ReadFrame(void * buffer, PINDEX amount);
    virtual PINDEX CreateSilenceFrame(void * buffer, PINDEX amount);
    virtual PBoolean IsSilenceFrame(const void * buf, PINDEX len) const;
};


#define TRAVERSE_NODE(name) \
  class PVXMLTraverse##name : public PVXMLNodeHandler { \
    virtual bool Start(PVXMLSession & session, PXMLElement & element) const \
    { return session.Traverse##name(element); } \
  }; \
  PFACTORY_CREATE(PVXMLNodeFactory, PVXMLTraverse##name, #name, true)

TRAVERSE_NODE(Audio);
TRAVERSE_NODE(Break);
TRAVERSE_NODE(Value);
TRAVERSE_NODE(SayAs);
TRAVERSE_NODE(Voice);
TRAVERSE_NODE(Goto);
TRAVERSE_NODE(Grammar);
TRAVERSE_NODE(ElseIf);
TRAVERSE_NODE(Else);
TRAVERSE_NODE(Exit);
TRAVERSE_NODE(Var);
TRAVERSE_NODE(Assign);
TRAVERSE_NODE(Submit);
TRAVERSE_NODE(Choice);
TRAVERSE_NODE(Property);
TRAVERSE_NODE(Disconnect);
TRAVERSE_NODE(Script);

#define TRAVERSE_NODE2(name) \
  class PVXMLTraverse##name : public PVXMLNodeHandler { \
    virtual bool Start(PVXMLSession & session, PXMLElement & element) const \
    { return session.Traverse##name(element); } \
    virtual bool Finish(PVXMLSession & session, PXMLElement & element) const \
    { return session.Traversed##name(element); } \
  }; \
  PFACTORY_CREATE(PVXMLNodeFactory, PVXMLTraverse##name, #name, true)

TRAVERSE_NODE2(Menu);
TRAVERSE_NODE2(Form);
TRAVERSE_NODE2(Field);
TRAVERSE_NODE2(Transfer);
TRAVERSE_NODE2(Record);
TRAVERSE_NODE2(Prompt);
TRAVERSE_NODE2(If);
TRAVERSE_NODE2(Block);


static PConstString const InternalEventStateAttribute("PTLibInternalEventState");

class PVXMLTraverseEvent : public PVXMLNodeHandler
{
  virtual bool Start(PVXMLSession &, PXMLElement & element) const
  {
    return element.GetAttribute(InternalEventStateAttribute).IsTrue();
  }

  virtual bool Finish(PVXMLSession & session, PXMLElement & element) const
  {
    if (!element.GetAttribute(InternalEventStateAttribute).IsTrue())
      return true;

    element.SetAttribute(InternalEventStateAttribute, false);

    // We got moved to another node
    if (session.m_currentNode != NULL)
      return false;

    // See if there is another event handler up the heirarchy
    PXMLElement * parent = element.GetParent();
    if (parent != NULL &&
        parent->GetParent() != NULL &&
        session.GoToEventHandler(*parent->GetParent(), element.GetName(), false))
      return false;

    // No other handler continue with the aprent
    session.m_currentNode = parent;
    return false;
  }
};
PFACTORY_CREATE(PVXMLNodeFactory, PVXMLTraverseEvent, "Filled", true);
PFACTORY_SYNONYM(PVXMLNodeFactory, PVXMLTraverseEvent, NoInput, "NoInput");
PFACTORY_SYNONYM(PVXMLNodeFactory, PVXMLTraverseEvent, NoMatch, "NoMatch");
PFACTORY_SYNONYM(PVXMLNodeFactory, PVXMLTraverseEvent, Error, "Error");
PFACTORY_SYNONYM(PVXMLNodeFactory, PVXMLTraverseEvent, Catch, "Catch");

#if PTRACING
class PVXMLTraverseLog : public PVXMLNodeHandler {
  virtual bool Start(PVXMLSession & session, PXMLElement & node) const
  {
    unsigned level = node.GetAttribute("level").AsUnsigned();
    if (level == 0)
      level = 3;
    PString log;
    session.EvaluateExpr(node, ExprAttribute, log);
    PTRACE_IF(level, !log.IsEmpty(), "VXML-Log", log);
    return true;
  }
};
PFACTORY_CREATE(PVXMLNodeFactory, PVXMLTraverseLog, "Log", true);
#endif


#define new PNEW


#define SMALL_BREAK_MSECS   1000
#define MEDIUM_BREAK_MSECS  2500
#define LARGE_BREAK_MSECS   5000


//////////////////////////////////////////////////////////

#if P_VXML_VIDEO

#define SIGN_LANGUAGE_PREVIEW_SCRIPT_FUNCTION "SignLanguageAnalyserPreview"
#define SIGN_LANGUAGE_CONTROL_SCRIPT_FUNCTION "SignLanguageAnalyserControl"

class PVXMLSignLanguageAnalyser : public PProcessStartup
{
  PDECLARE_READ_WRITE_MUTEX(m_mutex);
  PString      m_colourFormat;
  vector<bool> m_instancesInUse;


  struct Library : PDynaLink
  {
    EntryPoint<SLInitialiseFn> SLInitialise;
    EntryPoint<SLReleaseFn>    SLRelease;
    EntryPoint<SLAnalyseFn>    SLAnalyse;
    EntryPoint<SLPreviewFn>    SLPreview;
    EntryPoint<SLControlFn>    SLControl;
    EntryPoint<SLErrorTextFn>  SLErrorText;

    Library(const PFilePath & dllName)
      : PDynaLink(dllName)
      , P_DYNALINK_ENTRY_POINT(SLInitialise)
      , P_DYNALINK_OPTIONAL_ENTRY_POINT(SLRelease)
      , P_DYNALINK_ENTRY_POINT(SLAnalyse)
      , P_DYNALINK_OPTIONAL_ENTRY_POINT(SLPreview)
      , P_DYNALINK_OPTIONAL_ENTRY_POINT(SLControl)
      , P_DYNALINK_OPTIONAL_ENTRY_POINT(SLErrorText)
    {
    }

    ~Library()
    {
      if (SLRelease.IsPresent()) {
        PTRACE_PARAM(int result =) SLRelease();
        PTRACE(result < 0 ? 2 : 3, "Released Sign Language Analyser dynamic library"
                                  " \"" << GetName(true) << "\", result=" << result);
      }
    }
  } *m_library;

public:
  PVXMLSignLanguageAnalyser()
    : m_library(NULL)
  {
  }


  ~PVXMLSignLanguageAnalyser()
  {
    delete m_library; // Should always be NULL
  }


  PFACTORY_GET_SINGLETON(PProcessStartupFactory, PVXMLSignLanguageAnalyser);


  virtual void OnShutdown()
  {
    delete m_library;
    m_library = NULL;
  }


  bool CheckError(int code PTRACE_PARAM(, const char * msg))
  {
    if (code >= 0)
      return true;

#if PTRACING
    static unsigned const Level = 2;
    if (PTrace::CanTrace(2)) {
      ostream & trace = PTRACE_BEGIN(Level);
      trace << "Error " << code;
      if (m_library != NULL && m_library->SLErrorText.IsPresent()) {
        SLErrorData errorData;
        errorData.m_code = code;
        memset(errorData.m_text, 0, sizeof(errorData.m_text));
        m_library->SLErrorText(&errorData);
        if (errorData.m_text[0] != '\0')
          trace << " \"" << errorData.m_text << '"';
      }
      trace << ' ' << msg
            << " Sign Language Analyser dynamic library \"" << m_library->GetName(true) << '"'
            << PTrace::End;
    }
#endif
    return false;
  }


  bool SetAnalyser(const PFilePath & dllName)
  {
    PWriteWaitAndSignal lock(m_mutex);

    if (!dllName.IsEmpty()) {
      if (m_library != NULL && dllName == m_library->GetName(true))
        return true;

      delete m_library;
      m_library = new Library(dllName);

      if (!m_library->IsLoaded())
        PTRACE(2, NULL, PTraceModule(), "Could not open Sign Language Analyser dynamic library \"" << dllName << '"');
      else {
        SLAnalyserInit init;
        memset(&init, 0, sizeof(init));
        init.m_apiVersion = SL_API_VERSION;
        if (CheckError(m_library->SLInitialise(&init) PTRACE_PARAM(, "initialising"))) {
          switch (init.m_videoFormat) {
            case SL_GreyScale:
              m_colourFormat = "Grey";
              break;
            case SL_RGB24:
              m_colourFormat = "RGB24";
              break;
            case SL_BGR24:
              m_colourFormat = "BGR24";
              break;
            case SL_RGB32:
              m_colourFormat = "RGB32";
              break;
            case SL_BGR32:
              m_colourFormat = "BGR32";
              break;
            default:
              m_colourFormat = PVideoFrameInfo::YUV420P();
              break;
          }

          m_instancesInUse.resize(init.m_maxInstances);

          PTRACE(3, "Loaded Sign Language Analyser dynamic library"
                    " \"" << m_library->GetName(true) << "\","
                    " required format: " << m_colourFormat << " (" << init.m_videoFormat << ')');
          return true;
        }
      }
    }

    delete m_library;
    m_library = NULL;
    return false;
  }


  const PString & GetColourFormat() const
  {
    return m_colourFormat;
  }


  int AllocateInstance()
  {
    PWriteWaitAndSignal lock(m_mutex);
    if (m_library == NULL)
      return INT_MAX;

    for (size_t i = 0; i < m_instancesInUse.size(); ++i) {
      if (!m_instancesInUse[i]) {
        m_instancesInUse[i] = true;
        Control(i, "Instance Allocated");
        return i;
      }
    }

    return INT_MAX;
  }


  bool ReleaseInstance(int instance)
  {
    PWriteWaitAndSignal lock(m_mutex);
    if (instance >= (int)m_instancesInUse.size())
      return false;

    Control(instance, "Instance Released");
    m_instancesInUse[instance] = false;
    return true;
  }


  int Analyse(int instance, unsigned width, unsigned height, const PTimeInterval & timestamp, const void * pixels)
  {
    PReadWaitAndSignal lock(m_mutex);
    if (m_library == NULL || instance >= (int)m_instancesInUse.size())
      return 0;

    SLAnalyserData data;
    memset(&data, 0, sizeof(data));
    data.m_instance = instance;
    data.m_width = width;
    data.m_height = height;
    data.m_timestamp = static_cast<unsigned>(timestamp.GetMicroSeconds());
    data.m_pixels = pixels;

    PTRACE(5, "Sign Language Analyse of video frame:"
              " " << width << 'x' << height << ","
              " ts=" << timestamp << ","
              " instance=" << instance);
    int result = m_library->SLAnalyse(&data);
    CheckError(result PTRACE_PARAM(, "analysing"));
    return result;
  }


  class PreviewVideoDevice;

  bool CanPreview(int instance) const
  {
    PReadWaitAndSignal lock(m_mutex);
    return m_library != NULL &&
           m_library->SLPreview.IsPresent() &&
           instance >= 0 &&
           instance < (int)m_instancesInUse.size();
  }

  bool Preview(int instance, unsigned width, unsigned height, uint8_t * pixels)
  {
    PReadWaitAndSignal lock(m_mutex);
    if (!CanPreview(instance))
      return false;

    PTRACE(5, "Sign Language Preview video frame:"
              " " << width << 'x' << height << ","
              " instance=" << instance);
    SLPreviewData data;
    memset(&data, 0, sizeof(data));
    data.m_instance = instance;
    data.m_width = width;
    data.m_height = height;
    data.m_pixels = pixels;

    return CheckError(m_library->SLPreview(&data) PTRACE_PARAM(, "previewing"));
  }

  int Control(unsigned instance, const PString & ctrl)
  {
    PReadWaitAndSignal lock(m_mutex);
    if (m_library == NULL || !m_library->SLControl.IsPresent())
      return INT_MIN;

    PTRACE(5, "Sign Language Control:"
           " instance=" << instance << ","
           " data=" << ctrl.ToLiteral());
    SLControlData data;
    data.m_instance = instance;
    data.m_control = ctrl;
    int result = m_library->SLControl(&data);
    CheckError(result PTRACE_PARAM(, "controlling"));
    return result;
  }
};

PFACTORY_CREATE_SINGLETON(PProcessStartupFactory, PVXMLSignLanguageAnalyser);


class PVXMLSignLanguageAnalyser::PreviewVideoDevice : public PVideoInputEmulatedDevice
{
  const PVXMLSession::VideoReceiverDevice & m_receiver;

public:
  PreviewVideoDevice(const PVXMLSession::VideoReceiverDevice & receiver)
    : m_receiver(receiver)
  {
    m_colourFormat = GetInstance().GetColourFormat();
    m_channelNumber = Channel_PlayAndRepeat;
  }

  virtual PStringArray GetDeviceNames() const
  {
    return SIGN_LANGUAGE_PREVIEW_SCRIPT_FUNCTION;
  }

  virtual PBoolean Open(const PString &, PBoolean)
  {
    return IsOpen();
  }

  virtual PBoolean IsOpen()
  {
    return PVXMLSignLanguageAnalyser::GetInstance().CanPreview(m_receiver.GetAnalayserInstance());
  }

  virtual PBoolean Close()
  {
    return true;
  }

  virtual PBoolean SetFrameSize(unsigned width, unsigned height)
  {
    return width == m_frameWidth && height == m_frameHeight;
  }

  PBoolean SetColourFormat(const PString & colourFormat)
  {
    return colourFormat == m_colourFormat;
  }


protected:
  virtual bool InternalReadFrameData(BYTE * buffer)
  {
    if (!IsOpen())
      return false;

    if (PVXMLSignLanguageAnalyser::GetInstance().Preview(m_receiver.GetAnalayserInstance(), m_frameWidth, m_frameHeight, buffer))
      return true;

    // Sign language system may not be resizing, but using input resolution, so,
    // If remote resolution changes, we need to change the preview as well
    unsigned oldWidth, oldHeight;
    GetFrameSize(oldWidth, oldHeight);
    unsigned newWidth, newHeight;
    m_receiver.GetFrameSize(newWidth, newHeight);
    if (newWidth != oldWidth || newHeight != oldHeight) {
      m_frameWidth = newWidth;
      m_frameHeight = newHeight;
      if (m_converter != NULL ? m_converter->SetSrcFrameSize(newWidth, newHeight) : SetFrameSizeConverter(oldWidth, oldHeight)) {
        PTRACE(3, "Preview resolution change from " << oldWidth << 'x' << oldHeight << " to " << newWidth << 'x' << newHeight);
        return false; // This will call us again due to m_channelNumber = Channel_PlayAndRepeat
      }
    }
    else
      PTRACE(2, "Preview failed, without resolution change.");

    m_channelNumber = Channel_PlayAndClose; // Abort!
    return false;
  }
};
#endif // P_VXML_VIDEO


//////////////////////////////////////////////////////////

PVXMLPlayable::PVXMLPlayable()
  : m_vxmlChannel(NULL)
  , m_subChannel(NULL)
  , m_repeat(1)
  , m_delay(0)
  , m_autoDelete(false)
  , m_delayDone(false)
{
  PTRACE(4, "Constructed PVXMLPlayable " << this);
}


PVXMLPlayable::~PVXMLPlayable()
{
  OnStop();
  PTRACE(4, "Destroyed PVXMLPlayable " << this);
}


PBoolean PVXMLPlayable::Open(PVXMLChannel & channel, const PString &, PINDEX delay, PINDEX repeat, PBoolean autoDelete)
{
  m_vxmlChannel = &channel;
  m_delay = delay;
  m_repeat = repeat;
  m_autoDelete = autoDelete;
  return true;
}


bool PVXMLPlayable::OnRepeat()
{
  if (PAssertNULL(m_vxmlChannel) == NULL)
    return false;

  if (m_repeat <= 1)
    return false;

  --m_repeat;
  return true;
}


bool PVXMLPlayable::OnDelay()
{
  if (m_delayDone)
    return false;

  m_delayDone = true;
  if (m_delay == 0)
    return false;

  if (PAssertNULL(m_vxmlChannel) == NULL)
    return false;

  m_vxmlChannel->SetSilence(m_delay);
  return true;
}


void PVXMLPlayable::OnStop()
{
  if (m_vxmlChannel == NULL || m_subChannel == NULL)
    return;

  PTRACE(4, "Playable stop: " << *m_subChannel);
  if (m_vxmlChannel->GetReadChannel() == m_subChannel)
    m_vxmlChannel->SetReadChannel(NULL, false, true);

  delete m_subChannel;
  m_subChannel = NULL;
}


///////////////////////////////////////////////////////////////

bool PVXMLPlayableStop::OnStart()
{
  if (m_vxmlChannel == NULL)
    return false;

  m_vxmlChannel->SetSilence(500);
  return false; // Return false so always stops
}


///////////////////////////////////////////////////////////////

PBoolean PVXMLPlayableFile::Open(PVXMLChannel & chan, const PString & fn, PINDEX delay, PINDEX repeat, PBoolean autoDelete)
{
  return chan.AdjustMediaFilename(fn, m_filePath) && PVXMLPlayable::Open(chan, fn, delay, repeat, autoDelete);
}


bool PVXMLPlayableFile::OnStart()
{
  if (PAssertNULL(m_vxmlChannel) == NULL)
    return false;

  m_subChannel = m_vxmlChannel->OpenMediaFile(m_filePath, false);
  if (m_subChannel == NULL)
    return false;

  if (m_vxmlChannel->SetReadChannel(m_subChannel, false)) {
    PTRACE(3, "Playing file \"" << m_filePath << '"');
    return true;
  }

  PTRACE(2, "Could not set read channel for file \"" << m_filePath << '"');
  return false;
}


bool PVXMLPlayableFile::OnRepeat()
{
  if (!PVXMLPlayable::OnRepeat())
    return false;

  PFile * file = dynamic_cast<PFile *>(m_subChannel);
  return PAssert(file != NULL, PLogicError) && PAssertOS(file->SetPosition(0));
}


void PVXMLPlayableFile::OnStop()
{
  PVXMLPlayable::OnStop();

  if (m_autoDelete && !m_filePath.IsEmpty()) {
    PTRACE(3, "Deleting file \"" << m_filePath << "\"");
    PFile::Remove(m_filePath);
  }
}


PFACTORY_CREATE(PFactory<PVXMLPlayable>, PVXMLPlayableFile, "file");
PFACTORY_SYNONYM(PFactory<PVXMLPlayable>, PVXMLPlayableFile, Cap, "File");


///////////////////////////////////////////////////////////////

PVXMLPlayableFileList::PVXMLPlayableFileList()
  : m_currentIndex(0)
{
}


PBoolean PVXMLPlayableFileList::Open(PVXMLChannel & chan, const PString & fileList, PINDEX delay, PINDEX repeat, PBoolean autoDelete)
{
  PStringArray list = fileList.Lines();
  for (PINDEX i = 0; i < list.GetSize(); ++i) {
    PFilePath fn;
    if (chan.AdjustMediaFilename(list[i], fn))
      m_fileNames.AppendString(fn);
  }

  if (m_fileNames.IsEmpty()) {
    PTRACE(2, "No files in list exist.");
    return false;
  }

  m_currentIndex = 0;

  return PVXMLPlayable::Open(chan, PString::Empty(), delay, ((repeat > 0) ? repeat : 1) * m_fileNames.GetSize(), autoDelete);
}


bool PVXMLPlayableFileList::OnStart()
{
  if (!PAssert(!m_fileNames.IsEmpty(), PLogicError))
    return false;

  m_filePath = m_fileNames[m_currentIndex++ % m_fileNames.GetSize()];
  return PVXMLPlayableFile::OnStart();
}


bool PVXMLPlayableFileList::OnRepeat()
{
  if (!PVXMLPlayable::OnRepeat())
    return false;

  PVXMLPlayable::OnStop();
  return OnStart();
}


void PVXMLPlayableFileList::OnStop()
{
  m_filePath.MakeEmpty();

  PVXMLPlayableFile::OnStop();

  if (m_autoDelete)  {
    for (PINDEX i = 0; i < m_fileNames.GetSize(); ++i) {
      PTRACE(3, "Deleting file \"" << m_fileNames[i] << "\"");
      PFile::Remove(m_fileNames[i]);
    }
  }
}

PFACTORY_CREATE(PFactory<PVXMLPlayable>, PVXMLPlayableFileList, "FileList");


///////////////////////////////////////////////////////////////

#if P_PIPECHAN

PBoolean PVXMLPlayableCommand::Open(PVXMLChannel & chan, const PString & cmd, PINDEX delay, PINDEX repeat, PBoolean autoDelete)
{
  if (cmd.IsEmpty()) {
    PTRACE(2, "Empty command line.");
    return false;
  }

  m_command = cmd;
  return PVXMLPlayable::Open(chan, cmd, delay, repeat, autoDelete);
}


bool PVXMLPlayableCommand::OnStart()
{
  if (PAssertNULL(m_vxmlChannel) == NULL)
    return false;

  PString cmd = m_command;
  cmd.Replace("%s", PString(PString::Unsigned, m_vxmlChannel->GetSampleRate()));
  cmd.Replace("%f", m_format);

  // execute a command and send the output through the stream
  PPipeChannel * pipe = new PPipeChannel;
  if (!pipe->Open(cmd, PPipeChannel::ReadOnly)) {
    PTRACE(2, "Cannot open command \"" << cmd << '"');
    delete pipe;
    return false;
  }

  if (!pipe->Execute()) {
    PTRACE(2, "Cannot start command \"" << cmd << '"');
    return false;
  }

  PTRACE(3, "Playing command \"" << cmd << '"');
  m_subChannel = pipe;
  return m_vxmlChannel->SetReadChannel(pipe, false);
}


void PVXMLPlayableCommand::OnStop()
{
  PPipeChannel * pipe = dynamic_cast<PPipeChannel *>(m_subChannel);
  if (PAssert(pipe != NULL, PLogicError))
    pipe->WaitForTermination();

  PVXMLPlayable::OnStop();
}

PFACTORY_CREATE(PFactory<PVXMLPlayable>, PVXMLPlayableCommand, "Command");

#endif


///////////////////////////////////////////////////////////////

PBoolean PVXMLPlayableData::Open(PVXMLChannel & chan, const PString & hex, PINDEX delay, PINDEX repeat, PBoolean autoDelete)
{
  return PVXMLPlayable::Open(chan, hex, delay, repeat, autoDelete);
}


void PVXMLPlayableData::SetData(const PBYTEArray & data)
{
  m_data = data;
}


bool PVXMLPlayableData::OnStart()
{
  if (PAssertNULL(m_vxmlChannel) == NULL)
    return false;

  m_subChannel = new PMemoryFile(m_data);
  PTRACE(3, "Playing " << m_data.GetSize() << " bytes of memory");
  return m_vxmlChannel->SetReadChannel(m_subChannel, false);
}


bool PVXMLPlayableData::OnRepeat()
{
  if (!PVXMLPlayable::OnRepeat())
    return false;

  PMemoryFile * memfile = dynamic_cast<PMemoryFile *>(m_subChannel);
  return PAssert(memfile != NULL, PLogicError) && PAssertOS(memfile->SetPosition(0));
}

PFACTORY_CREATE(PFactory<PVXMLPlayable>, PVXMLPlayableData, "PCM Data");


///////////////////////////////////////////////////////////////

#if P_DTMF

PBoolean PVXMLPlayableTone::Open(PVXMLChannel & chan, const PString & toneSpec, PINDEX delay, PINDEX repeat, PBoolean autoDelete)
{
  // populate the tone buffer
  PTones tones;

  if (!tones.Generate(toneSpec)) {
    PTRACE(2, "Could not generate tones with \"" << toneSpec << '"');
    return false;
  }

  PINDEX len = tones.GetSize() * sizeof(short);
  memcpy(m_data.GetPointer(len), tones.GetPointer(), len);

  return PVXMLPlayable::Open(chan, toneSpec, delay, repeat, autoDelete);
}

PFACTORY_CREATE(PFactory<PVXMLPlayable>, PVXMLPlayableTone, "Tone");

#endif // P_DTMF


///////////////////////////////////////////////////////////////

PBoolean PVXMLPlayableHTTP::Open(PVXMLChannel & chan, const PString & url, PINDEX delay, PINDEX repeat, PBoolean autoDelete)
{
  if (!m_url.Parse(url)) {
    PTRACE(2, "Invalid URL \"" << url << '"');
    return false;
  }

  return PVXMLPlayable::Open(chan, url, delay, repeat, autoDelete);
}


bool PVXMLPlayableHTTP::OnStart()
{
  if (PAssertNULL(m_vxmlChannel) == NULL)
    return false;

  // open the resource
  PVXMLSession & session = m_vxmlChannel->GetSession();
  PHTTPClient * client = session.CreateHTTPClient();
  client->SetPersistent(false);
  PMIMEInfo outMIME, replyMIME;
  session.AddCookie(outMIME, m_url);
  int code = client->GetDocument(m_url, outMIME, replyMIME);
  if ((code != 200) || (replyMIME(PHTTP::TransferEncodingTag()) *= PHTTP::ChunkedTag())) {
    delete client;
    return false;
  }

  m_subChannel = client;
  return m_vxmlChannel->SetReadChannel(client, false);
}

PFACTORY_CREATE(PFactory<PVXMLPlayable>, PVXMLPlayableHTTP, "http");
PFACTORY_SYNONYM(PFactory<PVXMLPlayable>, PVXMLPlayableHTTP, HTTPS, "https");


///////////////////////////////////////////////////////////////

PVXMLRecordable::PVXMLRecordable()
  : m_finalSilence(3000)
  , m_maxDuration(30000)
{
}


///////////////////////////////////////////////////////////////

PBoolean PVXMLRecordableFilename::Open(const PString & arg)
{
  m_fileName = arg;
  return true;
}


bool PVXMLRecordableFilename::OnStart(PVXMLChannel & outgoingChannel)
{
  PChannel * file = outgoingChannel.OpenMediaFile(m_fileName, true);
  if (file == NULL)
    return false;

  PTRACE(3, "Recording to file \"" << m_fileName << "\","
            " duration=" << m_maxDuration << ", silence=" << m_finalSilence);
  outgoingChannel.SetWriteChannel(file, true);

  m_silenceTimer = m_finalSilence;
  m_recordTimer = m_maxDuration;
  return true;
}


PBoolean PVXMLRecordableFilename::OnFrame(PBoolean isSilence)
{
  if (isSilence) {
    if (m_silenceTimer.HasExpired()) {
      PTRACE(4, "Recording silence detected.");
      return true;
    }
  }
  else
    m_silenceTimer = m_finalSilence;

  if (m_recordTimer.HasExpired()) {
    PTRACE(3, "Recording finished due to max time exceeded.");
    return true;
  }

  return false;
}


///////////////////////////////////////////////////////////////

static PVXMLCache DefaultCache;
static const PConstString KeyFileType(".key");

PVXMLCache::PVXMLCache()
  : m_directory("cache")
{
}


void PVXMLCache::SetDirectory(const PDirectory & directory)
{
  LockReadWrite();
  m_directory = directory;
  UnlockReadWrite();
}


PFilePath PVXMLCache::CreateFilename(const PString & prefix, const PString & key, const PString & suffix)
{
  if (!m_directory.Exists()) {
    if (!m_directory.Create()) {
      PTRACE(2, "Could not create cache directory \"" << m_directory << '"');
    }
  }

  PMessageDigest5::Result digest;
  PMessageDigest5::Encode(key, digest);

  return PSTRSTRM(m_directory << prefix << '_' << hex << digest << suffix);
}


bool PVXMLCache::Get(const PString & prefix,
                     const PString & key,
                     const PString & suffix,
                         PFilePath & filename)
{
  PAssert(!prefix.IsEmpty() && !key.IsEmpty(), PInvalidParameter);

  PSafeLockReadOnly mutex(*this);

  PTextFile keyFile(CreateFilename(prefix, key, KeyFileType), PFile::ReadOnly);
  PFile dataFile(CreateFilename(prefix, key, suffix), PFile::ReadOnly);

  if (dataFile.Open()) {
    if (keyFile.Open()) {
      if (keyFile.ReadString(P_MAX_INDEX) == key) {
        if (dataFile.GetLength() != 0) {
          PTRACE(5, "Cache data found for \"" << key << '"');
          filename = dataFile.GetFilePath();
          return true;
        }
        else {
          PTRACE(2, "Cached data empty for \"" << key << '"');
        }
      }
      else {
        PTRACE(2, "Cache coherence problem for \"" << key << '"');
      }
    }
    else {
      PTRACE(2, "Cannot open cache key file \"" << keyFile.GetFilePath() << "\""
                " for \"" << key << "\", error: " << keyFile.GetErrorText());
    }
  }
  else {
    PTRACE(2, "Cannot open cache data file \"" << dataFile.GetFilePath() << "\""
              " for \"" << key << "\", error: " << dataFile.GetErrorText());
  }

  keyFile.Remove(true);
  dataFile.Remove(true);
  return false;
}


bool PVXMLCache::PutWithLock(const PString & prefix,
                             const PString & key,
                             const PString & suffix,
                                     PFile & dataFile)
{
  PSafeLockReadWrite mutex(*this);

  // create the filename for the cache files
  if (!dataFile.Open(CreateFilename(prefix, key, suffix), PFile::WriteOnly, PFile::Create|PFile::Truncate)) {
    PTRACE(2, "Cannot create cache data file \"" << dataFile.GetFilePath() << "\""
              " for \"" << key << "\", error: " << dataFile.GetErrorText());
    return false;
  }

  // write the content type file
  PTextFile keyFile(CreateFilename(prefix, key, KeyFileType), PFile::WriteOnly, PFile::Create|PFile::Truncate);
  if (keyFile.IsOpen()) {
    if (keyFile.WriteString(key)) {
      LockReadWrite();
      PTRACE(5, "Cache data created for \"" << key << '"');
      return true;
    }
    else {
      PTRACE(2, "Cannot write cache key file \"" << keyFile.GetFilePath() << "\""
                " for \"" << key << "\", error: " << keyFile.GetErrorText());
    }
  }
  else {
    PTRACE(2, "Cannot create cache key file \"" << keyFile.GetFilePath() << "\""
              " for \"" << key << "\", error: " << keyFile.GetErrorText());
  }

  dataFile.Remove(true);
  return false;
}


//////////////////////////////////////////////////////////

PVXMLSession::PVXMLSession()
  : m_textToSpeech(NULL)
  , m_ttsCache(NULL)
  , m_autoDeleteTextToSpeech(false)
#if P_VXML_VIDEO
  , m_videoReceiver(*this)
#endif
  , m_vxmlThread(NULL)
  , m_abortVXML(false)
  , m_currentNode(NULL)
  , m_speakNodeData(true)
  , m_bargeIn(true)
  , m_bargingIn(false)
  , m_promptCount(0)
  , m_defaultMenuDTMF('N') /// Disabled
  , m_scriptContext(PScriptLanguage::Create(PSimpleScript::LanguageName()))
  , m_recordingStatus(NotRecording)
  , m_recordStopOnDTMF(false)
  , m_recordingStartTime(0)
  , m_transferStatus(NotTransfering)
  , m_transferStartTime(0)
{
#if P_VXML_VIDEO
  PVideoInputDevice::OpenArgs videoArgs;
  videoArgs.driverName = P_FAKE_VIDEO_DRIVER;
  videoArgs.deviceName = P_FAKE_VIDEO_BLACK;
  m_videoSender.SetActualDevice(PVideoInputDevice::CreateOpenedDevice(videoArgs));
#endif // P_VXML_VIDEO

  // Create always present objects
  m_scriptContext->CreateComposite(PropertyScope);
  m_scriptContext->CreateComposite(SessionScope);
  m_scriptContext->CreateComposite(ApplicationScope);
  // Point dialog scope to same object as application scope
  m_scriptContext->Run(PSTRSTRM(DocumentScope << '=' << ApplicationScope));

  PTRACE(4, "Created session: " << this);
}


PVXMLSession::~PVXMLSession()
{
  PTRACE(4, "Destroying session: " << this);

  Close();

  if (m_autoDeleteTextToSpeech)
    delete m_textToSpeech;
}


PTextToSpeech * PVXMLSession::SetTextToSpeech(PTextToSpeech * tts, PBoolean autoDelete)
{
  PWaitAndSignal mutex(m_sessionMutex);

  if (m_autoDeleteTextToSpeech)
    delete m_textToSpeech;

  m_autoDeleteTextToSpeech = autoDelete;
  return m_textToSpeech = tts;
}


PTextToSpeech * PVXMLSession::SetTextToSpeech(const PString & ttsName)
{
  PFactory<PTextToSpeech>::Key_T name = (const char *)ttsName;
  if (ttsName.IsEmpty()) {
    PFactory<PTextToSpeech>::KeyList_T engines = PFactory<PTextToSpeech>::GetKeyList();
    if (engines.empty())
      return SetTextToSpeech(NULL, false);

#ifdef _WIN32
    name = "Microsoft SAPI";
    if (std::find(engines.begin(), engines.end(), name) == engines.end())
#endif
      name = engines[0];
  }

  return SetTextToSpeech(PFactory<PTextToSpeech>::CreateInstance(name), true);
}


bool PVXMLSession::SetSpeechRecognition(const PString & srName)
{
  PSpeechRecognition * sr = PSpeechRecognition::Create(srName);
  if (sr == NULL) {
    PTRACE(2, "Cannot use Speech Recognition \"" << srName << '"');
    return false;
  }
  delete sr;

  PWaitAndSignal lock(m_grammersMutex);
  m_speechRecognition = srName;
  return true;
}


PSpeechRecognition * PVXMLSession::CreateSpeechRecognition()
{
  return PSpeechRecognition::Create(m_speechRecognition);
}


void PVXMLSession::SetCache(PVXMLCache & cache)
{
  m_sessionMutex.Wait();
  m_ttsCache = &cache;
  m_sessionMutex.Signal();
}


PVXMLCache & PVXMLSession::GetCache()
{
  PWaitAndSignal mutex(m_sessionMutex);

  if (m_ttsCache == NULL)
    m_ttsCache = &DefaultCache;

  return *m_ttsCache;
}


PBoolean PVXMLSession::Load(const PString & source)
{
  // See if we have been given a file
  PFilePath file = source;
  if (PFile::Exists(file))
    return LoadFile(file);

  // see if a URL
  PURL url(source, NULL);
  if (!url.IsEmpty())
    return LoadURL(url);

  // Try and parse it as direct VXML
  return LoadVXML(source);
}


PBoolean PVXMLSession::LoadFile(const PFilePath & filename, const PString & firstForm)
{
  PTRACE(4, "Loading file: " << filename);

  PTextFile file(filename, PFile::ReadOnly);
  if (!file.IsOpen()) {
    PTRACE(1, "Cannot open " << filename);
    return false;
  }

  m_documentURL = PURL(filename);
  return InternalLoadVXML(file.ReadString(P_MAX_INDEX), firstForm);
}


bool PVXMLSession::LoadResource(const PURL & url, PBYTEArray & data)
{
  PTRACE(4, "LoadResource " << url);

  if (url.GetScheme().NumCompare("http") != EqualTo)
    return url.LoadResource(data);

  PAutoPtr<PHTTPClient> http(CreateHTTPClient());
  PMIMEInfo outMIME, replyMIME;

  AddCookie(outMIME, url);

  if (!http->GetDocument(url, outMIME, replyMIME))
    return false;

  m_cookieMutex.Wait();
  if (m_cookies.Parse(replyMIME, url)) {
    PTRACE(4, "Cookie found: " << m_cookies);
    InternalSetVar(DocumentScope, "cookie", m_cookies.AsString());
  }
  m_cookieMutex.Signal();

  return http->ReadContentBody(replyMIME, data);
}


#if P_SSL
void PVXMLSession::SetSSLCredentials(const PString & authority,
                                     const PString & certificate,
                                     const PString & privateKey)
{
  PWaitAndSignal lock(m_httpMutex);
  m_httpAuthority = authority;
  m_httpCertificate = certificate;
  m_httpPrivateKey = privateKey;
}
#endif


PHTTPClient * PVXMLSession::CreateHTTPClient() const
{
  PHTTPClient * http = new PHTTPClient("PTLib VXML Client");
#if P_SSL
  PWaitAndSignal lock(m_httpMutex);
  http->SetSSLCredentials(m_httpAuthority, m_httpCertificate, m_httpPrivateKey);
#endif
  return http;
}


void PVXMLSession::AddCookie(PMIMEInfo & mime, const PURL & url) const
{
  PWaitAndSignal lock(m_cookieMutex);
  m_cookies.AddCookie(mime, url);
}


PBoolean PVXMLSession::LoadURL(const PURL & url)
{
  PTRACE(4, "Loading URL " << url);

  // retreive the document (may be a HTTP get)
  PBYTEArray xmlData;
  if (LoadResource(url, xmlData)) {
    m_documentURL = url;
    return InternalLoadVXML(PString(xmlData), url.GetFragment());
  }

  PTRACE(1, "Cannot load document " << url);
  return false;
}


PBoolean PVXMLSession::LoadVXML(const PString & xmlText, const PString & firstForm)
{
  m_documentURL = PString::Empty();
  return InternalLoadVXML(xmlText, firstForm);
}


bool PVXMLSession::InternalLoadVXML(const PString & xmlText, const PString & firstForm)
{
  PTRACE(4, "Loading VXML: " << xmlText.length() << " bytes\n" << xmlText);

  PWaitAndSignal mutex(m_sessionMutex);

  m_speakNodeData = true;
  m_bargeIn = true;
  m_bargingIn = false;
  m_recordingStatus = NotRecording;
  m_transferStatus = NotTransfering;
  m_currentNode = NULL;

  FlushInput();

  // parse the XML
  PAutoPtr<PXML> xml(new PXML);
  if (!xml->Load(xmlText)) {
    m_lastXMLError = PSTRSTRM(m_documentURL <<
                              '(' << xml->GetErrorLine() <<
                              ':' << xml->GetErrorColumn() << ")"
                              " " << xml->GetErrorString());
    PTRACE(1, "Cannot parse root document: " << m_lastXMLError);
    return false;
  }

  PXMLElement * root = xml->GetRootElement();
  if (root == NULL) {
    PTRACE(1, "No root element");
    return false;
  }

  if (root->GetName() != "vxml") {
    PTRACE(1, "Invalid root element: " << root->PrintTrace());
    return false;
  }

  m_xmlLanguage = root->GetAttribute("xml:lang");

  if (firstForm.IsEmpty()) {
    if (root->GetElement(FormElement) == NULL && root->GetElement(MenuElement) == NULL) {
      PTRACE(1, "No form or menu element.");
      return false;
    }
  }
  else {
    if (root->GetElement(FormElement, IdAttribute, firstForm) == NULL) {
      PTRACE(1, "No form or menu element with id=\"" << firstForm << '"');
      return false;
    }
  }

  if (m_rootURL.IsEmpty())
    m_rootURL = m_documentURL;

  if (m_currentXML.get() == NULL)
    m_currentXML.transfer(xml);
  else
    m_newXML.transfer(xml);
  m_newFormName = firstForm;
  InternalStartThread();
  return true;
}


PURL PVXMLSession::NormaliseResourceName(const PString & src)
{
  if (src.empty())
    return src;

  PURL url;
  if (url.Parse(src, NULL) && !url.GetRelativePath())
    return url;

  PURL path = m_documentURL;
  path.ChangePath(PString::Empty()); // Remove last element of document URL

  if (url.IsEmpty())
    path.AppendPathStr(src);
  else if (path.GetScheme() == url.GetScheme())
    path.AppendPathSegments(url.GetPath());
  else
    return url;

  return path;
}


void PVXMLSession::ClearScopes()
{
  static char const * const keeperScopeNames[] = { SessionScope, ApplicationScope, DocumentScope };
  static PStringSet const keeperScopes(PARRAYSIZE(keeperScopeNames), keeperScopeNames);
  while (!m_scriptContext->GetScopeChain().empty()) {
    if (keeperScopes.Contains(m_scriptContext->GetScopeChain().back()))
      break;
    m_scriptContext->PopScopeChain(true);
  }
}


bool PVXMLSession::SetCurrentForm(const PString & searchId, bool fullURI)
{
  ClearBargeIn();
  m_eventCount.clear();
  m_promptCount = 0;

  PTRACE(4, "Setting current form to \"" << searchId << '"');

  PString id = searchId;

  if (fullURI) {
    ClearScopes();

    if (searchId.IsEmpty()) {
      PTRACE(3, "Full URI required for this form/menu search");
      return false;
    }

    if (searchId[0] != '#') {
      PTRACE(4, "Searching form/menu \"" << searchId << '"');
      return LoadURL(NormaliseResourceName(searchId));
    }

    id = searchId.Mid(1);
  }

  // Only handle search of top level nodes for <form>/<menu> element
  // NOTE: should have some flag to know if it is loaded
  PXMLElement * root = m_currentXML->GetRootElement();
  if (root != NULL) {
    for (PINDEX i = 0; i < root->GetSize(); i++) {
      PXMLObject * xmlObject = root->GetElement(i);
      if (xmlObject->IsElement()) {
        PXMLElement * xmlElement = (PXMLElement*)xmlObject;
        if (
              (xmlElement->GetName() == FormElement || xmlElement->GetName() == MenuElement) &&
              (id.IsEmpty() || (xmlElement->GetAttribute(IdAttribute) *= id))
           ) {
          PTRACE(3, "Found <" << xmlElement->GetName() << " id=\"" << xmlElement->GetAttribute(IdAttribute) << "\">");

          if (m_currentNode != NULL) {
            PXMLElement * element = m_currentNode->GetParent();
            while (element != NULL) {
              PVXMLNodeHandler * handler = PVXMLNodeFactory::CreateInstance(element->GetName());
              if (handler != NULL) {
                handler->Finish(*this, *element);
                PTRACE(4, "Processed VoiceXML element: " << element->PrintTrace());
              }
              element = element->GetParent();
            }
          }

          m_currentNode = xmlObject;
          return true;
        }
      }
    }
  }

  PTRACE(3, "No form/menu with id \"" << searchId << '"');
  return false;
}


PVXMLChannel * PVXMLSession::GetAndLockVXMLChannel()
{
  m_sessionMutex.Wait();
  if (IsOpen())
    return GetVXMLChannel();

  m_sessionMutex.Signal();
  return NULL;
}


bool PVXMLSession::Open(const PString & mediaFormat, unsigned sampleRate, unsigned channels)
{
  PVXMLChannel * chan = PFactory<PVXMLChannel>::CreateInstance(mediaFormat);
  if (chan == NULL) {
    PTRACE(1, "Cannot create VXML channel with format " << mediaFormat);
    return false;
  }

  if (!chan->Open(this, sampleRate, channels)) {
    delete chan;
    return false;
  }

  // set the underlying channel
  if (!PIndirectChannel::Open(chan, chan))
    return false;

  InternalStartThread();
  return true;
}


void PVXMLSession::InternalStartThread()
{
  PWaitAndSignal mutex(m_sessionMutex);

  if (IsOpen() && IsLoaded()) {
    if (m_vxmlThread == NULL)
      m_vxmlThread = new PThreadObj<PVXMLSession>(*this, &PVXMLSession::InternalThreadMain, false, "VXML");
    else
      Trigger();
  }
}


PBoolean PVXMLSession::Close()
{
  m_sessionMutex.Wait();

  ClearGrammars();

  // Stop condition for thread
  m_abortVXML = true;
  Trigger();
  PThread::WaitAndDelete(m_vxmlThread, 10000, &m_sessionMutex, false);

#if P_VXML_VIDEO
  m_videoReceiver.Close();
  m_videoSender.Close();
#endif // P_VXML_VIDEO

  return PIndirectChannel::Close();
}


/* These functions will add in the owner composite structures in the scripting language,
so session.connection.redirect.ani = "1234" will work even if session.connection.redirect,
or session.connection, do not yet exist. */
static bool CreateComposites(PScriptLanguage & scriptContext, const PString & fullVarName)
{
  PStringArray tokens = fullVarName.Tokenise(".");
  PString composite;
  for (size_t i = 0; i < tokens.size()-1; ++i) {
    PString token = tokens[i];
    PINDEX bracket = token.Find('[');
    if (bracket != P_MAX_INDEX) {
      if (!scriptContext.CreateComposite(composite + token.Left(bracket),
                                         token.Mid(bracket+1).AsUnsigned()+1))
        return false;
    }
    composite += token;
    if (!scriptContext.CreateComposite(composite))
      return false;
    composite += '.';
  }
  return true;
}

static bool CreateScriptVariable(PScriptLanguage & scriptContext, const PString & fullVarName, const PString & value)
{
  if (!CreateComposites(scriptContext, fullVarName))
    return false;

  size_t nameLen = fullVarName.length();
  size_t valueLen = value.length();
  if (fullVarName.NumCompare(".$", 2, nameLen-2) != PObject::EqualTo &&
      value.NumCompare(".$", 2, valueLen-2) != PObject::EqualTo)
    return scriptContext.SetString(fullVarName, value);

  if (value.empty())
    return true;

  if (!CreateComposites(scriptContext, value))
    return false;

  return scriptContext.Run(PSTRSTRM(fullVarName.Left(nameLen-2) << '=' << value.Left(valueLen-2)));
}


void PVXMLSession::InternalThreadMain()
{
  PTRACE(4, "Execution thread started.");

  m_sessionMutex.Wait();

  static const char * Languages[] = { "JavaScript", "Lua" };
  PScriptLanguage * newScript = PScriptLanguage::CreateOne(PStringArray(PARRAYSIZE(Languages), Languages));
  if (newScript != NULL) {
    newScript->CreateComposite(PropertyScope);
    newScript->CreateComposite(SessionScope);
    newScript->CreateComposite(ApplicationScope);
    #if P_VXML_VIDEO
      newScript->SetFunction(SIGN_LANGUAGE_PREVIEW_SCRIPT_FUNCTION, PCREATE_NOTIFIER(SignLanguagePreviewFunction));
      newScript->SetFunction(SIGN_LANGUAGE_CONTROL_SCRIPT_FUNCTION, PCREATE_NOTIFIER(SignLanguageControlFunction));
    #endif

    PSimpleScript * simpleScript = dynamic_cast<PSimpleScript *>(m_scriptContext.get());
    if (simpleScript != NULL) {
      PStringToString variables = simpleScript->GetAllVariables();
      for (PStringToString::iterator it = variables.begin(); it != variables.end(); ++it)
        CreateScriptVariable(*newScript, it->first, it->second);
    }
    m_scriptContext.reset(newScript);
  }

  m_scriptContext->PushScopeChain(SessionScope, false);
  m_scriptContext->PushScopeChain(ApplicationScope, false);

  PTime now;
  InternalSetVar(SessionScope, "time", now.AsString());
  InternalSetVar(SessionScope, "timeISO8601", now.AsString(PTime::ShortISO8601));
  InternalSetVar(SessionScope, "timeEpoch", now.GetTimeInSeconds());

  InternalSetVar(PropertyScope, TimeoutProperty , "10s");
  InternalSetVar(PropertyScope, BargeInProperty, true);
  InternalSetVar(PropertyScope, CachingProperty, SafeKeyword);
  InternalSetVar(PropertyScope, InputModesProperty, DtmfAttribute & VoiceAttribute);
  InternalSetVar(PropertyScope, InterDigitTimeoutProperty, "5s");
  InternalSetVar(PropertyScope, TermTimeoutProperty, "0");
  InternalSetVar(PropertyScope, TermCharProperty, "#");
  InternalSetVar(PropertyScope, CompleteTimeoutProperty, "2s");
  InternalSetVar(PropertyScope, IncompleteTimeoutProperty, "5s");

  InternalStartVXML(false);

  while (!m_abortVXML) {
    // process current node in the VXML script
    bool processChildren = ProcessNode();

    /* wait for something to happen, usually output of some audio. But under
        some circumstances we want to abort the script, but we  have to make
        sure the script has been run to the end so submit actions etc. can be
        performed. Record and audio and other user interaction commands can
        be skipped, so we don't wait for them */
    do {
      while (ProcessEvents())
        ;
    } while (NextNode(processChildren));

    if (m_newXML.get() != NULL) {
      /* Replace old XML with new, now it is safe to do so and no XML elements
         pointers are being referenced by the code any more */
      InternalStartVXML(true);
    }

    // Determine if we should quit
    if (m_currentNode != NULL)
      continue;

    PTRACE(3, "End of VoiceXML elements.");

    m_sessionMutex.Signal();
    OnEndDialog();
    m_sessionMutex.Wait();

    // Wait for anything OnEndDialog plays to complete.
    while (ProcessEvents())
      ;

    if (m_newXML.get() != NULL) {
      // OnEndDialog() loaded some more XML to do.
      InternalStartVXML(true);
    }

    if (m_currentNode == NULL)
      m_abortVXML = true;
  }

  m_sessionMutex.Signal();

  OnEndSession();

  PTRACE(4, "Execution thread ended");
}


void PVXMLSession::OnEndDialog()
{
}


void PVXMLSession::OnEndSession()
{
  Close();
}


bool PVXMLSession::ProcessEvents()
{
  // m_sessionMutex already locked

  if (m_abortVXML || !IsOpen())
    return false;

  PVXMLChannel * vxmlChannel = GetVXMLChannel();
  if (PAssertNULL(vxmlChannel) == NULL)
    return false;

  PString input;

  m_userInputMutex.Wait();
  if (!m_userInputQueue.empty()) {
    input = m_userInputQueue.front();
    m_userInputQueue.pop();
  }
  m_userInputMutex.Signal();

  if (!input.empty()) {
    if (m_recordingStatus == RecordingInProgress) {
      PTRACE(3, "Recording ended via user input \"" << input << '"');
      if (m_recordStopOnDTMF && vxmlChannel->EndRecording(false)) {
        if (!m_recordingName.IsEmpty())
          InternalSetVar(m_recordingName, "termchar", input[0]);
      }
    }
    else if (m_bargeIn) {
      PTRACE(3, "Executing bargein with user input \"" << input << '"');
      m_bargingIn = true;
      vxmlChannel->FlushQueue();
      OnUserInputToGrammars(input);
    }
    else if (vxmlChannel->IsPlaying()) {
      PTRACE(3, "No bargein, ignoring user input \"" << input << '"');
    }
    else {
      PTRACE(3, "Handling user input \"" << input << '"');
      OnUserInputToGrammars(input);
    }
  }

  if (vxmlChannel->IsPlaying()) {
    PTRACE(4, "Is playing, awaiting event");
  }
  else if (vxmlChannel->IsRecording()) {
    PTRACE(4, "Is recording, awaiting event");
  }
  else if (IsGrammarRunning()) {
    PTRACE(4, "Grammar awaiting input, awaiting event");
  }
  else if (m_transferStatus == TransferInProgress) {
    PTRACE(4, "Transfer in progress, awaiting event");
  }
  else {
    PTRACE(4, "Nothing happening, processing next node");
    return false;
  }

  m_sessionMutex.Signal();
  m_waitForEvent.Wait();
  m_sessionMutex.Wait();

  if (m_newXML.get() == NULL)
    return true;

  PTRACE(4, "XML changed, flushing queue");

  // Clear out any audio being output, so can start fresh on new VXML.
  if (IsOpen())
    GetVXMLChannel()->FlushQueue();

  return false;
}


void PVXMLSession::InternalStartVXML(bool leafDocument)
{
  ClearGrammars();

  if (leafDocument) {
    PURL appURL = NormaliseResourceName(m_newXML->GetRootElement()->GetAttribute("application"));
    if (appURL.IsEmpty() || appURL != m_rootURL)
      m_rootURL = m_documentURL;
    m_currentXML.transfer(m_newXML);
  }

  if (m_scriptContext->GetScopeChain().back() == DocumentScope)
    m_scriptContext->PopScopeChain(true);
  else
    m_scriptContext->ReleaseVariable(DocumentScope);

  if (m_rootURL == m_documentURL)
    m_scriptContext->Run(PSTRSTRM(DocumentScope << '=' << ApplicationScope));
  else
    m_scriptContext->PushScopeChain(DocumentScope, true);

  InternalSetVar(DocumentScope, "uri", m_documentURL);  // Non-standard but potentially useful

  PTRACE(4, "Processing global elements.");
  m_currentNode = m_currentXML->GetRootElement()->GetElement(0);
  do {
    PXMLElement * element;
    while ((element = dynamic_cast<PXMLElement *>(m_currentNode)) != NULL &&
           (element->GetName() == MenuElement || element->GetName() == FormElement)) {
      if (m_newFormName.empty() || m_newFormName == element->GetAttribute("id"))
        m_currentNode = NULL;
      else
        m_currentNode = m_currentNode->GetNextObject();
    }

    bool processGlobalChildren = ProcessNode();
    while (NextNode(processGlobalChildren))
      ;
  } while (m_currentNode != NULL);

  PTRACE(4, "Setting initial form \"" << m_newFormName << '"');
  SetCurrentForm(m_newFormName, false);
}


bool PVXMLSession::NextNode(bool processChildren)
{
  // m_sessionMutex already locked

  if (m_abortVXML)
    return false;

  // No more nodes
  if (m_currentNode == NULL)
    return false;

  // Check for new XML to replace current
  if (m_newXML.get() != NULL)
    return false;

  {
    PWaitAndSignal lock(m_grammersMutex);
    for (Grammars::iterator grammar = m_grammars.begin(); grammar != m_grammars.end(); ++grammar) {
      if (grammar->Process()) {
        // Note the PXML objects may have been replaced at this time,
        // pointers can't be used as we work back up the call stack.
        PTRACE(2, "Processed grammar: " << *grammar << ", moving to node " << PXMLObject::PrintTrace(m_currentNode));
        ClearGrammars();
        return false;
      }
    }
  }

  PXMLElement * element = dynamic_cast<PXMLElement *>(m_currentNode);
  if (element != NULL) {
    // if the current node has children, then process the first child
    if (processChildren && (m_currentNode = element->GetSubObject(0)) != NULL)
      return false;
  }
  else {
    // Data node
    PXMLObject * sibling = m_currentNode->GetNextObject();
    if (sibling != NULL) {
      m_currentNode = sibling;
      return false;
    }
    if ((element = m_currentNode->GetParent()) == NULL) {
      m_currentNode = NULL;
      return false;
    }
  }

  // No children, move to sibling
  do {
    PVXMLNodeHandler * handler = PVXMLNodeFactory::CreateInstance(element->GetName());
    if (handler != NULL) {
      PTRACE(4, "Finish processing VoiceXML element: " << element->PrintTrace());
      if (!handler->Finish(*this, *element)) {
        if (m_currentNode != NULL) {
          PTRACE(4, "Moved node after processing VoiceXML element:"
                    " from=" << element->PrintTrace() << ","
                    " to=" << m_currentNode->PrintTrace());
          return false;
        }

        PTRACE(4, "Continue processing VoiceXML element: " << element->PrintTrace());
        m_currentNode = element;
        return true;
      }
      PTRACE(4, "Processed VoiceXML element: " << element->PrintTrace());
    }

    if ((m_currentNode = element->GetNextObject()) != NULL)
      break;

  } while ((element = element->GetParent()) != NULL);

  return false;
}


void PVXMLSession::ClearBargeIn()
{
  PTRACE_IF(4, m_bargingIn, "Ending barge in");
  m_bargingIn = false;
}


void PVXMLSession::FlushInput()
{
  m_userInputMutex.Wait();
  while (!m_userInputQueue.empty())
    m_userInputQueue.pop();
  m_userInputMutex.Signal();
}


bool PVXMLSession::ProcessNode()
{
  // m_sessionMutex already locked

  if (m_abortVXML)
    return false;

  if (m_currentNode == NULL)
    return false;

  PXMLData * nodeData = dynamic_cast<PXMLData *>(m_currentNode);
  if (nodeData != NULL) {
    if (m_speakNodeData)
      PlayText(nodeData->GetString().Trim());
    return true;
  }

  m_speakNodeData = true;

  PXMLElement * element = dynamic_cast<PXMLElement *>(m_currentNode);
  PVXMLNodeHandler * handler = PVXMLNodeFactory::CreateInstance(element->GetName());
  if (handler == NULL) {
    PTRACE(2, "Unknown/unimplemented VoiceXML element: " << element->PrintTrace());
    return false;
  }

  PTRACE(3, "Processing VoiceXML element: " << element->PrintTrace());
  bool started = handler->Start(*this, *element);
  PTRACE_IF(4, !started, "Skipping VoiceXML element: " << element->PrintTrace());
  return started;
}


void PVXMLSession::OnUserInput(const PString & str)
{
  m_userInputMutex.Wait();
  m_userInputQueue.push(str);
  m_userInputMutex.Signal();
  Trigger();
}


PBoolean PVXMLSession::TraverseRecord(PXMLElement & element)
{
  if (!ExecuteCondition(element))
    return false;

  m_recordingStatus = NotRecording;
  return true;
}


PBoolean PVXMLSession::TraversedRecord(PXMLElement & element)
{
  if (m_abortVXML)
    return true; // True is not "good" but "done", that is move to next element in VXML.

  switch (m_recordingStatus) {
    case RecordingInProgress :
      return false;

    case RecordingComplete :
      return !GoToEventHandler(element, FilledElement, false);

    default :
      break;
  }

  static const PConstString supportedFileType(".wav");
  PCaselessString typeMIME = element.GetAttribute("type");
  if (typeMIME.IsEmpty())
    typeMIME = PMIMEInfo::GetContentType(supportedFileType);

  if (typeMIME != PMIMEInfo::GetContentType(supportedFileType)) {
    PTRACE(2, "Cannot save to file type \"" << typeMIME << '"');
    return true;
  }

  // see if we need a beep
  if (element.GetAttribute("beep").IsTrue()) {
    PBYTEArray beepData;
    GetBeepData(beepData, 1000);
    if (beepData.GetSize() != 0)
      PlayData(beepData);
  }

  PFilePath destination;

  // Get the destination filename (dest) which is a private extension, not standard VXML
  if (element.HasAttribute(DestAttribute)) {
    PURL uri;
    if (uri.Parse(element.GetAttribute(DestAttribute), "file"))
      destination = uri.AsFilePath();
  }
  else if (element.HasAttribute(DestExprAttribute)) {
    PString dest;
    if (EvaluateExpr(element, DestExprAttribute, dest, true))
      return true; // Threw exception

    if (!dest.IsEmpty()) {
      PURL uri;
      if (!uri.Parse(dest, "file"))
        return ThrowSemanticError(element, "Bad URI");
      destination = uri.AsFilePath();
    }
  }

  PString name = InternalGetName(element, false);
  if (name.empty())
    return false;

  if (destination.IsEmpty()) {
    if (!m_recordDirectory.Create()) {
      PTRACE(2, "Could not create recording directory \"" << m_recordDirectory << '"');
    }
    destination = PSTRSTRM(m_recordDirectory << name << '_' << PTime().AsString("yyyyMMdd_hhmmss") << supportedFileType);
  }

  m_recordingName = name + '$';
  InternalSetVar(m_recordingName, "type", typeMIME);
  InternalSetVar(m_recordingName, "uri", PURL(destination));
  InternalSetVar(m_recordingName, "maxtime", false);
  InternalSetVar(m_recordingName, "termchar", ' ');
  InternalSetVar(m_recordingName, "duration" , 0);
  InternalSetVar(m_recordingName, "size", 0);

  // Disable stop on DTMF if attribute explicitly false, default is true
  m_recordStopOnDTMF = !element.GetAttribute("dtmfterm").IsFalse();

  PFile::Remove(destination);

  PVXMLRecordableFilename * recordable = new PVXMLRecordableFilename();
  if (!recordable->Open(destination)) {
    delete recordable;
    return true;
  }

  recordable->SetFinalSilence(StringToTime(element.GetAttribute("finalsilence"), 3000));
  recordable->SetMaxDuration(StringToTime(element.GetAttribute("maxtime"), INT_MAX));

  if (!GetVXMLChannel()->QueueRecordable(recordable))
    return true;

  m_recordingStatus = RecordingInProgress;
  m_recordingStartTime.SetCurrentTime();
  return false; // Are recording, stay in <record> element
}


PBoolean PVXMLSession::TraverseScript(PXMLElement & element)
{
  PString src = element.GetAttribute(SrcAttribute);
  PString script = src.empty() ? element.GetData() : src;

  PTRACE(4, "Traverse <script> " << script.ToLiteral());
  return RunScript(element, script);
}


bool PVXMLSession::RunScript(PXMLElement & element, const PString & script)
{
  if (m_scriptContext->Run(script))
    return false;

  return ThrowSemanticError(element, PSTRSTRM(m_scriptContext->GetLanguageName()
                                              << " error \"" << m_scriptContext->GetLastErrorText()
                                              << "\" executing " << script.ToLiteral()));
}


bool PVXMLSession::EvaluateExpr(PXMLElement & element, const PString & attrib, PString & result, bool allowEmpty)
{
  result.MakeEmpty();

  PString expr = element.GetAttribute(attrib);
  if (expr.IsEmpty())
    return !allowEmpty && ThrowSemanticError(element, "Empty expression");

  // Optimisation, if simple string - starts/ends with quote and no other quotes in expression
  if (expr.GetLength() > 1 && expr[0] == '\'' && expr.Find('\'', 1) == (expr.GetLength()-1)) {
    result = expr(1, expr.GetLength()-2);
    PTRACE(4, "Evaluated simple expression: " << expr.ToLiteral() << " -> " << result.ToLiteral());
    return false;
  }

  static PConstString const EvalVarName("PTLibVXMLExpressionResult");
  m_scriptContext->SetString(EvalVarName, "");
  if (m_scriptContext->Run(PSTRSTRM(EvalVarName<<'='<<expr))) {
    result = m_scriptContext->GetString(EvalVarName);
    PTRACE(4, "Evaluated expression: " << expr.ToLiteral() << " -> " << result.ToLiteral());
    return false;
  }

  PTRACE(2, "Could not evaluate expression \"" << expr << "\" with script language " << m_scriptContext->GetLanguageName());
  return ThrowSemanticError(element, PSTRSTRM(m_scriptContext->GetLanguageName()
                                              << " error \"" << m_scriptContext->GetLastErrorText()
                                              << "\" evaluating " << expr.ToLiteral()));
}


PStringToString PVXMLSession::GetVariables() const
{
  PStringToString vars;
  return vars;
}


PCaselessString PVXMLSession::GetVar(const PString & varName) const
{
  static const char * const AllScopes[] = { "application.", "document.", "dialog.", "property.", "session." };
  for (PINDEX i = 0; i < PARRAYSIZE(AllScopes); ++i) {
    if (varName.NumCompare(AllScopes[i]))
      return m_scriptContext->GetString(varName);
  }
  static PConstString const EvalVarName("PTLibVXMLVarResult");
  if (m_scriptContext->Run(PSTRSTRM(EvalVarName<<'='<<varName)))
    return m_scriptContext->GetString(EvalVarName);

  PTRACE(2, "Could not get variable \"" << varName << "\" in " << m_scriptContext->GetLanguageName());
  return PString::Empty();
}


bool PVXMLSession::SetVar(const PString & varName, const PString & value)
{
  return CreateScriptVariable(*m_scriptContext, varName, value);
}


void PVXMLSession::SetConnectionVars(const PString & localURI,
                                     const PString & remoteURI,
                                     const PString & protocolName,
                                     const PString & protocolVersion,
                                     const PArray<PStringToString> & redirects,
                                     const PStringToString & aai,
                                     bool originator)
{
  m_scriptContext->CreateComposite(SessionScope);
  m_scriptContext->CreateComposite("session.connection");
  m_scriptContext->CreateComposite("session.connection.local");
  m_scriptContext->CreateComposite("session.connection.remote");
  m_scriptContext->CreateComposite("session.connection.protocol");
  m_scriptContext->CreateComposite("session.connection.aai");

  m_scriptContext->SetString("session.connection.local.uri", localURI);
  m_scriptContext->SetString("session.connection.remote.uri", remoteURI);
  m_scriptContext->SetString("session.connection.protocol.name", protocolName);
  m_scriptContext->SetString("session.connection.protocol.version", protocolVersion);
  for (size_t i = 0; i < redirects.size(); ++i) {
    m_scriptContext->CreateComposite(PSTRSTRM("session.connection.redirect[" << i << ']'));
    for (PStringToString::iterator it = redirects[i].begin(); it != redirects[i].end(); ++it)
      m_scriptContext->SetString(PSTRSTRM("session.connection.redirect[" << i << "]." << it->first), it->second);
  }
  for (PStringToString::const_iterator it = aai.begin(); it != aai.end(); ++it)
    m_scriptContext->SetString("session.connection.aai." + it->first, it->second);
  m_scriptContext->SetString("session.connection.originator",
                             originator ? "session.connection.local.$" : "session.connection.remote.$");
}


PCaselessString PVXMLSession::GetProperty(const PString & propName) const
{
  return InternalGetVar(PropertyScope, propName);
}


void PVXMLSession::SetDialogVar(const PString & varName, const PString & value)
{
  InternalSetVar(DialogScope, varName, value);
}


PCaselessString PVXMLSession::InternalGetVar(const PString & scope, const PString & varName) const
{
  return m_scriptContext->GetString(PSTRSTRM(scope << '.' << varName));
}


void PVXMLSession::InternalSetVar(const PString & scope, const PString & varName, const PString & value)
{
  m_scriptContext->SetString(PSTRSTRM(scope << '.' << varName), value);
}


PBoolean PVXMLSession::PlayFile(const PString & fn, PINDEX repeat, PINDEX delay, PBoolean autoDelete)
{
  return IsOpen() && !m_bargingIn && GetVXMLChannel()->QueuePlayable("File", fn, repeat, delay, autoDelete);
}


PBoolean PVXMLSession::PlayCommand(const PString & cmd, PINDEX repeat, PINDEX delay)
{
  return IsOpen() && !m_bargingIn && GetVXMLChannel()->QueuePlayable("Command", cmd, repeat, delay, true);
}


PBoolean PVXMLSession::PlayData(const PBYTEArray & data, PINDEX repeat, PINDEX delay)
{
  return IsOpen() && !m_bargingIn && GetVXMLChannel()->QueueData(data, repeat, delay);
}


PBoolean PVXMLSession::PlayTone(const PString & toneSpec, PINDEX repeat, PINDEX delay)
{
  return IsOpen() && !m_bargingIn && GetVXMLChannel()->QueuePlayable("Tone", toneSpec, repeat, delay, true);
}


PBoolean PVXMLSession::PlayElement(PXMLElement & element)
{
  if (m_bargingIn)
    return false;

  PString str = element.GetAttribute(SrcAttribute).Trim();
  if (str.IsEmpty()) {
    bool retval = EvaluateExpr(element, ExprAttribute, str);
    if (str.IsEmpty())
      return retval;
  }

  if (str[0] == '|')
    return PlayCommand(str.Mid(1));

  PINDEX repeat = std::max(DWORD(1), element.GetAttribute("repeat").AsUnsigned());

  // files on the local file system get loaded locally
  PURL url(str);
  if (url.GetScheme() == "file" && url.GetHostName().IsEmpty())
    return PlayFile(url.AsFilePath(), repeat);

  // get a normalised name for the resource
  bool safe = SafeKeyword == InternalGetVar(PropertyScope, CachingProperty) || SafeKeyword == element.GetAttribute(CachingProperty);

  PString fileType;
  {
    const PStringArray & path = url.GetPath();
    if (!path.IsEmpty())
      fileType = PFilePath(path[path.GetSize() - 1]).GetType();
    else
      fileType = ".dat";
  }

  if (!safe) {
    PFilePath filename;
    if (GetCache().Get("url", url.AsString(), fileType, filename))
      return PlayFile(filename, repeat);
  }

  PBYTEArray data;
  if (!LoadResource(url, data)) {
    PTRACE(2, "Cannot load resource " << url);
    return false;
  }

  PFile cacheFile;
  if (!GetCache().PutWithLock("url", url.AsString(), fileType, cacheFile))
    return false;

  // write the data in the file
  cacheFile.Write(data.GetPointer(), data.GetSize());

  GetCache().UnlockReadWrite();
  return PlayFile(cacheFile.GetFilePath(), repeat, 0, safe); // make sure we delete the file if not cacheing
}


void PVXMLSession::GetBeepData(PBYTEArray & data, unsigned ms)
{
  if (IsOpen())
    GetVXMLChannel()->GetBeepData(data, ms);
}


PBoolean PVXMLSession::PlaySilence(const PTimeInterval & timeout)
{
  return PlaySilence((PINDEX)timeout.GetMilliSeconds());
}


PBoolean PVXMLSession::PlaySilence(PINDEX msecs)
{
  PBYTEArray nothing;
  return IsOpen() && !m_bargingIn && GetVXMLChannel()->QueueData(nothing, 1, msecs);
}


PBoolean PVXMLSession::PlayStop()
{
  return IsOpen() && GetVXMLChannel()->QueuePlayable(new PVXMLPlayableStop());
}


PBoolean PVXMLSession::PlayResource(const PURL & url, PINDEX repeat, PINDEX delay)
{
  return IsOpen() && !m_bargingIn && GetVXMLChannel()->QueuePlayable(url.GetScheme().ToLower(), url.AsFilePath(), repeat, delay, false);
}


void PVXMLSession::LoadGrammar(const PString & type, const PVXMLGrammarInit & init)
{
  PCaselessString adjustedType = type.empty() ? SRGS : type;
  PVXMLGrammar * grammar = PVXMLGrammarFactory::CreateInstance(adjustedType, init);
  if (grammar == NULL) {
    PTRACE(2, "Could not set grammar of type \"" << adjustedType << '"');
    GoToEventHandler(init.m_field, "error.unsupported.builtin", true);
    return;
  }

  PVXMLGrammar::GrammarState state = grammar->GetState();
  if (state != PVXMLGrammar::Idle) {
    delete grammar;
    if (state == PVXMLGrammar::BadFetch)
      GoToEventHandler(init.m_field, ErrorBadFetch, true);
    else {
      PTRACE(2, "Illegal/unsupported grammar of type \"" << adjustedType << '"');
      GoToEventHandler(init.m_field, "error.unsupported.format", true);
    }
    return;
  }

  PTRACE(2, "Grammar set to " << *grammar << " from \"" << adjustedType << '"');
  m_grammersMutex.Wait();
  m_grammars.Append(grammar);
  m_grammersMutex.Signal();
}


void PVXMLSession::ClearGrammars()
{
  ClearBargeIn();
  PWaitAndSignal lock(m_grammersMutex);
  if (m_grammars.empty())
    return;

  PTRACE(2, "Clearing Grammars: " << m_grammars);
  for (Grammars::iterator grammar = m_grammars.begin(); grammar != m_grammars.end(); ++grammar)
    grammar->Stop();
  m_grammars.RemoveAll();
}


void PVXMLSession::StartGrammars()
{
  PWaitAndSignal lock(m_grammersMutex);
  for (Grammars::iterator it = m_grammars.begin(); it != m_grammars.end(); ++it)
    it->Start();
}


void PVXMLSession::OnUserInputToGrammars(const PString & input)
{
  PWaitAndSignal lock(m_grammersMutex);
  for (Grammars::iterator it = m_grammars.begin(); it != m_grammars.end(); ++it)
    it->OnUserInput(input);
}


void PVXMLSession::OnAudioInputToGrammars(const short * samples, size_t count)
{
  PWaitAndSignal lock(m_grammersMutex);
  for (Grammars::iterator it = m_grammars.begin(); it != m_grammars.end(); ++it)
    it->OnAudioInput(samples, count);
}


bool PVXMLSession::IsGrammarRunning() const
{
  PWaitAndSignal lock(m_grammersMutex);

  if (m_grammars.empty())
    return false;

  for (Grammars::const_iterator it = m_grammars.begin(); it != m_grammars.end(); ++it) {
    switch (it->GetState()) {
      case PVXMLGrammar::Started:
      case PVXMLGrammar::PartFill:
        break;
      default:
        return false;
    }
  }

  return true;
}


PBoolean PVXMLSession::PlayText(const PString & textToPlay,
                    PTextToSpeech::TextType type,
                                     PINDEX repeat,
                                     PINDEX delay)
{
  if (!IsOpen() || textToPlay.IsEmpty() || m_bargingIn)
    return false;

  PTRACE(4, "Converting " << textToPlay.ToLiteral() << " to speech");

  PString prefix(PString::Printf, "tts%i", type);
  bool useCache = SafeKeyword != InternalGetVar(PropertyScope, CachingProperty);

  PString fileList;

  PString suffix = GetVXMLChannel()->GetMediaFileSuffix() + ".wav";

  // Convert each line into it's own cached WAV file.
  PStringArray lines = textToPlay.Lines();
  for (PINDEX i = 0; i < lines.GetSize(); i++) {
    PString line = lines[i].Trim();
    if (line.IsEmpty())
      continue;

    // see if we have converted this text before
    if (useCache) {
      PFilePath cachedFilename;
      if (GetCache().Get(prefix, line, suffix, cachedFilename)) {
        fileList += cachedFilename + '\n';
        continue;
      }
    }

    PFile wavFile;
    if (!GetCache().PutWithLock(prefix, line, suffix, wavFile))
      continue;

    // Really want to use OpenChannel() but it isn't implemented yet.
    // So close file and just use filename.
    wavFile.Close();

    bool ok = m_textToSpeech->SetSampleRate(GetVXMLChannel()->GetSampleRate()) &&
              m_textToSpeech->SetChannels(GetVXMLChannel()->GetChannels()) &&
              m_textToSpeech->OpenFile(wavFile.GetFilePath()) &&
              m_textToSpeech->Speak(line, type) &&
              m_textToSpeech->Close();

    GetCache().UnlockReadWrite();

    if (ok)
        fileList += wavFile.GetFilePath() + '\n';
  }

  return GetVXMLChannel()->QueuePlayable("FileList", fileList, repeat, delay, !useCache);
}


void PVXMLSession::SetPause(PBoolean pause)
{
  if (IsOpen())
    GetVXMLChannel()->SetPause(pause);
}


PBoolean PVXMLSession::TraverseBlock(PXMLElement & element)
{
  unsigned col, line;
  element.GetFilePosition(col, line);
  PString anonymous = PSTRSTRM("anonymous_" << line << '_' << col);
  m_scriptContext->PushScopeChain(anonymous, true);
  return ExecuteCondition(element);
}


PBoolean PVXMLSession::TraversedBlock(PXMLElement & element)
{
  m_scriptContext->PopScopeChain(true);
  return ExecuteCondition(element);
}


PBoolean PVXMLSession::TraverseAudio(PXMLElement & element)
{
  if (PlayElement(element))
    return false; // Playing files, so don't process children

#if P_VXML_VIDEO
  SetRealVideoSender(NULL);
#endif // P_VXML_VIDEO
  return true;
}


PBoolean PVXMLSession::TraverseBreak(PXMLElement & element)
{
  // msecs is VXML 1.0
  if (element.HasAttribute("msecs"))
    return PlaySilence(element.GetAttribute("msecs").AsInteger());

  // time is VXML 2.0
  if (element.HasAttribute("time"))
    return PlaySilence(StringToTime(element.GetAttribute("time"), 1000));

  if (element.HasAttribute("size")) {
    PString size = element.GetAttribute("size");
    if (size *= "none")
      return true;
    if (size *= "small")
      return PlaySilence(SMALL_BREAK_MSECS);
    if (size *= "large")
      return PlaySilence(LARGE_BREAK_MSECS);
    return PlaySilence(MEDIUM_BREAK_MSECS);
  }

  // default to medium pause
  return PlaySilence(MEDIUM_BREAK_MSECS);
}


PBoolean PVXMLSession::TraverseValue(PXMLElement & element)
{
  PString className = element.GetAttribute("class");
  PString value;
  bool retval = EvaluateExpr(element, ExprAttribute, value);
  if (value.IsEmpty())
    return retval;
  PString voice = element.GetAttribute("voice");
  if (voice.IsEmpty())
    voice = InternalGetVar(PropertyScope, "voice");
  SayAs(className, value, voice);
  return true;
}


PBoolean PVXMLSession::TraverseSayAs(PXMLElement & element)
{
  SayAs(element.GetAttribute("class"), element.GetData());
  return true;
}


static bool UnsupportedVoiceAttribute(const PStringArray & attr)
{
  for (PINDEX i = 0; i < attr.GetSize(); ++i) {
    PCaselessString a = attr[i];
    if (a != "name" || a != "languages")
      return true;
  }
    return false;
}


PBoolean PVXMLSession::TraverseVoice(PXMLElement & element)
{
  if (m_textToSpeech == NULL)
    return true;

  PStringArray names = element.GetAttribute("name").Tokenise(" ", false);
  PStringArray languages = element.GetAttribute("languages").Tokenise(" ", false);
  if (names.empty() && languages.empty())
    return ThrowSemanticError(element, "<voice> must have name or language");

  PStringArray required = element.GetAttribute("required").Tokenise(" ", false);
  PStringArray ordering = element.GetAttribute("ordering").Tokenise(" ", false);
  if (UnsupportedVoiceAttribute(required) && UnsupportedVoiceAttribute(ordering))
    return ThrowSemanticError(element, "<voice> has unsupported required/ordering");

  if (names.empty() || languages.empty() || ordering.empty() || (ordering[0] *= "name")) {
    if (languages.empty()) {
      for (PINDEX i = 0; i < names.GetSize(); ++i) {
        if (m_textToSpeech->SetVoice(names[i]))
          return true;
      }
    }
    else if (names.empty()) {
      for (PINDEX i = 0; i < languages.GetSize(); ++i) {
        if (m_textToSpeech->SetVoice(':' + languages[i]))
          return true;
      }
    }
    else {
      for (PINDEX i = 0; i < names.GetSize(); ++i) {
        for (PINDEX j = 0; j < languages.GetSize(); ++j) {
          if (m_textToSpeech->SetVoice(PSTRSTRM(names[i] << ':' << languages[j])))
            return true;
        }
      }
    }
  }
  else {
    for (PINDEX j = 0; j < languages.GetSize(); ++j) {
      for (PINDEX i = 0; i < names.GetSize(); ++i) {
        if (m_textToSpeech->SetVoice(PSTRSTRM(names[i] << ':' << languages[j])))
          return true;
      }
    }
  }

  PString failure = element.GetAttribute("onvoicefailure");
  PTRACE(3, "Could not set voice to any of [" << setfill(',') << names << "], onvoicefailure=" << failure);

  if (failure.empty() || (failure *= "priorityselect"))
    m_textToSpeech->SetVoice(":");
  else if (failure *= "processorchoice")
    m_textToSpeech->SetVoice(PString::Empty());

  return true;
}


PBoolean PVXMLSession::TraverseGoto(PXMLElement & element)
{
  bool fullURI = false;
  PString target;

  if (element.HasAttribute("nextitem"))
    target = element.GetAttribute("nextitem");
  else if (element.HasAttribute("expritem")) {
    bool retval = EvaluateExpr(element, "expritem", target);
    if (target.IsEmpty())
      return retval;
  }
  else if (element.HasAttribute(ExprAttribute)) {
    fullURI = true;
    bool retval = EvaluateExpr(element, ExprAttribute, target);
    if (target.IsEmpty())
      return retval;
  }
  else if (element.HasAttribute(NextAttribute)) {
    fullURI = true;
    target = element.GetAttribute(NextAttribute);
  }

  if (SetCurrentForm(target, fullURI))
    return ProcessNode();

  return GoToEventHandler(element, ErrorBadFetch, true);
}


PBoolean PVXMLSession::TraverseGrammar(PXMLElement & element)
{
  m_speakNodeData = false;
  LoadGrammar(element.GetAttribute("type"), PVXMLGrammarInit(*this, *element.GetParent(), &element));
  return false; // Skip subelements
}


// Finds the proper event hander for 'noinput', 'filled', 'nomatch' and 'error'
// by searching the scope hiearchy from the current from
bool PVXMLSession::GoToEventHandler(PXMLElement & element, const PString & eventName, bool exitIfNotFound)
{
  PXMLElement * level = &element;
  PXMLElement * handler = NULL;

  unsigned actualCount = ++m_eventCount[eventName];

  // Look in all the way up the tree for a handler either explicitly or in a catch
  while ((handler = FindElementWithCount(*level, eventName, actualCount)) == NULL) {
    level = level->GetParent();
    if (level == NULL) {
      PTRACE(4, "No event handler found for \"" << eventName << '"');
      static PConstString const MatchAll(".");
      if (eventName != MatchAll) {
        PINDEX dot = eventName.FindLast('.');
        return GoToEventHandler(element, dot == P_MAX_INDEX ? static_cast<PString>(MatchAll) : eventName.Left(dot), exitIfNotFound);
      }

      if (exitIfNotFound) {
        PTRACE(3, "Exiting script.");
        m_currentNode = NULL;
      }
      return false;
    }
  }

  handler->SetAttribute(InternalEventStateAttribute, true);
  m_currentNode = handler;
  PTRACE(4, "Setting event handler to node " << handler->PrintTrace() << " for \"" << eventName << '"');
  return true;
}


bool PVXMLSession::ThrowSemanticError(PXMLElement & element, const PString & PTRACE_PARAM(reason))
{
  PTRACE(2, "Semantic error thown in " << element.PrintTrace() << " - " << reason);
  return GoToEventHandler(element, ErrorSemantic, true);
}


static unsigned GetCountAttribute(PXMLElement & element)
{
  PString str = element.GetAttribute("count");
  return str.IsEmpty() ? 1 : str.AsUnsigned();
}


PXMLElement * PVXMLSession::FindElementWithCount(PXMLElement & parent, const PString & name, unsigned count)
{
  std::map<unsigned, PXMLElement *> elements;
  PXMLElement * element;
  PINDEX idx = 0;
  while ((element = parent.GetElement(idx++)) != NULL) {
    if (element->GetName() == name)
      elements[GetCountAttribute(*element)] = element;
    else if (element->GetName() == "catch") {
      PStringArray events = element->GetAttribute("event").Tokenise(" ", false);
      for (size_t i = 0; i < events.size(); ++i) {
        if (events[i] *= name) {
          elements[GetCountAttribute(*element)] = element;
          break;
        }
      }
    }
  }

  if (elements.empty())
    return NULL;

  while (count > 0) {
    std::map<unsigned, PXMLElement *>::iterator it = elements.find(count);
    if (it != elements.end() && ExecuteCondition(*it->second))
      return it->second;
    --count;
  }

  return NULL;
}


void PVXMLSession::SayAs(const PString & className, const PString & text)
{
  SayAs(className, text, InternalGetVar(PropertyScope, "voice"));
}


void PVXMLSession::SayAs(const PString & className, const PString & textToSay, const PString & voice)
{
  if (m_textToSpeech != NULL)
    m_textToSpeech->SetVoice(voice);

  PString text = textToSay.Trim();
  if (!text.IsEmpty()) {
    PTextToSpeech::TextType type = PTextToSpeech::Literal;

    if (className *= "digits")
      type = PTextToSpeech::Digits;

    else if (className *= "literal")
      type = PTextToSpeech::Literal;

    else if (className *= "number")
      type = PTextToSpeech::Number;

    else if (className *= "currency")
      type = PTextToSpeech::Currency;

    else if (className *= "time")
      type = PTextToSpeech::Time;

    else if (className *= "date")
      type = PTextToSpeech::Date;

    else if (className *= "phone")
      type = PTextToSpeech::Phone;

    else if (className *= "ipaddress")
      type = PTextToSpeech::IPAddress;

    else if (className *= "duration")
      type = PTextToSpeech::Duration;

    PlayText(text, type);
  }
}


PTimeInterval PVXMLSession::StringToTime(const PString & str, int dflt)
{
  if (str.IsEmpty())
    return dflt;

  PCaselessString units = str.Mid(str.FindSpan("0123456789")).Trim();
  if (units ==  "s")
    return PTimeInterval(0, str.AsInteger());
  else if (units ==  "m")
    return PTimeInterval(0, 0, str.AsInteger());
  else if (units ==  "h")
    return PTimeInterval(0, 0, 0, str.AsInteger());

  return str.AsInt64();
}


bool PVXMLSession::ExecuteCondition(PXMLElement & element)
{
  PString result;
  return EvaluateExpr(element, "cond", result, true) || result.IsEmpty() || result.IsTrue();
}


static PXMLObject * NextElseIfNode(PXMLElement & ifElement, PINDEX pos)
{
  PXMLObject * previousObject = ifElement.GetSubObject(pos);
  for (PINDEX i = pos; i < ifElement.GetSize(); ++i) {
    PXMLObject * thisObject = ifElement.GetSubObject(i);
    if (thisObject->IsElement()) {
      PXMLElement * thisElement = dynamic_cast<PXMLElement *>(thisObject);
      if (thisElement->GetName() == "else")
        return thisObject;
      if (thisElement->GetName() == "elseif") {
        if (previousObject->IsElement()) {
          // Need a non-element as previous, so NextNode() logic works
          previousObject = new PXMLData(PString::Empty());
          previousObject->SetParent(&ifElement);
          ifElement.GetSubObjects().InsertAt(i, previousObject);
        }
        return previousObject;
      }
    }
    previousObject = thisObject;
  }

  return NULL;
}


static PConstString const IfStateAttributeName("PTLibInternalIfState");

PBoolean PVXMLSession::TraverseIf(PXMLElement & element)
{
  // If 'cond' parameter evaluates to true, enter child entities, else
  // go to next element.

  if (ExecuteCondition(element)) {
    element.SetAttribute(IfStateAttributeName, "done");
    return true;
  }

  PXMLObject * nextNode = NextElseIfNode(element, 0);
  if (nextNode == NULL)
    return false; // Skip if subelements

  m_currentNode = nextNode;
  return true; // Do subelements
}


PBoolean PVXMLSession::TraversedIf(PXMLElement & element)
{
  element.SetAttribute(IfStateAttributeName, PString::Empty());
  return true;
}


PBoolean PVXMLSession::TraverseElseIf(PXMLElement & element)
{
  PXMLElement * ifElement = element.GetParent();
  PString state = ifElement->GetAttribute(IfStateAttributeName);
  if (state == "done") {
    m_currentNode = element.GetParent();
    return false;
  }

  if (ExecuteCondition(element)) {
    ifElement->SetAttribute(IfStateAttributeName, "done");
    return true;
  }

  PXMLObject * nextNode = NextElseIfNode(*ifElement, ifElement->FindObject(&element)+1);
  if (nextNode == NULL)
    return false; // Skip if subelements

  m_currentNode = nextNode;
  return true; // Do subelements
}


PBoolean PVXMLSession::TraverseElse(PXMLElement & element)
{
  m_currentNode = element.GetParent();
  return false;
}


PBoolean PVXMLSession::TraverseExit(PXMLElement &)
{
  PTRACE(2, "Exiting, fast forwarding through script");
  m_abortVXML = true;
  Trigger();
  return true;
}


PBoolean PVXMLSession::TraverseSubmit(PXMLElement & element)
{
  PString urlStr;
  if (element.HasAttribute(ExprAttribute)) {
    bool retval = EvaluateExpr(element, ExprAttribute, urlStr);
    if (urlStr.IsEmpty())
      return retval;
  }
  else if (element.HasAttribute(NextAttribute))
    urlStr = element.GetAttribute(NextAttribute);
  else
    return ThrowSemanticError(element, "<submit> does not contain \"next\" or \"expr\" attribute.");

  PURL url;
  if (!url.Parse(urlStr)) {
    PTRACE(1, "<submit> has an invalid URL.");
    return GoToEventHandler(element, ErrorBadFetch, true);
  }

  bool urlencoded;
  PCaselessString str = element.GetAttribute("enctype");
  if (str.IsEmpty() || str == PHTTP::FormUrlEncoded())
    urlencoded = true;
  else if (str == "multipart/form-data")
    urlencoded = false;
  else {
    PTRACE(1, "<submit> has unknown \"enctype\" attribute of \"" << str << '"');
    return GoToEventHandler(element, ErrorBadFetch, true);
  }

  bool get;
  str = element.GetAttribute("method");
  if (str.IsEmpty())
    get = urlencoded;
  else if (str == "GET")
    get = true;
  else if (str == "POST")
    get = false;
  else {
    PTRACE(1, "<submit> has unknown \"method\" attribute of \"" << str << '"');
    return GoToEventHandler(element, ErrorBadFetch, true);
  }

  PAutoPtr<PHTTPClient> http(CreateHTTPClient());
  http->SetReadTimeout(StringToTime(element.GetAttribute("fetchtimeout"), 10000));

  PStringSet namelist = element.GetAttribute("namelist").Tokenise(" \t", false);
  if (namelist.IsEmpty())
    namelist = m_dialogFieldNames;

  PMIMEInfo sendMIME, replyMIME;
  AddCookie(sendMIME, url);

  if (get) {
    for (PStringSet::iterator it = namelist.begin(); it != namelist.end(); ++it)
      url.SetQueryVar(*it, GetVar(*it));

    PString body;
    if (http->GetDocument(url, sendMIME, replyMIME) && http->ReadContentBody(replyMIME, body)) {
      PTRACE(4, "<submit> GET " << url << " succeeded and returned body size=" << body.length());
      return InternalLoadVXML(body, PString::Empty());
    }

    PTRACE(1, "<submit> GET " << url << " failed with "
           << http->GetLastResponseCode() << ' ' << http->GetLastResponseInfo());
    return GoToEventHandler(element, ErrorBadFetch, true);
  }

  if (urlencoded) {
    PStringToString vars;
    for (PStringSet::iterator it = namelist.begin(); it != namelist.end(); ++it)
      vars.SetAt(*it, GetVar(*it));

    PString replyBody;
    if (http->PostData(url, sendMIME, vars, replyMIME, replyBody)) {
      PTRACE(4, "<submit> POST " << url << " succeeded and returned body:\n" << replyBody);
      return InternalLoadVXML(replyBody, PString::Empty());
    }

    PTRACE(1, "<submit> POST " << url << " failed with "
           << http->GetLastResponseCode() << ' ' << http->GetLastResponseInfo());
    return GoToEventHandler(element, PSTRSTRM(ErrorBadFetch << '.' << url.GetScheme() << '.' << http->GetLastResponseCode()), true);
  }

  // Put in boundary
  PString boundary = "--------012345678901234567890123458VXML";
  sendMIME.SetAt( PHTTP::ContentTypeTag(), "multipart/form-data; boundary=" + boundary);

  // After this all boundaries have a "--" prepended
  boundary.Splice("--", 0, 0);

  PStringStream entityBody;

  for (PStringSet::iterator itName = namelist.begin(); itName != namelist.end(); ++itName) {
    PString type = GetVar(*itName + ".type");
    if (type.empty()) {
      PMIMEInfo part1;

      part1.Set(PMIMEInfo::ContentTypeTag, PMIMEInfo::TextPlain());
      part1.Set(PMIMEInfo::ContentDispositionTag, PSTRSTRM("form-data; name=\"" << *itName << "\"; "));

      entityBody << "--" << boundary << "\r\n"
                 << part1 << GetVar(*itName) << "\r\n";

      continue;
    }

    if (GetVar(*itName + ".type") != "audio/wav" ) {
      PTRACE(1, "<submit> does not (yet) support submissions of types other than \"audio/wav\"");
      continue;
    }

    PFile file(GetVar(*itName + ".filename"), PFile::ReadOnly);
    if (!file.IsOpen()) {
      PTRACE(1, "<submit> could not find file \"" << file.GetFilePath() << '"');
      continue;
    }

    PMIMEInfo part1, part2;
    part1.Set(PMIMEInfo::ContentTypeTag, "audio/wav");
    part1.Set(PMIMEInfo::ContentDispositionTag,
              "form-data; name=\"voicemail\"; filename=\"" + file.GetFilePath().GetFileName() + '"');
    // Make PHP happy?
    // Anyway, this shows how to add more variables, for when namelist containes more elements
    part2.Set(PMIMEInfo::ContentDispositionTag, "form-data; name=\"MAX_FILE_SIZE\"\r\n\r\n3000000");

    entityBody << "--" << boundary << "\r\n"
               << part1 << "\r\n"
               << file.ReadString(file.GetLength())
               << "--" << boundary << "\r\n"
               << part2
               << "\r\n";
  }

  if (entityBody.IsEmpty()) {
    PTRACE(1, "<submit> could not find anything to send using \"" << setfill(',') << namelist << '"');
    return false;
  }

  PString replyBody;
  if (http->PostData(url, sendMIME, entityBody, replyMIME, replyBody)) {
    PTRACE(1, "<submit> POST " << url << " succeeded and returned body:\n" << replyBody);
    return InternalLoadVXML(replyBody, PString::Empty());
  }

  PTRACE(1, "<submit> POST " << url << " failed with "
         << http->GetLastResponseCode() << ' ' << http->GetLastResponseInfo());
  return false;
}


PString PVXMLSession::InternalGetName(PXMLElement & element, bool allowScope)
{
  PString name = element.GetAttribute(NameAttribute);

  if (name.empty()) {
    ThrowSemanticError(element, "No \"name\" attribute, or attribute empty");
    return PString::Empty();
  }

  if (!isalpha(name[0]) || name[name.length()-1] == '$') {
    ThrowSemanticError(element, PSTRSTRM("Illegal \"name\" attribute \"" << name << '"'));
    return PString::Empty();
  }

  if (allowScope)
    return name;

  if (name.find('.') != string::npos) {
    ThrowSemanticError(element, PSTRSTRM("Scope not allowed in \"name\" attribute \"" << name << '"'));
    return PString::Empty();
  }

  return name;
}


PBoolean PVXMLSession::TraverseProperty(PXMLElement & element)
{
  PString name = InternalGetName(element, false);
  if (name.empty())
    return false;

  InternalSetVar(PropertyScope, name, element.GetAttribute("value"));
  return true;
}


PBoolean PVXMLSession::TraverseTransfer(PXMLElement & element)
{
  if (!ExecuteCondition(element))
    return false;

  m_transferStatus = NotTransfering;
  return true;
}


PBoolean PVXMLSession::TraversedTransfer(PXMLElement & element)
{
  PString elementName = InternalGetName(element, false);
  if (elementName.empty())
    return false;

  bool error = true;

  switch (m_transferStatus) {
    case TransferCompleted :
      return true;

    case NotTransfering :
    {
      TransferType type = BridgedTransfer;
      if (element.GetAttribute("bridge").IsFalse())
        type = BlindTransfer;
      else {
        PCaselessString typeStr = element.GetAttribute("type");
        if (typeStr == "blind")
          type = BlindTransfer;
        else if (typeStr == "consultation")
          type = ConsultationTransfer;
      }

      m_transferStartTime.SetCurrentTime();

      bool started = false;
      if (element.HasAttribute(DestAttribute))
        started = OnTransfer(element.GetAttribute(DestAttribute), type);
      else if (element.HasAttribute(DestExprAttribute)) {
        PString str;
        bool retval = EvaluateExpr(element, DestExprAttribute, str);
        if (str.IsEmpty())
          return retval;
        started = OnTransfer(str, type);
      }

      if (started) {
        m_transferStatus = TransferInProgress;
        return false;
      }
      break;
    }

    case TransferSuccessful :
      error = false;
      // Do default case

    default :
      InternalSetVar(elementName + '$', "duration", (PTime() - m_transferStartTime).AsString(0, PTimeInterval::SecondsOnly));
  }

  m_transferStatus = TransferCompleted;

  return !GoToEventHandler(element, error ? "error" : FilledElement, error);
}


void PVXMLSession::SetTransferComplete(bool state)
{
  PTRACE(3, "Transfer " << (state ? "completed" : "failed"));
  m_transferStatus = state ? TransferSuccessful : TransferFailed;
  Trigger();
}


PBoolean PVXMLSession::TraverseMenu(PXMLElement & element)
{
  m_scriptContext->PushScopeChain(DialogScope, true);
  LoadGrammar(MenuElement, PVXMLGrammarInit(*this, element));
  m_defaultMenuDTMF = element.GetAttribute(DtmfAttribute).IsTrue() ? '1' : 'N';
  ++m_promptCount;
  return true;
}


PBoolean PVXMLSession::TraversedMenu(PXMLElement &)
{
  StartGrammars();
  ClearScopes();
  return false;
}


PBoolean PVXMLSession::TraverseChoice(PXMLElement & element)
{
  if (m_defaultMenuDTMF <= '9') {
    if (!element.HasAttribute(DtmfAttribute))
      element.SetAttribute(DtmfAttribute, m_defaultMenuDTMF);
    ++m_defaultMenuDTMF;
  }
  return true;
}


PBoolean PVXMLSession::TraverseVar(PXMLElement & element)
{
  PString name = InternalGetName(element, false);
  if (name.empty())
    return ThrowSemanticError(element, "No name");

  return RunScript(element, PSTRSTRM(m_scriptContext->GetScopeChain().back() << '.' << name
                                     << '=' << element.GetAttribute(ExprAttribute)));
}


PBoolean PVXMLSession::TraverseAssign(PXMLElement & element)
{
  PString name = InternalGetName(element, true);
  if (name.empty())
    return ThrowSemanticError(element, "No name");

  return RunScript(element, PSTRSTRM(name << '=' << element.GetAttribute(ExprAttribute)));
}


PBoolean PVXMLSession::TraverseDisconnect(PXMLElement & element)
{
  return GoToEventHandler(element, "connection.disconnect", true);
}


PBoolean PVXMLSession::TraverseForm(PXMLElement &)
{
  m_dialogFieldNames.clear();
  m_scriptContext->PushScopeChain(DialogScope, true);
  ++m_promptCount;
  return true;
}


PBoolean PVXMLSession::TraversedForm(PXMLElement &)
{
  ClearScopes();
  return true;
}


PBoolean PVXMLSession::TraversePrompt(PXMLElement & element)
{
  PXMLElement * validPrompt = FindElementWithCount(*element.GetParent(), PromptElement, m_promptCount);
  if (validPrompt == NULL || GetCountAttribute(*validPrompt) != GetCountAttribute(element)) {
    PTRACE(3, "Prompt count/cond attribute preventing execution: " << element.PrintTrace());
    return false;
  }

  if (element.GetAttribute(BargeInProperty).IsFalse() || InternalGetVar(PropertyScope, BargeInProperty).IsFalse()) {
    PTRACE(3, "Prompt bargein disabled.");
    m_bargeIn = false;
    ClearBargeIn();
    FlushInput();
  }
  return true;
}


PBoolean PVXMLSession::TraversedPrompt(PXMLElement& element)
{
  // Update timeout of current recognition (if 'timeout' attribute is set)
  m_grammersMutex.Wait();
  for (Grammars::iterator it = m_grammars.begin(); it != m_grammars.end(); ++it)
    it->SetTimeout(element.GetAttribute(TimeoutProperty));
  m_grammersMutex.Signal();

  m_bargeIn = !InternalGetVar(PropertyScope, BargeInProperty).IsFalse();
  return true;
}


PBoolean PVXMLSession::TraverseField(PXMLElement & element)
{
  if (!ExecuteCondition(element))
    return false;

  PString name = InternalGetName(element, false);
  if (name.empty())
    return false;

  if (m_dialogFieldNames.Contains(name)) {
    ThrowSemanticError(element, PSTRSTRM("Already has name \"" << name << '"'));
    return false;
  }

  m_dialogFieldNames += name;

  m_scriptContext->CreateComposite(PSTRSTRM(DialogScope << '.' << name << '$'));
  SetDialogVar(name, PString::Empty());

  PString type = element.GetAttribute("type");
  if (!type.empty())
    LoadGrammar(type, PVXMLGrammarInit(*this, element));
  return true;
}


PBoolean PVXMLSession::TraversedField(PXMLElement &)
{
  StartGrammars();
  return false;
}


void PVXMLSession::OnEndRecording(PINDEX bytesRecorded, bool timedOut)
{
  if (!m_recordingName.IsEmpty()) {
    InternalSetVar(m_recordingName, "duration" , (PTime() - m_recordingStartTime).GetMilliSeconds());
    InternalSetVar(m_recordingName, "size", bytesRecorded);
    InternalSetVar(m_recordingName, "maxtime", timedOut);
  }

  m_recordingStatus = RecordingComplete;
  Trigger();
}


void PVXMLSession::Trigger()
{
  PTRACE(4, "Event triggered");
  m_waitForEvent.Signal();
}


#if P_VXML_VIDEO

bool PVXMLSession::SetSignLanguageAnalyser(const PString & dllName)
{
  return PVXMLSignLanguageAnalyser::GetInstance().SetAnalyser(dllName);
}


void PVXMLSession::SetRealVideoSender(PVideoInputDevice * device)
{
  if (device != NULL && !device->IsOpen()) {
    delete device;
    device = NULL;
  }

  if (device == NULL) {
    PVideoInputDevice::OpenArgs videoArgs;
    videoArgs.driverName = P_FAKE_VIDEO_DRIVER;
    videoArgs.deviceName = P_FAKE_VIDEO_TEXT "=" + PProcess::Current().GetName();
    device = PVideoInputDevice::CreateOpenedDevice(videoArgs);
  }

  m_videoSender.SetActualDevice(device);
}


void PVXMLSession::SignLanguagePreviewFunction(PScriptLanguage &, PScriptLanguage::Signature &)
{
  SetRealVideoSender(new PVXMLSignLanguageAnalyser::PreviewVideoDevice(m_videoReceiver));
}


void PVXMLSession::SignLanguageControlFunction(PScriptLanguage &, PScriptLanguage::Signature & sig)
{
  int instance = m_videoReceiver.GetAnalayserInstance();
  PString control;
  if (!sig.m_arguments.empty())
    control = sig.m_arguments.front().AsString();

  int result = PVXMLSignLanguageAnalyser::GetInstance().Control(instance, control);
  sig.m_results.push_back(PVarType(result));

  PTRACE(3, SIGN_LANGUAGE_CONTROL_SCRIPT_FUNCTION "(" << instance << ',' << control.ToLiteral() << ") = " << result);
}


PVXMLSession::VideoReceiverDevice::VideoReceiverDevice(PVXMLSession & vxmlSession)
  : m_vxmlSession(vxmlSession)
  , m_analayserInstance(PVXMLSignLanguageAnalyser::GetInstance().AllocateInstance())
{
  m_colourFormat = PVXMLSignLanguageAnalyser::GetInstance().GetColourFormat();
}


PStringArray PVXMLSession::VideoReceiverDevice::GetDeviceNames() const
{
  return "SignLanguageAnalyser";
}


PBoolean PVXMLSession::VideoReceiverDevice::Open(const PString &, PBoolean)
{
  return IsOpen();
}


PBoolean PVXMLSession::VideoReceiverDevice::IsOpen()
{
  return m_analayserInstance >= 0;
}


PBoolean PVXMLSession::VideoReceiverDevice::Close()
{
  if (!IsOpen())
    return false;

  if (PVXMLSignLanguageAnalyser::GetInstance().ReleaseInstance(m_analayserInstance)) {
    PTRACE(3, "Closing SignLanguageAnalyser instance " << m_analayserInstance);
  }

  m_analayserInstance = -1;
  return true;
}


PBoolean PVXMLSession::VideoReceiverDevice::SetColourFormat(const PString & colourFormat)
{
  return colourFormat == m_colourFormat;
}


PBoolean PVXMLSession::VideoReceiverDevice::SetFrameData(const FrameData & frameData)
{
  if (m_analayserInstance < 0 || frameData.partialFrame || frameData.x != 0 || frameData.y != 0)
    return false;

  const void * pixels;
  if (m_converter != NULL) {
    BYTE * storePtr = m_frameStore.GetPointer(m_converter->GetMaxDstFrameBytes());
    if (!m_converter->Convert(frameData.pixels, storePtr))
      return false;
    pixels = storePtr;
  }
  else
    pixels = frameData.pixels;

  int result = PVXMLSignLanguageAnalyser::GetInstance().Analyse(m_analayserInstance, frameData.width, frameData.height, frameData.sampleTime, pixels);
  if (result >= ' ') {
    PTRACE(4, "Sign language analyser detected " << result << " '" << (char)result << '\'');
    m_vxmlSession.OnUserInput((char)result);
  }

  return true;
}


PVXMLSession::VideoSenderDevice::VideoSenderDevice()
  : m_running(true)
{
}


void PVXMLSession::VideoSenderDevice::SetActualDevice(PVideoInputDevice * actualDevice, bool autoDelete)
{
  m_running = true;
  PVideoInputDeviceIndirect::SetActualDevice(actualDevice, autoDelete);
}


PBoolean PVXMLSession::VideoSenderDevice::Start()
{
  m_running = true;
  return PVideoInputDeviceIndirect::Start();
}


bool PVXMLSession::VideoSenderDevice::InternalGetFrameData(BYTE * buffer, PINDEX & bytesReturned, bool & keyFrame, bool wait)
{
  if (m_running && !PVideoInputDeviceIndirect::InternalGetFrameData(buffer, bytesReturned, keyFrame, wait))
    m_running = false;

  return true; // Always return true or upstream closes down straight away.
}
#endif // P_VXML_VIDEO


/////////////////////////////////////////////////////////////////////////////////////////

PVXMLGrammar::PVXMLGrammar(const PVXMLGrammarInit & init)
  : PVXMLGrammarInit(init)
  , m_recogniser(NULL)
  , m_allowDTMF(true)
  , m_fieldName(m_field.GetAttribute(NameAttribute))
  , m_terminators(m_session.GetProperty(TermCharProperty))
  , m_state(Idle)
  , m_noInputTimeout(PVXMLSession::StringToTime(m_session.GetProperty(TimeoutProperty)))
  , m_partFillTimeout(PVXMLSession::StringToTime(m_session.GetProperty(InterDigitTimeoutProperty)))
{
  PString inputModes = m_session.GetProperty(InputModesProperty);
  if (!inputModes.empty()) {
    m_allowDTMF = inputModes.Find(DtmfAttribute) != P_MAX_INDEX;

    bool allowVoice = inputModes.Find(VoiceAttribute) != P_MAX_INDEX;
    if (init.m_grammarElement != NULL) {
      PCaselessString attrib = init.m_grammarElement->GetAttribute("mode");
      if (!attrib.empty())
        allowVoice = attrib == VoiceAttribute;
    }

    if (allowVoice) {
      m_recogniser = m_session.CreateSpeechRecognition();
      if (m_recogniser == NULL)
        m_allowDTMF = true; // Turn on despite element indication, as no other way for input!
      else {
        PTimeInterval incomplete = PVXMLSession::StringToTime(m_session.GetProperty(IncompleteTimeoutProperty));
        if (incomplete < m_partFillTimeout)
          m_partFillTimeout = incomplete;
      }
    }
  }

  m_timer.SetNotifier(PCREATE_NOTIFIER(OnTimeout), "VXMLGrammar");
}


void PVXMLGrammar::OnRecognition(PSpeechRecognition &, PSpeechRecognition::Transcript transcript)
{
  if (Start()) {
    if (!m_fieldName.empty()) {
      m_session.SetDialogVar(m_fieldName+"$.utterance", PSTRSTRM(transcript.m_content));
      m_session.SetDialogVar(m_fieldName+"$.confidence", PSTRSTRM(transcript.m_confidence));
      m_session.SetDialogVar(m_fieldName+"$.inputmode", VoiceAttribute);
    }

    OnInput(transcript.m_content);
  }
}


void PVXMLGrammar::OnUserInput(const PString & input)
{
  if (m_allowDTMF && Start()) {
    m_session.SetDialogVar(m_fieldName+"$.utterance", input);
    m_session.SetDialogVar(m_fieldName+"$.confidence", "1.0");
    m_session.SetDialogVar(m_fieldName+"$.inputmode", DtmfAttribute);

    for (size_t i = 0; i < input.length(); ++i)
      OnInput(input[i]);
  }
}


void PVXMLGrammar::OnAudioInput(const short * samples, size_t count)
{
  if (m_recogniser)
    m_recogniser->Listen(samples, count);
}


void PVXMLGrammar::SetTimeout(const PString & timeoutStr)
{
  PTimeInterval timeout = PVXMLSession::StringToTime(timeoutStr);
  if (timeout > 0) {
    m_noInputTimeout = timeout;
    PTRACE(4, "Grammar " << *this << " set timeout=" << m_noInputTimeout << ", state=" << m_state);
    if (m_state == Started)
      m_timer = timeout;
  }
}


bool PVXMLGrammar::Start()
{
  GrammarState prev = Idle;
  if (!m_state.compare_exchange_strong(prev, Started))
    return prev == Started || prev == PartFill;

  m_timer = m_noInputTimeout;

  if (m_recogniser != NULL) {
    PString lang;
    if (m_grammarElement != NULL)
      lang = m_grammarElement->GetAttribute("xml:lang");
    if (lang.empty())
      lang = m_session.GetLanguage();
    if (!lang.empty() && !m_recogniser->SetLanguage(lang)) {
      PTRACE(2, "Grammar " << *this << " could not set speech recognition language to \"" << lang << '"');
      m_session.GoToEventHandler(m_field, "error.unsupported.language", true);
      return false;
    }

    // Currentlly, in AWS world, it takes several seconds to create a vocabulary.
    // So we need to figure out another solution than setting it up here.
    //PStringArray words;
    //GetWordsToRecognise(words);
    //m_recogniser->SetVocabulary()
    m_recogniser->Open(PCREATE_NOTIFIER(OnRecognition));
  }

  PTRACE(3, "Grammar " << *this << " started:"
         " noInputTimeout=" << m_noInputTimeout << ","
         " partFillTimeout=" << m_partFillTimeout);
  return true;
}


void PVXMLGrammar::SetIdle()
{
  m_state = Idle;
  m_timer = m_noInputTimeout;
}


void PVXMLGrammar::SetPartFilled(const PString & input)
{
  m_value += input;

  GrammarState prev = Started;
  if (m_state.compare_exchange_strong(prev, PartFill)) {
    PTRACE(3, "Grammar " << *this << " start part fill: value=" << m_value.ToLiteral());
    m_session.ClearBargeIn();
  }
  else {
    PTRACE_IF(4, !m_value.empty(), "Grammar " << *this << " part filled: value=" << m_value.ToLiteral());
  }

  m_timer = m_partFillTimeout;
}


void PVXMLGrammar::SetFilled(const PString & input)
{
  GrammarState prev = Started;
  if (!m_state.compare_exchange_strong(prev, Filled)) {
    prev = PartFill;
    if (!m_state.compare_exchange_strong(prev, Filled)) {
      PTRACE(2, "Grammar " << *this << " in incorrect state for fill: state=" << prev);
      return;
    }
  }

  m_value += input;
  PTRACE(3, "Grammar " << *this << " filled: value=" << m_value.ToLiteral());
  m_timer.Stop(false);
  m_session.Trigger();
}


void PVXMLGrammar::Stop()
{
  if (m_recogniser != NULL) {
    m_recogniser->Close();
    delete m_recogniser;
    m_recogniser = NULL;
  }
}


void PVXMLGrammar::GetWordsToRecognise(PStringArray &) const
{
}


void PVXMLGrammar::OnTimeout(PTimer &, P_INT_PTR)
{
  GrammarState prev = Started;
  if (m_state.compare_exchange_strong(prev, NoInput))
    PTRACE(3, "Grammar " << *this << " timed out with no input");
  else {
    prev = PartFill;
    if (m_state.compare_exchange_strong(prev, Filled) && !m_value.empty())
      PTRACE(3, "Grammar " << *this << " timed out with partial input: " << m_value.ToLiteral());
    else {
      PTRACE(3, "Grammar " << *this << " timed out with no match: state=" << prev);
      m_state = NoMatch;
    }
  }
  m_session.Trigger();
}


bool PVXMLGrammar::Process()
{
  PString handler;

  // Figure out what happened
  switch (m_state.load()) {
    case Filled:
      m_timer.Stop(false);
      m_session.SetDialogVar(m_fieldName, m_value);
      handler = FilledElement;
      break;

    case NoInput:
      handler = "noinput";
      break;

    case NoMatch:
      handler = "nomatch";
      break;

    default:
      return false;
  }

  if (m_session.GoToEventHandler(m_field, FilledElement, false))
    return true;

  PTRACE(2, "Grammar: " << *this << ", restarting node " << PXMLObject::PrintTrace(&m_field));
  SetIdle();
  return false;
}


//////////////////////////////////////////////////////////////////

PFACTORY_CREATE(PVXMLGrammarFactory, PVXMLMenuGrammar, MenuElement);

PVXMLMenuGrammar::PVXMLMenuGrammar(const PVXMLGrammarInit & init)
  : PVXMLGrammar(init)
{
}


void PVXMLMenuGrammar::OnInput(const PString & input)
{
  // Sadly, this is only English
  static const char * const Numerals[] = { "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine" };
  for (PINDEX i = 0; i < PARRAYSIZE(Numerals); ++i) {
    if (input *= Numerals[i]) {
      SetFilled((char)(i+'0'));
      return;
    }
  }

  SetFilled(input);
}


bool PVXMLMenuGrammar::Process()
{
  if (m_state == Filled) {
    PXMLElement * choice;
    PINDEX index = 0;
    while ((choice = m_field.GetElement("choice", index++)) != NULL) {
      // Check if DTMF value for grammarResult matches the DTMF value for the choice
      if (choice->GetAttribute(DtmfAttribute) == m_value) {
        PTRACE(3, "Grammar " << *this << " matched menu choice: " << m_value << " to " << choice->PrintTrace());

        PString next = choice->GetAttribute(NextAttribute);
        if (next.IsEmpty()) {
          bool retval = m_session.EvaluateExpr(*choice, ExprAttribute, next);
          if (next.IsEmpty())
            return retval;
        }

        if (m_session.SetCurrentForm(next, true))
          return true;

        if (m_session.GoToEventHandler(m_field, choice->GetAttribute("event"), false))
          return true;
      }
    }

    PTRACE(3, "Grammar " << *this << " no match on menu choice: " << m_value.ToLiteral() << " on " << m_field.PrintTrace());
    m_state = NoMatch;
  }

  return PVXMLGrammar::Process();
}


void PVXMLMenuGrammar::GetWordsToRecognise(PStringArray & words) const
{
  PXMLElement * choice;
  PINDEX index = 0;
  while ((choice = m_field.GetElement("choice", index++)) != NULL) {
    if (choice->HasAttribute(DtmfAttribute))
      words.AppendString(choice->GetAttribute(DtmfAttribute));
  }
}


//////////////////////////////////////////////////////////////////

PFACTORY_CREATE(PVXMLGrammarFactory, PVXMLDigitsGrammar, "digits");
PFACTORY_SYNONYM(PVXMLGrammarFactory, PVXMLDigitsGrammar, OPAL_digits, "X-OPAL/digits");

PVXMLDigitsGrammar::PVXMLDigitsGrammar(const PVXMLGrammarInit & init)
  : PVXMLGrammar(init)
  , m_minDigits(1)
  , m_maxDigits(UINT_MAX)
{
  if (init.m_grammarElement == NULL)
    return;

  PStringOptions tokens;
  PURL::SplitVars(init.m_grammarElement->GetData(), tokens, ';', '=');
  m_minDigits = tokens.GetInteger("minDigits", 1);
  m_maxDigits = tokens.GetInteger("maxDigits", 10),
  m_terminators = tokens.GetString("terminators", m_terminators);

  PAssert(m_minDigits <= m_maxDigits, PInvalidParameter);
}


void PVXMLDigitsGrammar::OnInput(const PString & input)
{
  size_t len = m_value.length();

  // is this char the terminator?
  if (m_terminators.Find(input) != P_MAX_INDEX) {
    if (len >= m_minDigits && len <= m_maxDigits)
      SetFilled(PString::Empty());
    else {
      PTRACE(4, "Grammar " << *this << " has NOT yet been filled:"
             " value=" << m_value << ","
             " length=" << len << ","
             " min=" << m_minDigits << ","
             " max=" << m_maxDigits);
      m_state = NoMatch;
      m_session.Trigger();
    }
  }
  else if (len >= m_maxDigits)
    SetFilled(input);
  else if (len >= m_minDigits)
    SetPartFilled(input);
  else
    m_value += input; // Add to collected digits string
}


void PVXMLDigitsGrammar::GetWordsToRecognise(PStringArray & words) const
{
  for (char c = '0'; c <= '9'; ++c)
    words.AppendString(c);
  for (size_t i = 0; i < m_terminators.length(); ++i)
    words.AppendString(m_terminators[i]);
}


//////////////////////////////////////////////////////////////////

PFACTORY_CREATE(PVXMLGrammarFactory, PVXMLBooleanGrammar, "boolean");

PVXMLBooleanGrammar::PVXMLBooleanGrammar(const PVXMLGrammarInit & init)
  : PVXMLGrammar(init)
{
}


void PVXMLBooleanGrammar::OnInput(const PString & input)
{
  if (input == "1" || (input *= "yes"))
    SetFilled(PString(true));
  else if (input == "2" || (input *= "no"))
    SetFilled(PString(false));
}


void PVXMLBooleanGrammar::GetWordsToRecognise(PStringArray & words) const
{
  words.AppendString("yes");
  words.AppendString("no");
}


//////////////////////////////////////////////////////////////////

PFACTORY_CREATE(PVXMLGrammarFactory, PVXMLTextGrammar, "text");

PVXMLTextGrammar::PVXMLTextGrammar(const PVXMLGrammarInit & init)
  : PVXMLGrammar(init)
{
}


void PVXMLTextGrammar::OnRecognition(PSpeechRecognition& sr, PSpeechRecognition::Transcript transcript)
{
  if (transcript.m_final)
    PVXMLGrammar::OnRecognition(sr, transcript);
  else
    SetPartFilled(PString::Empty());
}


void PVXMLTextGrammar::OnUserInput(const PString & input)
{
  SetPartFilled(input);
}


void PVXMLTextGrammar::OnInput(const PString & input)
{
  SetFilled(input);
}


//////////////////////////////////////////////////////////////////

PFACTORY_CREATE(PVXMLGrammarFactory, PVXMLGrammarSRGS, SRGS);
PFACTORY_SYNONYM(PVXMLGrammarFactory, PVXMLGrammarSRGS, SRGS_XML, SRGS+"+xml");

PVXMLGrammarSRGS::PVXMLGrammarSRGS(const PVXMLGrammarInit & init)
  :PVXMLGrammar(init)
{
  if (init.m_grammarElement == NULL) {
    PTRACE(2, "Attempt to use SRGS outside of <grammar>");
    m_state = Illegal;
    return;
  }

  PAutoPtr<PXML> loadedXML;
  PXMLElement * grammar;
  PString src = init.m_grammarElement->GetAttribute("src");
  if (src.empty())
    grammar = init.m_grammarElement;
  else {
    loadedXML.reset(new PXML);
    PBYTEArray xmlText;
    if (!m_session.LoadResource(src, xmlText) || !loadedXML->Load(xmlText)) {
      PTRACE(2, "Could not load external <grammar> from " << src);
      m_state = BadFetch;
      return;
    }
    grammar = loadedXML->GetRootElement();
    if (grammar == NULL || grammar->GetName() != "grammar") {
      PTRACE(2, "External SRC was not a <grammar> in " << src);
      m_state = Illegal;
      return;
    }
  }

  // Load the root rule
  PString rootName = grammar->GetAttribute("root");
  if (m_rule.Parse(*grammar,
                   rootName.empty() ? grammar->GetElement("rule")
                                    : grammar->GetElement("rule", "id", rootName)))
    return;

  PTRACE(2, "Could not parse SRGS <grammar>");
  m_state = Illegal;
}


bool PVXMLGrammarSRGS::Item::Parse(PXMLElement & grammar, PXMLElement * element)
{
  if (element == NULL)
    return false;

  if (element->GetName() == "item") {
    m_token = element->GetData();
    PString minRepeat, maxRepeat;
    if (element->GetAttribute("repeat").Split('-', minRepeat, maxRepeat,
                  PString::SplitDefaultToBefore|PString::SplitBeforeNonEmpty|PString::SplitTrim)) {
      m_minRepeat = minRepeat.AsUnsigned();
      m_maxRepeat = std::max(m_minRepeat, (unsigned)maxRepeat.AsUnsigned());
    }
  }

  PINDEX idx = 0;
  PXMLElement * subElement;
  while ((subElement = element->GetElement(idx++)) != NULL) {
    if (subElement->GetName() == "item") {
      Item item;
      if (!item.Parse(grammar, subElement))
        return false;
      m_items.push_back(std::vector<Item>(1, item));
    }
    else if (subElement->GetName() == "one-of") {
      m_items.push_back(std::vector<Item>());
      std::vector<Item> & alternatives = m_items.back();
      PINDEX alt = 0;
      PXMLElement * altElement;
      while ((altElement = subElement->GetElement(alt++)) != NULL) {
        Item item;
        if (!item.Parse(grammar, altElement))
          return false;
        alternatives.push_back(item);
      }
    }
  }

  return true;
}


PVXMLGrammar::GrammarState PVXMLGrammarSRGS::Item::OnInput(const PString & input)
{
  if (m_token == input) {
    m_value = m_token;
    return PartFill;
  }

  if (m_currentItem >= m_items.size())
    return Started;

  for (std::vector<Item>::iterator alt = m_items[m_currentItem].begin(); alt != m_items[m_currentItem].end(); ++alt) {
    GrammarState state = alt->OnInput(input);
    switch (state) {
      case Started:
        break;
      case PartFill:
      case Filled:
        m_value += alt->m_value;
        if (++m_currentItem >= m_items.size())
          return Filled;
        break;
      default:
        return state;
    }
  }

  return Started;
}


void PVXMLGrammarSRGS::OnInput(const PString & input)
{
  switch (m_rule.OnInput(input)) {
    case PartFill:
      SetPartFilled(PString::Empty());
      break;
    case Filled:
      SetFilled(m_rule.m_value);
      break;
    default:
      ;
  }
}


void PVXMLGrammarSRGS::GetWordsToRecognise(PStringArray & words) const
{
  m_rule.AddWordsToRecognise(words);
}


void PVXMLGrammarSRGS::Item::AddWordsToRecognise(PStringArray & words) const
{
  if (!m_token.empty())
    words.AppendString(m_token);
  for (std::vector<Item>::const_iterator alt = m_items[m_currentItem].begin(); alt != m_items[m_currentItem].end(); ++alt)
    alt->AddWordsToRecognise(words);
}


//////////////////////////////////////////////////////////////////

PVXMLChannel::PVXMLChannel(unsigned frameDelay, PINDEX frameSize)
  : PDelayChannel(DelayReadsAndWrites, frameDelay, frameSize)
  , m_vxmlSession(NULL)
  , m_closed(false)
  , m_paused(false)
  , m_totalData(0)
  , m_recordable(NULL)
  , m_currentPlayItem(NULL)
{
  PTRACE(4, "Constructed channel " << this);
}


PBoolean PVXMLChannel::Open(PVXMLSession * session, unsigned sampleRate, unsigned channels)
{
  PTRACE(4, "Opening channel:"
            " this=" << this << ","
            " session=" << session << ","
            " format=" << GetAudioFormat() << ","
            " rate=" << sampleRate << ","
            " channels=" << channels);

  m_currentPlayItem = NULL;
  m_vxmlSession = session;
  m_silenceTimer.SetInterval(500); // 1/2 a second delay before we start outputting stuff
  return SetSampleRate(sampleRate) && SetChannels(channels);
}


PVXMLChannel::~PVXMLChannel()
{
  Close();
  PTRACE(4, "Destroyed channel " << this);
}


PBoolean PVXMLChannel::IsOpen() const
{
  return !m_closed;
}


PBoolean PVXMLChannel::Close()
{
  if (!m_closed) {
    PTRACE(4, "Closing channel " << this);

    EndRecording(true);
    FlushQueue();

    m_closed = true;

    PDelayChannel::Close();
  }

  return true;
}


PString PVXMLChannel::GetMediaFileSuffix() const
{
  PStringStream suffix;

  if (!IsMediaPCM())
    suffix << '_' << GetAudioFormat().Replace(".", "", true).ToLower();
  else {
    switch (GetChannels()) {
      case 1:
        suffix << "_mono_";
        break;
      case 2:
        suffix << "_stereo_";
        break;
      default:
        suffix << '_' << GetChannels() << 'x';
    }
    suffix << PScaleSI(GetSampleRate(), 1, "Hz");
  }

  return suffix;
}


bool PVXMLChannel::AdjustMediaFilename(const PFilePath & ifn, PFilePath & ofn)
{
  PFilePathString title = ifn.GetTitle();
  PFilePathString suffix = GetMediaFileSuffix();
  if (suffix != title.Right(suffix.GetLength())) {
    ofn = PSTRSTRM(ifn.GetDirectory() << title << suffix << ifn.GetType());
    if (PFile::Exists(ofn))
      return true;
  }

  if (PFile::Exists(ifn)) {
    ofn = ifn;
    return true;
  }

  PTRACE(2, "Playable file \"" << ifn << "\" not found.");
  return false;
}


PChannel * PVXMLChannel::OpenMediaFile(const PFilePath & fn, bool recording)
{
#if P_WAVFILE
  if (fn.GetType() == ".wav") {
    PWAVFile * wav = new PWAVFile;
    if (recording) {
      wav->SetChannels(GetChannels());
      wav->SetSampleSize(16);
      wav->SetSampleRate(GetSampleRate());
      if (!wav->SetFormat(GetAudioFormat()))
        PTRACE(2, "Unsupported codec " << GetAudioFormat());
      else if (!wav->Open(fn, PFile::WriteOnly))
        PTRACE(2, "Could not create WAV file \"" << wav->GetName() << "\" - " << wav->GetErrorText());
      else if (!wav->SetAutoconvert())
        PTRACE(2, "WAV file cannot convert to " << GetAudioFormat());
      else
        return wav;
    }
    else {
      if (!wav->Open(fn, PFile::ReadOnly))
        PTRACE(2, "Could not open WAV file \"" << wav->GetName() << "\" - " << wav->GetErrorText());
      else if (wav->GetFormatString() != GetAudioFormat() && !wav->SetAutoconvert())
        PTRACE(2, "WAV file cannot convert from " << wav->GetFormatString());
      else if (wav->GetChannels() != GetChannels())
        PTRACE(2, "WAV file has unsupported channel count " << wav->GetChannels());
      else if (wav->GetSampleSize() != 16)
        PTRACE(2, "WAV file has unsupported sample size " << wav->GetSampleSize());
      else if (wav->GetSampleRate() != GetSampleRate())
        PTRACE(2, "WAV file has unsupported sample rate " << wav->GetSampleRate());
      else
        return wav;
    }
    delete wav;
    return NULL;
  }
#endif // P_WAVFILE

#if P_MEDIAFILE
  PMediaFile::Ptr mediaFile = PMediaFile::Create(fn);
  if (!mediaFile.IsNULL()) {
    PSoundChannel * audio = new PMediaFile::SoundChannel(mediaFile);
    PSoundChannel::Params params;
    params.m_direction = recording ? PSoundChannel::Player : PSoundChannel::Recorder; // Counter intuitive
    params.m_driver = fn;
    params.m_sampleRate = GetSampleRate();
    params.m_channels = GetChannels();
    if (audio->Open(params)) {
#if P_VXML_VIDEO
      PVideoInputDevice * video = new PMediaFile::VideoInputDevice(mediaFile);
      video->SetChannel(PMediaFile::VideoInputDevice::Channel_PlayAndClose);
      if (video->Open(fn))
        m_vxmlSession->SetRealVideoSender(video);
      else
        delete video;
#endif //P_VXML_VIDEO
      return audio;
    }
    delete audio;
  }
  else
    PTRACE(2, "No media file handler for " << fn);
#endif // P_MEDIAFILE

  // Assume file just has bytes of correct media format
  PFile * file = new PFile(fn, recording ? PFile::WriteOnly : PFile::ReadOnly);
  if (file->Open(PFile::ReadOnly)) {
    return file;
  }

  PTRACE(2, "Could not open raw audio file \"" << fn << '"');
  delete file;
  return NULL;
}


PBoolean PVXMLChannel::Write(const void * buf, PINDEX len)
{
  if (m_closed)
    return false;

  m_vxmlSession->OnAudioInputToGrammars(reinterpret_cast<const short *>(buf), len/sizeof(short));

  m_recordingMutex.Wait();

  // let the recordable do silence detection
  if (m_recordable != NULL && m_recordable->OnFrame(IsSilenceFrame(buf, len)))
    EndRecording(true);

  m_recordingMutex.Signal();

  // write the data and do the correct delay
  if (WriteFrame(buf, len))
    m_totalData += GetLastWriteCount();
  else {
    EndRecording(true);
    SetLastWriteCount(len);
    Wait(len, nextWriteTick);
  }

  return true;
}


PBoolean PVXMLChannel::QueueRecordable(PVXMLRecordable * newItem)
{
  m_totalData = 0;

  // shutdown any existing recording
  EndRecording(true);

  // insert the new recordable
  PWaitAndSignal mutex(m_recordingMutex);
  m_recordable = newItem;
  m_totalData = 0;
  SetReadTimeout(frameDelay);
  return newItem->OnStart(*this);
}


PBoolean PVXMLChannel::EndRecording(bool timedOut)
{
  PWaitAndSignal mutex(m_recordingMutex);

  if (m_recordable == NULL)
    return false;

  PTRACE(3, "Finished recording " << m_totalData << " bytes");
  SetWriteChannel(NULL, false, true);
  m_recordable->OnStop();
  delete m_recordable;
  m_recordable = NULL;
  m_vxmlSession->OnEndRecording(m_totalData, timedOut);

  return true;
}


PBoolean PVXMLChannel::Read(void * buffer, PINDEX amount)
{
  for (;;) {
    if (m_closed)
      return false;

    if (m_paused || m_silenceTimer.IsRunning())
      break;

    // if the read succeeds, we are done
    if (ReadFrame(buffer, amount)) {
      m_totalData += GetLastReadCount();
#if P_VXML_VIDEO
      if (m_vxmlSession->m_videoSender.IsRunning())
        return true; // Already done real time delay
#endif
    }

    // if a timeout, send silence, try again in a bit
    if (GetErrorCode(LastReadError) == Timeout)
      break;

    // Other errors mean end of the playable
    PWaitAndSignal mutex(m_playQueueMutex);

    bool wasPlaying = false;

    // if current item still active, check for trailing actions
    if (m_currentPlayItem != NULL) {
      PTRACE(3, "Finished playing " << *m_currentPlayItem << ", " << m_totalData << " bytes");

      if (m_currentPlayItem->OnRepeat())
        continue;

      // see if end of queue delay specified
      if (m_currentPlayItem->OnDelay()) 
        break;

      // stop the current item
      m_currentPlayItem->OnStop();
      delete m_currentPlayItem;
      m_currentPlayItem = NULL;
      wasPlaying = true;
    }

    for (;;) {
      // check the queue for the next action, if none, send silence
      m_currentPlayItem = m_playQueue.Dequeue();
      if (m_currentPlayItem == NULL) {
        if (wasPlaying)
          m_vxmlSession->Trigger(); // notify about the end of queue
        goto double_break;
      }

      // start the new item
      if (m_currentPlayItem->OnStart())
        break;

      delete m_currentPlayItem;
    }

    PTRACE(4, "Started playing " << *m_currentPlayItem);
    SetReadTimeout(frameDelay);
    m_totalData = 0;
  }

double_break:
  SetLastReadCount(CreateSilenceFrame(buffer, amount));
  Wait(GetLastReadCount(), nextReadTick);
  return true;
}


void PVXMLChannel::SetSilence(unsigned msecs)
{
  PTRACE(3, "Playing silence for " << msecs << "ms");
  m_silenceTimer.SetInterval(msecs);
}


PBoolean PVXMLChannel::QueuePlayable(const PString & type,
                                 const PString & arg,
                                 PINDEX repeat,
                                 PINDEX delay,
                                 PBoolean autoDelete)
{
  if (repeat <= 0)
    repeat = 1;

  PVXMLPlayable * item = PFactory<PVXMLPlayable>::CreateInstance(type);
  if (item == NULL) {
    PBYTEArray data;
    if (!m_vxmlSession->LoadResource(arg, data)) {
      PTRACE(2, "Cannot find playable of type " << type << " - \"" << arg << '"');
      return false;
    }
  }

  if (item->Open(*this, arg, delay, repeat, autoDelete)) {
    PTRACE(3, "Enqueueing playable " << type << " with arg \"" << arg
           << "\" for playing " << repeat << " times, followed by " << delay << "ms silence");
    return QueuePlayable(item);
  }

  delete item;
  return false;
}


PBoolean PVXMLChannel::QueuePlayable(PVXMLPlayable * newItem)
{
  if (!IsOpen()) {
    delete newItem;
    return false;
  }

  m_playQueueMutex.Wait();
  m_playQueue.Enqueue(newItem);
  m_playQueueMutex.Signal();
  return true;
}


PBoolean PVXMLChannel::QueueData(const PBYTEArray & data, PINDEX repeat, PINDEX delay)
{
  PTRACE(3, "Enqueueing " << data.GetSize() << " bytes for playing, followed by " << delay << "ms silence");
  PVXMLPlayableData * item = PFactory<PVXMLPlayable>::CreateInstanceAs<PVXMLPlayableData>("PCM Data");
  if (item == NULL) {
    PTRACE(2, "Cannot find playable of type 'PCM Data'");
    delete item;
    return false;
  }

  if (!item->Open(*this, "", delay, repeat, true)) {
    PTRACE(2, "Cannot open playable of type 'PCM Data'");
    delete item;
    return false;
  }

  item->SetData(data);

  return QueuePlayable(item);
}


void PVXMLChannel::FlushQueue()
{
  PTRACE(4, "Flushing playable queue");

  PWaitAndSignal mutex(m_playQueueMutex);

  PVXMLPlayable * qItem;
  while ((qItem = m_playQueue.Dequeue()) != NULL) {
    qItem->OnStop();
    delete qItem;
  }

  if (m_currentPlayItem != NULL) {
    m_currentPlayItem->OnStop();
    delete m_currentPlayItem;
    m_currentPlayItem = NULL;
  }

  m_silenceTimer.Stop();

  PTRACE(5, "Flushed playable queue");
}


///////////////////////////////////////////////////////////////

PFACTORY_CREATE(PFactory<PVXMLChannel>, PVXMLChannelPCM, VXML_PCM16);

PVXMLChannelPCM::PVXMLChannelPCM()
  : PVXMLChannel(10, 160)
  , m_sampleRate(8000)
  , m_channels(1)
{
}


PString PVXMLChannelPCM::GetAudioFormat() const
{
  return VXML_PCM16;
}


unsigned PVXMLChannelPCM::GetSampleRate() const
{
  return m_sampleRate;
}


bool PVXMLChannelPCM::SetSampleRate(unsigned rate)
{
  if (!PAssert(rate > 0, PInvalidParameter))
    return false;

  m_sampleRate = rate;
  frameSize = rate*2*m_channels*frameDelay/1000;
  return true;
}


unsigned PVXMLChannelPCM::GetChannels() const
{
  return m_channels;
}


bool PVXMLChannelPCM::SetChannels(unsigned channels)
{
  if (!PAssert(channels > 0, PInvalidParameter))
    return false;

  m_channels = channels;
  return true;
}


PBoolean PVXMLChannelPCM::WriteFrame(const void * buf, PINDEX len)
{
  return PDelayChannel::Write(buf, len);
}


PBoolean PVXMLChannelPCM::ReadFrame(void * buffer, PINDEX amount)
{
  PINDEX len = 0;
  while (len < amount)  {
    if (!PDelayChannel::Read(len + (char *)buffer, amount-len))
      return false;
    len += GetLastReadCount();
  }

  SetLastReadCount(len);
  return true;
}


PINDEX PVXMLChannelPCM::CreateSilenceFrame(void * buffer, PINDEX amount)
{
  memset(buffer, 0, amount);
  return amount;
}


PBoolean PVXMLChannelPCM::IsSilenceFrame(const void * buf, PINDEX len) const
{
  // Calculate the average signal level of this frame
  int sum = 0;

  const short * pcmPtr = (const short *)buf;
  const short * endPtr = pcmPtr + len/2;
  while (pcmPtr != endPtr) {
    if (*pcmPtr < 0)
      sum -= *pcmPtr++;
    else
      sum += *pcmPtr++;
  }

  // calc average
  unsigned level = sum / (len / 2);

  return level < 500; // arbitrary level
}


static short beepData[] = { 0, 18784, 30432, 30400, 18784, 0, -18784, -30432, -30400, -18784 };


void PVXMLChannelPCM::GetBeepData(PBYTEArray & data, unsigned ms)
{
  data.SetSize(0);
  while (data.GetSize() < (PINDEX)(ms * 16)) {
    PINDEX len = data.GetSize();
    data.SetSize(len + sizeof(beepData));
    memcpy(len + data.GetPointer(), beepData, sizeof(beepData));
  }
}

///////////////////////////////////////////////////////////////

PFACTORY_CREATE(PFactory<PVXMLChannel>, PVXMLChannelG7231, VXML_G7231);

PVXMLChannelG7231::PVXMLChannelG7231()
  : PVXMLChannel(30, 0)
{
}


PString PVXMLChannelG7231::GetAudioFormat() const
{
  return VXML_G7231;
}


unsigned PVXMLChannelG7231::GetSampleRate() const
{
  return 8000;
}


bool PVXMLChannelG7231::SetSampleRate(unsigned rate)
{
  return rate == 8000;
}


unsigned PVXMLChannelG7231::GetChannels() const
{
  return 1;
}


bool PVXMLChannelG7231::SetChannels(unsigned channels)
{
  return channels == 1;
}


static const PINDEX g7231Lens[] = { 24, 20, 4, 1 };

PBoolean PVXMLChannelG7231::WriteFrame(const void * buffer, PINDEX actualLen)
{
  PINDEX len = g7231Lens[(*(BYTE *)buffer)&3];
  if (len > actualLen)
    return false;

  return PDelayChannel::Write(buffer, len);
}


PBoolean PVXMLChannelG7231::ReadFrame(void * buffer, PINDEX /*amount*/)
{
  if (!PDelayChannel::Read(buffer, 1))
    return false;

  PINDEX len = g7231Lens[(*(BYTE *)buffer)&3];
  if (len != 1) {
    if (!PIndirectChannel::Read(1+(BYTE *)buffer, len-1))
      return false;
    SetLastReadCount(GetLastReadCount()+1);
  }

  return true;
}


PINDEX PVXMLChannelG7231::CreateSilenceFrame(void * buffer, PINDEX /* len */)
{
  ((BYTE *)buffer)[0] = 2;
  memset(((BYTE *)buffer)+1, 0, 3);
  return 4;
}


PBoolean PVXMLChannelG7231::IsSilenceFrame(const void * buf, PINDEX len) const
{
  if (len == 4)
    return true;
  if (buf == NULL)
    return false;
  return ((*(const BYTE *)buf)&3) == 2;
}

///////////////////////////////////////////////////////////////

PFACTORY_CREATE(PFactory<PVXMLChannel>, PVXMLChannelG729, VXML_G729);

PVXMLChannelG729::PVXMLChannelG729()
  : PVXMLChannel(10, 0)
{
}


PString PVXMLChannelG729::GetAudioFormat() const
{
  return VXML_G729;
}


unsigned PVXMLChannelG729::GetSampleRate() const
{
  return 8000;
}


bool PVXMLChannelG729::SetSampleRate(unsigned rate)
{
  return rate == 8000;
}


unsigned PVXMLChannelG729::GetChannels() const
{
  return 1;
}


bool PVXMLChannelG729::SetChannels(unsigned channels)
{
  return channels == 1;
}


PBoolean PVXMLChannelG729::WriteFrame(const void * buf, PINDEX /*len*/)
{
  return PDelayChannel::Write(buf, 10);
}

PBoolean PVXMLChannelG729::ReadFrame(void * buffer, PINDEX /*amount*/)
{
  return PDelayChannel::Read(buffer, 10); // No silence frames so always 10 bytes
}


PINDEX PVXMLChannelG729::CreateSilenceFrame(void * buffer, PINDEX /* len */)
{
  memset(buffer, 0, 10);
  return 10;
}


PBoolean PVXMLChannelG729::IsSilenceFrame(const void * /*buf*/, PINDEX /*len*/) const
{
  return false;
}


#endif   // P_VXML


///////////////////////////////////////////////////////////////
