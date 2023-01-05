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
static PConstString const SessionScope("session");

static PConstString const RootURIVar("root_uri");
static PConstString const DocumentURIVar("uri");

static PConstString const TimeoutProperty("timeout");
static PConstString const BargeInProperty("bargein");
static PConstString const CachingProperty("caching");
static PConstString const InputModesProperty("inputmodes");
static PConstString const InterDigitTimeoutProperty("interdigittimeout");
static PConstString const TermTimeoutProperty("termtimeout");
static PConstString const TermCharProperty("termchar");
static PConstString const CompleteTimeoutProperty("completetimeout");
static PConstString const IncompleteTimeoutProperty("incompletetimeout");
static PConstString const FetchTimeoutProperty("fetchtimeout");
static PConstString const DocumentMaxAgeProperty("documentmaxage");
static PConstString const DocumentMaxStaleProperty("documentmaxstale");
static PConstString const AudioMaxAgeProperty("audiomaxage");
static PConstString const AudioMaxStaleProperty("audiomaxstale");
static PConstString const GrammarMaxAgeProperty("grammarmaxage");
static PConstString const GrammarMaxStaleProperty("grammarmaxstale");
static PConstString const ScriptMaxAgeProperty("scriptmaxage");
static PConstString const ScriptMaxStaleProperty("scriptmaxstale");

static PConstCaselessString const SafeKeyword("safe");

static PConstString const FormElement("form");
static PConstString const FieldElement("field");
static PConstString const MenuElement("menu");
static PConstString const ChoiceElement("choice");
static PConstString const PromptElement("prompt");
static PConstString const FilledElement("filled");
static PConstString const NoInputElement("noinput");
static PConstString const NoMatchElement("nomatch");
static PConstString const ErrorElement("error");
static PConstString const CatchElement("catch");
static PConstString const NameAttribute("name");
static PConstString const TypeAttribute("type");
static PConstString const ModeAttribute("mode");
static PConstString const LanguagesAttribute("languages");
static PConstString const GenderAttribute("gender");
static PConstString const CountAttribute("count");
static PConstString const IdAttribute("id");
static PConstString const ExprAttribute("expr");
static PConstString const SrcAttribute("src");
static PConstString const NextAttribute("next");
static PConstString const DtmfAttribute("dtmf");
static PConstString const VoiceAttribute("voice");
static PConstString const DestAttribute("dest");
static PConstString const DestExprAttribute("destexpr");
static PConstString const MaxAgeAttribute("maxage");
static PConstString const MaxStaleAttribute("maxstale");
static PConstString const EventAttribute("event");
static PConstString const EventExprAttribute("eventexpr");
static PConstString const ErrorSemantic("error.semantic");
static PConstString const ErrorBadFetch("error.badfetch");

static PConstCaselessString const SRGS("application/srgs");


#if PTRACING
  #define ThrowError2(session, element, errorName, reason) ( \
      PTRACE(2, errorName << " thrown in " << (element).PrintTrace() << " - " << reason), \
      (session).GoToEventHandler(element, errorName, true, true) \
    )
#else
  #define ThrowError2(session, element, errorName, reason) (session).GoToEventHandler(element, errorName, true, true)
#endif

#define ThrowSemanticError2(session, element, reason)  ThrowError2(session, element, ErrorSemantic, reason)
#define ThrowBadFetchError2(session, element, reason)  ThrowError2(session, element, ErrorBadFetch, reason)
#define ThrowError(element, errorName, reason)         ThrowError2(*this, element, errorName, reason)
#define ThrowSemanticError(element, reason)            ThrowError2(*this, element, ErrorSemantic, reason)
#define ThrowBadFetchError(element, reason)            ThrowError2(*this, element, ErrorBadFetch, reason)



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


static PConstString const InternalTraversingNodeAttribute("PTLibInternalTraversingNode");

bool PVXMLNodeHandler::StartTraversal(PVXMLSession & session, PXMLElement & node) const
{
//  if (IsTraversing(node))
//    return false;

  PTRACE(4, &session, "Traversing " << GetDescription() << ' ' << node.PrintTrace());
  node.SetAttribute(InternalTraversingNodeAttribute, true);
  if (node.GetName() != "property")
    session.m_properties.push_back(PVXMLSession::Properties(PTRACE_PARAM(node)));
  return true;
}

bool PVXMLNodeHandler::FinishTraversal(PVXMLSession & session, PXMLElement & node) const
{
  bool traversing = IsTraversing(node);
  if (traversing && node.GetName() != "property")
    session.m_properties.pop_back();
  PTRACE(4, &session, (traversing ? "Traversed " : "Traversal skipped ") << GetDescription() << ' ' << node.PrintTrace());
  node.SetAttribute(InternalTraversingNodeAttribute, false);
  return true;
}

bool PVXMLNodeHandler::IsTraversing(PXMLElement & node)
{
  return node.GetAttribute(InternalTraversingNodeAttribute).IsTrue();
}


#define TRAVERSE_NODE(name) \
  typedef PVXMLSinglePhaseNodeHandler<&PVXMLSession::Traverse##name> PVXMLTraverse##name; \
  PFACTORY_CREATE(PVXMLNodeFactory, PVXMLTraverse##name, #name, true)

TRAVERSE_NODE(RootNode);
TRAVERSE_NODE(Reprompt);
TRAVERSE_NODE(Audio);
TRAVERSE_NODE(Break);
TRAVERSE_NODE(Value);
TRAVERSE_NODE(Voice);
TRAVERSE_NODE(Goto);
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
  typedef PVXMLDualPhaseNodeHandler<&PVXMLSession::Traverse##name, &PVXMLSession::Traversed##name> PVXMLTraverse##name; \
  PFACTORY_CREATE(PVXMLNodeFactory, PVXMLTraverse##name, #name, true)

TRAVERSE_NODE2(Menu);
TRAVERSE_NODE2(Form);
TRAVERSE_NODE2(Field);
TRAVERSE_NODE2(Grammar);
TRAVERSE_NODE2(Transfer);
TRAVERSE_NODE2(Record);
TRAVERSE_NODE2(Prompt);
TRAVERSE_NODE2(If);
TRAVERSE_NODE2(Block);
TRAVERSE_NODE2(SayAs);
PFACTORY_SYNONYM(PVXMLNodeFactory, PVXMLTraverseSayAs, SSML, "say-as");


static PConstString const InternalEventStateAttribute("PTLibInternalEventState");
static PConstString const InternalFilledStateAttribute("PTLibInternalFilledState");

class PVXMLTraverseEvent : public PVXMLNodeHandler
{
protected:
  virtual bool StartTraversal(PVXMLSession & session, PXMLElement & element) const
  {
    if (!PVXMLNodeHandler::StartTraversal(session, element))
      return false;

    if (!element.GetAttribute(InternalEventStateAttribute).IsTrue())
      return false;

    if (session.m_promptMode != PVXMLSession::e_FinalProcessing)
      session.m_promptMode = PVXMLSession::e_EventPrompt;
    return true;
  }

  virtual bool FinishTraversal(PVXMLSession & session, PXMLElement & element) const
  {
    PVXMLNodeHandler::FinishTraversal(session, element);
    if (!element.GetAttribute(InternalEventStateAttribute).IsTrue())
      return true;

    element.SetAttribute(InternalEventStateAttribute, false);

    if (session.m_promptMode == PVXMLSession::e_EventPrompt)
      session.m_promptMode = PVXMLSession::e_NormalPrompt;

    // We got moved to another node
    if (session.m_currentNode != NULL)
      return false;

    // See if there is another event handler up the heirarchy
    PXMLElement * parent = element.GetParent();
    if (parent != NULL && parent->GetParent() != NULL) {
      PString name = element.GetName();
      if (name == CatchElement)
        name = element.GetAttribute(EventAttribute);
      if (session.GoToEventHandler(*parent->GetParent(), name, false, false))
        return false;
    }

    // No other handler continue with the aprent
    session.SetCurrentNode(parent);
    return false;
  }

#if PTRACING
  virtual const char * GetDescription() const { return EventAttribute; }
#endif
};
PFACTORY_CREATE(PVXMLNodeFactory, PVXMLTraverseEvent, CatchElement, true);
PFACTORY_SYNONYM(PVXMLNodeFactory, PVXMLTraverseEvent, NoInput, NoInputElement);
PFACTORY_SYNONYM(PVXMLNodeFactory, PVXMLTraverseEvent, NoMatch, NoMatchElement);
PFACTORY_SYNONYM(PVXMLNodeFactory, PVXMLTraverseEvent, Error, ErrorElement);

class PVXMLTraverseFilled : public PVXMLTraverseEvent
{
  virtual bool StartTraversal(PVXMLSession & session, PXMLElement & element) const
  {
    if (!PVXMLNodeHandler::StartTraversal(session, element))
      return false;

    if (!element.GetAttribute(InternalEventStateAttribute).IsTrue())
      return false;

    if (element.GetName() != FilledElement)
      return true;

    PXMLElement * parent = element.GetParent();
    if (parent == NULL)
      return ThrowBadFetchError2(session, element, "No parent on <filled>");

    PCaselessString mode = element.GetAttribute(ModeAttribute);
    PStringSet namelist = element.GetAttribute("namelist").Tokenise(" \t", false);

    if (parent->GetName() != "form") {
      if (mode.empty() && namelist.empty())
        return true;

      // Having a mode or namelist for other then <form> is illegal
      return ThrowBadFetchError2(session, element, "mode/nodelist on <filled> in " << parent->PrintTrace());
    }

    if (namelist.IsEmpty())
      namelist = session.m_dialogFieldNames;

    // Validate the mode and nodelist
    if (!mode.empty() && mode != "all" && mode != "any")
      return ThrowSemanticError2(session, element, "Invalid mode/nodelist on <filled>");

    // Move to next unfilled field
    PXMLElement * nextField = NULL;
    for (PStringSet::iterator it = namelist.begin(); it != namelist.end(); ++it) {
      PXMLElement * namedField = parent->GetElement(FieldElement, NameAttribute, *it);
      if (namedField == NULL)
        return ThrowSemanticError2(session, element, "Invalid field name on <filled> nodelist");

      if (namedField->GetAttribute(InternalFilledStateAttribute).IsTrue()) {
        if (mode == "all")
          continue;
        PTRACE(4, "An input " << namedField->PrintTrace() << " was filled for " << parent->PrintTrace());
        return true;
      }

      if (mode == "any")
        continue;

      nextField = namedField;
      break;
    }

    if (nextField == NULL) {
      PTRACE(4, "All required input fields filled for " << parent->PrintTrace());
      return true;
    }

    PTRACE(4, "Setting next input field " << nextField->PrintTrace() << " for " << parent->PrintTrace());
    return session.SetCurrentNode(nextField);
  }
};

PFACTORY_CREATE(PVXMLNodeFactory, PVXMLTraverseFilled, FilledElement, true);

#if PTRACING
class PVXMLTraverseLog : public PVXMLNodeHandler {
  virtual bool StartTraversal(PVXMLSession & session, PXMLElement & node) const
  {
    unsigned level = node.GetAttribute("level").AsUnsigned();
    if (level == 0)
      level = 3;
    PString log;
    session.EvaluateExpr(node, ExprAttribute, log);
    PTRACE_IF(level, !log.IsEmpty(), "VXML-Log", log);
    return true;
  }
  virtual bool FinishTraversal(PVXMLSession &, PXMLElement &) const
  {
    return true;
  }
#if PTRACING
  virtual const char * GetDescription() const { return NULL; }
#endif
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
  client->SetReadTimeout(session.GetTimeProperty(FetchTimeoutProperty));
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

static const PConstString KeyFileType(".key");

PVXMLCache::PVXMLCache()
  : m_directory("cache")
{
}


void PVXMLCache::SetDirectory(const PDirectory & directory)
{
  PTRACE(3, "Cache directory set to " << directory);
  LockReadWrite();
  m_directory = directory;
  UnlockReadWrite();
}


PFilePath PVXMLCache::CreateFilename(const Params & params)
{
  if (!m_directory.Exists()) {
    if (!m_directory.Create()) {
      PTRACE(2, "Could not create cache directory \"" << m_directory << '"');
    }
  }

  PMessageDigest5::Result digest;
  PMessageDigest5::Encode(params.m_key, digest);

  return PSTRSTRM(m_directory << params.m_prefix << '_' << hex << digest << params.m_suffix);
}


static PTime GetTimeFromElement(PXMLElement * root, const char * name)
{
  PXMLElement * element = root->GetElement(name);
  return element != NULL ? PTime(element->GetData()) : PTime(0);
}


bool PVXMLCache::StartCache(Params & params)
{
  PAssert(!params.m_prefix.IsEmpty() && !params.m_key.IsEmpty(), PInvalidParameter);

  if (!LockReadWrite())
    return false;

  params.m_file.SetFilePath(CreateFilename(params));
  PFilePath keyFilePath = params.m_file.GetFilePath();
  keyFilePath.SetType(KeyFileType);

  if (PFile::Exists(keyFilePath) && params.m_maxAge > 0) {
    PXML xml;
    if (!xml.LoadFile(keyFilePath)) {
      PTRACE(2, "Cannot open cache key file"
             " \"" << keyFilePath << "\""
             " (" << xml.GetErrorLine() << ':' << xml.GetErrorColumn() << ")"
             " " << xml.GetErrorString());
    }
    else if (!params.m_file.Open(PFile::ReadOnly)) {
      PTRACE(2, "Cannot open cache data file \"" << params.m_file.GetFilePath() << "\""
             " for \"" << params.m_key << "\", error: " << params.m_file.GetErrorText());
    }
    else if ((params.m_size = params.m_file.GetLength()) == 0) {
      PTRACE(2, "Cached data empty for \"" << params.m_key << '"');
    }
    else {
      PTime now;
      PXMLElement * root = xml.GetRootElement();
      PXMLElement * key = root->GetElement("key");
      PTime date = GetTimeFromElement(root, "date");
      PTime expires = GetTimeFromElement(root, "expires");

      if (key == NULL || key->GetData() != params.m_key) {
        PTRACE(2, "Cache coherence problem for \"" << params.m_key << '"');
      }
      else if (params.m_maxAge >= 0 && date.IsValid() && now > date + params.m_maxAge) {
        PTRACE(2, "Cache age reached for \"" << params.m_key << '"');
      }
      else if (params.m_maxStale >= 0 && expires.IsValid() && now > expires + params.m_maxStale) {
        PTRACE(2, "Cache stale reached for \"" << params.m_key << '"');
      }
      else {
        PTRACE(4, "Cache data found for \"" << params.m_key << '"');
        // Leave locked, if return true, Finish must be called.
        return true;
      }
    }
    PFile::Remove(keyFilePath); // Is bad
  }

  params.m_file.Remove(true); // Just in case
  params.m_size = 0; // indicate needs writing

  // create the file for the cached resource
  if (params.m_file.Open(PFile::WriteOnly, PFile::Create|PFile::Truncate))
    return true;

  // Can't cache at all!
  UnlockReadWrite();
  PTRACE(2, "Cannot create cache data file \"" << params.m_file.GetFilePath() << "\""
         " for \"" << params.m_key << "\", error: " << params.m_file.GetErrorText());
  return false;
}


bool PVXMLCache::FinishCache(Params & params, bool success)
{
  PFilePath keyFilePath = params.m_file.GetFilePath();
  keyFilePath.SetType(KeyFileType);

  if (success && params.m_size == 0) {
    if (params.m_file.GetLength() <= 0) {
      PTRACE(2, "Cannot cache zero length file \"" << params.m_file.GetFilePath() << '"');
      success = false;
    }

    if (!params.m_file.Close()) {
      PTRACE(2, "Could not close cache file \"" << params.m_file.GetFilePath() << '"');
      success = false;
    }

    if (success) {
      PXML xml(PXML::Indent|PXML::NewLineAfterElement);
      PXMLRootElement * root = new PXMLRootElement(xml, "cache");
      xml.SetRootElement(root);

      root->AddElement("key")->SetData(params.m_key);
      if (params.m_expires.IsValid())
        root->AddElement("expires")->SetData(params.m_expires.AsString(PTime::LongISO8601));
      root->AddElement("date")->SetData(params.m_date.AsString(PTime::LongISO8601));

      if (!xml.SaveFile(keyFilePath)) {
        PTRACE(2, "Cannot write cache key file \"" << keyFilePath << '"');
        success = false;
      }
    }
  }

  if (!success) {
    params.m_file.Remove(true);
    PFile::Remove(keyFilePath);
  }

  UnlockReadWrite();
  return success;
}

//////////////////////////////////////////////////////////

PVXMLSession::PVXMLSession()
  : m_textToSpeech(NULL)
  , m_autoDeleteTextToSpeech(false)
#if P_VXML_VIDEO
  , m_videoReceiver(*this)
#endif
  , m_vxmlThread(NULL)
  , m_closing(false)
  , m_abortVXML(false)
  , m_currentNode(NULL)
  , m_speakNodeData(true)
  , m_bargeIn(true)
  , m_bargingIn(false)
  , m_promptCount(0)
  , m_promptMode(e_NormalPrompt)
  , m_promptType(PTextToSpeech::Default)
  , m_defaultMenuDTMF('N') /// Disabled
  , m_scriptContext(PScriptLanguage::Create(PSimpleScript::LanguageName()))
  , m_recordingStatus(NotRecording)
  , m_recordStopOnDTMF(false)
  , m_recordingStartTime(0)
  , m_transferStatus(NotTransfering)
  , m_transferStartTime(0)
{
  static PVXMLSession::CachePtr DefaultCache(new PVXMLCache);
  SetCache(DefaultCache);

#if P_VXML_VIDEO
  PVideoInputDevice::OpenArgs videoArgs;
  videoArgs.driverName = P_FAKE_VIDEO_DRIVER;
  videoArgs.deviceName = P_FAKE_VIDEO_BLACK;
  m_videoSender.SetActualDevice(PVideoInputDevice::CreateOpenedDevice(videoArgs));
#endif // P_VXML_VIDEO

#if PTRACING
  Properties props(SessionScope);
#else
  Properties props;
#endif
  props.SetAt(TimeoutProperty, "10s");
  props.SetAt(BargeInProperty, true);
  props.SetAt(CachingProperty, "86400s");
  props.SetAt(InputModesProperty, DtmfAttribute & VoiceAttribute);
  props.SetAt(InterDigitTimeoutProperty, "5s");
  props.SetAt(TermTimeoutProperty, "0");
  props.SetAt(TermCharProperty, "#");
  props.SetAt(CompleteTimeoutProperty, "2s");
  props.SetAt(IncompleteTimeoutProperty, "5s");
  props.SetAt(FetchTimeoutProperty, "5s");
  props.SetAt(DocumentMaxAgeProperty, "3600s");
  props.SetAt(DocumentMaxStaleProperty, "1s");
  props.SetAt(AudioMaxAgeProperty, "3600s");
  props.SetAt(AudioMaxStaleProperty, "1s");
  props.SetAt(GrammarMaxAgeProperty, "3600s");
  props.SetAt(GrammarMaxStaleProperty, "1s");
  props.SetAt(ScriptMaxAgeProperty, "3600s");
  props.SetAt(ScriptMaxStaleProperty, "1");
  m_properties.push_back(props);

  // Create always present objects
  m_scriptContext->CreateComposite(SessionScope);
  m_scriptContext->CreateComposite(ApplicationScope);
  // Point dialog scope to same object as application scope
  m_scriptContext->Run(PSTRSTRM(DocumentScope << '=' << ApplicationScope));

  m_transferTimeout.SetNotifier(PCREATE_NOTIFIER(OnTransferTimeout));

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

  if (m_autoDeleteTextToSpeech && tts != m_textToSpeech)
    delete m_textToSpeech;

  m_autoDeleteTextToSpeech = autoDelete;

  if (tts != NULL) {
    PStringToString options;
    m_httpMutex.Wait();
    m_httpProxies.ToOptions(options);
    m_httpMutex.Signal();
    tts->SetOptions(options);
    PTRACE_IF(4, m_textToSpeech != tts, "Text to Speech set: " << *tts);
  }

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

  PTextToSpeech * tts = PFactory<PTextToSpeech>::CreateInstance(name);
  PTRACE_IF(2, tts == NULL && !(ttsName *= "none"), "Could not create TextToSpeech for \"" << ttsName << '"');
  return SetTextToSpeech(tts, true);
}


bool PVXMLSession::SetSpeechRecognition(const PString & srName)
{
  if (!(srName *= "none")) {
    PSpeechRecognition * sr = PSpeechRecognition::Create(srName);
    if (sr == NULL) {
      PTRACE(2, "Cannot use Speech Recognition \"" << srName << '"');
      return false;
    }
    delete sr;
  }

  PTRACE(4, "Speech Recognition set to \"" << srName << '"');
  PWaitAndSignal lock(m_grammersMutex);
  m_speechRecognition = srName;
  return true;
}


PSpeechRecognition * PVXMLSession::CreateSpeechRecognition()
{
  return PSpeechRecognition::Create(m_speechRecognition);
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
  if (file.IsOpen())
    return InternalLoadVXML(filename, file.ReadString(P_MAX_INDEX), firstForm);

  PTRACE(1, "Cannot open " << filename);
  return false;
}


bool PVXMLSession::LoadActualResource(const PURL & url,
                                      const PTimeInterval & timeout,
                                      PBYTEArray & data,
                                      PVXMLCache::Params & cacheParams)
{
  PAutoPtr<PHTTPClient> http(CreateHTTPClient());

  http->SetReadTimeout(timeout);

  PMIMEInfo outMIME, replyMIME;
  AddCookie(outMIME, url);

  if (!http->GetDocument(url, outMIME, replyMIME)) {
    PTRACE(2, "GET " << url << " failed with "
           << http->GetLastResponseCode() << ' ' << http->GetLastResponseInfo());
    return false;
  }

  cacheParams.m_date = replyMIME.GetVar(PHTTP::DateTag, PTime());
  cacheParams.m_expires = replyMIME.GetVar(PHTTP::ExpiresTag, PTime(0));
  PStringOptions cacheControl(replyMIME.GetString("Cache-Control"));
  if (!cacheControl.empty()) {
    static PConstCaselessString const nocache("no-cache");
    static PConstCaselessString const nostore("no-store");
    static PConstCaselessString const maxage("max-age");
    if (cacheControl.Contains(nocache) || cacheControl.Contains(nostore))
      cacheParams.m_maxAge = 0;
    else if (cacheControl.Contains(maxage))
      cacheParams.m_maxAge = PTimeInterval(cacheControl.GetInteger(maxage));
  }

  m_cookieMutex.Wait();
  if (m_cookies.Parse(replyMIME, url)) {
    PTRACE(4, "Cookie found: " << m_cookies);
    InternalSetVar(DocumentScope, "cookie", m_cookies.AsString());
  }
  m_cookieMutex.Signal();

  if (!http->ReadContentBody(replyMIME, data)) {
    PTRACE(2, "GET " << url << " read body failed with "
           << http->GetLastResponseCode() << ' ' << http->GetLastResponseInfo());
    return false;
  }

  PTRACE(4, "GET " << url << " succeeded: " << data.size());
  return true;
}


bool PVXMLSession::LoadCachedResource(const PURL & url,
                                      PXMLElement * overrideElement,
                                      const PString & maxagePropName,
                                      const PString & maxstalePropName,
                                      PBYTEArray & data)
{
  PTRACE(4, "LoadCachedResource " << url);

  PTimeInterval timeout = GetTimeProperty(FetchTimeoutProperty, overrideElement);

  if (url.GetScheme().NumCompare("http") != EqualTo)
    return url.LoadResource(data, PURL::LoadParams(PString::Empty(), timeout));

  PVXMLCache::Params cacheParams;
  cacheParams.m_prefix = "url";
  cacheParams.m_key = url.AsString();
  cacheParams.m_maxAge = GetTimeProperty(maxagePropName, overrideElement, MaxAgeAttribute);
  cacheParams.m_maxStale = GetTimeProperty(maxstalePropName, overrideElement, MaxStaleAttribute);

  const PStringArray & path = url.GetPath();
  if (!path.empty())
    cacheParams.m_suffix = PFilePath(path[path.GetSize() - 1]).GetType();
  if (cacheParams.m_suffix.empty())
    cacheParams.m_suffix = ".dat";

  CachePtr cache = m_resourceCache;

  if (cache == NULL || !cache->StartCache(cacheParams))
    return LoadActualResource(url, timeout, data, cacheParams);

  if (cacheParams.m_size > 0)
    return cache->FinishCache(cacheParams, cacheParams.m_file.Read(data.GetPointer(cacheParams.m_size), cacheParams.m_size));

  return cache->FinishCache(cacheParams,
                            LoadActualResource(url, timeout, data, cacheParams) &&
                            cacheParams.m_file.Write(data, data.GetSize()));
}


void PVXMLSession::SetProxies(const PHTTP::Proxies & proxies)
{
  m_httpMutex.Wait();
  m_httpProxies = proxies;
  m_httpMutex.Signal();
  SetTextToSpeech(m_textToSpeech, m_autoDeleteTextToSpeech);
}


PHTTPClient * PVXMLSession::CreateHTTPClient() const
{
  PHTTPClient * http = new PHTTPClient("PTLib VXML Client");
  http->SetSSLCredentials(*this);
  PWaitAndSignal lock(m_httpMutex);
  http->SetProxies(m_httpProxies);
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
  if (LoadCachedResource(url, NULL, DocumentMaxAgeProperty, DocumentMaxStaleProperty, xmlData))
    return InternalLoadVXML(url, PString(xmlData), url.GetFragment());

  PTRACE(1, "Cannot load document " << url);
  return false;
}


PBoolean PVXMLSession::LoadVXML(const PString & xmlText, const PString & firstForm)
{
  PURL url("data:text/xml;charset=utf-8,placeholder");
  url.SetContents(xmlText);
  return InternalLoadVXML(url, xmlText, firstForm);
}


bool PVXMLSession::InternalLoadVXML(const PURL & url, const PString & xmlText, const PString & firstForm)
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
    m_lastXMLError = PSTRSTRM(url.AsString().Left(100) <<
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

  m_newURL = url;
  m_newXML.transfer(xml);
  m_newFormName = firstForm;
  InternalStartThread();
  return true;
}


PURL PVXMLSession::NormaliseResourceName(const PString & src)
{
  if (src.empty())
    return PURL();  // Throw error.badfetch

  PURL srcURL;
  if (srcURL.Parse(src, NULL) && !srcURL.GetRelativePath())
    return srcURL;

  PURL documentURI = InternalGetVar(DocumentScope, DocumentURIVar);
  documentURI.ChangePath(PString::Empty()); // Remove last element of document URL

  if (srcURL.IsEmpty()) {
    // Use PathOnly as we don't want all the query argements etc
    if (srcURL.Parse(PSTRSTRM(documentURI.AsString(PURL::PathOnly) << '/' << src), documentURI.GetScheme()))
      return srcURL;
  }
  else if (documentURI.GetScheme() == srcURL.GetScheme()) {
    srcURL.SetHostName(documentURI.GetHostName());
    srcURL.SetPort(documentURI.GetPort());
    srcURL.SetUserName(documentURI.GetUserName());
    srcURL.SetPassword(documentURI.GetPassword());
    srcURL.SetPath(documentURI.GetPath() + srcURL.GetPath());
    srcURL.SetRelativePath(documentURI.GetRelativePath());
    return srcURL;
  }

  PTRACE(2, "NormaliseResourceName failed: document=" << documentURI << ", src=\"" << src << '"');
  return PURL();  // Throw error.badfetch
}


bool PVXMLSession::SetCurrentForm(const PString & searchId, bool fullURI)
{
  ClearBargeIn();
  m_eventCount.clear();

  if (IsFinalProcessing())
    return false; // Cannot go to new form when in final

  m_promptMode = e_NormalPrompt;
  m_promptCount = 0;

  PTRACE(4, "Setting current form to \"" << searchId << '"');

  PString id = searchId;

  if (fullURI) {
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
          return SetCurrentNode(xmlElement);
        }
      }
    }
  }

  PTRACE(3, "No form/menu with id \"" << searchId << '"');
  return false;
}


bool PVXMLSession::SetCurrentNode(PXMLObject * newNode)
{
  if (m_currentNode != NULL && dynamic_cast<PXMLElement *>(newNode) != NULL) {
    PXMLElement * element = m_currentNode->GetParent();
    while (element != NULL) {
      PVXMLNodeHandler * handler = PVXMLNodeFactory::CreateInstance(element->GetName());
      if (handler != NULL)
        handler->FinishTraversal(*this, *element);
      element = element->GetParent();
    }
  }

  m_currentNode = newNode;
  return newNode != NULL;
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

  PTRACE(4, "VXML Session opened");
  InternalStartThread();
  return true;
}


void PVXMLSession::InternalStartThread()
{
  PWaitAndSignal mutex(m_sessionMutex);

  if (IsOpen() && m_promptMode != e_FinalProcessing && m_vxmlThread == NULL && m_newXML.get() != NULL)
    m_vxmlThread = new PThreadObj<PVXMLSession>(*this, &PVXMLSession::InternalThreadMain, false, "VXML");
  else
    Trigger();
}


PBoolean PVXMLSession::Close()
{
  PTRACE(4, "VXML Session closing");
  m_sessionMutex.Wait();

  ClearGrammars();

  // Indicate closing and throw disconnect exception
  m_closing = true;
  Trigger();
  PThread::WaitAndDelete(m_vxmlThread, 30000, &m_sessionMutex, false);

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

  // Check for PSimpleScript object reference assignment
  bool varComposite = fullVarName.Right(2) == ".$";
  bool valComposite = value.Right(2) == ".$";
  if (!(varComposite || valComposite))
    return scriptContext.SetString(fullVarName, value);

  if (value.empty())
    return true;

  PString adjustedValue = valComposite ? value.Left(value.length()-2) : value;
  PString adjustedVarName = varComposite ? fullVarName.Left(fullVarName.length()-2) : fullVarName;

  // Make sure right hand side of reference assignment exists as a composite
  if (!CreateComposites(scriptContext, adjustedValue + ".placeholder"))
    return false;

  return scriptContext.Run(PSTRSTRM(adjustedVarName << '=' << adjustedValue));
}


void PVXMLSession::InternalThreadMain()
{
  if (m_newXML.get() == NULL) {
    PTRACE(2, "Execution thread started unexpectedly, exiting.");
    return;
  }

  PTRACE(4, "Execution thread started.");

  m_sessionMutex.Wait();

  static const char * Languages[] = { "JavaScript", "Lua" };
  PScriptLanguage * newScript = PScriptLanguage::CreateOne(PStringArray(PARRAYSIZE(Languages), Languages));
  if (newScript != NULL) {
    newScript->CreateComposite(SessionScope);
    newScript->CreateComposite(ApplicationScope);
    InternalSetVar(ApplicationScope, RootURIVar, PString::Empty());

    PSimpleScript * simpleScript = dynamic_cast<PSimpleScript *>(m_scriptContext.get());
    if (simpleScript != NULL) {
      PStringToString variables = simpleScript->GetAllVariables();
      for (PStringToString::iterator it = variables.begin(); it != variables.end(); ++it)
        CreateScriptVariable(*newScript, it->first, it->second);
    }
    m_scriptContext.reset(newScript);
  }

#if P_VXML_VIDEO
  m_scriptContext->SetFunction(SIGN_LANGUAGE_PREVIEW_SCRIPT_FUNCTION, PCREATE_NOTIFIER(SignLanguagePreviewFunction));
  m_scriptContext->SetFunction(SIGN_LANGUAGE_CONTROL_SCRIPT_FUNCTION, PCREATE_NOTIFIER(SignLanguageControlFunction));
#endif
  m_scriptContext->PushScopeChain(SessionScope, false);
  m_scriptContext->PushScopeChain(ApplicationScope, false);

  PTime now;
  InternalSetVar(SessionScope, "time", now.AsString());
  InternalSetVar(SessionScope, "timeISO8601", now.AsString(PTime::ShortISO8601));
  InternalSetVar(SessionScope, "timeEpoch", now.GetTimeInSeconds());

  InternalStartVXML();

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

    if (m_closing) {
      if (m_currentNode == NULL)
        ProcessEvents(); // Move to connection.disconnect event
    }
    else {
      /* Replace old XML with new, now it is safe to do so and no XML elements
          pointers are being referenced by the code any more */
      if (m_newXML.get() != NULL)
        InternalStartVXML();
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
      InternalStartVXML();
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

  if (m_abortVXML || m_promptMode == e_FinalProcessing || m_currentNode == NULL || !IsOpen())
    return false;

  PVXMLChannel * vxmlChannel = GetVXMLChannel();
  if (PAssertNULL(vxmlChannel) == NULL)
    return false;

  if (m_closing) {
    m_closing = false;
    PTRACE(4, "Entering final processing.");
    m_promptMode = e_FinalProcessing;
    vxmlChannel->FlushQueue();

    PXMLElement * element = dynamic_cast<PXMLElement *>(m_currentNode);
    if (element == NULL)
      element = m_currentNode->GetParent();
    GoToEventHandler(*element,
                     m_transferStatus != NotTransfering
                         ? "connection.disconnect.transfer"
                         : "connection.disconnect.hangup",
                     true,
                     true);
    return false;
  }

  if (!m_userInputQueue.empty()) {
    if (m_recordingStatus == RecordingInProgress) {
      PTRACE(3, "Recording ended via user input");
      if (m_recordStopOnDTMF && vxmlChannel->EndRecording(false)) {
        if (!m_recordingName.IsEmpty()) {
          PString input;
          if (m_userInputQueue.Dequeue(input))
            InternalSetVar(m_recordingName, "termchar", input[0]);
        }
      }
    }
    else if (vxmlChannel->IsPlaying()) {
      if (m_bargeIn) {
        PTRACE_IF(3, !m_bargingIn, "Executing bargein");
        m_bargingIn = true;
        vxmlChannel->FlushQueue();
      }
      else {
        PTRACE(3, "No bargein");
        FlushInput(); // Ignore input so far
      }
    }
    else if (!m_bargingIn) {
      OnUserInputToGrammars();
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
  else {
    switch (m_transferStatus) {
      case TransferInProgress:
        PTRACE(4, "Transfer in progress, awaiting event");
        break;
      case TransferSuccessful :
        PTRACE(4, "Transfer successful");
        return false;
      case TransferFailed:
        PTRACE(4, "Transfer failed");
        return false;
      default :
        PTRACE(4, "Nothing happening, processing next node");
        return false;
    }
  }

  m_sessionMutex.Signal();
  m_waitForEvent.Wait();
  m_sessionMutex.Wait();

  if (m_closing || m_newXML.get() == NULL)
    return true;

  PTRACE(4, "XML changed, flushing queue");

  // Clear out any audio being output, so can start fresh on new VXML.
  // Don't use vxmlChannel as may have changed during m_waitForEvent
  if (IsOpen())
    GetVXMLChannel()->FlushQueue();

  return false;
}


void PVXMLSession::InternalStartVXML()
{
  ClearGrammars();

  PURL rootURL = InternalGetVar(ApplicationScope, RootURIVar);
  PURL appURL = NormaliseResourceName(m_newXML->GetRootElement()->GetAttribute("application"));
  PTRACE(4, "InternalStartVXML: root=" << rootURL << ", application=" << appURL << ", new=" << m_newURL);
  if (appURL.IsEmpty() || appURL != rootURL) {
    rootURL = m_newURL;
    InternalSetVar(ApplicationScope, RootURIVar, rootURL);
  }

  while (m_scriptContext->GetScopeChain().back() != ApplicationScope)
    m_scriptContext->PopScopeChain(true);

  while (m_properties.size() > 1)
    m_properties.pop_back();

  if (rootURL == m_newURL)
    m_scriptContext->Run(PSTRSTRM(DocumentScope << '=' << ApplicationScope));
  else {
    m_scriptContext->ReleaseVariable(DocumentScope);
    m_scriptContext->PushScopeChain(DocumentScope, true);
    m_properties.push_back(Properties(PTRACE_PARAM(DocumentScope)));
  }

  InternalSetVar(DocumentScope, DocumentURIVar, m_newURL);  // Non-standard but potentially useful

  PTRACE(4, "Processing global elements.");
  m_currentXML.transfer(m_newXML);
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

    m_speakNodeData = false;
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
    if (m_transferStatus == TransferSuccessful || m_transferStatus == TransferFailed) {
      CompletedTransfer(*element);
      return false;
    }

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
      if (!handler->FinishTraversal(*this, *element)) {
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


bool PVXMLSession::SelectMenuChoice(PXMLElement & choice)
{
  if ((choice.HasAttribute(NextAttribute) +
       choice.HasAttribute(ExprAttribute) +
       choice.HasAttribute(EventAttribute) +
       choice.HasAttribute(EventExprAttribute)) != 1)
    return ThrowBadFetchError(choice, "Only one of next, nextexpr, event, eventexpr alllowed");

  PString next = choice.GetAttribute(NextAttribute);
  if (next.empty() && EvaluateExpr(choice, ExprAttribute, next))
    return true;

  if (!next.empty())
    return SetCurrentForm(next, true);

  next = choice.GetAttribute(EventAttribute);
  if (next.empty() && EvaluateExpr(choice, EventExprAttribute, next))
    return true;

  return !next.empty() && GoToEventHandler(choice, next, false, true);
}


void PVXMLSession::ClearBargeIn()
{
  PTRACE_IF(4, m_bargingIn, "Ending barge in");
  m_bargingIn = false;
}


void PVXMLSession::FlushInput()
{
  PTRACE(4, "Flushing user input queue");
  m_userInputQueue.Close(false);
  m_userInputQueue.Restart();
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
      PlayText(nodeData->GetString().Trim(), m_promptType);
    return true;
  }

  m_speakNodeData = true;

  for (unsigned failsafe = 0; failsafe < 100; ++failsafe) {
    PXMLElement * element = dynamic_cast<PXMLElement *>(m_currentNode);
    PVXMLNodeHandler * handler = PVXMLNodeFactory::CreateInstance(element->GetName());
    if (handler == NULL) {
      PTRACE(2, "Unknown/unimplemented VoiceXML element: " << element->PrintTrace());
      return false;
    }

    bool started = handler->StartTraversal(*this, *element);
    if (element == m_currentNode) {
      PTRACE_IF(4, !started, "Skipping VoiceXML element: " << element->PrintTrace());
      return started;
    }

    PTRACE(4, "Moved node after processing VoiceXML element:"
           " from=" << element->PrintTrace() << ","
           " to=" << m_currentNode->PrintTrace());

    handler->FinishTraversal(*this, *element);
    if (m_currentNode == NULL)
      return false;
  }

  PTRACE(2, "Too many redirections in VoiceXML element: " << m_currentNode->PrintTrace());
  return false;
}


void PVXMLSession::OnUserInput(const PString & str)
{
  m_userInputQueue.Enqueue(str);
  Trigger();
}


PBoolean PVXMLSession::TraverseRecord(PXMLElement & element)
{
  if (IsFinalProcessing())
    return false;

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
      return !GoToEventHandler(element, FilledElement, false, true);

    default :
      break;
  }

  static const PConstString supportedFileType(".wav");
  PCaselessString typeMIME = element.GetAttribute(TypeAttribute);
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
  InternalSetVar(m_recordingName, TypeAttribute, typeMIME);
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

  recordable->SetFinalSilence(PTimeInterval(element.GetAttribute("finalsilence")));
  recordable->SetMaxDuration(PTimeInterval(element.GetAttribute("maxtime")));

  if (!GetVXMLChannel()->QueueRecordable(recordable))
    return true;

  m_recordingStatus = RecordingInProgress;
  m_recordingStartTime.SetCurrentTime();
  return false; // Are recording, stay in <record> element
}


PBoolean PVXMLSession::TraverseScript(PXMLElement & element)
{
  PString src = element.GetAttribute(SrcAttribute);
  if (src.empty()) {
    PString script = element.GetData();
    PTRACE(4, "Executing script " << script.ToLiteral());
    return RunScript(element, script);
  }

  PBYTEArray data;
  if (LoadCachedResource(NormaliseResourceName(src),
                         &element,
                         ScriptMaxAgeProperty,
                         ScriptMaxStaleProperty,
                         data)) {
    PTRACE(4, "Executing script of " << data.size() << " bytes");
    return RunScript(element, PString(data));
  }

  return ThrowBadFetchError(element, "Could not load <script> from \"" << src << '"');
}


bool PVXMLSession::RunScript(PXMLElement & element, const PString & script)
{
  if (m_scriptContext->Run(script))
    return false;

  return ThrowSemanticError(element,
                            m_scriptContext->GetLanguageName()
                            << " error \"" << m_scriptContext->GetLastErrorText()
                            << "\" executing " << script.ToLiteral());
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
  return ThrowSemanticError(element, 
                            m_scriptContext->GetLanguageName()
                            << " error \"" << m_scriptContext->GetLastErrorText()
                            << "\" evaluating " << expr.ToLiteral());
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
  m_scriptContext->Run(originator ? "session.connection.originator=session.connection.local"
                                  : "session.connection.originator=session.connection.remote");
}


PCaselessString PVXMLSession::GetProperty(const PString & propName,
                                          const PXMLElement * overrideElement,
                                          const PString & attrName) const
{
  if (overrideElement != NULL) {
    PString value = overrideElement->GetAttribute(attrName.empty() ? propName : attrName);
    if (!value.empty())
      return value;
  }

  for (std::list<Properties>::const_reverse_iterator it = m_properties.rbegin(); it != m_properties.rend(); ++it) {
    PString * value = it->GetAt(propName);
    if (value != NULL) {
      PTRACE(4, "Property " << it->m_node << ' ' << propName << "=\"" << *value << '"');
      return *value;
    }
  }
  PTRACE(4, "No property " << propName << " in stack of " << m_properties.size());
  return PString::Empty();
}


void PVXMLSession::SetProperty(const PString & name, const PString & value, bool session)
{
  PWaitAndSignal lock(m_sessionMutex);
  Properties & props = session ? m_properties.front() : m_properties.back();
  props.SetAt(name, value);
  PTRACE(3, "Set property " << props.m_node << ' ' << name << " to \"" << value << '"');
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

  PBYTEArray data;
  if (LoadCachedResource(url,
                         &element,
                         AudioMaxAgeProperty,
                         AudioMaxStaleProperty,
                         data))
    return PlayData(data, repeat);

  return ThrowBadFetchError(element, "Could not load audio from " << url);
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


void PVXMLSession::LoadGrammar(const PVXMLGrammarInit & init)
{
  PVXMLGrammar * grammar = PVXMLGrammarFactory::CreateInstance(init.m_type, init);
  if (grammar == NULL) {
    ThrowError(init.m_field, "error.unsupported.builtin", "Could not set grammar of type \"" << init.m_type << '"');
    return;
  }

  PVXMLGrammar::GrammarState state = grammar->GetState();
  if (state != PVXMLGrammar::Idle) {
    delete grammar;
    if (state == PVXMLGrammar::BadFetch)
      ThrowBadFetchError(init.m_field, "Bad fetch in grammar " << *grammar);
    else
      ThrowError(init.m_field, "error.unsupported.format", "Illegal/unsupported grammar of type \"" << init.m_type << '"');
    return;
  }

  PTRACE(2, "Grammar set to " << *grammar << " from \"" << init.m_type << '"');
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
  PTRACE(3, "Starting " << m_grammars.size() << " grammars");
  for (Grammars::iterator it = m_grammars.begin(); it != m_grammars.end(); ++it)
    it->Start();
  OnUserInputToGrammars();
}


void PVXMLSession::OnUserInputToGrammars()
{
  PWaitAndSignal lock(m_grammersMutex);
  if (m_grammars.empty())
    return;

  PString input;
  while (m_userInputQueue.Dequeue(input, 0)) {
    PTRACE(3, "Sending \"" << input << "\" to " << m_grammars.size() << " grammars");
    for (Grammars::iterator it = m_grammars.begin(); it != m_grammars.end(); ++it)
      it->OnUserInput(input);
  }
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

  CachePtr cache = m_resourceCache;

  if (cache == NULL || GetProperty(CachingProperty) == SafeKeyword) {
    PFilePath fn("tts", NULL, ".wav");
    return m_textToSpeech != NULL &&
      m_textToSpeech->SetSampleRate(GetVXMLChannel()->GetSampleRate()) &&
      m_textToSpeech->SetChannels(GetVXMLChannel()->GetChannels()) &&
      m_textToSpeech->OpenFile(fn) &&
      m_textToSpeech->Speak(textToPlay, type) &&
      m_textToSpeech->Close() &&
      PlayFile(fn, repeat, delay, true);
  }

  PString fileList;

  PVXMLCache::Params cacheParams;
  cacheParams.m_prefix.sprintf("tts%i", type);
  cacheParams.m_suffix = GetVXMLChannel()->GetMediaFileSuffix() + ".wav";
  cacheParams.m_maxAge = GetTimeProperty(AudioMaxAgeProperty);

  // Convert each line into it's own cached WAV file.
  PStringArray lines = textToPlay.Lines();
  for (PINDEX i = 0; i < lines.GetSize(); i++) {
    cacheParams.m_key = lines[i].Trim();
    if (cacheParams.m_key.IsEmpty())
      continue;

    // see if we have converted this text before
    if (!cache->StartCache(cacheParams))
      continue;

    if (cacheParams.m_size > 0) {
      fileList += cacheParams.m_file.GetFilePath() + '\n';
      cache->FinishCache(cacheParams, true);
      continue;
    }

    // Really want to use OpenChannel() but it isn't implemented yet.
    // So close file and just use filename.
    cacheParams.m_file.Close();

    cache->FinishCache(cacheParams,
                       m_textToSpeech != NULL &&
                       m_textToSpeech->SetSampleRate(GetVXMLChannel()->GetSampleRate()) &&
                       m_textToSpeech->SetChannels(GetVXMLChannel()->GetChannels()) &&
                       m_textToSpeech->OpenFile(cacheParams.m_file.GetFilePath()) &&
                       m_textToSpeech->Speak(cacheParams.m_key, type) &&
                       m_textToSpeech->Close() &&
                       cacheParams.m_file.Open(PFile::ReadOnly));
    fileList += cacheParams.m_file.GetFilePath() + '\n';
  }

  return GetVXMLChannel()->QueuePlayable("FileList", fileList, repeat, delay);
}


void PVXMLSession::SetPause(PBoolean pause)
{
  if (IsOpen())
    GetVXMLChannel()->SetPause(pause);
}


PBoolean PVXMLSession::TraverseRootNode(PXMLElement &)
{
  m_abortVXML = true;
  return false;
}


PBoolean PVXMLSession::TraverseBlock(PXMLElement & element)
{
  unsigned col, line;
  element.GetFilePosition(col, line);
  m_scriptContext->PushScopeChain(PSTRSTRM("anonymous_" << line << '_' << col), true);
  return ExecuteCondition(element);
}


PBoolean PVXMLSession::TraversedBlock(PXMLElement & element)
{
  m_scriptContext->PopScopeChain(true);
  return ExecuteCondition(element);
}


PBoolean PVXMLSession::TraverseReprompt(PXMLElement &)
{
  if (m_promptMode != e_FinalProcessing)
    m_promptMode = e_Reprompt;
  return false;
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
  PString time = element.GetAttribute("time");
  if (!time.empty())
    return PlaySilence(PTimeInterval(time+"ms"));

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
  PString value;
  EvaluateExpr(element, ExprAttribute, value);
  PlayText(value.Trim(), m_promptType);
  return false;
}


PBoolean PVXMLSession::TraverseSayAs(PXMLElement & element)
{
  PCaselessString interpretAs = element.GetAttribute("interpret-as");
  static const struct
  {
    PConstCaselessString m_name;
    PTextToSpeech::TextType m_value;
  } TextTypes[] = {
    { "vxml:boolean",     PTextToSpeech::Boolean },
    { "vxml:date",        PTextToSpeech::Date },
    { "vxml:digits",      PTextToSpeech::Digits },
    { "vxml:currency",    PTextToSpeech::Currency },
    { "vxml:number",      PTextToSpeech::Number },
    { "vxml:phone",       PTextToSpeech::Phone },
    { "vxml:time",        PTextToSpeech::Time },
    { "x-opal:ip",        PTextToSpeech::IPAddress },
    { "x-opal:ipaddress", PTextToSpeech::IPAddress },
    { "x-opal:duration",  PTextToSpeech::Duration }
  };
  for (PINDEX i = 0; i < PARRAYSIZE(TextTypes); ++i) {
    if (TextTypes[i].m_name == interpretAs) {
      m_promptType = TextTypes[i].m_value;
      break;
    }
  }

  return true;
}


PBoolean PVXMLSession::TraversedSayAs(PXMLElement &)
{
  m_promptType = PTextToSpeech::Default;
  return true;
}


static bool UnsupportedVoiceAttribute(const PStringArray & attr)
{
  for (PINDEX i = 0; i < attr.GetSize(); ++i) {
    PCaselessString a = attr[i];
    if (a != NameAttribute || a != LanguagesAttribute || a != GenderAttribute)
      return true;
  }
  return false;
}


PBoolean PVXMLSession::TraverseVoice(PXMLElement & element)
{
  if (m_textToSpeech == NULL)
    return true;

  PStringArray names = element.GetAttribute(NameAttribute).Tokenise(" ", false);
  PStringArray languages = element.GetAttribute(LanguagesAttribute).Tokenise(" ", false);
  PString gender = element.GetAttribute("gender");
  if (names.empty() && languages.empty() && gender.empty())
    return ThrowSemanticError(element, "<voice> must have one of name, language or gender");

  PStringArray required = element.GetAttribute("required").Tokenise(" ", false);
  PStringArray ordering = element.GetAttribute("ordering").Tokenise(" ", false);
  if (UnsupportedVoiceAttribute(required) && UnsupportedVoiceAttribute(ordering))
    return ThrowSemanticError(element, "<voice> has unsupported required/ordering");

  PINDEX pos = ordering.GetValuesIndex(NameAttribute);
  if (names.empty())
    ordering.RemoveAt(pos);
  else if (pos == P_MAX_INDEX)
    ordering.AppendString(NameAttribute);

  pos = ordering.GetValuesIndex(LanguagesAttribute);
  if (languages.empty())
    ordering.RemoveAt(pos);
  else if (pos == P_MAX_INDEX)
    ordering.AppendString(LanguagesAttribute);

  pos = ordering.GetValuesIndex(GenderAttribute);
  if (gender.empty())
    ordering.RemoveAt(pos);
  else if (pos == P_MAX_INDEX)
    ordering.AppendString(GenderAttribute);

  bool prioritySelect = !element.HasAttribute("onvoicefailure") || (element.GetAttribute("onvoicefailure") *= "priorityselect");

  switch (ordering.size()) {
    case 1 :
      if (ordering[0] == NameAttribute) {
        for (PINDEX i = 0; i < names.GetSize(); ++i)
          if (m_textToSpeech->SetVoice(PSTRSTRM("name=" << names[i])))
            return true;
      }
      else if (ordering[0] == LanguagesAttribute) {
        for (PINDEX i = 0; i < languages.GetSize(); ++i)
          if (m_textToSpeech->SetVoice(PSTRSTRM("language=" << languages[i])))
            return true;
      }
      else {
        if (m_textToSpeech->SetVoice(PSTRSTRM("gender=" << gender)))
          return true;
      }
      break;
    case 2 :
      if (ordering[0] == NameAttribute && ordering[1] == LanguagesAttribute) {
        for (PINDEX i = 0; i < names.GetSize(); ++i)
          for (PINDEX j = 0; j < languages.GetSize(); ++j)
            if (m_textToSpeech->SetVoice(PSTRSTRM("name=" << names[i] << ";language=" << languages[j])))
              return true;
        if (prioritySelect) {
          for (PINDEX i = 0; i < names.GetSize(); ++i)
            if (m_textToSpeech->SetVoice(PSTRSTRM("name=" << names[i])))
              return true;
        }
      }
      else if (ordering[0] == LanguagesAttribute && ordering[1] == NameAttribute) {
        for (PINDEX j = 0; j < languages.GetSize(); ++j)
          for (PINDEX i = 0; i < names.GetSize(); ++i)
            if (m_textToSpeech->SetVoice(PSTRSTRM("name=" << names[i] << ";language=" << languages[j])))
              return true;
        if (prioritySelect) {
          for (PINDEX i = 0; i < languages.GetSize(); ++i)
            if (m_textToSpeech->SetVoice(PSTRSTRM("language=" << languages[i])))
              return true;
        }
      }
      else if (ordering[0] == NameAttribute || ordering[1] == NameAttribute) {
        for (PINDEX i = 0; i < names.GetSize(); ++i)
          if (m_textToSpeech->SetVoice(PSTRSTRM("name=" << names[i] << ";gender=" << gender)))
            return true;
        if (prioritySelect) {
          for (PINDEX i = 0; i < names.GetSize(); ++i)
            if (m_textToSpeech->SetVoice(PSTRSTRM("name=" << names[i])))
              return true;
        }
      }
      else {
        for (PINDEX i = 0; i < languages.GetSize(); ++i)
          if (m_textToSpeech->SetVoice(PSTRSTRM("language=" << languages[i] << ";gender=" << gender)))
            return true;
        if (prioritySelect) {
          for (PINDEX i = 0; i < languages.GetSize(); ++i)
            if (m_textToSpeech->SetVoice(PSTRSTRM("language=" << languages[i])))
              return true;
        }
      }
      break;
    default :
      if (ordering.GetValuesIndex(NameAttribute) < ordering.GetValuesIndex(LanguagesAttribute)) {
        for (PINDEX i = 0; i < names.GetSize(); ++i)
          for (PINDEX j = 0; j < languages.GetSize(); ++j)
            if (m_textToSpeech->SetVoice(PSTRSTRM("name=" << names[i] << ";language=" << languages[j] << ";gender=" << gender)))
              return true;
        if (prioritySelect) {
          for (PINDEX i = 0; i < names.GetSize(); ++i)
            if (m_textToSpeech->SetVoice(PSTRSTRM("name=" << names[i] << ";gender=" << gender)))
              return true;
          for (PINDEX i = 0; i < names.GetSize(); ++i)
            if (m_textToSpeech->SetVoice(PSTRSTRM("name=" << names[i])))
              return true;
        }
      }
      else {
        for (PINDEX j = 0; j < languages.GetSize(); ++j)
          for (PINDEX i = 0; i < names.GetSize(); ++i)
            if (m_textToSpeech->SetVoice(PSTRSTRM("name=" << names[i] << ";language=" << languages[j] << ";gender=" << gender)))
              return true;
        if (prioritySelect) {
          for (PINDEX i = 0; i < languages.GetSize(); ++i)
            if (m_textToSpeech->SetVoice(PSTRSTRM("language=" << languages[i] << ";gender=" << gender)))
              return true;
          for (PINDEX i = 0; i < languages.GetSize(); ++i)
            if (m_textToSpeech->SetVoice(PSTRSTRM("language=" << languages[i])))
              return true;
        }
      }
  }

  PTRACE(3, "Could not set voice to any of [" << setfill(',') << names << "],"
            " [" << setfill(',') << languages << "], gender=\"" << gender << '"');

  if (prioritySelect)
    m_textToSpeech->SetVoice("*");

  return ThrowError(element, "error.voice", "<voice> cannot be used.");
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
    return false;

  return ThrowBadFetchError(element, "<goto> has invalid URI \"" << target << '"');
}


PBoolean PVXMLSession::TraverseGrammar(PXMLElement & element)
{
  m_speakNodeData = false;

  PString type = element.GetAttribute(TypeAttribute);
  if (type.empty()) {
    PString src = element.GetAttribute(SrcAttribute);
    PConstString builtin("builtin:");
    if (src.NumCompare(builtin, builtin.length()) == EqualTo) {
      PINDEX pos = src.Find('/');
      if (pos == P_MAX_INDEX || pos <= builtin.length())
        type = src.Mid(builtin.length());
      else {
        type = src.Mid(pos+1);
        PCaselessString srcMode = src(8, pos-1);
        PCaselessString modeAttr = element.GetAttribute(ModeAttribute);
        if (modeAttr.empty())
          element.SetAttribute(ModeAttribute, srcMode);
        else if (srcMode != modeAttr)
          return ThrowSemanticError(element, "Builtin disagrees with mode attribute");
      }
    }
  }

  LoadGrammar(PVXMLGrammarInit(type.empty() ? SRGS : type, *this, *element.GetParent(), &element));
  return false; // Skip subelements
}


PBoolean PVXMLSession::TraversedGrammar(PXMLElement &)
{
  m_speakNodeData = true;
  return true;
}


// Finds the proper event hander for 'noinput', 'filled', 'nomatch' and 'error'
// by searching the scope hiearchy from the current from
bool PVXMLSession::GoToEventHandler(PXMLElement & element, const PString & eventName, bool exitIfNotFound, bool firstInHierarchy)
{
  if (eventName.empty())
    return false;

  PXMLElement * level = &element;
  PXMLElement * handler = NULL;

  unsigned actualCount = firstInHierarchy ? ++m_eventCount[eventName] : m_eventCount[eventName];

  // Look in all the way up the tree for a handler either explicitly or in a catch
  while ((handler = FindElementWithCount(*level, eventName, actualCount)) == NULL) {
    level = level->GetParent();
    if (level == NULL) {
      PTRACE(4, "No event handler found for \"" << eventName << "\" (" << actualCount << ')');
      static PConstString const MatchAll(".");
      if (eventName != MatchAll) {
        PINDEX dot = eventName.FindLast('.');
        return GoToEventHandler(element,
                                dot == P_MAX_INDEX
                                    ? static_cast<PString>(MatchAll)
                                    : eventName.Left(dot),
                                exitIfNotFound,
                                firstInHierarchy);
      }

      if (exitIfNotFound || m_closing) {
        PTRACE(3, "Exiting script.");
        SetCurrentNode(NULL);
      }
      return false;
    }
  }

  handler->SetAttribute(InternalEventStateAttribute, true);
  PTRACE(4, "Setting event handler to node " << handler->PrintTrace() << " for \"" << eventName << "\" (" << actualCount << ')');
  return SetCurrentNode(handler);
}


PXMLElement * PVXMLSession::FindElementWithCount(PXMLElement & parent, const PString & name, unsigned count)
{
  typedef std::map<unsigned, PXMLElement *, std::greater<unsigned> > ElementsByCount;
  ElementsByCount elements;
  PXMLElement * element;
  PINDEX idx = 0;
  while ((element = parent.GetElement(idx++)) != NULL) {
    PString str = element->GetAttribute(CountAttribute);
    unsigned elementCount = str.IsEmpty() ? 1 : str.AsUnsigned();

    if (element->GetName() == name)
      elements[elementCount] = element;
    else if (element->GetName() == CatchElement) {
      PStringArray events = element->GetAttribute(EventAttribute).Tokenise(" ", false);
      for (size_t i = 0; i < events.size(); ++i) {
        if (events[i] *= name) {
          elements[elementCount] = element;
          break;
        }
      }
    }
  }

  ElementsByCount::iterator it = elements.lower_bound(count);
  return it != elements.end() ? it->second : NULL;
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

  return SetCurrentNode(nextNode); // Do subelements
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
    SetCurrentNode(element.GetParent());
    return false;
  }

  if (ExecuteCondition(element)) {
    ifElement->SetAttribute(IfStateAttributeName, "done");
    return true;
  }

  PXMLObject * nextNode = NextElseIfNode(*ifElement, ifElement->FindObject(&element)+1);
  if (nextNode == NULL)
    return false; // Skip if subelements

  return SetCurrentNode(nextNode); // Do subelements
}


PBoolean PVXMLSession::TraverseElse(PXMLElement & element)
{
  SetCurrentNode(element.GetParent());
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

  PURL url = NormaliseResourceName(urlStr);
  if (url.IsEmpty())
    return ThrowBadFetchError(element, "<submit> has an invalid URL \"" << urlStr << '"');

  bool urlencoded;
  PCaselessString str = element.GetAttribute("enctype");
  if (str.IsEmpty() || str == PHTTP::FormUrlEncoded())
    urlencoded = true;
  else if (str == "multipart/form-data")
    urlencoded = false;
  else
    return ThrowBadFetchError(element, "<submit> has unknown \"enctype\" attribute of \"" << str << '"');

  bool get;
  str = element.GetAttribute("method");
  if (str.IsEmpty())
    get = urlencoded;
  else if (str == "GET")
    get = true;
  else if (str == "POST")
    get = false;
  else
    return ThrowBadFetchError(element, "<submit> has unknown \"method\" attribute of \"" << str << '"');

  PStringSet namelist = element.GetAttribute("namelist").Tokenise(" \t", false);
  if (namelist.IsEmpty())
    namelist = m_dialogFieldNames;

  if (get) {
    for (PStringSet::iterator it = namelist.begin(); it != namelist.end(); ++it)
      url.SetQueryVar(*it, GetVar(*it));

    PBYTEArray body;
    if (LoadCachedResource(url, &element, DocumentMaxAgeProperty, DocumentMaxStaleProperty, body) &&
        InternalLoadVXML(url, PString(body), PString::Empty()))
      return true;
    return ThrowBadFetchError(element, "Could not GET " << url);
  }

  PAutoPtr<PHTTPClient> http(CreateHTTPClient());
  http->SetReadTimeout(GetTimeProperty(FetchTimeoutProperty, &element));

  PMIMEInfo sendMIME, replyMIME;
  AddCookie(sendMIME, url);

  if (urlencoded) {
    PStringToString vars;
    for (PStringSet::iterator it = namelist.begin(); it != namelist.end(); ++it)
      vars.SetAt(*it, GetVar(*it));

    PString replyBody;
    if (http->PostData(url, sendMIME, vars, replyMIME, replyBody) &&
        InternalLoadVXML(url, replyBody, PString::Empty()))
      return true;

    return ThrowError(element, PSTRSTRM(ErrorBadFetch << '.' << url.GetScheme() << '.' << http->GetLastResponseCode()),
                      "<submit> POST " << url << " failed with " << http->GetLastResponseCode() << ' ' << http->GetLastResponseInfo());
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
      PTRACE(2, "<submit> does not (yet) support submissions of types other than \"audio/wav\"");
      continue;
    }

    PFile file(GetVar(*itName + ".filename"), PFile::ReadOnly);
    if (!file.IsOpen()) {
      PTRACE(2, "<submit> could not find file \"" << file.GetFilePath() << '"');
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

  if (entityBody.IsEmpty())
    return ThrowError(element, PSTRSTRM(ErrorBadFetch << '.' << url.GetScheme() << '.' << http->GetLastResponseCode()),
                      "<submit> could not find anything to send using \"" << setfill(',') << namelist << '"');

  PString replyBody;
  if (http->PostData(url, sendMIME, entityBody, replyMIME, replyBody) &&
      InternalLoadVXML(url, replyBody, PString::Empty()))
    return true;

  return ThrowError(element, PSTRSTRM(ErrorBadFetch << '.' << url.GetScheme() << '.' << http->GetLastResponseCode()),
                    "<submit> POST " << url << " failed with " << http->GetLastResponseCode() << ' ' << http->GetLastResponseInfo());
}


PString PVXMLSession::InternalGetName(PXMLElement & element, bool allowScope)
{
  PString name = element.GetAttribute(NameAttribute);

  if (name.empty()) {
    ThrowSemanticError(element, "No \"name\" attribute, or attribute empty");
    return PString::Empty();
  }

  if (!isalpha(name[0]) ||
      name.FindOneOf("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.$") == P_MAX_INDEX ||
      name[name.length()-1] == '$') {
    ThrowSemanticError(element, "Illegal \"name\" attribute \"" << name << '"');
    return PString::Empty();
  }

  if (allowScope)
    return name;

  if (name.find('.') != string::npos) {
    ThrowSemanticError(element, "Scope not allowed in \"name\" attribute \"" << name << '"');
    return PString::Empty();
  }

  return name;
}


PBoolean PVXMLSession::TraverseProperty(PXMLElement & element)
{
  PString name = InternalGetName(element, false);
  if (name.empty())
    return false;

  SetProperty(name, element.GetAttribute("value"));
  return true;
}


PBoolean PVXMLSession::TraverseTransfer(PXMLElement & element)
{
  if (IsFinalProcessing())
    return false;

  if (!ExecuteCondition(element))
    return false;

  m_transferStatus = NotTransfering;
  return true;
}


PBoolean PVXMLSession::TraversedTransfer(PXMLElement & element)
{
  PString elementName = InternalGetName(element, false);
  if (elementName.empty())
    return ThrowSemanticError(element, "<transfer> must have a name");

  if (!PAssert(m_transferStatus == NotTransfering, PLogicError))
    return true;

  TransferType type = BridgedTransfer;
  if (element.GetAttribute("bridge").IsFalse())
    type = BlindTransfer;
  else {
    PCaselessString typeStr = element.GetAttribute(TypeAttribute);
    if (typeStr == "blind")
      type = BlindTransfer;
    else if (typeStr == "consultation")
      type = ConsultationTransfer;
  }

  m_transferStartTime.SetCurrentTime();

  PString dest = element.GetAttribute(DestAttribute);
  if (dest.empty()) {
    if (EvaluateExpr(element, DestExprAttribute, dest))
      return true;
  }

  m_transferStatus = TransferInProgress;
  if (OnTransfer(dest, type)) {
    if (type == BlindTransfer) {
      if (element.HasAttribute("connecttimeout"))
        m_transferTimeout = PTimeInterval(element.GetAttribute("connecttimeout"));
      else
        m_transferTimeout.SetInterval(0, 0, 1);
    }
    return false;
  }

  CompletedTransfer(element);
  return false;
}


bool PVXMLSession::OnTransfer(const PString & /*destination*/, TransferType /*type*/)
{
  return true;//false;
}


void PVXMLSession::OnTransferTimeout(PTimer&, P_INT_PTR)
{
  SetTransferComplete(false);
}


void PVXMLSession::SetTransferComplete(bool state)
{
  PTRACE(3, "Transfer " << (state ? "completed" : "failed"));
  m_transferStatus = state ? TransferSuccessful : TransferFailed;
  Trigger();
}


void PVXMLSession::CompletedTransfer(PXMLElement & element)
{
  InternalSetVar(element.GetAttribute(NameAttribute) + '$',
                 "duration",
                 (PTime() - m_transferStartTime).AsString(0, PTimeInterval::SecondsOnly));

  if (m_transferStatus != TransferSuccessful) {
    PTRACE(3, "Transfer failed in " << element.PrintTrace());
    GoToEventHandler(element, ErrorElement, true, true);
  }
  else {
    PTRACE(3, "Transfer successful in " << element.PrintTrace());
    GoToEventHandler(element, FilledElement, false, true);
    m_closing = true;
  }
  m_transferStatus = TransferCompleted;
}


PBoolean PVXMLSession::TraverseMenu(PXMLElement & element)
{
  if (IsFinalProcessing())
    return false;

  if (element.GetParent() && element.GetParent()->GetName() != "vxml")
    return ThrowSemanticError(element, "<menu> must be at top level.");

  while (m_scriptContext->GetScopeChain().back() != ApplicationScope && m_scriptContext->GetScopeChain().back() != DocumentScope)
    m_scriptContext->PopScopeChain(true);
  m_scriptContext->PushScopeChain(DialogScope, true);

  LoadGrammar(PVXMLGrammarInit(MenuElement, *this, element));
  m_defaultMenuDTMF = element.GetAttribute(DtmfAttribute).IsTrue() ? '1' : 'N';
  ++m_promptCount;
  return true;
}


PBoolean PVXMLSession::TraversedMenu(PXMLElement &)
{
  StartGrammars();
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
  m_promptMode = e_FinalProcessing;
  return GoToEventHandler(element, "connection.disconnect.hangup", true, true);
}


PBoolean PVXMLSession::TraverseForm(PXMLElement & element)
{
  if (IsFinalProcessing())
    return false;

  if (element.GetParent() && element.GetParent()->GetName() != "vxml")
    return ThrowSemanticError(element, "<form> must be at top level.");

  m_dialogFieldNames.clear();

  PXMLElement * field;
  for (PINDEX i = 0; (field = element.GetElement(FieldElement, i)) != NULL; ++i) {
    PString name = field->GetAttribute(NameAttribute);
    if (m_dialogFieldNames.Contains(name)) {
      ThrowSemanticError(*field, PSTRSTRM("Already has name \"" << name << '"'));
      return false;
    }
    m_dialogFieldNames += name;
  }

  while (m_scriptContext->GetScopeChain().back() != ApplicationScope && m_scriptContext->GetScopeChain().back() != DocumentScope)
    m_scriptContext->PopScopeChain(true);
  m_scriptContext->PushScopeChain(DialogScope, true);
  ++m_promptCount;
  return true;
}


PBoolean PVXMLSession::TraversedForm(PXMLElement &)
{
  return true;
}


PBoolean PVXMLSession::TraversePrompt(PXMLElement & element)
{
  if (m_promptCount > 1 && m_promptMode == e_NormalPrompt) {
    PTRACE(3, "No reprompt preventing execution: " << element.PrintTrace());
    return false;
  }

  PXMLElement * validPrompt = FindElementWithCount(*element.GetParent(), PromptElement, m_promptCount);
  if (validPrompt != &element) {
    PTRACE(3, "Prompt count attribute preventing execution: " << element.PrintTrace());
    return false;
  }

  if (GetProperty(BargeInProperty, &element).IsFalse()) {
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

  m_bargeIn = !GetProperty(BargeInProperty).IsFalse();
  return true;
}


PBoolean PVXMLSession::TraverseField(PXMLElement & element)
{
  if (IsFinalProcessing())
    return false;

  if (!ExecuteCondition(element))
    return false;

  PString name = InternalGetName(element, false);
  if (name.empty())
    return false;

  m_scriptContext->CreateComposite(PSTRSTRM(DialogScope << '.' << name << '$'));
  SetDialogVar(name, PString::Empty());

  PString type = element.GetAttribute(TypeAttribute);
  if (!type.empty())
    LoadGrammar(PVXMLGrammarInit(type, *this, element));

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
  , m_usingDTMF(false)
  , m_confidence(1.0)
  , m_state(Idle)
{
  m_timer.SetNotifier(PCREATE_NOTIFIER(OnTimeout), "VXMLGrammar");
}


void PVXMLGrammar::OnRecognition(PSpeechRecognition &, PSpeechRecognition::Transcript transcript)
{
  if (Start()) {
    m_usingDTMF = false;
    if (m_confidence > transcript.m_confidence)
      m_confidence = transcript.m_confidence;
    OnInput(transcript.m_content);
  }
}


void PVXMLGrammar::OnUserInput(const PString & input)
{
  if (m_allowDTMF && Start()) {
    m_usingDTMF = true;
    m_confidence = 1.0;
    OnInput(input);
  }
}


void PVXMLGrammar::OnAudioInput(const short * samples, size_t count)
{
  if (m_recogniser)
    m_recogniser->Listen(samples, count);
}


void PVXMLGrammar::SetTimeout(const PTimeInterval & timeout)
{
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

  if (m_terminators.empty())
    m_terminators = m_session.GetProperty(TermCharProperty);
  if (m_noInputTimeout == 0)
    m_noInputTimeout = m_session.GetTimeProperty(TimeoutProperty);

  PString inputModes = m_session.GetProperty(InputModesProperty, m_grammarElement, ModeAttribute);

  // Can't be empty, default to DTMF
  m_allowDTMF = inputModes.empty() || inputModes.Find(DtmfAttribute) != P_MAX_INDEX;

  bool allowVoice = inputModes.Find(VoiceAttribute) != P_MAX_INDEX;
  if (m_grammarElement != NULL) {
    PCaselessString attrib = m_grammarElement->GetAttribute(ModeAttribute);
    if (!attrib.empty())
      allowVoice = attrib == VoiceAttribute;
  }

  if (allowVoice) {
    m_recogniser = m_session.CreateSpeechRecognition();
    if (m_recogniser == NULL) {
      PTRACE(2, "Grammar " << *this << " could not create speech recognition on \"" << m_session.GetSpeechRecognition() << '"');
      m_allowDTMF = true; // Turn on despite element indication, as no other way for input!
    }
    else {
      PStringToString options;
      m_session.m_httpMutex.Wait();
      m_session.m_httpProxies.ToOptions(options);
      m_session.m_httpMutex.Signal();
      m_recogniser->SetOptions(options);
    }
  }

  m_timer = m_noInputTimeout;

  if (m_recogniser != NULL) {
    PString lang;
    if (m_grammarElement != NULL)
      lang = m_grammarElement->GetAttribute("xml:lang");
    if (lang.empty())
      lang = m_session.GetLanguage();
    if (!lang.empty() && !m_recogniser->SetLanguage(lang))
      return ThrowError2(m_session, m_field, "error.unsupported.language",
                         "Grammar " << *this << " could not set speech recognition language to \"" << lang << '"');

    // Currentlly, in AWS world, it takes several seconds to create a vocabulary.
    // So we need to figure out another solution than setting it up here.
    //PStringArray words;
    //GetWordsToRecognise(words);
    //m_recogniser->SetVocabulary()
    if (!m_recogniser->Open(PCREATE_NOTIFIER(OnRecognition))) {
      PTRACE(2, "Grammar " << *this << " could not open speech recognition language to \"" << lang << '"');
      delete m_recogniser;
      m_recogniser = NULL;
    }
  }

  PTRACE(3, "Grammar " << *this << " started:"
         " inputModes=" << inputModes << ","
         " dtmf=" << boolalpha << m_allowDTMF << ","
         " voice=" << (m_recogniser != NULL) << ","
         " noInputTimeout=" << m_noInputTimeout);
  return true;
}


void PVXMLGrammar::SetPartFilled(const PString & input)
{
  m_value += input;

  PTimeInterval timeout = m_session.GetTimeProperty(m_usingDTMF ? InterDigitTimeoutProperty : IncompleteTimeoutProperty);

  GrammarState prev = Started;
  if (m_state.compare_exchange_strong(prev, PartFill)) {
    PTRACE(3, "Grammar " << *this << " start part fill: value=" << m_value.ToLiteral() << ", timeout=" << timeout);
    m_session.ClearBargeIn();
  }
  else {
    PTRACE_IF(4, !m_value.empty(), "Grammar " << *this << " part filled: value=" << m_value.ToLiteral() << ", timeout=" << timeout);
  }

  m_timer = timeout;
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

  m_timer.Stop(false);
  m_value += input;
  PTRACE(3, "Grammar " << *this << " filled: value=" << m_value.ToLiteral());
  Finished();
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
  Finished();
}


void PVXMLGrammar::Finished()
{
  if (!m_fieldName.empty() && !m_value.empty()) {
    m_session.SetDialogVar(m_fieldName+"$.utterance", m_value);
    m_session.SetDialogVar(m_fieldName+"$.confidence", PSTRSTRM(m_confidence));
    m_session.SetDialogVar(m_fieldName+"$.inputmode", m_usingDTMF ? DtmfAttribute : VoiceAttribute);
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
      if (m_field.GetName() == ChoiceElement)
        return m_session.SelectMenuChoice(m_field);
      m_session.SetDialogVar(m_fieldName, m_value);
      m_field.SetAttribute(InternalFilledStateAttribute, true);
      handler = FilledElement;
      break;

    case NoInput:
      handler = NoInputElement;
      break;

    case NoMatch:
      handler = NoMatchElement;
      break;

    default:
      return false;
  }

  if (m_session.GoToEventHandler(m_field, handler, false, true))
    return true;

  PTRACE(2, "Grammar: " << *this << ", restarting node " << PXMLObject::PrintTrace(&m_field));
  m_state = Idle;
  m_timer = m_noInputTimeout;
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
  for (PINDEX i = 0; i < input.length(); ++i) {
    if (isdigit(input[i])) {
      SetFilled(input);
      break;
    }
  }
}


bool PVXMLMenuGrammar::Process()
{
  if (m_state == Filled) {
    PXMLElement * choice;
    PINDEX index = 0;
    while ((choice = m_field.GetElement(ChoiceElement, index++)) != NULL) {
      // Check if DTMF value for grammarResult matches the DTMF value for the choice
      if (choice->GetAttribute(DtmfAttribute) == m_value) {
        PTRACE(3, "Grammar " << *this << " matched menu choice: " << m_value << " to " << choice->PrintTrace());
        return m_session.SelectMenuChoice(*choice);
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
  while ((choice = m_field.GetElement(ChoiceElement, index++)) != NULL) {
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
  , m_maxDigits(10)
{
  if (init.m_grammarElement == NULL)
    return;

  if (init.m_parameters.empty()) {
    PStringOptions tokens;
    PURL::SplitVars(init.m_grammarElement->GetData(), tokens, ';', '=');

    m_minDigits = tokens.GetVar("minDigits", m_minDigits);
    m_maxDigits = tokens.GetVar("maxDigits", m_maxDigits);

    if (tokens.Contains("terminators")) {
      m_terminators = tokens.GetString("terminators");

      // An empty terminators token is set to an impossible DTMF value, as if m_terminators
      // is an empty string, it is set to the terminators property in PVXMLGrammar::Start()
      if (m_terminators.empty())
        m_terminators = '\n';
    }
  }
  else if (init.m_parameters.Contains("length"))
    m_minDigits = m_maxDigits = init.m_parameters.GetVar("length", m_minDigits);
  else {
    m_minDigits = init.m_parameters.GetVar("minlength", m_minDigits);
    m_maxDigits = init.m_parameters.GetVar("maxlength", m_maxDigits);
  }

  if (m_minDigits == 0)
    m_minDigits = 1;

  if (m_maxDigits < m_minDigits)
    m_maxDigits = m_minDigits;
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
  else {
    len += input.length();
    if (len >= m_maxDigits)
      SetFilled(input);
    else if (len >= m_minDigits)
      SetPartFilled(input);
    else
      m_value += input; // Add to collected digits string
  }
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
  , m_yes(init.m_parameters.Get("y", "1"))
  , m_no(init.m_parameters.Get("n", "2"))
{
}


void PVXMLBooleanGrammar::OnInput(const PString & input)
{
  if (m_yes == input || (input *= "yes"))
    SetFilled(PString(true));
  else if (m_no == input || (input *= "no"))
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


void PVXMLTextGrammar::OnInput(const PString & input)
{
  if (m_terminators.Find(input) != P_MAX_INDEX)
    SetFilled(input);
  else
    SetPartFilled(input);
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
  PString src = init.m_grammarElement->GetAttribute(SrcAttribute);
  if (src.empty())
    grammar = init.m_grammarElement;
  else {
    loadedXML.reset(new PXML);
    PBYTEArray xmlText;
    if (!m_session.LoadCachedResource(src,
                                      init.m_grammarElement,
                                      GrammarMaxAgeProperty,
                                      GrammarMaxStaleProperty,
                                      xmlText) ||
        !loadedXML->Load(xmlText)) {
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
    if (!m_vxmlSession->LoadCachedResource(arg, NULL,AudioMaxAgeProperty, AudioMaxStaleProperty, data)) {
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
