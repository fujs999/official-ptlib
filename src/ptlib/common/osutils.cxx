/*
 * osutils.cxx
 *
 * Operating System utilities.
 *
 * Portable Tools Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
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
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 */

#include <ptlib.h>
#include <vector>
#include <stack>
#include <map>
#include <fstream>
#include <algorithm>

#include <ctype.h>
#include <ptlib/pfactory.h>
#include <ptlib/id_generator.h>
#include <ptlib/pprocess.h>
#include <ptlib/svcproc.h>
#include <ptlib/pluginmgr.h>
#include <ptlib/syslog.h>
#include <ptclib/random.h>
#include "../../../version.h"
#include "../../../revision.h"

#ifdef _WIN32
  #include <ptlib/msos/ptlib/debstrm.h>
#elif defined(P_MACOSX)
  #include <mach-o/dyld.h>
  #include <ptlib/videoio.h>
#endif


#define PTraceModule() "PTLib"

static const char DefaultRollOverPattern[] = "_yyyy_MM_dd_hh_mm";

class PExternalThread : public PThread
{
  PCLASSINFO(PExternalThread, PThread);
  public:
    PExternalThread()
      : PThread(Type::IsExternal, "External", PThread::NumPriority)
    {
      InternalThreadStarted();
      PProcess::Current().InternalThreadStarted(this);
      PTRACE(5, "Created external thread " << *this);
    }

    ~PExternalThread()
    {
      PTRACE(5, "Destroyed external thread " << *this);
    }

    virtual void Main()
    {
    }

    virtual void Terminate()
    {
      PTRACE(2, "Cannot terminate external thread " << this << ","
                " thread-id=" << GetThreadId() << ","
                " unique-id=" << GetUniqueIdentifier());
    }
};


class PSimpleThread : public PThread
{
    PCLASSINFO(PSimpleThread, PThread);
  public:
    PSimpleThread(
      const PNotifier & notifier,
      intptr_t parameter,
      AutoDeleteFlag deletion,
      Priority priorityLevel,
      const PString & threadName
    );

    ~PSimpleThread()
    {
        WaitForTermination();
    }

    void Main();

  protected:
    PNotifier callback;
    intptr_t parameter;
};


#define new PNEW


#ifndef __NUCLEUS_PLUS__
static ostream * PErrorStream = &cerr;
#else
static ostream * PErrorStream = NULL;
#endif

ostream & PGetErrorStream()
{
  return *PErrorStream;
}


void PSetErrorStream(ostream * s)
{
#ifndef __NUCLEUS_PLUS__
  PErrorStream = s != NULL ? s : &cerr;
#else
  PErrorStream = s;
#endif
}

//////////////////////////////////////////////////////////////////////////////

#if PTRACING

unsigned PTrace::MaxStackWalk = 32;

class PTraceInfo : public PTrace
{
  /* NOTE you cannot have any complex types in this structure. Anything
     that might do an asert or PTRACE will crash due to recursion.
   */

public:
  atomic<unsigned> m_currentLevel;
  atomic<unsigned> m_thresholdLevel;
  atomic<unsigned> m_options;
  PCaselessString  m_filename;
  ostream        * m_stream;
  PTimeInterval    m_startTick;
  PString          m_rolloverPattern;
  unsigned         m_lastRotate;
  atomic<PINDEX>   m_maxLength;
  std::recursive_mutex m_mutex;

  struct Context : PObject
  {
    Context(unsigned level, const char * fileName, int lineNum, const PObject * instance, const char * module)
      : m_level(level)
      , m_fileName(fileName)
      , m_lineNum(lineNum)
      , m_objectAddress(instance)
      , m_threadAddress(PThread::Current())
      , m_threadName(m_threadAddress ? m_threadAddress->GetThreadName() : "")
      , m_contextIdentifier(instance ? instance->GetTraceContextIdentifier() : 0)
      , m_module(module)
      , m_tick(PTimer::Tick())
      , m_blockIndentLevel(0)
    {
      if (m_contextIdentifier == 0 && m_threadAddress != NULL)
        m_contextIdentifier = m_threadAddress->GetTraceContextIdentifier();
      if (instance)
        m_objectClass = instance->GetClassName();
    }

    unsigned      m_level;
    PFilePath     m_fileName;
    int           m_lineNum;
    void  const * m_objectAddress;
    PString       m_objectClass;
    PThread     * m_threadAddress;
    PString       m_threadName;
    unsigned      m_contextIdentifier;
    PString       m_module;
    PTime         m_dateTime;
    PTimeInterval m_tick;
    unsigned      m_blockIndentLevel;
    ostringstream m_stream;
  };
  static thread_local std::stack<Context> sm_threadContext;

  PTraceInfo()
    : m_currentLevel(0)
    , m_thresholdLevel(0)
    , m_options(Blocks | DateAndTime | Thread | FileAndLine | HasFilePermissions | (PFileInfo::DefaultPerms << FilePermissionShift))
#ifdef __NUCLEUS_PLUS__
    , m_stream(NULL)
#else
    , m_stream(&cerr)
#endif
    , m_startTick(PTimer::Tick())
    , m_rolloverPattern(DefaultRollOverPattern)
    , m_lastRotate(0)
    , m_maxLength(10000)
  {
  }

  void InitialiseFromEnvironment()
  {
    PString levelEnv = PConfig::GetEnv("PTLIB_TRACE_LEVEL");
    PString fileEnv = PConfig::GetEnv("PTLIB_TRACE_FILE");
    PString optEnv = PConfig::GetEnv("PWLIB_TRACE_OPTIONS");
    PString mutEnv = PConfig::GetEnv("PTLIB_MUTEX_CTOR_DTOR_LOG");
    if (!mutEnv.empty())
      PTimedMutex::CtorDtorLogLevel = atoi(mutEnv);

    if (levelEnv != NULL || fileEnv != NULL || optEnv != NULL)
      InternalInitialise(levelEnv != NULL ? atoi(levelEnv) : m_thresholdLevel.load(),
                         fileEnv,
                         NULL,
                         optEnv != NULL ? atoi(optEnv) : m_options.load());
  }

  ~PTraceInfo()
  {
    if (m_stream != &cerr && m_stream != &cout)
      delete m_stream;
  }

  static PTraceInfo & Instance()
  {
    static PTraceInfo info;
    return info;
  }

  void SetStream(ostream * newStream)
  {
#ifndef __NUCLEUS_PLUS__
    if (newStream == NULL)
      newStream = &cerr;
#endif

    // Delete the old stream outside the lock to avoid potential deadlock caused by
    // stream flush; the PSystemLog mutex should be locked before the PTraceInfo one
    ostream * oldStream = NULL;

    m_mutex.lock();

    if (m_stream != &cerr && m_stream != &cout)
      delete m_stream;
    m_stream = newStream;

    m_mutex.unlock();

    delete oldStream;
  }

  bool AdjustOptions(unsigned addedOptions, unsigned removedOptions)
  {
    /* Not strictly thread safe when this function is called by multiple
       threads, but this should be very, very rare as options are usually
       set up during initialisation and not everywhere in the code */
    unsigned newOptions = (m_options&~removedOptions)|addedOptions;
    if ((newOptions & HasFilePermissions) == 0)
      newOptions |= HasFilePermissions | (PFileInfo::DefaultPerms << FilePermissionShift);

    if (m_options.exchange(newOptions) == newOptions)
      return false;

#if P_SYSTEMLOG
    bool syslogBit = (newOptions&SystemLogStream) != 0;
    bool syslogStrm = dynamic_cast<PSystemLog *>(m_stream) != NULL;
    if (syslogBit != syslogStrm) {
      SetStream(syslogBit ? new PSystemLog : &cerr);
      PSystemLog::GetTarget().SetThresholdLevel(PSystemLog::LevelFromInt(m_thresholdLevel));
    }
#endif

    return true;
  }


  bool HasOption(unsigned options) const { return (m_options & options) != 0; }

  void OpenTraceFile(const char * newFilename, bool outputFirstLog, const PTime & now = PTime())
  {
    PMEMORY_IGNORE_ALLOCATIONS_FOR_SCOPE;

    if ((m_options & RotateLogMask) == 0 && m_filename == newFilename)
      return;

    m_filename = newFilename == NULL || *newFilename == '\0' ? "stderr" : newFilename;
    PStringArray tokens = m_filename.Tokenise(',');

    AdjustOptions(0, SystemLogStream);

    if (m_filename == "stderr" || !PProcess::IsInitialised())
      SetStream(&cerr);
    else if (m_filename == "stdout")
      SetStream(&cout);
#ifdef _WIN32
    else if (m_filename == "DEBUGSTREAM")
      SetStream(new PDebugStream);
#endif
#if P_SYSTEMLOG
#ifdef P_SYSTEMLOG_TO_SYSLOG
    else if (tokens[0] *= "syslog") {
      PSystemLog::SetTarget(new PSystemLogToSyslog(tokens[1],
                                                   tokens.GetSize() > 2 ? tokens[2].AsInteger() : -1,
                                                   tokens.GetSize() > 3 ? tokens[3].AsInteger() : -1,
                                                   tokens.GetSize() > 4 ? tokens[4].AsInteger() : -1));
      AdjustOptions(SystemLogStream, 0);
    }
#endif
    else if (tokens[0] *= "network") {
      switch (tokens.GetSize()) {
        case 1 :
          PSystemLog::SetTarget(new PSystemLogToNetwork("localhost"));
          break;

        case 2 :
          PSystemLog::SetTarget(new PSystemLogToNetwork(tokens[1]));
          break;

        default :
          PSystemLog::SetTarget(new PSystemLogToNetwork(tokens[1], PSystemLogToNetwork::RFC3164_Port, tokens[2].AsInteger()));
      }
      AdjustOptions(SystemLogStream, 0);
    }
#endif
    else {
      PDirectory dir(m_filename);
      if (dir.Exists())
        m_filename = dir + "%N_%P.log";

      PFilePath fn(m_filename);

      PString rollover;
      if ((m_options & RotateLogMask) != 0)
        rollover = now.AsString(m_rolloverPattern, ((m_options&GMTTime) ? PTime::GMT : PTime::Local));

      fn.Replace("%D", rollover, true);
      fn.Replace("%N", PProcess::Current().GetName(), true);
      fn.Replace("%P", PString(PProcess::GetCurrentProcessID()), true);

      if (!rollover.empty() && fn.Find(rollover) == P_MAX_INDEX)
        fn = PSTRSTRM(fn.GetDirectory() << fn.GetTitle() << rollover << fn.GetType());

      PFile::OpenOptions options = PFile::Create;
      if ((m_options & AppendToFile) == 0)
        options |= PFile::Truncate;
      PFileInfo::Permissions permissions = PFileInfo::DefaultPerms;
      if ((m_options & HasFilePermissions) != 0)
        permissions.FromBits((m_options&FilePermissionMask)>>FilePermissionShift);

      PFile * traceOutput = new PTextFile();
      if (traceOutput->Open(fn, PFile::WriteOnly, options, permissions)) {
        traceOutput->SetPosition(0, PFile::End);
        SetStream(traceOutput);
        outputFirstLog = true;
      }
      else {
        ostringstream msgstrm;
        if (PProcess::IsInitialised())
          msgstrm << PProcess::Current().GetName() << ": ";
        msgstrm << "Could not open trace output file  \"" << fn << "\"\n"
                << traceOutput->GetErrorText();
#ifdef WIN32
        PVarString msg(msgstrm.str().c_str());
        MessageBox(NULL, msg, NULL, MB_OK|MB_ICONERROR);
#else
        fputs(msgstrm.str().c_str(), stderr);
#endif
        delete traceOutput;
        SetStream(&cerr);
      }
    }

    if (outputFirstLog) {
      sm_threadContext.emplace(1, nullptr, 0, nullptr, PTraceModule());
      ostream & log = sm_threadContext.top().m_stream;

      if (PProcess::IsInitialised()) {
        PProcess & process = PProcess::Current();
        log << process.GetName()
            << " version " << process.GetVersion(true);
#if _DEBUG
        log << " [debug]";
#endif
        log << " by " << process.GetManufacturer()
            << " on ";
      }

      log << PProcess::GetOSClass() << ' ' << PProcess::GetOSName()
          << " (" << PProcess::GetOSVersion() << '-' << PProcess::GetOSHardware() << ")"
             " with PTLib (v" << PProcess::GetLibVersion() << ")"
             " at " << now.AsString("yyyy/M/d h:mm:ss.uuu") << ","
             " level=" << m_thresholdLevel << ", to ";
      if ((m_options & RotateLogMask) == 0)
        log << '"' << m_filename;
      else {
        log << " rollover every ";
        switch (m_options & RotateLogMask) {
          case RotateDaily :
            log << "day";
            break;
          case RotateHourly :
            log << "hour";
            break;
          case RotateMinutely :
            log << "minute";
            break;
        }
        PFilePath fn(m_filename);
        log << " to \"" << fn.GetDirectory() << fn.GetTitle() << m_rolloverPattern << fn.GetType();
      }
      log << '"' << flush;
      InternalEnd(log);
    }
  }

  void InternalInitialise(unsigned level, const char * filename, const char * rolloverPattern, unsigned options);
  std::ostream & InternalBegin(unsigned level, const char * fileName, int lineNum, const PObject * instance, const char * module);
  void InternalEnd(ostream & paramStream);
};

thread_local std::stack<PTraceInfo::Context> PTraceInfo::sm_threadContext;


void PTrace::SetStream(ostream * s)
{
  PTraceInfo & info = PTraceInfo::Instance();
  ostream * before = info.m_stream;
  info.SetStream(s);
  ostream * after = info.m_stream;
  PTRACE_IF(2, before != after, "Trace stream set to " << after << " (" << s << ')');
}


ostream * PTrace::GetStream()
{
  return PTraceInfo::Instance().m_stream;
}


int PTrace::GetTimeZone()
{
  return (GetOptions()&PTrace::GMTTime) ? PTime::GMT : PTime::Local;
}


static inline char PlusMinus(unsigned options, unsigned bit)
{
  return (options&bit) == bit ? '+' : '-';
}

static std::string OptionsToString(unsigned options)
{
  std::ostringstream strm;
  strm << PlusMinus(options, PTrace::Timestamp) << "time "
       << PlusMinus(options, PTrace::DateAndTime) << "date "
       << PlusMinus(options, PTrace::GMTTime) << "gmt "
       << PlusMinus(options, PTrace::Thread) << "thread "
       << PlusMinus(options, PTrace::FileAndLine) << "source "
       << PlusMinus(options, PTrace::ObjectInstance) << "object "
       << PlusMinus(options, PTrace::ContextIdentifier) << "context "
       << PlusMinus(options, PTrace::TraceLevel) << "level "
       << PlusMinus(options, PTrace::Blocks) << "block "
       << PlusMinus(options, PTrace::AppendToFile) << "append "
       << PlusMinus(options, PTrace::SingleLine) << "single "
       << PlusMinus(options, PTrace::OutputJSON) << "json ";

  switch (options&PTrace::RotateLogMask) {
    case PTrace::RotateDaily :
      strm << "+daily ";
      break;
    case PTrace::RotateHourly :
      strm << "+hour ";
      break;
    case PTrace::RotateMinutely :
      strm << "+minute ";
      break;
  }

  return strm.str();
}


ostream & PTrace::PrintInfo(ostream & strm, bool crlf)
{
  PTraceInfo & info = PTraceInfo::Instance();

  strm << "Level: " << info.m_thresholdLevel << ", Output: ";

  if (info.m_stream == NULL)
    strm << "null";
  else if (info.m_stream == &cout)
    strm << "stdout";
  else if (info.m_stream == &cerr)
    strm << "stderr";
  else if (dynamic_cast<PFile *>(info.m_stream) != NULL)
    strm << dynamic_cast<PFile *>(info.m_stream)->GetFilePath();
#ifdef _WIN32
  else if (dynamic_cast<PDebugStream *>(info.m_stream) != NULL)
    strm << "debugstream";
#endif
#if P_SYSTEMLOG
#ifdef P_SYSTEMLOG_TO_SYSLOG
  else if (dynamic_cast<PSystemLogToSyslog *>(info.m_stream) != NULL)
    strm << "syslog";
#endif
  else if (dynamic_cast<PSystemLogToNetwork *>(info.m_stream) != NULL)
    strm << "network: " << dynamic_cast<PSystemLogToNetwork *>(info.m_stream)->GetServer();
#endif
  else
    strm << PObject::GetClassName(typeid(*info.m_stream));

  strm << ", Options:" << OptionsToString(info.m_options) << info.m_rolloverPattern;

  if (crlf)
    strm << endl;

  return strm;
}


static void SetOptionBit(unsigned & options, unsigned option)
{
  options |= option;
}


static void ClearOptionBit(unsigned & options, unsigned option)
{
  options &= ~option;
}


static PString GetOptionOrParameter(const PArgList & args, const char * opt, const char * dflt = NULL)
{
  if (opt == NULL)
    return dflt;

  if (strspn(opt, "0123456789") < strlen(opt))
    return args.GetOptionString(opt, dflt);

  PINDEX optNum = atoi(opt);
  if (optNum < args.GetCount())
    return args[optNum];

  return dflt;
}


static unsigned OptionsFromString(const PCaselessString & optStr, unsigned options)
{
  PINDEX pos = 0;
  while ((pos = optStr.FindOneOf("+-", pos)) != P_MAX_INDEX) {
    void (*operation)(unsigned & options, unsigned option) = optStr[pos++] == '+' ? SetOptionBit : ClearOptionBit;
    if (optStr.NumCompare("block", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::Blocks);
    else if (optStr.NumCompare("date", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::DateAndTime);
    else if (optStr.NumCompare("time", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::Timestamp);
    else if (optStr.NumCompare("thread", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::Thread);
    else if (optStr.NumCompare("level", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::TraceLevel);
    else if (optStr.NumCompare("file", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::FileAndLine);
    else if (optStr.NumCompare("source", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::FileAndLine);
    else if (optStr.NumCompare("object", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::ObjectInstance);
    else if (optStr.NumCompare("context", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::ContextIdentifier);
    else if (optStr.NumCompare("single", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::SingleLine);
    else if (optStr.NumCompare("json", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::OutputJSON);
    else if (optStr.NumCompare("gmt", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::GMTTime);
    else if (optStr.NumCompare("utc", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::GMTTime);
    else if (optStr.NumCompare("daily", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::RotateDaily);
    else if (optStr.NumCompare("hour", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::RotateHourly);
    else if (optStr.NumCompare("minute", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::RotateMinutely);
    else if (optStr.NumCompare("append", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PTrace::AppendToFile);
    else if (optStr.NumCompare("ax", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, (PFileInfo::WorldExecute|PFileInfo::GroupExecute|PFileInfo::UserExecute) << PTrace::FilePermissionShift);
    else if (optStr.NumCompare("aw", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, (PFileInfo::WorldWrite|PFileInfo::GroupWrite|PFileInfo::UserWrite) << PTrace::FilePermissionShift);
    else if (optStr.NumCompare("ar", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, (PFileInfo::WorldRead|PFileInfo::GroupRead|PFileInfo::UserRead) << PTrace::FilePermissionShift);
    else if (optStr.NumCompare("ox", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PFileInfo::WorldExecute << PTrace::FilePermissionShift);
    else if (optStr.NumCompare("ow", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PFileInfo::WorldWrite << PTrace::FilePermissionShift);
    else if (optStr.NumCompare("or", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PFileInfo::WorldRead << PTrace::FilePermissionShift);
    else if (optStr.NumCompare("gx", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PFileInfo::GroupExecute << PTrace::FilePermissionShift);
    else if (optStr.NumCompare("gw", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PFileInfo::GroupWrite << PTrace::FilePermissionShift);
    else if (optStr.NumCompare("gr", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PFileInfo::GroupRead << PTrace::FilePermissionShift);
    else if (optStr.NumCompare("ux", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PFileInfo::UserExecute << PTrace::FilePermissionShift);
    else if (optStr.NumCompare("uw", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PFileInfo::UserWrite << PTrace::FilePermissionShift);
    else if (optStr.NumCompare("ur", P_MAX_INDEX, pos) == PObject::EqualTo)
      operation(options, PFileInfo::UserRead << PTrace::FilePermissionShift);
  }

  return options;
}


void PTrace::Initialise(const PArgList & args,
                        unsigned options,
                        const char * traceCount,
                        const char * outputFile,
                        const char * traceOpts,
                        const char * traceRollover,
                        const char * traceLevel)
{
  PTraceInfo & info = PTraceInfo::Instance();

  PCaselessString optStr = GetOptionOrParameter(args, traceOpts);
  if (optStr.IsEmpty())
    options = info.m_options;
  else {
    if ((options & HasFilePermissions) == 0)
      options |= HasFilePermissions | (PFileInfo::DefaultPerms << FilePermissionShift);
    options = OptionsFromString(optStr, options);
  }

  int level;
  if (traceCount != NULL && args.HasOption(traceCount))
    level = args.GetOptionCount(traceCount);
  else if ((level = GetOptionOrParameter(args, traceLevel, "-1").AsInteger()) < 0)
    level = info.m_thresholdLevel;

  info.InternalInitialise(level,
                          GetOptionOrParameter(args, outputFile, info.m_filename),
                          GetOptionOrParameter(args, traceRollover),
                          options);
}


void PTrace::Initialise(unsigned level, const char * filename, unsigned options, const char * rolloverPattern)
{
  PTraceInfo::Instance().InternalInitialise(level, filename, rolloverPattern, options);
}


static unsigned GetRotateVal(unsigned options)
{
  PTime now;
  if (options & PTrace::RotateDaily)
    return now.GetDayOfYear();
  if (options & PTrace::RotateHourly) 
    return now.GetHour();
  if (options & PTrace::RotateMinutely)
    return now.GetMinute();
  return 0;
}


void PTraceInfo::InternalInitialise(unsigned level, const char * filename, const char * rolloverPattern, unsigned options)
{
  m_rolloverPattern = rolloverPattern;
  if (m_rolloverPattern.IsEmpty())
    m_rolloverPattern = DefaultRollOverPattern;
  m_lastRotate = GetRotateVal(options);
  m_thresholdLevel = level;
  AdjustOptions(options, UINT_MAX);
  OpenTraceFile(filename, level > 0);
}


void PTrace::SetFilename(const char * filename)
{
  PTraceInfo::Instance().OpenTraceFile(filename, GetLevel() > 0);
}


const char * PTrace::GetFilename()
{
  return PTraceInfo::Instance().m_filename;
}


void PTrace::SetMaxLength(PINDEX length)
{
  PTraceInfo::Instance().m_maxLength = std::max(PINDEX(50), length);
}


PINDEX PTrace::GetMaxLength()
{
  return PTraceInfo::Instance().m_maxLength;
}


void PTrace::SetOptionsByName(const char * options)
{
  SetOptions(OptionsFromString(options, GetOptions()));
}


std::string PTrace::GetOptionsByName()
{
  return OptionsToString(GetOptions());
}


void PTrace::SetOptions(unsigned options)
{
  PTraceInfo & info = PTraceInfo::Instance();
  if (info.AdjustOptions(options, 0)) {
    PTRACE(2, "Trace options 0x" << hex << options << " added, now 0x" << info.m_options);
  }
}


void PTrace::ClearOptions(unsigned options)
{
  PTraceInfo & info = PTraceInfo::Instance();
  if (info.AdjustOptions(0, options)) {
    PTRACE(2, "Trace options 0x" << hex << options << " removed, now 0x" << info.m_options);
  }
}


unsigned PTrace::GetOptions()
{
  return PTraceInfo::Instance().m_options;
}


void PTrace::SetLevel(unsigned level)
{
  if (PTraceInfo::Instance().m_thresholdLevel.exchange(level) != level) {
    PTRACE(1, "Trace threshold set to " << level);
  }
}


unsigned PTrace::GetLevel()
{
  return PTraceInfo::Instance().m_thresholdLevel;
}


bool PTrace::CanTrace(unsigned level)
{
  return PProcess::IsInitialised() && level <= GetLevel();
}


ostream & PTrace::Begin(unsigned level, const char * fileName, int lineNum, const PObject * instance, const char * module)
{
  return PTraceInfo::Instance().InternalBegin(level, fileName, lineNum, instance, module);
}


ostream & PTraceInfo::InternalBegin(unsigned level, const char * fileName, int lineNum, const PObject * instance, const char * module)
{
  if (!PProcess::IsInitialised())
    return *GetStream();

  sm_threadContext.emplace(level, fileName, lineNum, instance, module);

  m_mutex.lock();

  if (!m_filename.IsEmpty() && HasOption(RotateLogMask)) {
    unsigned rotateVal = GetRotateVal(m_options);
    if (rotateVal != m_lastRotate || GetStream() == &cerr) {
      m_lastRotate = rotateVal;
      OpenTraceFile(m_filename, true);
    }
  }

  m_mutex.unlock();

  return sm_threadContext.top().m_stream;
}


ostream & PTrace::End(ostream & paramStream)
{
  paramStream.flush();
  PTraceInfo::Instance().InternalEnd(paramStream);
  return paramStream;
}


void PTraceInfo::InternalEnd(ostream & paramStream)
{
  if (!PProcess::IsInitialised() || sm_threadContext.empty())
    return;

  if (&sm_threadContext.top().m_stream != &paramStream) {
    sm_threadContext.pop();
    return;
  }

  auto & context = sm_threadContext.top();
  std::stringstream output;

  bool outputJSON = HasOption(OutputJSON);
  if (outputJSON)
    output << '{';

  if (outputJSON || HasOption(DateAndTime)) {
    // Use "@timestamp" for compatibility with ELK systems
    if (outputJSON)
      output << "\"@timestamp\":\"" << context.m_dateTime.AsString(PTime::LongISO8601) << "\",";
    else if (!HasOption(SystemLogStream))
      output << context.m_dateTime.AsString(PTime::LoggingFormat, HasOption(GMTTime) ? PTime::GMT : PTime::Local) << '\t';
  }

  if (HasOption(Timestamp)) {
    if (outputJSON)
      output << "\"TimeSinceStart\":" << scientific << (context.m_tick-m_startTick) << ',';
    else
      output << setprecision(3) << setw(10) << (context.m_tick-m_startTick) << '\t';
  }

  if (HasOption(TraceLevel)) {
    if (outputJSON)
      output << "\"LogLevel\":";
    output << context.m_level << (outputJSON ? ',' : '\t');
  }

  if (HasOption(Thread)) {
#if P_64BIT && !defined(WIN32) && !P_HAS_UNIQUE_THREAD_ID_FMT
    static const PINDEX ThreadNameWidth = 31;
#else
    static const PINDEX ThreadNameWidth = 23;
#endif
    if (outputJSON)
      output << "\"ThreadName\":" << context.m_threadName.ToLiteral() << ',';
    else
      output << setw(ThreadNameWidth) << context.m_threadName.Ellipsis(ThreadNameWidth, ThreadNameWidth/2-2) << '\t';
  }

  if (HasOption(ThreadAddress)) {
    if (outputJSON)
      output << "\"ThreadAddress\":" << context.m_threadAddress << ',';
    else
      output << context.m_threadAddress << '\t';
  }

  if (HasOption(FileAndLine) && !context.m_fileName.IsEmpty()) {
    if (outputJSON)
      output << "\"FilePath\":" << context.m_fileName.ToLiteral() << ","
                "\"FileLine\":" << context.m_lineNum << ',';
    else {
      static unsigned const FileWidth = 16;
      output << setw(FileWidth) << context.m_fileName.GetFileName().Left(FileWidth);
      if (context.m_lineNum > 0)
        output << '(' << context.m_lineNum << ')';
      else
        output << "       ";
      output << '\t';
    }
  }

  if (HasOption(ObjectInstance)) {
    if (outputJSON)
      output << "\"ObjectClass\":" << context.m_objectClass.ToLiteral() << ","
                "\"ObjectAddress\":" << context.m_objectAddress << ',';
    else {
      static unsigned const ObjWidth = 31;
      if (context.m_objectAddress == NULL)
        output << setw(ObjWidth/2) << '-' << setw(ObjWidth/2+1) << ' ';
      else {
        PString addr(PSTRSTRM(hex << (uintptr_t)context.m_objectAddress));
        unsigned width = ObjWidth - addr.GetLength() - 1;
        output << setw(width) << context.m_objectClass.Ellipsis(width) << ':' << addr;
      }
      output << '\t';
    }
  }

  if (HasOption(ContextIdentifier)) {
    if (outputJSON)
      output << "\"ContextIdentifier\":" << context.m_contextIdentifier << ',';
    else {
      if (context.m_contextIdentifier != 0)
        output << setfill('0') << setw(13) << context.m_contextIdentifier << setfill(' ');
      else
        output << "- - - - - - -";
      output << '\t';
    }
  }

  PString message = context.m_stream.str();
  if (context.m_module.IsEmpty()) {
    PINDEX tab = message.Find('\t');
    if (tab != P_MAX_INDEX && tab < 16) {
      context.m_module = message.Left(tab);
      message.Delete(0, tab+1);
    }
  }

  if (outputJSON)
    output << "\"Module\":" << context.m_module.ToLiteral() << ',';
  else
    output << left << setw(8) << context.m_module << right << '\t';

  if (HasOption(SingleLine)) {
    message.Replace("\\", "\\\\", true);
    message.Replace("\r", "\\r", true);
    message.Replace("\n", "\\n", true);
  }

  if (message.GetLength() > m_maxLength)
    message.Splice("...", m_maxLength - 4, P_MAX_INDEX);

  if (outputJSON)
    output << "\"Message\":" << message.ToLiteral() << '}';
  else
    output << message;

  if (HasOption(SystemLogStream))
    PSystemLog::OutputToTarget(PSystemLog::LevelFromInt(context.m_level), output.str().c_str());
  else {
    m_mutex.lock();
    *m_stream << output.str() << endl;
    m_mutex.unlock();
  }

  sm_threadContext.pop();
}


PTrace::Block::Block(const char * fileName, int lineNum, const char * traceName)
  : file(fileName)
  , line(lineNum)
  , name(traceName)
{
  if (PTraceInfo::Instance().HasOption(Blocks)) {
    unsigned indent = 20;

    if (!PTraceInfo::Instance().sm_threadContext.empty())
      indent = (PTraceInfo::Instance().sm_threadContext.top().m_blockIndentLevel += 2);

    ostream & s = PTrace::Begin(1, file, line);
    s << "B-Entry\t";
    for (unsigned i = 0; i < indent; i++)
      s << '=';
    s << "> " << name << PTrace::End;
  }
}


PTrace::Block::Block(const Block & obj)
  : file(obj.file)
  , line(obj.line)
  , name(obj.name)
{
}


PTrace::Block::~Block()
{
  if (PTraceInfo::Instance().HasOption(Blocks)) {
    unsigned indent = 20;

    if (!PTraceInfo::Instance().sm_threadContext.empty()) {
      PTraceInfo::Context & context = PTraceInfo::Instance().sm_threadContext.top();
      indent = context.m_blockIndentLevel;
      context.m_blockIndentLevel -= 2;
    }

    ostream & s = PTrace::Begin(1, file, line);
    s << "B-Exit\t<";
    for (unsigned i = 0; i < indent; i++)
      s << '=';
    s << ' ' << name << PTrace::End;
  }
}


PTrace::ThrottleBase::ThrottleBase(unsigned lowLevel, unsigned interval, unsigned highLevel, unsigned maxShown)
  : m_interval(interval)
  , m_lowLevel(lowLevel)
  , m_highLevel(highLevel)
  , m_maxShown(maxShown)
  , m_currentLevel(highLevel)
  , m_nextLog(0)
  , m_repeatCount(0)
  , m_hiddenCount(0)
{
}


PTrace::ThrottleBase::ThrottleBase(const ThrottleBase & other)
  : m_interval(other.m_interval)
  , m_lowLevel(other.m_lowLevel)
  , m_highLevel(other.m_highLevel)
  , m_maxShown(other.m_maxShown)
  , m_currentLevel(other.m_highLevel)
  , m_nextLog(0)
  , m_repeatCount(0)
  , m_hiddenCount(0)
{
}


bool PTrace::ThrottleBase::CanTrace(int64_t now)
{
  if (!PTrace::CanTrace(m_lowLevel))
    return false;

  if (PTrace::CanTrace(m_highLevel))
    return true;

  if (now == 0)
    now = PTimer::Tick().GetMilliSeconds();

  int64_t nextLog = m_nextLog.load();
  if (now > nextLog) {
    if (m_nextLog.compare_exchange_strong(nextLog, now + m_interval)) {
      m_currentLevel = m_lowLevel;
      m_repeatCount = 1;
    }
    /* Note, there is a race in the else clause here, where one extra log could
       "escape" without being counted in m_hiddenCount or m_repeatCount. It is
       a minor thing, really, and deemed better to have that one extra output
       than to suppress it */
  }
  else if (++m_repeatCount <= m_maxShown)
    m_hiddenCount = 0; // Note this only occurs if m_currentLevel == m_lowLevel
  else {
    unsigned otherLevel = m_lowLevel;
    if (m_currentLevel.compare_exchange_strong(otherLevel, m_highLevel))
      m_hiddenCount = 0;
  }

  if (PTrace::CanTrace(m_currentLevel))
    return true;

  ++m_hiddenCount;
  return false;
}


std::ostream & operator<<(ostream & strm, const PTrace::ThrottleBase & throttle)
{
  if (throttle.m_hiddenCount > 1)
    strm << " (repeated " << std::dec << throttle.m_hiddenCount << " times)";
  return strm;
}


#if PTRACING==2

static atomic<uint32_t> g_lastContextIdentifer(0);

unsigned PTrace::GetNextContextIdentifier()
{
  return ++g_lastContextIdentifer;
}


PTraceSaveContextIdentifier::PTraceSaveContextIdentifier(const PObject & obj)
  : m_currentThread(PThread::Current())
  , m_savedContextIdentifier(m_currentThread->GetTraceContextIdentifier())
{
  m_currentThread->SetTraceContextIdentifier(obj.GetTraceContextIdentifier());
}


PTraceSaveContextIdentifier::PTraceSaveContextIdentifier(const PObject * obj)
  : m_currentThread(PThread::Current())
  , m_savedContextIdentifier(m_currentThread->GetTraceContextIdentifier())
{
  m_currentThread->SetTraceContextIdentifier(obj->GetTraceContextIdentifier());
}


PTraceSaveContextIdentifier::~PTraceSaveContextIdentifier()
{
  if (m_currentThread != NULL)
    m_currentThread->SetTraceContextIdentifier(m_savedContextIdentifier);
}

#endif // PTRACING==2

static struct PMutexLeakCheck : PCriticalSection, std::set<const PObject *> { } * s_MutexLeakCheck;

#endif // PTRACING


///////////////////////////////////////////////////////////////////////////////
// PDirectory

PDirectory::PDirectory()
  : PFilePathString(PFilePath::Canonicalise(PString::Empty(), true))
{
  Construct();
}


PDirectory::PDirectory(const char * cpathname)  
  : PFilePathString(PFilePath::Canonicalise(cpathname, true))
{
  Construct();
}
  

PDirectory::PDirectory(const PString & pathname)
  : PFilePathString(PFilePath::Canonicalise(pathname, true))
{
  Construct();
}


PDirectory::PDirectory(const PDirectory & dir)
  : PFilePathString(dir)
{
  Construct();
}


PDirectory PDirectory::GetTemporary()
{
  PString tmpdir = PConfig::GetEnv("TMPDIR");
  if (tmpdir.empty()) {
    tmpdir = PConfig::GetEnv("TMP");
    if (tmpdir.empty())
      tmpdir = PConfig::GetEnv("TEMP");
  }

  if (!tmpdir.empty())
    return tmpdir;

#ifdef _WIN32
  return PDirectory("C:\\");
#else
  return PDirectory("/tmp");
#endif
}


bool PDirectory::Create(const PString & p, int perm, bool recurse)
{
  PAssert(!p.IsEmpty(), "attempt to create dir with empty name");

  PDirectory dir = p;
  
#if defined(WIN32)
  if (_wmkdir(dir.AsWide()) == 0)
#elif defined(P_VXWORKS)
  if (mkdir(dir) == 0)
#else    
  if (mkdir(dir.Left(dir.GetLength()-1), perm) == 0)
#endif
    return true;

  return recurse && !dir.IsRoot() && dir.GetParent().Create(perm, true) && dir.Create(perm, false);
}


PDirectory::Entries PDirectory::GetEntries(const PString & glob, Sorting sortBy)
{
  Entries entries;
  GetEntries(entries, sortBy, glob);
  return entries;
}


PDirectory::Entries PDirectory::GetEntries(const PString & glob, const PCaselessString & sortBy)
{
  Entries entries;
  GetEntries(entries, sortBy, glob);
  return entries;
}


bool PDirectory::GetEntries(Entries & entries, const PCaselessString & sortBy, const PString & glob)
{
  if (sortBy.NumCompare("name") == EqualTo)
    return GetEntries(entries, SortByName, glob);

  if (sortBy.NumCompare("type") == EqualTo)
    return GetEntries(entries, SortByType, glob);

  if (sortBy.NumCompare("size") == EqualTo)
    return GetEntries(entries, SortBySize, glob);

  if (sortBy.NumCompare("created") == EqualTo)
    return GetEntries(entries, SortByCreated, glob);

  if (sortBy.NumCompare("modified") == EqualTo)
    return GetEntries(entries, SortByModified, glob);

  if (sortBy.NumCompare("accessed") == EqualTo)
    return GetEntries(entries, SortByAccessed, glob);

  if (sortBy.NumCompare("permissions") == EqualTo)
    return GetEntries(entries, SortByPermission, glob);

  return GetEntries(entries, Unsorted, glob);
}

struct PDirectorySortPred
{
  PDirectory::Sorting m_sortBy;
  PDirectorySortPred(PDirectory::Sorting sortBy) : m_sortBy(sortBy) { }

  struct Base {
    ~Base() {}
    virtual bool Compare(const PDirectory::Entry& first, const PDirectory::Entry& second) const = 0;
  };

  #define SORT_BY(key, field) \
      static struct key##Pred : Base { \
        key##Pred() { s_PredTable.m_entry[PDirectory::key] = this; } \
        bool Compare(const PDirectory::Entry& first, const PDirectory::Entry& second) const { return first.field < second.field; } \
      } s_##key##Pred

  struct Table {
    Base const * m_entry[PDirectory::NumSorting];
    Table()
    {
      SORT_BY(SortByName, m_name);
      SORT_BY(SortByType, type);
      SORT_BY(SortBySize, size);
      SORT_BY(SortByCreated, created);
      SORT_BY(SortByModified, modified);
      SORT_BY(SortByAccessed, accessed);
      SORT_BY(SortByPermission, permissions);
    }
  };
  static Table s_PredTable;

  bool operator()(const PDirectory::Entry& first, const PDirectory::Entry& second)
  {
    return s_PredTable.m_entry[m_sortBy]->Compare(first, second);
  }
};

PDirectorySortPred::Table PDirectorySortPred::s_PredTable;


bool PDirectory::GetEntries(Entries & entries, Sorting sortBy, const PString & glob)
{
  if (!Open())
    return false;

  // Convert glob to regex
  PString regexStr = glob;
  if (glob.GetLength() > 1) {
    regexStr = PRegularExpression::EscapeString(glob);
    for (PINDEX i = 0; i < regexStr.GetLength()-1; ++i) {
      if ((i == 0 || regexStr[i-1] != '\\') && regexStr[i] == '\\') {
        switch (regexStr[i+1]) {
          case '*' :
            regexStr[i] = '.';
            break;
          case '?' :
            regexStr.Splice(".", i, 2);
            break;
          case '[' :
          case ']' :
            regexStr.Delete(i, 1);
        }
      }
    }
  }
  PRegularExpression regex(regexStr);

  do {
    Entry entry;
    if (GetInfo(entry)) {
      entry.m_name = GetEntryName();
      if (glob.IsEmpty() || entry.m_name.MatchesRegEx(regex))
        entries.push_back(entry);
    }
  } while (Next());

  std::sort(entries.begin(), entries.end(), PDirectorySortPred(sortBy));

  return true;
}


#if P_TIMERS

///////////////////////////////////////////////////////////////////////////////
// PTimeInterval

int64_t PTimeInterval::InternalGet() const
{
  return m_nanoseconds.load();
}


void PTimeInterval::InternalSet(int64_t t)
{
  m_nanoseconds.store(t);
}


///////////////////////////////////////////////////////////////////////////////
// PSimpleTimer

PSimpleTimer::PSimpleTimer(long milliseconds,
                           int seconds,
                           int minutes,
                           int hours,
                           int days)
  : PTimeInterval(milliseconds, seconds, minutes, hours, days)
  , m_startTick(PTimer::Tick())
{
}


PSimpleTimer::PSimpleTimer(const PTimeInterval & time)
  : PTimeInterval(time)
  , m_startTick(PTimer::Tick())
{
}


PSimpleTimer::PSimpleTimer(const PSimpleTimer & timer)
  : PTimeInterval(timer)
  , m_startTick(PTimer::Tick())
{
}


PSimpleTimer & PSimpleTimer::operator=(uint32_t milliseconds)
{
  PTimeInterval::operator=(milliseconds);
  m_startTick = PTimer::Tick();
  return *this;
}


PSimpleTimer & PSimpleTimer::operator=(const PTimeInterval & time)
{
  PTimeInterval::operator=(time);
  m_startTick = PTimer::Tick();
  return *this;
}


PSimpleTimer & PSimpleTimer::operator=(const PSimpleTimer & timer)
{
  PTimeInterval::operator=(timer);
  m_startTick = PTimer::Tick();
  return *this;
}


void PSimpleTimer::InternalSet(int64_t t)
{
  PTimeInterval::InternalSet(t);
  m_startTick = PTimer::Tick();
}


PTimeInterval PSimpleTimer::GetRemaining() const
{
  PTimeInterval remaining = *this - GetElapsed();
  return remaining > 0 ? remaining : PTimeInterval(0);
}


///////////////////////////////////////////////////////////////////////////////
// PTimer

static PIdGenerator s_handleGenerator;

PTimer::PTimer(long millisecs, int seconds, int minutes, int hours, int days)
  : PTimeInterval(millisecs, seconds, minutes, hours, days)
  , m_handle(s_handleGenerator.Create())
  , m_running(false)
  , m_callbackMutex(PDebugLocation("PTimerCallback"))
{
  InternalStart(true, PTimeInterval::InternalGet());
}


PTimer::PTimer(const PTimeInterval & time)
  : PTimeInterval(time)
  , m_handle(s_handleGenerator.Create())
  , m_running(false)
  , m_callbackMutex(PDebugLocation("PTimerCallback"))
{
  InternalStart(true, PTimeInterval::InternalGet());
}


PTimer::PTimer(const PTimer & timer)
  : PTimeInterval(timer.GetResetTime())
  , m_handle(s_handleGenerator.Create())
  , m_running(false)
  , m_callbackMutex(PDebugLocation("PTimerCallback"))
{
  InternalStart(true, PTimeInterval::InternalGet());
}


PTimer::List * PTimer::TimerList()
{
  return PProcess::Current().m_timerList;
}


PTimer::~PTimer()
{
  Stop(); // And wait, if necessary
  s_handleGenerator.Release(m_handle);
}


PTimer & PTimer::operator=(const PTimer & timer)
{
  SetMilliSeconds(timer.GetMilliSeconds());
  return *this;
}

void PTimer::PrintOn(ostream & strm) const
{
  PTimeInterval::PrintOn(strm);
  strm << '/' << GetResetTime() << '[' << m_handle << ']';
}


int64_t PTimer::InternalGet() const
{
  if (!m_running)
      return 0;

  PTimeInterval diff = m_absoluteTime - Tick();
  if (diff < 0)
    diff = 0;
  return diff.GetNanoSeconds();
}


bool PTimer::IsRunning() const
{
  if (m_running)
      return true;

  if (!m_callbackMutex.Try())
      return true;

  m_callbackMutex.Signal();
  return false;
}


void PTimer::InternalSet(int64_t nanoseconds)
{
  InternalStart(m_oneshot, nanoseconds);
}


void PTimer::RunContinuous(const PTimeInterval & time)
{
  if (!m_running || GetResetTime() != time)
    InternalStart(false, time.GetNanoSeconds());
}


void PTimer::InternalStart(bool once, int64_t resetTime)
{
  List * list = TimerList();
  if (PAssertNULL(list) == NULL)
    return;

  Stop();

  /* We can use non-mutexed access here because the current implementation
     guarantees that system threads can't access the timer while it's stopped. */

  m_oneshot = once;
  PTimeInterval::InternalSet(resetTime);

  if (resetTime > 0) {
    m_absoluteTime = Tick() + GetResetTime();
    list->m_timersMutex.Wait();
    list->m_timers[m_handle] = this;
    m_running = true;
    list->m_timersMutex.Signal();

    PProcess::Current().SignalTimerChange();
  }
}


void PTimer::Stop(bool wait)
{
  List * list = TimerList();
  if (list == NULL)
    return;

  unsigned retry = 0;
  do {
    /* Take out of timer list first, so when callback is waited for it's
       completion it cannot then be called again. Note, the bitwise OR is
       intentional! We don't want McCarthy breaking things. */
    list->m_timersMutex.Wait();
    PAssert((list->m_timers.erase(m_handle) == 1) | !m_running.exchange(false), PLogicError);
    list->m_timersMutex.Signal();

    if (wait) {
      m_callbackMutex.Wait();
      m_callbackMutex.Signal();
    }

    // We loop in case the callback function restarted the timer.
  } while (m_running && PAssert(++retry < 5, PLogicError));
}


void PTimer::Reset()
{
  InternalStart(m_oneshot, PTimeInterval::InternalGet());
}


// called only from the timer thread
void PTimer::OnTimeout()
{
  if (!m_callback)
    return;

  PString oldName;
  PThread * thread = PThread::Current();
  if (thread != NULL && !m_threadName.IsEmpty()) {
    oldName = thread->GetThreadName();
    thread->SetThreadName(m_threadName);
  }

  m_callback(*this, IsRunning());

  if (!oldName.IsEmpty())
    thread->SetThreadName(oldName);
}


///////////////////////////////////////////////////////////////////////////////
// PTimer::List

PTimer::List::List()
  : m_threadPool(10, 0, "OnTimeout")
{
}


void PTimer::List::Timeout::Work()
{
  PTimer::List * list;
  while ((list = PTimer::TimerList()) != NULL) {
    PTRACE(6, NULL, PTraceModule(), "Timer: [" << m_handle << "] working");
    if (list->OnTimeout(m_handle))
      return;

    PThread::Sleep(10);
  }
}

bool PTimer::List::OnTimeout(PIdGenerator::Handle handle)
{
  PTimer * timer = NULL;

  {
    PWaitAndSignal mutex1(m_timersMutex);

    TimerMap::iterator it = m_timers.find(handle);
    if (it == m_timers.end())
      return true; // Don't try again

    timer = it->second;

    if (!timer->m_oneshot && !timer->m_running)
      return true; // Was recurring timer and was stopped

    if (!timer->m_callbackMutex.Try()) {
      PTRACE(4, timer, PTraceModule(), "Timer: [handle=" << handle << ","
                                              " ptr=" << timer << ","
                                              " name=\"" << timer->m_threadName << "\"]"
                                              " already in OnTimeout(), waiting.");
      return false; // Try again
    }

    // Remove the expired one shot timers from map
    if (timer->m_oneshot && !timer->m_running)
      m_timers.erase(it);
  }

  // Must be outside of m_timersMutex and timer->m_timerMutex mutexes
  timer->OnTimeout();
  timer->m_callbackMutex.Signal();
  return true; // Done
}


PTimeInterval PTimer::List::Process()
{
  PTimeInterval now = PTimer::Tick();

  // Calculate interval before next Process() call
  PTimeInterval nextInterval(0, 1);

  m_timersMutex.Wait();

  for (TimerMap::iterator it = m_timers.begin(); it != m_timers.end(); ++it) {
    PTimer & timer = *it->second;
    if (timer.m_running) {
      PTimeInterval delta = timer.m_absoluteTime - now;
      if (delta > 0) {
        if (nextInterval > delta)
          nextInterval = delta;
      }
      else if (timer.m_callbackMutex.Try()) {
        /* PTimer is stopped and completely removed from the list before it's
           properties are changed from the external code, making this thread
           safe without a mutex. */
        if (timer.m_oneshot)
          timer.m_running = false;
        else {
          timer.m_absoluteTime = now + timer.GetResetTime();
          if (nextInterval > timer.GetResetTime())
            nextInterval = timer.GetResetTime();
        }
        timer.m_callbackMutex.Signal();

        m_threadPool.AddWork(new Timeout(it->first));
        PTRACE(6, &timer, "Timer: " << timer << " work added, lateness=" << -delta);
      }
    }
  }

  m_timersMutex.Signal();

  if (nextInterval < 10)
    nextInterval = 10;

  PTRACE(6, NULL, PTraceModule(), m_timers.size() << " timers processed, next=" << nextInterval);
  return nextInterval;
}


#endif //P_TIMERS


///////////////////////////////////////////////////////////////////////////////
// PArgList

PArgList::PArgList(const char * theArgStr,
                   const char * theArgumentSpec,
                   bool optionsBeforeParams)
{
  // get the program arguments
  if (theArgStr != NULL)
    SetArgs(theArgStr);
  else
    SetArgs(PStringArray());

  // if we got an argument spec - so process them
  if (theArgumentSpec != NULL)
    Parse(theArgumentSpec, optionsBeforeParams);
}


PArgList::PArgList(const PString & theArgStr,
                   const char * argumentSpecPtr,
                   bool optionsBeforeParams)
{
  // get the program arguments
  SetArgs(theArgStr);

  // if we got an argument spec - so process them
  if (argumentSpecPtr != NULL)
    Parse(argumentSpecPtr, optionsBeforeParams);
}


PArgList::PArgList(const PString & theArgStr,
                   const PString & argumentSpecStr,
                   bool optionsBeforeParams)
{
  // get the program arguments
  SetArgs(theArgStr);

  // if we got an argument spec - so process them
  Parse(argumentSpecStr, optionsBeforeParams);
}


PArgList::PArgList(int theArgc, char ** theArgv,
                   const char * theArgumentSpec,
                   bool optionsBeforeParams)
{
  // get the program arguments
  SetArgs(theArgc, theArgv);

  // if we got an argument spec - so process them
  if (theArgumentSpec != NULL)
    Parse(theArgumentSpec, optionsBeforeParams);
}


PArgList::PArgList(int theArgc, char ** theArgv,
                   const PString & theArgumentSpec,
                   bool optionsBeforeParams)
{
  // get the program name and path
  SetArgs(theArgc, theArgv);
  // we got an argument spec - so process them
  Parse(theArgumentSpec, optionsBeforeParams);
}


void PArgList::PrintOn(ostream & strm) const
{
  for (PINDEX i = 0; i < m_argumentArray.GetSize(); i++) {
    if (i > 0)
      strm << strm.fill();
    strm << m_argumentArray[i];
  }
}


void PArgList::ReadFrom(istream & strm)
{
  PString line;
  strm >> line;
  SetArgs(line);
}


ostream & PArgList::Usage(ostream & strm, const char * usage, const char * prefix) const
{
  if (!m_parseError.IsEmpty())
    strm << m_parseError << "\n\n";

  PStringArray usages = PString(usage).Lines();
  switch (usages.GetSize()) {
    case 0 :
      break;

    case 1 :
      strm << prefix << GetCommandName() << ' ' << usage << '\n';
      break;

    default :
      strm << prefix << '\n';
      PINDEX i;
      for (i = 0; i < usages.GetSize(); ++i) {
        if (usages[i].IsEmpty())
          break;
        strm << "   " << GetCommandName() << ' ' << usages[i] << '\n';
      }

      for (; i < usages.GetSize(); ++i)
        strm << usages[i] << '\n';
  }

  size_t i;
  PINDEX maxNameLength = 0;
  for (i = 0; i < m_options.size(); ++i) {
    PINDEX len = m_options[i].m_name.GetLength();
    if (maxNameLength < len)
      maxNameLength = len;
  }
  if (maxNameLength > 0)
    maxNameLength += 6;

  for (i = 0; i < m_options.size(); ++i) {
    const OptionSpec & opt = m_options[i];
    if (!opt.m_section.IsEmpty())
      strm << '\n' << opt.m_section << '\n';

    strm << "  ";
    if (opt.m_letter != '\0')
      strm << '-' << opt.m_letter;
    else
      strm << "  ";

    strm << (maxNameLength > 0 ? opt.m_letter == '\0' || opt.m_name.IsEmpty() ? "    ":  " or " : " ")
         << left;

    if (opt.m_name.IsEmpty())
      strm << setw(maxNameLength+5) << (opt.m_type == NoString ? "     " : "<arg>");
    else {
      strm << "--";
      if (opt.m_type == NoString)
        strm << setw(maxNameLength) << opt.m_name;
      else
        strm << opt.m_name << setw(maxNameLength - opt.m_name.GetLength()) << " <arg>";
    }

    PStringArray lines = opt.m_usage.Lines();
    if (lines.IsEmpty())
      strm << '\n';
    else {
      strm << "  : " << lines[0] << '\n';
      for (PINDEX j = 1; j < lines.GetSize(); ++j)
        strm << setw(maxNameLength+14) << ' ' << lines[j] << '\n';
    }
  }

  return strm;
}


PString PArgList::Usage(const char * usage, const char * prefix) const
{
  PStringStream str;
  Usage(str, usage, prefix);
  return str;
}


void PArgList::SetArgs(const char * str)
{
  m_argumentArray.SetSize(0);

  for (;;) {
    while (isspace(*str)) // Skip leading whitespace
      str++;
    if (*str == '\0')
      break;

    PString & arg = m_argumentArray[m_argumentArray.GetSize()];
    while (*str != '\0' && !isspace(*str)) {
      if (*str != '"')
        arg += *str++;
      else {
        ++str;
        while (*str != '\0' && *str != '"') {
          if (str[0] != '\\' || str[1] != '"')
            arg += *str++;
          else {
            arg += '"';
            str += 2;
          }
        }
        if (*str != '\0')
          str++;
      }
    }
  }

  SetArgs(m_argumentArray);
}


void PArgList::SetArgs(const PStringArray & theArgs)
{
  if (!theArgs.IsEmpty())
    m_argumentArray = theArgs;

  m_parsed = false;
  m_shift = 0;
  m_options.clear();
  m_parameterIndex.SetSize(m_argumentArray.GetSize());
  for (PINDEX i = 0; i < m_argumentArray.GetSize(); i++)
    m_parameterIndex[i] = i;
  m_argsParsed = 0;
}


bool PArgList::InternalSpecificationError(bool isError, const PString & msg)
{
  if (!isError)
    return false;

  m_parseError = msg;
  PAssertAlways(msg);
  return true;
}


bool PArgList::Parse(const char * spec, bool optionsBeforeParams)
{
  m_parsed = false;
  m_parseError.MakeEmpty();

  PINDEX arg;

  // If not in parse all mode, have been parsed before, and had some parameters
  // from last time, then start argument parsing somewhere along instead of start.
  if (optionsBeforeParams && m_argsParsed > 0)
    arg = m_argsParsed;
  else
    arg = m_shift;

  if (spec == NULL) {
    if (InternalSpecificationError(m_options.empty() || !optionsBeforeParams, "Cannot reparse without any options."))
      return false;
    for (size_t opt = 0; opt < m_options.size(); ++opt) {
      m_options[opt].m_count = 0;
      m_options[opt].m_string.MakeEmpty();
    }
  }
  else {
    // Parse the option specification
    m_options.clear();

    while (*spec != '\0') {
      OptionSpec opts;

      if (*spec == '[') {
        const char * end = strchr(++spec, ']');
        if (InternalSpecificationError(end == NULL || spec == end, "Unbalanced [] clause in specification."))
          return false;
        opts.m_section = PString(spec, end-spec);
        spec = end+1;
        if (InternalSpecificationError(opts.m_section.IsEmpty(), "Empty [] clause in specification."))
          return false;
      }

      if (*spec != '-')
        opts.m_letter = *spec++;

      if (*spec == '-') {
        size_t pos = strcspn(++spec, ".:; ");
        if (InternalSpecificationError(pos < 2, "Empty long form for option."))
          return false;
        opts.m_name = PString(spec, pos);
        spec += pos;
      }

      switch (*spec++) {
        case '.' :
          opts.m_type = NoString;
          break;
        case ':' :
          opts.m_type = HasString;
          break;
        case ';' :
          opts.m_type = OptionalString;
          break;
        default :
          --spec;
      }

      if (*spec == ' ') {
        while (*spec == ' ')
          ++spec;
        const char * end = strchr(spec, '\n');
        if (end == NULL) {
          opts.m_usage = spec;
          while (*spec != '\0')
            ++spec;
        }
        else {
          opts.m_usage = PString(spec, end-spec);
          spec = end+1;
        }
      }

      // Check for duplicates
      for (size_t i = 0; i < m_options.size(); ++i) {
        if (InternalSpecificationError(opts.m_letter != '\0' && opts.m_letter == m_options[i].m_letter,
                        "Duplicate option character '" + PString(opts.m_letter) + "' in specification") ||
            InternalSpecificationError(!opts.m_name.IsEmpty() && opts.m_name == m_options[i].m_name,
                           "Duplicate option string \"" + PString(opts.m_name) + "\" in specification"))
          return false;
      }

      m_options.push_back(opts);
    }
  }

  // Clear parameter indexes
  m_parameterIndex.SetSize(0);
  m_shift = 0;

  // Now work through the parameters and split out the options
  PINDEX param = 0;
  bool hadMinusMinus = m_options.empty();
  while (arg < m_argumentArray.GetSize()) {
    const PString & argStr = m_argumentArray[arg];
    if (hadMinusMinus || argStr[0] != '-' || argStr.length() <= 1) {
      // have a parameter string
      m_parameterIndex.SetSize(param+1);
      m_parameterIndex[param++] = arg;
    }
    else if (argStr.FindSpan("-") == P_MAX_INDEX) { // Nothing but dashes
      if (optionsBeforeParams) {
        ++arg;
        break;
      }
      hadMinusMinus = true; // ALL remaining args are parameters not options
    }
    else if (optionsBeforeParams && m_parameterIndex.GetSize() > 0)
      break;
    else if (argStr[1] == '-') {
      PINDEX equals = argStr.Find('=');
      if (equals != P_MAX_INDEX)
        ++equals;
      if (InternalParseOption(argStr(2, equals-2), equals, arg) < 0)
        return false;
    }
    else {
      for (PINDEX i = 1; i < argStr.GetLength(); i++) {
        int result = InternalParseOption(argStr[i], i+1, arg);
        if (result < 0)
          return false;
        if (result > 0)
          break;
      }
    }

    arg++;
  }

  if (optionsBeforeParams)
    m_argsParsed = arg;

  m_parsed = true;
  return param > 0;
}


size_t PArgList::InternalFindOption(const PString & name) const
{
  size_t opt;
  for (opt = 0; opt < m_options.size(); ++opt) {
    if (name.GetLength() == 1 ? (m_options[opt].m_letter == name[0]) : (m_options[opt].m_name == name))
      break;
  }
  return opt;
}


int PArgList::InternalParseOption(const PString & optStr, PINDEX offset, PINDEX & arg)
{
  size_t idx = InternalFindOption(optStr);
  if (idx >= m_options.size())
    m_parseError = "Unknown option ";
  else {
    OptionSpec & opt = m_options[idx];
    ++opt.m_count;
    if (opt.m_type == NoString)
      return 0;

    if (!opt.m_string.IsEmpty())
      opt.m_string += '\n';

    if (opt.m_type == OptionalString && (offset == P_MAX_INDEX || offset == m_argumentArray[arg].GetLength()))
      return 0;

    if (offset != P_MAX_INDEX && offset != m_argumentArray[arg].GetLength()) {
      opt.m_string += m_argumentArray[arg].Mid(offset);
      return 1;
    }

    if (++arg < m_argumentArray.GetSize()) {
      opt.m_string += m_argumentArray[arg];
      return 1;
    }

    m_parseError = "Argument required for option ";
  }

  m_parseError += offset == 0 ? "\"--" : "\"-";
  m_parseError += optStr;
  m_parseError += '"';
  return -1;
}


PINDEX PArgList::GetOptionCount(char option) const
{
  return InternalGetOptionCountByIndex(InternalFindOption(option));
}


PINDEX PArgList::GetOptionCount(const char * option) const
{
  return InternalGetOptionCountByIndex(InternalFindOption(PString(option)));
}


PINDEX PArgList::GetOptionCount(const PString & option) const
{
  return InternalGetOptionCountByIndex(InternalFindOption(option));
}


PINDEX PArgList::InternalGetOptionCountByIndex(size_t idx) const
{
  return idx < m_options.size() ? m_options[idx].m_count : 0;
}


PString PArgList::GetOptionString(char option, const char * dflt) const
{
  return InternalGetOptionStringByIndex(InternalFindOption(option), dflt);
}


PString PArgList::GetOptionString(const char * option, const char * dflt) const
{
  return InternalGetOptionStringByIndex(InternalFindOption(option), dflt);
}


PString PArgList::GetOptionString(const PString & option, const char * dflt) const
{
  return InternalGetOptionStringByIndex(InternalFindOption(option), dflt);
}


PString PArgList::InternalGetOptionStringByIndex(size_t idx, const char * dflt) const
{
  if (idx < m_options.size() && m_options[idx].m_count > 0)
    return m_options[idx].m_string;

  if (dflt != NULL)
    return dflt;

  return PString::Empty();
}


PStringArray PArgList::GetParameters(PINDEX first, PINDEX last) const
{
  PStringArray params;

  if (m_parameterIndex.IsEmpty())
    return params;

  if (last != P_MAX_INDEX)
    last += m_shift;

  if (last >= m_parameterIndex.GetSize())
    last = m_parameterIndex.GetSize()-1;

  if (m_shift < 0 && first < (PINDEX)-m_shift)
    return params;

  first += m_shift;
  if (first > last)
    return params;

  params.SetSize(last - first + 1);

  PINDEX idx = 0;
  while (first <= last)
    params[idx++] = m_argumentArray[m_parameterIndex[first++]];

  return params;
}


PString PArgList::GetParameter(PINDEX num) const
{
  int idx = m_shift + (int)num;
  if (idx >= 0 && idx < (int)m_parameterIndex.GetSize())
    return m_argumentArray[m_parameterIndex[idx]];

  return PString::Empty();
}


void PArgList::Shift(int sh) 
{
  m_shift += sh;
  if (m_shift < 0)
    m_shift = 0;
  else if (m_shift > (int)m_parameterIndex.GetSize())
    m_shift = m_parameterIndex.GetSize() - 1;
}


#ifdef P_CONFIG_FILE

///////////////////////////////////////////////////////////////////////////////
// PConfigArgs

PConfigArgs::PConfigArgs(const PArgList & args)
  : PArgList(args)
  , m_config(new PConfig)
  , m_sectionName(m_config->GetDefaultSection())
  , m_negationPrefix("no-")
{
}


PConfigArgs::~PConfigArgs()
{
  delete m_config;
}


PINDEX PConfigArgs::GetOptionCount(char option) const
{
  PINDEX count;
  if ((count = PArgList::GetOptionCount(option)) > 0)
    return count;

  PString stropt = CharToString(option);
  if (stropt.IsEmpty())
    return 0;

  return GetOptionCount(stropt);
}


PINDEX PConfigArgs::GetOptionCount(const char * option) const
{
  return GetOptionCount(PString(option));
}


PINDEX PConfigArgs::GetOptionCount(const PString & option) const
{
  // if specified on the command line, use that option
  PINDEX count = PArgList::GetOptionCount(option);
  if (count > 0)
    return count;

  // if user has specified "no-option", then ignore config file
  if (PArgList::GetOptionCount(m_negationPrefix + option) > 0)
    return 0;

  return m_config->HasKey(m_sectionName, option) ? 1 : 0;
}


PString PConfigArgs::GetOptionString(char option, const char * dflt) const
{
  if (PArgList::GetOptionCount(option) > 0)
    return PArgList::GetOptionString(option, dflt);

  PString stropt = CharToString(option);
  if (stropt.IsEmpty()) {
    if (dflt != NULL)
      return dflt;
    return PString();
  }

  return GetOptionString(stropt, dflt);
}


PString PConfigArgs::GetOptionString(const char * option, const char * dflt) const
{
  return GetOptionString(PString(option), dflt);
}


PString PConfigArgs::GetOptionString(const PString & option, const char * dflt) const
{
  // if specified on the command line, use that option
  if (PArgList::GetOptionCount(option) > 0)
    return PArgList::GetOptionString(option, dflt);

  // if user has specified "no-option", then ignore config file
  if (PArgList::HasOption(m_negationPrefix + option)) {
    if (dflt != NULL)
      return dflt;
    return PString();
  }

  return m_config->GetString(m_sectionName, option, dflt != NULL ? dflt : "");
}


void PConfigArgs::Save(const PString & saveOptionName)
{
  if (PArgList::GetOptionCount(saveOptionName) == 0)
    return;

  m_config->DeleteSection(m_sectionName);

  for (size_t i = 0; i < m_options.size(); i++) {
    PString optionName = m_options[i].m_name;
    if (m_options[i].m_count > 0 && optionName != saveOptionName) {
      if (!m_options[i].m_string.IsEmpty())
        m_config->SetString(m_sectionName, optionName, m_options[i].m_string);
      else
        m_config->SetBoolean(m_sectionName, optionName, true);
    }
  }
}


PString PConfigArgs::CharToString(char letter) const
{
  for (size_t opt = 0; opt < m_options.size(); ++opt) {
    if (m_options[opt].m_letter == letter)
      return m_options[opt].m_name;
  }
  return PString::Empty();
}

#endif // P_CONFIG_ARGS

///////////////////////////////////////////////////////////////////////////////
// PProcess

PProcess * PProcessInstance = NULL;


int PProcess::InternalMain(void *)
{
  setlocale(LC_CTYPE, "UTF-8"); // A PString is documented as UTF-8, so all conversions go there

#if P_SDL && defined(P_MACOSX)
  PVideoOutputDevice * device = PVideoOutputDevice::CreateDeviceByName(P_SDL_VIDEO_PREFIX);
  if (device != NULL && device->ApplicationMain())
    return m_terminationValue;
#endif
  
  Main();

  return m_terminationValue;
}


void PProcess::PreInitialise(int c, char ** v)
{
  if (m_executableFile.IsEmpty()) {
    PString execFile = v[0];
    if (PFile::Exists(execFile))
      m_executableFile = execFile;
    else {
      execFile += ".exe";
      if (PFile::Exists(execFile))
        m_executableFile = execFile;
    }
  }

  if (m_productName.IsEmpty())
    m_productName = m_executableFile.GetTitle().ToLower();

  m_arguments.SetArgs(c-1, v+1);
  m_arguments.SetCommandName(m_executableFile.GetTitle());
}


PProcess::PProcess(const char * manuf, const char * name,
                   unsigned major, unsigned minor, CodeStatus stat, unsigned patch,
                   bool library, bool suppressStartup, unsigned oemVersion)
  : PThread(Type::IsProcess, name)
  , m_library(library)
  , m_terminationValue(0)
  , m_manufacturer(manuf)
  , m_productName(name)
  , m_maxHandles(INT_MAX)
  , m_shuttingDown(false)
  , m_keepingHouse(false)
  , m_houseKeeper(NULL)
  , m_processID(GetCurrentProcessID())
  , m_previousRunTimeSignalHandlers(SIGRTMAX)
  , m_RunTimeSignalsQueueBuffer(10)
  , m_RunTimeSignalsQueueIn(0)
  , m_RunTimeSignalsQueueOut(0)
{
  #if PTRACING
  // Do this before PProcessInstance is set to avoid a recursive loop with PTimedMutex
  PTraceInfo::Instance();
  #endif

  PAssert(PProcessInstance == NULL, "Only one instance of PProcess allowed");
  PProcessInstance = this;

  // Versions
  m_version.m_major = major;
  m_version.m_minor = minor;
  m_version.m_status = stat;
  m_version.m_patch = patch;
  m_version.m_oem = oemVersion;
  m_version.m_svn = 0;
  m_version.m_git = NULL;

  m_activeThreads[GetThreadId()] = this;
  PThread::InternalThreadStarted();

#if PTRACING
  PTraceInfo::Instance().InitialiseFromEnvironment();
#endif

  /* Try to get the real image path for this process using platform dependent
     code, if this fails, then use the value urigivally set via argv[0] */
#if defined(_WIN32)
  TCHAR shortName[_MAX_PATH];
  if (GetModuleFileName(GetModuleHandle(NULL), shortName, sizeof(shortName)) > 0) {
    TCHAR longName[32768]; // Space for long image path
    if (GetLongPathName(shortName, longName, sizeof(longName)) > 0)
      m_executableFile = longName;
    else
      m_executableFile = shortName;
    m_executableFile.Replace("\\\\?\\", ""); // Name can contain \\?\ prefix, remove it
  }
#elif defined(P_MACOSX)
  char path[10000];
  uint32_t size = sizeof(path);
  if (_NSGetExecutablePath(path, &size) == 0)
    m_executableFile = path;
#elif defined(P_RTEMS)
  cout << "Enter program arguments:\n";
  arguments.ReadFrom(cin);
#else
  // Hope for a /proc, certainly works for Linux
  char path[10000];
  int len = readlink("/proc/self/exe", path, sizeof(path));
  if (len >= 0) {
    path[len] = '\0';
    m_executableFile = path;
  }
#endif // _WIN32

  if (m_productName.IsEmpty())
    m_productName = m_executableFile.GetTitle();

  SetThreadName(GetName());

  PlatformConstruct();

#if P_TIMERS
  m_timerList = new PTimer::List();
#endif

  if (!suppressStartup)
    Startup();

#if PMEMORY_HEAP
  // Now we start looking for memory leaks!
  PMemoryHeap::SetIgnoreAllocations(false);
#endif
}


void PProcess::Startup()
{
  PMEMORY_IGNORE_ALLOCATIONS_FOR_SCOPE;

  if (!m_library)
    AddRunTimeSignalHandlers();

  // create one instance of each class registered in the PProcessStartup abstract factory
  // But make sure we have plugins first, to avoid bizarre behaviour where static objects
  // are initialised multiple times when libraries are loaded in Linux.
  PProcessStartupFactory::KeyList_T list = PProcessStartupFactory::GetKeyList();
#ifdef PLUGIN_LOADER_STARTUP_NAME
  std::swap(list.front(), *std::find(list.begin(), list.end(), PLUGIN_LOADER_STARTUP_NAME));
#endif
  list.insert(list.begin(), "SetTraceLevel");
  for (PProcessStartupFactory::KeyList_T::const_iterator it = list.begin(); it != list.end(); ++it) {
    PProcessStartup * startup = PProcessStartupFactory::CreateInstance(*it);
    if (startup != NULL) {
      PTRACE(5, "Startup factory " << *it);
      startup->OnStartup();
    }
    else {
      PTRACE(1, "Could not create startup factory " << *it);
    }
  }

#if PTRACING
  if (PTimedMutex::CtorDtorLogLevel != UINT_MAX)
    s_MutexLeakCheck = new PMutexLeakCheck();
#endif
}


bool PProcess::IsServiceProcess() const
{
  return false;
}


bool PProcess::SignalTimerChange()
{
  if (!PAssert(IsInitialised(), PLogicError) || m_shuttingDown) 
    return false;

  if (m_keepingHouse.exchange(true))
    m_signalHouseKeeper.Signal();
  else
    m_houseKeeper = new PThreadObj<PProcess>(*this, &PProcess::HouseKeeping, false, "PTLib Housekeeper");

  return true;
}


void PProcess::HouseKeeping()
{
  while (m_keepingHouse) {
#if P_TIMERS
    PTimeInterval delay = m_timerList->Process();
    m_signalHouseKeeper.Wait(delay);
#endif

    m_RunTimeSignalsQueueMutex.Wait();
    while (m_RunTimeSignalsQueueIn != m_RunTimeSignalsQueueOut) {
      RunTimeSignalInfo info = m_RunTimeSignalsQueueBuffer[m_RunTimeSignalsQueueOut];
      m_RunTimeSignalsQueueOut = (m_RunTimeSignalsQueueOut + 1) % m_RunTimeSignalsQueueBuffer.size();
      m_RunTimeSignalsQueueMutex.Signal();
      InternalHandleRunTimeSignal(info);
      m_RunTimeSignalsQueueMutex.Wait();
    }
    m_RunTimeSignalsQueueMutex.Signal();
  }
}


PProcess::~PProcess()
{
  PTRACE(4, "Starting process destruction.");

  m_shuttingDown = true;

  RemoveRunTimeSignalHandlers();

  // Get rid of the house keeper (majordomocide)
  if (m_houseKeeper != NULL && m_houseKeeper->GetThreadId() != PThread::GetCurrentThreadId()) {
    PTRACE(4, "Terminating housekeeper thread.");
    m_keepingHouse = false;
    m_signalHouseKeeper.Signal();
    m_houseKeeper->WaitForTermination();
    delete m_houseKeeper;
    m_houseKeeper = NULL;
#if P_TIMERS
    delete m_timerList;
    m_timerList = NULL;
#endif
  }

  // Clean up factories
  PProcessStartupFactory::KeyList_T list = PProcessStartupFactory::GetKeyList();
  for (PProcessStartupFactory::KeyList_T::const_reverse_iterator it = list.rbegin(); it != list.rend(); ++it)
    PProcessStartupFactory::CreateInstance(*it)->OnShutdown();

  Sleep(100);  // Give threads time to die a natural death

m_threadMutex.Wait();

  // OK, if there are any other threads left, we get really insistent...
  PTRACE(4, "Cleaning up " << m_activeThreads.size()-1 << " remaining threads.");
  for (ThreadMap::iterator it = m_activeThreads.begin(); it != m_activeThreads.end(); ++it) {
    PThread & thread = *it->second;
    switch (thread.m_type) {
      case Type::IsAutoDelete:
      case Type::IsManualDelete:
        if (thread.IsTerminated()) {
          PTRACE(5, "Already terminated thread " << thread);
        }
        else {
          PTRACE(3, "Terminating thread " << thread);
          thread.Terminate();  // With extreme prejudice
        }
        break;
      case Type::IsExternal :
        PTRACE(5, "Remaining external thread " << thread);
        break;
      default :
        break;
    }
  }
  m_activeThreads.clear();

  m_threadMutex.Signal();

  OnThreadEnded(*this);

  PlatformDestruct();

  PFactoryBase::GetFactories().DestroySingletons();
  PIPSocket::ClearNameCache();

#if PTRACING
  if (s_MutexLeakCheck != NULL) {
    ostream & trace = PTRACE_BEGIN(PTimedMutex::CtorDtorLogLevel, NULL, "Mutex");
    if (s_MutexLeakCheck->empty())
      trace << "No mutex leaks.";
    else {
      trace << "Mutex Leaks:\n";
      for (PMutexLeakCheck::iterator it = s_MutexLeakCheck->begin(); it != s_MutexLeakCheck->end(); ++it)
        trace << "  " << **it << '\n';
    }
    trace << PTrace::End;
  }
#endif

  // Last chance to log anything ...
  PTRACE(4, PProcessInstance, "Completed process destruction.");

  // Can't do any more tracing after this ...
#if PTRACING
  PTrace::SetStream(NULL);
  PTrace::SetLevel(0);
#endif

  PProcessInstance = NULL;
}


PProcess & PProcess::Current()
{
  if (PProcessInstance == NULL) {
    PAssertAlways("Catastrophic failure, PProcess::Current() = NULL!!");
    std::abort();
  }
  return *PProcessInstance;
}


void PProcess::OnThreadStart(PThread & PTRACE_PARAM(thread))
{
  PTRACE(5, "Started thread " << thread);
}


static void OutputInterval(ostream & strm, const char * name, const PTimeInterval & interval)
{
  strm << name << '=';
  if (interval > 0 && interval < PTimeInterval(0, 100))
    strm << scientific << showbase << setprecision(3);
  else
    strm << fixed << noshowbase << setprecision(1);
  strm << interval << 's';
}

static void OutputWithPercent(ostream & strm, const char * name, const PTimeInterval & real, const PTimeInterval & cpu)
{
  strm << ", ";
  OutputInterval(strm, name, cpu);

  strm << " (";

  if (real == 0)
    strm << '0';
  else {
    unsigned percent = (unsigned)((cpu.GetMicroSeconds()*1000)/real.GetMicroSeconds());
    if (percent == 0)
      strm << '0';
    else
      strm << (percent/10) << '.' << (percent%10);
  }

  strm << "%)";
}


ostream & operator<<(ostream & strm, const PThread::Times & times)
{
  OutputInterval   (strm, "real",   times.m_real);
  OutputWithPercent(strm, "kernel", times.m_real, times.m_kernel);
  OutputWithPercent(strm, "user",   times.m_real, times.m_user);
  OutputWithPercent(strm, "both",   times.m_real, times.m_kernel + times.m_user);
  return strm;
}


void PProcess::OnThreadEnded(PThread &
#if PTRACING || P_PROFILING
                             thread
#endif
                             )
{
  if (!PProcess::IsInitialised())
    return;

#if P_PROFILING
  PThread::Times times;
  if (!thread.GetTimes(times))
    return;
  PProfiling::OnThreadEnded(thread, times.m_real, times.m_kernel, times.m_user);
#endif // P_PROFILING

#if PTRACING
  const int LogLevel = 3;
  if (PTrace::CanTrace(LogLevel)) {
    PThread::Times times;
    if (thread.GetTimes(times))
      PTRACE_BEGIN(LogLevel, "PTLib") << "Thread ended: " << thread << times << PTrace::End;
  }
#endif //PTRACING
}


static int SignalsToHandle[] = {
  SIGINT,
  SIGTERM,
#ifdef SIGHUP
  SIGHUP,
#endif
#ifdef SIGUSR1
  SIGUSR1,
#endif
#ifdef SIGUSR2
  SIGUSR2,
#endif
#ifdef SIGPIPE
  SIGPIPE,
#endif
#ifdef SIGWINCH
  SIGWINCH,
#endif
#ifdef SIGTRAP
  SIGTRAP,
#endif
  0
};


void PProcess::AddRunTimeSignalHandlers(const int * signals)
{
  if (signals != SignalsToHandle)
    PProcess::AddRunTimeSignalHandlers(SignalsToHandle);

  if (signals == NULL)
    return;

  int signal;
  while ((signal = *signals++) != 0) {
    PRunTimeSignalHandler & previous = m_previousRunTimeSignalHandlers[signal];
    if (previous == NULL)
      previous = PlatformSetRunTimeSignalHandler(signal);
  }
}


void PProcess::RemoveRunTimeSignalHandlers()
{
  for (size_t sig = 0; sig < m_previousRunTimeSignalHandlers.size(); ++sig) {
    if (m_previousRunTimeSignalHandlers[sig] != NULL) {
      PlatformResetRunTimeSignalHandler(sig, m_previousRunTimeSignalHandlers[sig]);
      m_previousRunTimeSignalHandlers[sig] = NULL;
    }
  }
}


void PProcess::AsynchronousRunTimeSignal(int signal, PProcessIdentifier source)
{
#if P_HAS_BACKTRACE && PTRACING
  if (signal == WalkStackSignal) {
    InternalWalkStackSignaled();
    return;
  }
#endif

  InternalPostRunTimeSignal(signal, source);
}


void PProcess::InternalPostRunTimeSignal(int signal, PProcessIdentifier source)
{
  m_RunTimeSignalsQueueMutex.Wait();
  m_RunTimeSignalsQueueBuffer[m_RunTimeSignalsQueueIn].m_signal = signal;
  m_RunTimeSignalsQueueBuffer[m_RunTimeSignalsQueueIn].m_source = source;
  m_RunTimeSignalsQueueIn = (m_RunTimeSignalsQueueIn + 1) % m_RunTimeSignalsQueueBuffer.size();
  m_RunTimeSignalsQueueMutex.Signal();

  SignalTimerChange(); // Inform house keeping thread we have a signal to be processed
}


void PProcess::InternalHandleRunTimeSignal(const RunTimeSignalInfo & signalInfo)
{
#if PTRACING
  unsigned level =
#ifdef SIGPIPE
                   signalInfo.m_signal == SIGPIPE ? 4 :
#endif
                   2;
  if (PTrace::CanTrace(level)) {
    ostream & trace = PTRACE_BEGIN(level);
    trace << "Received signal " << GetRunTimeSignalName(signalInfo.m_signal);
    if (signalInfo.m_source != 0) {
      trace << " from ";
      if (signalInfo.m_source == GetCurrentProcessID())
        trace << "self";
      else {
        PFile proc(PSTRSTRM("/proc/" << signalInfo.m_source << "/cmdline"), PFile::ReadOnly);
        if (!proc.IsOpen())
          trace << "source=" << signalInfo.m_source;
        else {
          PString cmdline;
          int c;
          while ((c = proc.ReadChar()) >= 0) {
            if (c == '\0')
              cmdline += ' ';
            else
              cmdline += (char)c;
          }
          trace << "pid=" << signalInfo.m_source << ", cmdline=\"" << cmdline.RightTrim() << '"';
        }
      }
    }
    trace << PTrace::End;
  }
#endif // PTRACING

  HandleRunTimeSignal(signalInfo);
}


void PProcess::HandleRunTimeSignal(const RunTimeSignalInfo & signalInfo)
{
  HandleRunTimeSignal(signalInfo.m_signal);
}


void PProcess::HandleRunTimeSignal(int signal)
{
  switch (signal) {
    case SIGINT:
    case SIGTERM:
#ifdef SIGHUP
    case SIGHUP:
#endif
      if (OnInterrupt(signal == SIGTERM))
        return;

      Terminate();
      _exit(1); // Shouldn't get here, but just in case ...
  }
}


const char * PProcess::GetRunTimeSignalName(int signal)
{
  for (POrdinalToString::Initialiser const * name = InternalSigNames; name->key != 0; ++name) {
    if ((int)name->key == signal)
      return name->value;
  }

  /* Not strictly thread safe, but should be extremely rare for two signal
     handlers in two differnt threads get an unknown signal at exactly
     the same time, and we don't want to do any heap operations here in
     case the heap is trashed and the signal is a SIGSEGV etc. */
  static char buffer[100];
  snprintf(buffer, sizeof(buffer), "SIG%i", signal);
  return buffer;
}


bool PProcess::OnInterrupt(bool)
{
  return false;
}


bool PProcess::IsInitialised()
{
  return PProcessInstance != NULL;
}


PObject::Comparison PProcess::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PProcess), PInvalidCast);
  return m_productName.Compare(((const PProcess &)obj).m_productName);
}


void PProcess::Terminate()
{
  // With extreme prejudice
  PTRACE(0, "Process \"" << GetName() << "\" terminating without clean up!");
  _exit(m_terminationValue);
}


PTime PProcess::GetStartTime() const
{ 
  return m_programStartTime; 
}


bool PProcess::IsMultipleInstance() const
{
  static PSemaphore s_multiTest(GetName()+" UniqueProcessSeampahore", 1, 1);
  static bool s_multiple = !s_multiTest.Wait(0);
  return s_multiple;
}


PString PProcess::GetVersion(bool full) const
{
  return m_version.AsString(full);
}


PString PProcess::GetLibVersion()
{
  static const VersionInfo ver = { MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, PATCH_VERSION, OEM_VERSION, SVN_REVISION, GIT_COMMIT };
  return ver.AsString();
}


PString PProcess::VersionInfo::AsString(bool full) const
{
  PStringStream str;
  str << m_major << '.' << m_minor;

  if (full) {
    switch (m_status) {
      case PProcess::AlphaCode :
        str << "alpha";
        break;

      case PProcess::BetaCode :
        str << "beta";
        break;

      default:
        str << '.';
    }

    str << m_patch;

    if (m_oem > 0)
      str << '-' << m_oem;

    if (m_git != NULL && *m_git != '\0')
      str << " (git:" << m_git << ')';
    else if (m_svn > 0)
      str << " (svn:" << m_svn << ')';
  }

  return str;
}


#if P_CONFIG_FILE
void PProcess::SetConfigurationPath(const PString & path)
{
  m_configurationPaths = path.Tokenise(PPATH_SEPARATOR, false);
  PTRACE(3, "Configuration path set to " << setfill(PPATH_SEPARATOR) << m_configurationPaths);
}
#endif


PThread * PProcess::GetThread(PThreadIdentifier threadId) const
{
  PWaitAndSignal mutex(m_threadMutex);
  ThreadMap::const_iterator it = m_activeThreads.find(threadId);
  return it != m_activeThreads.end() ? it->second : NULL;

}


void PProcess::InternalThreadStarted(PThread * thread)
{
  if (PAssertNULL(thread) == NULL)
    return;

  m_threadMutex.Wait();

  m_activeThreads[thread->GetThreadId()] = thread;

#if PTRACING
  size_t newHighWaterMark = 0;
  static size_t highWaterMark = 1; // Inside m_threadMutex so simple static is OK
  if (m_activeThreads.size() > highWaterMark+20)
    newHighWaterMark = highWaterMark = m_activeThreads.size();
#endif

  m_threadMutex.Signal();

  PTRACE_IF(2, newHighWaterMark  > 0, "Thread high water mark set: " << newHighWaterMark);

  SignalTimerChange();
}


void PProcess::InternalThreadEnded(PThread * thread)
{
  if (PAssertNULL(thread) == NULL)
    return;

  PWaitAndSignal mutex(m_threadMutex);

  ThreadMap::iterator it = m_activeThreads.find(thread->GetThreadId());
  if (it != m_activeThreads.end() && it->second == thread)
    m_activeThreads.erase(it); // Not already gone, or re-used the thread ID for new thread.
}


///////////////////////////////////////////////////////////////////////////////

bool PProcess::HostSystemURLHandlerInfo::RegisterTypes(const PString & _types, bool force)
{
  PStringArray types(_types.Lines());

  for (PINDEX i = 0; i < types.GetSize(); ++i) {
    PString type = types[i];
    HostSystemURLHandlerInfo handler(type);
    handler.SetIcon("%base");
    handler.SetCommand("open", "%exe %1");
    if (!handler.CheckIfRegistered()) {
      if (!force)
        return false;
      handler.Register();
    }
  }
  return true;
}


///////////////////////////////////////////////////////////////////////////////
// PThread

thread_local PThread * PThread::sm_currentThread;

PThread::PThread(AutoDeleteFlag deletion, Priority priority, PString threadName)
  : PThread(deletion == AutoDeleteThread ? Type::IsAutoDelete : Type::IsManualDelete, std::move(threadName), priority)
{
}


PThread::PThread(std::function<void()> mainFunction, PString threadName, bool startImmediate, AutoDeleteFlag deletion, Priority priority)
  : PThread(deletion == AutoDeleteThread ? Type::IsAutoDelete : Type::IsManualDelete, std::move(threadName), priority, mainFunction)
{
  if (startImmediate)
    Start();
}


PThread::PThread(PINDEX, AutoDeleteFlag deletion, Priority priority, PString threadName)
  : PThread(deletion == AutoDeleteThread ? Type::IsAutoDelete : Type::IsManualDelete, std::move(threadName), priority)
{
}


PThread::PThread(Type type, PString threadName, Priority priority, std::function<void()> mainFunction)
  : m_type(type)
  , m_mainFunction(std::move(mainFunction))
  , m_initialPriority(priority)
  , m_threadName(std::move(threadName))
  , m_uniqueId(0)
  , m_threadStartTime(0)
  , m_threadEndTime(0)
{
}


bool PThread::Start()
{
  if (!IsTerminated())
    return false;

  std::thread starter(std::bind(&PThread::InternalThreadMain, this));
  m_threadId = starter.get_id();
  m_nativeHandle = starter.native_handle();
  starter.detach(); // Do not require keeping std::thread, it's pretty useless

  PProcess::Current().InternalThreadStarted(this);
  return true;
}


void PThread::InternalThreadStarted()
{
  sm_currentThread = this;
  m_threadStartTime.SetCurrentTime();
  m_threadId = std::this_thread::get_id();
  m_uniqueId = GetCurrentUniqueIdentifier();

  #if _WIN32
    m_threadHandle.Duplicate(::GetCurrentThread());
  #endif

  SetThreadName(GetThreadName());

  if (m_initialPriority < PThread::NumPriority)
    SetPriority(m_initialPriority);
}


#if P_USE_THREAD_CANCEL
void PThread::PlatformThreadEnded(void* arg)
{
  reinterpret_cast<PThread*>(PAssertNULL(arg))->InternalThreadEnded();
}
#endif


void PThread::InternalThreadMain()
{
  const std::lock_guard<std::timed_mutex> running(m_threadRunning);

  #if P_USE_THREAD_CANCEL
    // make sure the cleanup routine is called when the thread is cancelled
    pthread_cleanup_push(&PThread::PlatformThreadEnded, this);
    // Allow cancel to happen any time
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  #endif

  InternalThreadStarted();
  PProcess::Current().OnThreadStart(*this);

  if (m_mainFunction)
    m_mainFunction();
  else
    Main();

  #if P_USE_THREAD_CANCEL
    pthread_cleanup_pop(0);
  #else
    InternalThreadEnded();
  #endif
}


void PThread::InternalThreadEnded()
{
  m_threadEndTime.SetCurrentTime();
  PProcess & process = PProcess::Current();
  process.OnThreadEnded(*this);
  process.InternalThreadEnded(this);
}


bool PThread::IsTerminated() const
{
  switch (m_type) {
    case Type::IsProcess:
      return false;
    case Type::IsExternal:
      return true;
    default:
      if (!m_threadRunning.try_lock())
        return false;
      m_threadRunning.unlock();
      return true;
  }
}


void PThread::WaitForTermination() const
{
  if (GetThreadId() == GetCurrentThreadId()) {
    PTRACE(3, "PTLib", "WaitForTermination short circuited");
    return;
  }

  switch (m_type) {
    case Type::IsProcess:
    case Type::IsExternal:
      break;
    default:
      m_threadRunning.lock();
      m_threadRunning.unlock();
  }
}


bool PThread::WaitForTermination(const PTimeInterval& maxWait) const
{
  if (GetThreadId() == GetCurrentThreadId()) {
    PTRACE(3, "PTLib", "WaitForTermination short circuited");
    return true;
  }

  switch (m_type) {
    case Type::IsProcess:
      return false;
    case Type::IsExternal:
      return true;
    default:
      if (!m_threadRunning.try_lock_for(maxWait.AsChronoNS()))
        return false;
      m_threadRunning.unlock();
      return true;
  }
}


PThread * PThread::Current()
{
  if (sm_currentThread)
    return sm_currentThread;

  if (!PAssert(PProcess::IsInitialised(), PLogicError))
    std::terminate();

  static thread_local std::unique_ptr<PThread> m_external;
  m_external = std::make_unique<PExternalThread>();
  sm_currentThread = m_external.get();
  return sm_currentThread;
}


PString PThread::GetThreadName(PThreadIdentifier id)
{
  if (id == PNullThreadIdentifier)
    return "(null)";

  if (PProcess::IsInitialised()) {
    PProcess & process = PProcess::Current();
    PWaitAndSignal mutex(process.m_threadMutex);
    PProcess::ThreadMap::iterator it = process.m_activeThreads.find(id);
    if (it != process.m_activeThreads.end())
      return it->second->GetThreadName();
  }

  return PSTRSTRM("Unknown thread, id=" << Identifiers(id, 0));
}


ostream & operator<<(ostream & strm, const PThread::Identifiers& ids)
{
  strm << ids.m_tid;
  #if P_HAS_UNIQUE_THREAD_ID_FMT
    if (ids.m_uid != 0)
      strm << " (" << ids.m_uid << ')';
  #endif
  return strm;
}


void PThread::PrintOn(ostream& strm) const
{
  strm << "obj=" << this << ", name=\"" << GetThreadName() << '"';
  #if P_HAS_UNIQUE_THREAD_ID_FMT
    strm  << ", tid=" << GetThreadId();
  #endif
}


PINDEX PThread::GetTotalCount()
{
    PProcess & process = PProcess::Current();
    PWaitAndSignal mutex(process.m_threadMutex);
    return process.m_activeThreads.size();
}


bool PThread::GetTimes(PThreadIdentifier id, Times & times)
{
  if (!PProcess::IsInitialised())
    return false;

  if (id == PNullThreadIdentifier)
    return Current()->GetTimes(times);

  PProcess & process = PProcess::Current();

  PWaitAndSignal mutex(process.m_threadMutex);
  PProcess::ThreadMap::iterator it = process.m_activeThreads.find(id);
  return it != process.m_activeThreads.end() && it->second->GetTimes(times);
}


bool PProcess::GetAllThreadIdentifiers(std::vector<PThreadIdentifier> & identifiers)
{
  if (!PProcess::IsInitialised())
    return false;

  PWaitAndSignal mutex(m_threadMutex);
  identifiers.reserve(m_activeThreads.size());
  for (PProcess::ThreadMap::iterator it = m_activeThreads.begin(); it != m_activeThreads.end(); ++it)
    identifiers.push_back(it->first);
  return !identifiers.empty();
}

void PThread::GetTimes(std::vector<Times> & allThreadTimes)
{
  std::vector<PThreadIdentifier> identifiers;
  if (PProcess::Current().GetAllThreadIdentifiers(identifiers)) {
    allThreadTimes.reserve(identifiers.size());
    Times threadTimes;
    for (std::vector<PThreadIdentifier>::iterator it = identifiers.begin(); it != identifiers.end(); ++it) {
      if (GetTimes(*it, threadTimes))
        allThreadTimes.push_back(threadTimes);
    }
  }
}


void PThread::GetTimes(std::list<Times> & allThreadTimes)
{
  std::vector<PThreadIdentifier> identifiers;
  if (PProcess::Current().GetAllThreadIdentifiers(identifiers)) {
    Times threadTimes;
    for (std::vector<PThreadIdentifier>::iterator it = identifiers.begin(); it != identifiers.end(); ++it) {
      if (GetTimes(*it, threadTimes))
        allThreadTimes.push_back(threadTimes);
    }
  }
}


void PThread::GetTimes(std::set<Times> & allThreadTimes)
{
  std::vector<PThreadIdentifier> identifiers;
  if (PProcess::Current().GetAllThreadIdentifiers(identifiers)) {
    Times threadTimes;
    for (std::vector<PThreadIdentifier>::iterator it = identifiers.begin(); it != identifiers.end(); ++it) {
      if (GetTimes(*it, threadTimes))
        allThreadTimes.insert(threadTimes);
    }
  }
}


int PThread::GetPercentageCPU(Times & previousTimes, const PTimeInterval & period, PThreadIdentifier id)
{
  if (!previousTimes.m_sample.IsValid())
    return GetTimes(id, previousTimes) ? -2 : -1;

  if (previousTimes.m_sample.GetElapsed() < period)
    return -2;

  Times newTimes;
  if (!GetTimes(id, newTimes))
    return -1;

  Times delta = newTimes - previousTimes;
  previousTimes = newTimes;
  return (delta.m_user + delta.m_kernel)*100/delta.m_real;
}


PThread::Times::Times()
  : m_threadId(PNullThreadIdentifier)
  , m_uniqueId(0)
  , m_sample(0)
{
}


float PThread::Times::AsPercentage() const
{
  PTimeInterval cpu = m_kernel + m_user;
  return (float)(cpu == 0 ? 0.0 : (100.0 * cpu.GetMicroSeconds() / m_real.GetMicroSeconds()));
}


PThread::Times PThread::Times::operator-(const Times & rhs) const
{
  Times diff;
  diff.m_real = m_real - rhs.m_real;
  diff.m_kernel = m_kernel - rhs.m_kernel;
  diff.m_user = m_user - rhs.m_user;
  return diff;
}


PThread::Times & PThread::Times::operator-=(const Times & rhs)
{
  m_real -= rhs.m_real;
  m_kernel -= rhs.m_kernel;
  m_user -= rhs.m_user;
  return *this;
}


PString PThread::GetThreadName() const
{
  std::lock_guard<std::mutex> lock(m_threadNameMutex);
  return m_threadName;
}


void PThread::SetThreadName(const PString & name)
{
  std::lock_guard<std::mutex> lock(m_threadNameMutex);

  if (!name.empty())
    m_threadName = name;
  else {
    m_threadName = typeid(*this).name();
    auto space = m_threadName.find(' ');
    if (space != string::npos)
      m_threadName.erase(0, space+1);
  }

  PUniqueThreadIdentifier uniqueId = GetUniqueIdentifier();
  if (uniqueId == 0)
    return;

  if (name.Find('%') != P_MAX_INDEX)
    m_threadName = psprintf(name, uniqueId);
  else
    m_threadName = PSTRSTRM(m_threadName << ':' << uniqueId);

  if (&PProcess::Current() != this)
    PlatformSetThreadName(m_threadName.Left(15));
}
 

void PThread::SetAutoDelete(AutoDeleteFlag deletion)
{
  static thread_local std::unique_ptr<PThread> autoDeleter;

  switch (m_type) {
    case Type::IsAutoDelete :
      if (deletion == AutoDeleteThread)
        return;
      autoDeleter.reset(this);
      break;

    case Type::IsManualDelete :
      if (deletion != AutoDeleteThread)
        return;
      autoDeleter.release();
      break;

    case Type::IsProcess :
      PAssert(deletion != AutoDeleteThread, PInvalidParameter);
      return;

    case Type::IsExternal :
      PAssert(deletion == AutoDeleteThread, PInvalidParameter);
      return;
  }

  m_type = deletion == AutoDeleteThread ? Type::IsAutoDelete : Type::IsManualDelete;
}


PThread * PThread::Create(const PNotifier & notifier,
                          intptr_t parameter,
                          AutoDeleteFlag deletion,
                          Priority priorityLevel,
                          const PString & threadName)
{
  PThread * thread = new PSimpleThread(notifier,
                                       parameter,
                                       deletion,
                                       priorityLevel,
                                       threadName);
  if (deletion != AutoDeleteThread)
    return thread;

  // Do not return a pointer to the thread if it is auto-delete as this
  // pointer is extremely dangerous to use, it could be deleted at any moment
  // from now on so using the pointer could crash the program.
  return NULL;
}


bool PThread::WaitAndDelete(PThread * & threadToDelete, const PTimeInterval & maxWait, PMutex * mutex, bool lock)
{
  if (mutex != NULL && lock)
    mutex->Wait();

  PThread * thread = threadToDelete;
  threadToDelete = NULL;

  if (mutex != NULL)
    mutex->Signal();

  if (thread == NULL)
    return false;

  if (PThread::Current() == thread) {
    // Is me!
    thread->SetAutoDelete();
    return false;
  }

  PTRACE(4, thread, "Waiting for thread " << *thread << " to terminate in " << maxWait << " seconds");
  if (thread->WaitForTermination(maxWait)) {
    // Orderly exit
    delete thread;
    return false;
  }

  ostringstream strm;
  strm << "Thread \"" << *thread << "\" failed to terminate"
#if PTRACING
          " at stack location:";
  PTrace::WalkStack(strm, thread->GetThreadId(), thread->GetUniqueIdentifier());
  strm << "\n   "
#endif
          " in " << maxWait << " seconds";
  PAssertAlways(strm.str().c_str());

  delete thread;

  return true;
}


PThread::~PThread()
{
  if (m_type != Type::IsProcess && m_type != Type::IsExternal && !WaitForTermination(1000)) {
    Terminate();
    WaitForTermination(1000);
  }

  PlatformDestroy();

  if (m_type != Type::IsProcess)
    PProcess::Current().InternalThreadEnded(this);
}


/////////////////////////////////////////////////////////////////////////////

PSimpleThread::PSimpleThread(const PNotifier & notifier,
                             intptr_t param,
                             AutoDeleteFlag deletion,
                             Priority priorityLevel,
                             const PString & threadName)
  : PThread(deletion, priorityLevel, threadName),
    callback(notifier),
    parameter(param)
{
  Start();
}


void PSimpleThread::Main()
{
  callback(*this, parameter);
}


/////////////////////////////////////////////////////////////////////////////

static PTimedMutex::DeadlockStackWalkModes InitialiseDeadlockStackWalkMode()
{
  auto env = PConfig::GetEnv("PTLIB_DEADLOCK_STACK_WALK_MODE");
  if (!env.empty())
    return (PTimedMutex::DeadlockStackWalkModes)std::min((int)PTimedMutex::DeadlockStackWalkOnPhantomRelease, std::abs(atoi(env)));

  if (!PConfig::GetEnv("PTLIB_DISABLE_DEADLOCK_STACK_WALK").empty())
    return PTimedMutex::DeadlockStackWalkDisabled;

  return PTimedMutex::DeadlockStackWalkEnabled;
}

PTimedMutex::DeadlockStackWalkModes PTimedMutex::DeadlockStackWalkMode = InitialiseDeadlockStackWalkMode();

#if PTRACING

unsigned PTimedMutex::CtorDtorLogLevel = UINT_MAX;

static void OutputThreadInfo(ostream & strm, PThreadIdentifier tid, PUniqueThreadIdentifier uid, bool noSymbols)
{
  strm << " id=" << PThread::Identifiers(tid, uid) << ", name=\"" << PThread::GetThreadName(tid) << '"';
  if (noSymbols)
    strm << ", stack=";
  PTrace::WalkStack(strm, tid, uid, noSymbols);
}

#endif // PTRACING


unsigned PTimedMutex::ExcessiveLockWaitTime;


PMutexExcessiveLockInfo::PMutexExcessiveLockInfo()
  : PMutexExcessiveLockInfo(PDebugLocation(), 0)
{
}

PMutexExcessiveLockInfo::PMutexExcessiveLockInfo(const PDebugLocation & location, unsigned timeout)
  : m_location(location)
  , m_excessiveLockActive(false)
  , m_startHeldSamplePoint(0)
{
  if (timeout > 0)
      m_excessiveLockTimeout = timeout;
  else {
    if (PTimedMutex::ExcessiveLockWaitTime == 0) {
      auto seconds = PConfig::GetEnv("PTLIB_DEADLOCK_TIME", "0").AsUnsigned();
      PTimedMutex::ExcessiveLockWaitTime = seconds > 0 ? seconds*1000 : 15000;
    }
    m_excessiveLockTimeout = PTimedMutex::ExcessiveLockWaitTime;
  }
}


PMutexExcessiveLockInfo::PMutexExcessiveLockInfo(const PMutexExcessiveLockInfo & other)
  : m_location(other.m_location)
  , m_excessiveLockTimeout(other.m_excessiveLockTimeout)
  , m_excessiveLockActive(false)
  , m_startHeldSamplePoint(0)
{
}


#if PTRACING
void PMutexExcessiveLockInfo::Constructed(const PObject & mutex) const
{
  if (s_MutexLeakCheck == NULL)
    return;

  s_MutexLeakCheck->Wait();
  bool ok = s_MutexLeakCheck->insert(&mutex).second;
  s_MutexLeakCheck->Signal();

  PTRACE(PTimedMutex::CtorDtorLogLevel, (ok ? "Constructed " : "Second construction of ") << mutex);
}


void PMutexExcessiveLockInfo::Destroyed(const PObject & mutex) const
{
  if (s_MutexLeakCheck == NULL)
    return;

  s_MutexLeakCheck->Wait();
  bool ok = s_MutexLeakCheck->erase(&mutex) == 1; // If not there, probably constructed before s_MutexLeakCheck created
  s_MutexLeakCheck->Signal();

  PTRACE_IF(PTimedMutex::CtorDtorLogLevel, ok, "Destroyed " << mutex);
}
#endif // PTRACING


void PMutexExcessiveLockInfo::PrintOn(ostream &strm) const
{
  strm << " (" << m_location << ',' << m_excessiveLockTimeout << "ms)";
}


void PMutexExcessiveLockInfo::ExcessiveLockPhantom(const PObject & mutex) const
{
#if PTRACING
  PTRACE_BEGIN(0, "PTLib") << "Assertion fail: Phantom deadlock in " << mutex << PTrace::End;
#else
  PAssertAlways(PSTRSTRM("Phantom deadlock in mutex " << mutex));
#endif
}


void PMutexExcessiveLockInfo::AcquiredLock(uint64_t, bool, const PDebugLocation &)
{
}


void PMutexExcessiveLockInfo::ReleasedLock(const PObject & mutex,
                                           uint64_t PTRACE_PARAM(startHeldSamplePoint),
                                           bool,
                                           const PDebugLocation & PTRACE_PARAM(location))
{
  if (m_excessiveLockActive) {
#if PTRACING
    PTime releaseTime;
    PNanoSeconds heldDuration(PProfiling::CyclesToNanoseconds(PProfiling::GetCycles() - startHeldSamplePoint));
    ostream & trace = PTRACE_BEGIN(0, "PTLib");
    trace << "Assertion fail: Released phantom deadlock"
          << ", held from " << (releaseTime - heldDuration).AsString(PTime::TodayFormat, PTrace::GetTimeZone())
          << " to " << releaseTime.AsString(PTime::TodayFormat, PTrace::GetTimeZone())
          << " (" << heldDuration << "s),"
          << " in " << mutex;
    location.PrintOn(trace, " at ");
    switch (PTimedMutex::DeadlockStackWalkMode) {
    case PTimedMutex::DeadlockStackWalkOnPhantomRelease:
    case PTimedMutex::DeadlockStackWalkEnabled:
      PTrace::WalkStack(trace);
      break;
    case PTimedMutex::DeadlockStackWalkNoSymbols :
      trace << " stack: ";
      PTrace::WalkStack(trace, PThread::GetCurrentThreadId(), PThread::GetCurrentUniqueIdentifier(), true);
      break;
    default :
      break;
    }
    trace << PTrace::End;
#else
    PAssertAlways(PSTRSTRM("Released phantom deadlock in mutex " << mutex));
#endif
    m_excessiveLockActive = false;
  }
}


unsigned PMutexExcessiveLockInfo::MinDeadlockTime(unsigned waitTime)
{
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
  /* The sanitizers slow things down a LOT, so increase the
      time before indication of phantom deadlocks. */
  return std::max(waitTime*10U, 10000U);
#else
  return std::max(waitTime*2U,  1000U);
#endif
}


///////////////////////////////////////////////////////////////////////////////

PTimedMutex::PTimedMutex()
  : PTimedMutex(PDebugLocation(), 0)
{
}


PTimedMutex::PTimedMutex(const PDebugLocation & location, unsigned timeout)
  : PMutexExcessiveLockInfo(location, timeout)
  , m_lockerId(PNullThreadIdentifier)
  , m_lastLockerId(PNullThreadIdentifier)
  , m_lastUniqueId(0)
  , m_lockCount(0)
{
  PMUTEX_CONSTRUCTED();
}


PTimedMutex::PTimedMutex(const PTimedMutex & other)
  : PTimedMutex(other.m_location, other.m_excessiveLockTimeout)
{
}


PTimedMutex::~PTimedMutex()
{
  PMUTEX_DESTROYED();
}


void PTimedMutex::PrintOn(ostream &strm) const
{
  strm << "timed mutex " << this;
  PMutexExcessiveLockInfo::PrintOn(strm);
}


void PTimedMutex::Wait()
{
  InternalWait(NULL);
}


bool PTimedMutex::Wait(const PTimeInterval & timeout)
{
  if (timeout == PMaxTimeInterval) {
    InternalWait(NULL);
    return true;
  }

  uint64_t startWaitCycle = PProfiling::GetCycles();

  if (!try_lock_for(timeout.AsChronoNS()))
    return false;

  InternalWaitComplete(startWaitCycle, NULL);
  return true;
}


void PTimedMutex::Signal()
{
  unlock();
}


void PTimedMutex::InternalWait(const PDebugLocation * location)
{
  uint64_t startWaitCycle = PProfiling::GetCycles();

  if (!try_lock_for(std::chrono::milliseconds(m_excessiveLockTimeout))) {
    m_excessiveLockActive = true;

#if PTRACING
    PThreadIdentifier lockerId = m_lockerId;
    PThreadIdentifier lastLockerId = m_lastLockerId;
    PUniqueThreadIdentifier lastUniqueId = m_lastUniqueId;

    ostream & trace = PTRACE_BEGIN(0, "PTLib");
    trace << "Assertion fail: Possible deadlock in " << *this;

    switch (DeadlockStackWalkMode) {
    case DeadlockStackWalkEnabled :
      trace << "\n  Blocked Thread";
      OutputThreadInfo(trace, PThread::GetCurrentThreadId(), PThread::GetCurrentUniqueIdentifier(), false);
      trace << "\n  ";
      break;
    case DeadlockStackWalkNoSymbols :
      trace << ", Blocked Thread:";
      OutputThreadInfo(trace, PThread::GetCurrentThreadId(), PThread::GetCurrentUniqueIdentifier(), true);
      // do next case
    default :
      trace << ", ";
    }

    if (lockerId != PNullThreadIdentifier)
      trace << "Owner Thread:";
    else {
      trace << "Owner no longer has lock, last owner:";
      lockerId = lastLockerId;
    }

    switch (DeadlockStackWalkMode) {
    case DeadlockStackWalkEnabled :
      trace << '\n';
      OutputThreadInfo(trace, lockerId, lastUniqueId, false);
      break;
    case DeadlockStackWalkNoSymbols :
      OutputThreadInfo(trace, lockerId, lastUniqueId, true);
      break;
    default :
      break;
    }

    trace << PTrace::End;
#else
    PAssertAlways(PSTRSTRM("Possible deadlock in " << *this));
#endif

    lock();
    ExcessiveLockPhantom(*this);
  }

  InternalWaitComplete(startWaitCycle, location);
}


void PTimedMutex::InternalWaitComplete(uint64_t startWaitCycle, const PDebugLocation * location)
{
  // Note this is protected by the mutex itself only the thread with
  // the lock can alter these variables.

  if (m_lockCount++ == 0) {
    AcquiredLock(startWaitCycle, false, location);
    m_startHeldSamplePoint = PProfiling::GetCycles();
    m_lastLockerId = m_lockerId = PThread::GetCurrentThreadId();
    m_lastUniqueId = PThread::GetCurrentUniqueIdentifier();
  }
}


bool PTimedMutex::InternalSignal(const PDebugLocation * location)
{
  if (--m_lockCount > 0)
    return true;

  ReleasedLock(*this, m_startHeldSamplePoint, false, location);
  m_lockerId = PNullThreadIdentifier;
  return false;
}


#if PTRACING
bool PInstrumentedMutex::InstrumentedWait(const PTimeInterval & timeout, const PDebugLocation & location)
{
  uint64_t startWaitCycle = PProfiling::GetCycles();

  if (timeout == PMaxTimeInterval) {
    InternalWait(&location);
    return true;
  }

  if (!try_lock_for(timeout.AsChronoNS()))
    return false;

  InternalWaitComplete(startWaitCycle, &location);
  return true;
}


void PInstrumentedMutex::AcquiredLock(uint64_t startWaitCycle, bool, const PDebugLocation & location)
{
  m_timeWaitContext.EndMeasurement(this, this, &location, startWaitCycle);
}


void PInstrumentedMutex::ReleasedLock(const PObject & mutex, uint64_t startHeldSamplePoint, bool readOnly, const PDebugLocation & location)
{
  m_timeHeldContext.EndMeasurement(this, this, &location, startHeldSamplePoint);
  PTimedMutex::ReleasedLock(mutex, startHeldSamplePoint, readOnly, location);
}
#endif


///////////////////////////////////////////////////////////////////////////////

PSyncPoint::PSyncPoint()
  : m_signalled(false)
{
}


PSyncPoint::PSyncPoint(const PSyncPoint &)
  : PSyncPoint()
{
}


void PSyncPoint::Wait()
{
  PPROFILE_PRE_SYSTEM();
  std::unique_lock<std::mutex> lock(m_mutex);
  m_cv.wait(lock, [this] { return m_signalled; });
  m_signalled = false;
  PPROFILE_POST_SYSTEM();
}


bool PSyncPoint::Wait(const PTimeInterval & waitTime)
{
  PPROFILE_PRE_SYSTEM();
  std::unique_lock<std::mutex> lock(m_mutex);
  bool success = m_cv.wait_for(lock, waitTime.AsChronoNS(), [this] { return m_signalled; });
  if (success)
    m_signalled = false;
  PPROFILE_POST_SYSTEM();
  return success;
}


void PSyncPoint::Signal()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_signalled = true;
  }
  m_cv.notify_all();
}


/////////////////////////////////////////////////////////////////////////////

void PSyncPointAck::Signal()
{
  PSyncPoint::Signal();
  ack.Wait();
}


void PSyncPointAck::Signal(const PTimeInterval & wait)
{
  PSyncPoint::Signal();
  ack.Wait(wait);
}


void PSyncPointAck::Acknowledge()
{
  ack.Signal();
}


/////////////////////////////////////////////////////////////////////////////

void PCondMutex::WaitCondition()
{
  for (;;) {
    Wait();
    if (Condition())
      return;
    PMutex::Signal();
    OnWait();
    syncPoint.Wait();
  }
}


void PCondMutex::Signal()
{
  if (Condition())
    syncPoint.Signal();
  PMutex::Signal();
}


void PCondMutex::OnWait()
{
  // Do nothing
}


/////////////////////////////////////////////////////////////////////////////

PIntCondMutex::PIntCondMutex(int val, int targ, Operation op)
{
  value = val;
  target = targ;
  operation = op;
}


void PIntCondMutex::PrintOn(ostream & strm) const
{
  strm << '(' << value;
  switch (operation) {
    case LT :
      strm << " < ";
    case LE :
      strm << " <= ";
    case GE :
      strm << " >= ";
    case GT :
      strm << " > ";
    default:
      strm << " == ";
  }
  strm << target << ')';
}


bool PIntCondMutex::Condition()
{
  switch (operation) {
    case LT :
      return value < target;
    case LE :
      return value <= target;
    case GE :
      return value >= target;
    case GT :
      return value > target;
    default :
      break;
  }
  return value == target;
}


PIntCondMutex & PIntCondMutex::operator=(int newval)
{
  Wait();
  value = newval;
  Signal();
  return *this;
}


PIntCondMutex & PIntCondMutex::operator++()
{
  Wait();
  value++;
  Signal();
  return *this;
}


PIntCondMutex & PIntCondMutex::operator+=(int inc)
{
  Wait();
  value += inc;
  Signal();
  return *this;
}


PIntCondMutex & PIntCondMutex::operator--()
{
  Wait();
  value--;
  Signal();
  return *this;
}


PIntCondMutex & PIntCondMutex::operator-=(int dec)
{
  Wait();
  value -= dec;
  Signal();
  return *this;
}


/////////////////////////////////////////////////////////////////////////////

PReadWriteMutex::PReadWriteMutex()
  : PMutexExcessiveLockInfo()
#if P_READ_WRITE_ALGO2
  , m_inSemaphore(1, 1)
  , m_inCount(0)
  , m_outSemaphore(1, 1)
  , m_outCount(0)
  , m_writeSemaphore(0, 1)
  , m_wait(false)
#else
  , m_readerSemaphore(1, 1)
  , m_readerMutex()
  , m_readerCount(0)
  , m_starvationPreventer()
  , m_writerSemaphore(1, 1)
  , m_writerMutex()
  , m_writerCount(0)
#endif
{
  PMUTEX_CONSTRUCTED();
}

PReadWriteMutex::PReadWriteMutex(const PDebugLocation & location, unsigned timeout)
  : PMutexExcessiveLockInfo(location, timeout)
#if P_READ_WRITE_ALGO2
  , m_inSemaphore(1, 1)
  , m_inCount(0)
  , m_outSemaphore(1, 1)
  , m_outCount(0)
  , m_writeSemaphore(0, 1)
  , m_wait(false)
#else
  , m_readerSemaphore(1, 1)
  , m_readerMutex(location, timeout)
  , m_readerCount(0)
  , m_starvationPreventer(location, timeout)
  , m_writerSemaphore(1, 1)
  , m_writerMutex(location, timeout)
  , m_writerCount(0)
#endif
{
  PMUTEX_CONSTRUCTED();
}


PReadWriteMutex::~PReadWriteMutex()
{
  EndNest(); // Destruction while current thread has a lock is OK

  /* There is a small window during destruction where another thread is on the
     way out of EndRead() or EndWrite() where it checks for nested locks.
     While the check is protected by mutex, there is a moment between one
     check and the next where the object is unlocked. This is normally fine,
     except for if a thread then goes and deletes the object out from under
     the threads about to do the second check.

     Note if this goes into an endless loop then there is a big problem with
     the user of the PReadWriteMutex, as it must be CONTINUALLY trying to use
     the object when someone wants it gone. Technically this fix should be
     done by the user of the class too, but it is easier to fix here than
     there so practicality wins out!
   */
  while (!m_nestedThreads.empty())
    PThread::Sleep(10);

  PMUTEX_DESTROYED();
}


PReadWriteMutex::Nest * PReadWriteMutex::GetNest()
{
  PWaitAndSignal mutex(m_nestingMutex);
  NestMap::iterator it = m_nestedThreads.find(PThread::GetCurrentThreadId());
  return it != m_nestedThreads.end() ? &it->second : NULL;
}


void PReadWriteMutex::EndNest()
{
  m_nestingMutex.Wait();
  m_nestedThreads.erase(PThread::GetCurrentThreadId());
  m_nestingMutex.Signal();
}


PReadWriteMutex::Nest & PReadWriteMutex::StartNest()
{
  PWaitAndSignal mutex(m_nestingMutex);
  // The std::map will create the entry if it doesn't exist
  return m_nestedThreads[PThread::GetCurrentThreadId()];
}


void PReadWriteMutex::InternalStartRead(const PDebugLocation * location)
{
  uint64_t startWaitCycle = PProfiling::GetCycles();

  // Get the nested thread info structure, create one it it doesn't exist
  Nest & nest = StartNest();

  // One more nested call to StartRead() by this thread, note this does not
  // need to be mutexed as it is always in the context of a single thread.
  nest.m_readerCount++;

  // If this is the first call to StartRead() and there has not been a
  // previous call to StartWrite() then actually do the text book read only
  // lock, otherwise we leave it as just having incremented the reader count.
  if (nest.m_readerCount == 1 && nest.m_writerCount == 0) {
    InternalStartReadWithNest(nest, location);
    AcquiredLock(startWaitCycle, true, location);
    nest.m_startHeldCycle = PProfiling::GetCycles();
  }
}


void PReadWriteMutex::InternalWait(Nest & nest, PSync & sync, const PDebugLocation & location) const
{
  nest.m_waiting = true;

  if (sync.InstrumentedWait(m_excessiveLockTimeout, location)) {
    nest.m_waiting = false;
    return;
  }

  m_excessiveLockActive = true;

  NestMap nestedThreadsToDump;
  {
    PWaitAndSignal mutex(m_nestingMutex);
    nestedThreadsToDump = m_nestedThreads;
  }

#if PTRACING
  {
    ostream & trace = PTRACE_BEGIN(0, "PTLib");
    trace << "Assertion fail: Possible deadlock in " << *this << " :\n";
    for (NestMap::const_iterator it = nestedThreadsToDump.begin(); it != nestedThreadsToDump.end(); ++it) {
      if (it != nestedThreadsToDump.begin())
        trace << '\n';
      trace << "  thread-id=" << it->first << " (0x" << std::hex << it->first << std::dec << "),"
        " unique-id=" << it->second.m_uniqueId << ","
        " readers=" << it->second.m_readerCount << ","
        " writers=" << it->second.m_writerCount;
      if (!it->second.m_waiting)
        trace << ", LOCKER";
      switch (PTimedMutex::DeadlockStackWalkMode) {
      case PTimedMutex::DeadlockStackWalkEnabled:
        PTrace::WalkStack(trace, it->first, it->second.m_uniqueId);
        break;
      case PTimedMutex::DeadlockStackWalkNoSymbols:
        trace << ", stack: ";
        PTrace::WalkStack(trace, it->first, it->second.m_uniqueId, true);
        break;
      default:
        break;
      }
    }
    trace << PTrace::End;
  }
#else
  PAssertAlways(PSTRSTRM("Possible deadlock in " << *this));
#endif

  sync.InstrumentedWait(PMaxTimeInterval, location);
  ExcessiveLockPhantom(*this);

  nest.m_waiting = false;
}


void PReadWriteMutex::InternalEndRead(const PDebugLocation * location)
{
  // Get the nested thread info structure for the curent thread
  Nest * nest = GetNest();

  // If don't have an active read or write lock or there is a write lock but
  // the StartRead() was never called, then assert and ignore call.
  if (nest == NULL || nest->m_readerCount == 0) {
    PAssertAlways("Unbalanced PReadWriteMutex::EndRead()");
    return;
  }

  // One less nested lock by this thread, note this does not
  // need to be mutexed as it is always in the context of a single thread.
  nest->m_readerCount--;

  // If this is a nested read or a write lock is present then we don't do the
  // real unlock, the decrement is enough.
  if (nest->m_readerCount > 0 || nest->m_writerCount > 0)
    return;

  ReleasedLock(*this, nest->m_startHeldCycle, true, location);

  // Do text book read lock
  InternalEndReadWithNest(*nest, location);

  // At this point all read and write locks are gone for this thread so we can
  // reclaim the memory.
  EndNest();
}


void PReadWriteMutex::InternalStartWrite(const PDebugLocation * location)
{
  uint64_t startWaitCycle = PProfiling::GetCycles();

  // Get the nested thread info structure, create one it it doesn't exist
  Nest & nest = StartNest();

  // One more nested call to StartWrite() by this thread, note this does not
  // need to be mutexed as it is always in the context of a single thread.
  nest.m_writerCount++;

  // If is a nested call to StartWrite() then simply return, the writer count
  // increment is all we haev to do.
  if (nest.m_writerCount > 1)
    return;

  // If have a read lock already in this thread then do the "real" unlock code
  // but do not change the lock count, calls to EndRead() will now just
  // decrement the count instead of doing the unlock (its already done!)
  if (nest.m_readerCount > 0)
    InternalEndReadWithNest(nest, location);

  // Note in this gap another thread could grab the write lock

  InternalStartWriteWithNest(nest, location);

  AcquiredLock(startWaitCycle, false, location);
  m_startHeldSamplePoint = PProfiling::GetCycles();
}


void PReadWriteMutex::InternalEndWrite(const PDebugLocation * location)
{
  // Get the nested thread info structure for the curent thread
  Nest * nest = GetNest();

  // If don't have an active read or write lock or there is a read lock but
  // the StartWrite() was never called, then assert and ignore call.
  if (nest == NULL || nest->m_writerCount == 0) {
    PAssertAlways("Unbalanced PReadWriteMutex::EndWrite()");
    return;
  }

  // One less nested lock by this thread, note this does not
  // need to be mutexed as it is always in the context of a single thread.
  nest->m_writerCount--;

  // If this is a nested write lock then the decrement is enough and we
  // don't do the actual write unlock.
  if (nest->m_writerCount > 0)
    return;

  ReleasedLock(*this, m_startHeldSamplePoint, false, location);

  InternalEndWriteWithNest(*nest, location);

  // Now check to see if there was a read lock present for this thread, if so
  // then reacquire the read lock (not changing the count) otherwise clean up the
  // memory for the nested thread info structure
  if (nest->m_readerCount > 0)
    InternalStartReadWithNest(*nest, location);
  else
    EndNest();
}


void PReadWriteMutex::InternalStartReadWithNest(Nest & nest, const PDebugLocation & location)
{
#if P_READ_WRITE_ALGO2
  InternalWait(nest, m_inSemaphore);
  ++m_inCount;
  m_inSemaphore.Signal();
#else
  InternalWait(nest, m_starvationPreventer, location);
   InternalWait(nest, m_readerSemaphore, location);
    InternalWait(nest, m_readerMutex, location);

     m_readerCount++;
     if (m_readerCount == 1)
       InternalWait(nest, m_writerSemaphore, location);

    m_readerMutex.InstrumentedSignal(location);
   m_readerSemaphore.InstrumentedSignal(location);
  m_starvationPreventer.InstrumentedSignal(location);
#endif
}


void PReadWriteMutex::InternalEndReadWithNest(Nest & nest, const PDebugLocation & location)
{
#if P_READ_WRITE_ALGO2
  InternalWait(nest, m_outSemaphore);
  ++m_outCount;
  if (m_wait && m_inCount == m_outCount)
    m_writeSemaphore.Signal();
  m_outSemaphore.Signal();
#else
  InternalWait(nest, m_readerMutex, location);

  m_readerCount--;
  if (m_readerCount == 0)
    m_writerSemaphore.InstrumentedSignal(location);

  m_readerMutex.InstrumentedSignal(location);
#endif
}


void PReadWriteMutex::InternalStartWriteWithNest(Nest & nest, const PDebugLocation & location)
{
#if P_READ_WRITE_ALGO2
  InternalWait(nest, m_inSemaphore);
  InternalWait(nest, m_outSemaphore);
  if (m_inCount == m_outCount)
    m_outSemaphore.Signal();
  else {
    m_wait = true;
    m_outSemaphore.Signal();
    InternalWait(nest, m_writeSemaphore);
    m_wait = false;
  }
#else
  InternalWait(nest, m_writerMutex, location);

  m_writerCount++;
  if (m_writerCount == 1)
    InternalWait(nest, m_readerSemaphore, location);

  m_writerMutex.InstrumentedSignal(location);

  InternalWait(nest, m_writerSemaphore, location);
#endif
}


void PReadWriteMutex::InternalEndWriteWithNest(Nest & nest, const PDebugLocation & location)
{
#if P_READ_WRITE_ALGO2
  m_inSemaphore.Signal();
#else
  m_writerSemaphore.InstrumentedSignal(location);

  InternalWait(nest, m_writerMutex, location);

  m_writerCount--;
  if (m_writerCount == 0)
    m_readerSemaphore.InstrumentedSignal(location);

  m_writerMutex.InstrumentedSignal(location);
#endif
}

void PReadWriteMutex::PrintOn(ostream & strm) const
{
  strm << "read/write mutex " << this;
  PMutexExcessiveLockInfo::PrintOn(strm);
}


#if PTRACING
void PInstrumentedReadWriteMutex::AcquiredLock(uint64_t startWaitCycle, bool readOnly, const PDebugLocation & location)
{
  if (readOnly)
    m_timeWaitReadOnlyContext.EndMeasurement(this, this, &location, startWaitCycle);
  else
    m_timeWaitReadWriteContext.EndMeasurement(this, this, &location, startWaitCycle);
}


void PInstrumentedReadWriteMutex::ReleasedLock(const PObject & mutex, uint64_t startHeldSamplePoint, bool readOnly, const PDebugLocation & location)
{
  if (readOnly)
    m_timeHeldReadOnlyContext.EndMeasurement(this, this, &location, startHeldSamplePoint);
  else
    m_timeHeldReadWriteContext.EndMeasurement(this, this, &location, startHeldSamplePoint);

  PReadWriteMutex::ReleasedLock(mutex, startHeldSamplePoint, readOnly, location);
}
#endif


///////////////////////////////////////////////////////////////////////////////

PObject * PCriticalSection::Clone() const
{
  return new PCriticalSection();
}


void PCriticalSection::Wait()
{
  lock();
}


bool PCriticalSection::Wait(const PTimeInterval & timeout)
{
  if (timeout == 0)
    return Try();

  PTRACE(2, "PCriticalSection::Wait() called, this is very inefficient, consider using PTimedMutex!");

  PSimpleTimer timer(timeout);
  do {
    if (Try())
      return true;
    PThread::Sleep(100);
  } while (timer.IsRunning());
  return false;
}


void PCriticalSection::Signal()
{
  unlock();
}


///////////////////////////////////////////////////////////////////////////////
// PIdGenerator

const PIdGenerator::Handle PIdGenerator::Invalid = 0;

PIdGenerator::PIdGenerator()
  : m_nextId(1)
  , m_mutex(new PCriticalSection)
{
}


PIdGenerator::~PIdGenerator()
{
  delete m_mutex;
  m_mutex = NULL;
}


PIdGenerator::Handle PIdGenerator::Create()
{
  if (m_mutex == NULL) // Before construction or after destruction
    return Invalid;

  Handle id;

  m_mutex->Wait();

  do {
    id = m_nextId++;
  } while (id == Invalid || !m_inUse.insert(id).second);

  m_mutex->Signal();

  return id;
}


void PIdGenerator::Release(Handle id)
{
  if (id == Invalid || m_mutex == NULL) // Before construction or after destruction
    return;

  m_mutex->Wait();

  m_inUse.erase(id);

  m_mutex->Signal();
}


bool PIdGenerator::IsValid(Handle id) const
{
  if (id == Invalid || m_mutex == NULL) // Before construction or after destruction
    return false;

  PWaitAndSignal mutex(*m_mutex);
  return m_inUse.find(id) != m_inUse.end();
}


///////////////////////////////////////////////////////////////////////

PFilePath::PFilePath(const PString & str)
  : PFilePathString(Canonicalise(str, false))
{
}


PFilePath::PFilePath(const char * cstr)
  : PFilePathString(Canonicalise(cstr, false))
{
}


PFilePath::PFilePath(const char * prefix, const char * dir, const char * suffix)
{
  PStringStream path;
  if (dir != NULL)
    path << PDirectory(dir);
  else
    path << PDirectory::GetTemporary();

  PProcess & process = PProcess::Current();
  if (prefix != NULL)
    path << prefix;
  else {
    PString appName = process.GetName().Left(10);
    appName.Replace(" ", "_", true);
    path << appName;
  }

  path << '_' << PProcess::Current().GetUniqueIdentifier() << '_';

  PThread * thread = PThread::Current();
  if (thread != &process)
    path << thread->GetUniqueIdentifier() << '_';

  if (suffix == NULL)
    suffix = "";

  PString base = path;
  path << setfill('0') << hex;
  for (unsigned tieBreak = 0; tieBreak < 1000; ++tieBreak) {
    path << setw(5) << PRandom::Number(0x100000) << suffix;
    *this = path;
    if (!PFile::Exists(*this))
      return;
    path = base;
  }

  PAssertAlways(PSTRSTRM("Could not generate a unique temporary file name, using base " << path));
}


PDirectory PFilePath::GetDirectory() const
{
  if (!IsEmpty()) {
    PINDEX sep = FindLast(PDIR_SEPARATOR);
    if (PAssert(sep != P_MAX_INDEX, "Bad file path"))
      return Left(sep);
  }
  return PString::Empty();
}


PFilePathString PFilePath::GetFileName() const
{
  if (!IsEmpty()) {
    PINDEX sep = FindLast(PDIR_SEPARATOR);
    if (PAssert(sep != P_MAX_INDEX, "Bad file path"))
      return Mid(sep+1);
  }
  return PString::Empty();
}


PFilePathString PFilePath::GetTitle() const
{
  PFilePathString filename = GetFileName();
  return filename.Left(filename.FindLast('.'));
}


PFilePathString PFilePath::GetType() const
{
  PFilePathString filename = GetFileName();
  return filename.Mid(filename.FindLast('.'));
}


void PFilePath::SetType(const PFilePathString & newType)
{
  PFilePathString oldType = GetType();
  if (oldType.IsEmpty())
    *this += newType;
  else
    Splice(newType, GetLength() - oldType.GetLength(), oldType.GetLength());
}


PFilePathString PFilePath::Sanitise(const PString & str, const PString & extra, char substitute)
{
  PFilePathString sanitised;
  for (PINDEX i = 0; i < str.GetLength(); ++i) {
    if (!PFilePath::IsValid(str[i]) || extra.Find(str[i]) != P_MAX_INDEX)
      sanitised += substitute;
    else
      sanitised += str[i];
  }
  return sanitised;
}


void PFile::SetFilePath(const PString & newName)
{
  Close();
  m_path = PFilePath::Canonicalise(newName, false);
}


///////////////////////////////////////////////////////////////////////////////

#undef  PTraceModule
#define PTraceModule() "FileRotate"


const PString & PFile::RotateInfo::DefaultTimestamp() { static PConstString s("_yyyy_MM_dd_hh_mm"); return s; }


PFile::RotateInfo::RotateInfo(const PDirectory & dir,
                              const PString & prefix,
                              const PString & suffix,
                              const PString & timestamp,
                              off_t maxSize,
                              off_t freeDisk)
  : m_directory(dir)
  , m_prefix(prefix.IsEmpty() ? PProcess::Current().GetName() : prefix)
  , m_timestamp(timestamp)
#if PTRACING
  , m_timeZone((PTrace::GetOptions()&PTrace::GMTTime) ? PTime::GMT : PTime::Local)
#else
  , m_timeZone(PTime::Local)
#endif
  , m_suffix(suffix)
  , m_maxSize(maxSize)
  , m_freeDisk(freeDisk)
  , m_period(SizeOnly)
  , m_maxFileCount(0)
  , m_lastTime(0)
{
}


PFile::RotateInfo::RotateInfo(const RotateInfo & other)
  : m_directory(other.m_directory)
  , m_prefix(other.m_prefix)
  , m_timestamp(other.m_timestamp)
  , m_timeZone(other.m_timeZone)
  , m_maxSize(other.m_maxSize)
  , m_freeDisk(other.m_freeDisk)
  , m_period(other.m_period)
  , m_maxFileCount(other.m_maxFileCount)
  , m_lastTime(0)
{
}


PFile::RotateInfo & PFile::RotateInfo::operator=(const RotateInfo & other)
{
  m_directory = other.m_directory;
  m_prefix = other.m_prefix;
  m_timestamp = other.m_timestamp;
  m_timeZone = other.m_timeZone;
  m_maxSize = other.m_maxSize;
  m_freeDisk = other.m_freeDisk;
  m_period = other.m_period;
  m_maxFileCount = other.m_maxFileCount;
  return *this;
}


bool PFile::RotateInfo::CanRotate() const
{
  return m_maxSize > 0 && !m_timestamp.IsEmpty();
}


typedef std::multimap<PTime, PFilePath> FilesByTime;

static void DeleteOne(PFile::RotateInfo & info, FilesByTime & rotatedFiles, const char * msg)
{
  FilesByTime::iterator itFile = rotatedFiles.begin();
  PFilePath filePath = itFile->second;
  if (PFile::Remove(filePath))
    info.OnMessage(false, PSTRSTRM("Removed " << msg << " file \"" << filePath << '"'));
  else
    info.OnMessage(true, PSTRSTRM("Could not remove " << msg << " file \"" << filePath << '"'));
  rotatedFiles.erase(itFile);
}


bool PFile::RotateInfo::Rotate(PFile & file, bool force, const PTime & now)
{
  if (m_maxSize == 0 || m_timestamp.IsEmpty())
    return false;

  if (file.IsOpen() && file.GetLength() > m_maxSize) {
    PTRACE(4, &file, "File greater than " << m_maxSize << " bytes");
    force = true;
  }

  switch (m_period) {
    case Hourly :
      if (now.GetHour() != m_lastTime.GetHour())
        force = true;
      break;

    case Daily :
      if (now.GetDay() != m_lastTime.GetDay())
        force = true;
      break;

    case Weekly :
      if (now.GetDayOfWeek() != m_lastTime.GetDayOfWeek() && now.GetDayOfWeek() == PTime::Sunday)
        force = true;
      break;

    case Monthly :
      if (now.GetMonth() != m_lastTime.GetMonth())
        force = true;
      break;

    default :
      break;
  }

  if (!force)
    return false;

  PTRACE(4, &file, "Rotating:"
         " last=" << m_lastTime.AsString(PTime::LoggingFormat, PTrace::GetTimeZone()) << ","
         " period=" << m_period << ","
         " maxFileCount=" << m_maxFileCount << ","
         " maxFileAge=" << m_maxFileAge.AsString(0, PTimeInterval::IncludeDays) << ","
         " freeDisk=" << m_freeDisk << ","
         " dir=" << m_directory << ","
         " prefix=" << m_prefix << ","
         " suffix=" << m_suffix);

  m_lastTime = now;

  PFilePath rotatedFile;
  PString timestamp = now.AsString(m_timestamp, m_timeZone);
  PString tiebreak;
  do {
      rotatedFile = PSTRSTRM(m_directory << m_prefix << timestamp << tiebreak << m_suffix);
      tiebreak = tiebreak.AsInteger()-1; // This appends "-1", "-2" etc
  } while (PFile::Exists(rotatedFile));

  if (file.IsOpen()) {
    OnCloseFile(file, rotatedFile);
    file.Close();
  }
  else {
    if (file.GetFilePath().IsEmpty())
      file.SetFilePath(m_directory + m_prefix + m_suffix);
  }

  bool badMove = file.Exists() && !PFile::Move(file.GetFilePath(), rotatedFile, false, true);

  bool ok = OnOpenFile(file);

  // Do the OnMessage for bad move after the reopen, in case it uses the newly reopened file.
  if (badMove)
    OnMessage(true, PSTRSTRM("Could not move file from " << file.GetFilePath() << " to " << rotatedFile));

  if (m_maxFileCount > 0 || m_maxFileAge > 0 || m_freeDisk != 0) {
    FilesByTime rotatedFiles;

    PDirectory dir(m_directory);
    if (dir.Open(PFileInfo::RegularFile)) {
      PFileInfo info;
      int failsafe = 10000;
      do {
        PFilePathString name = dir.GetEntryName();
        if (  name != file.GetFilePath().GetFileName() &&
              name.NumCompare(m_prefix, m_prefix.GetLength()) == EqualTo &&
              name.NumCompare(m_suffix, m_suffix.GetLength(), name.GetLength() - m_suffix.GetLength()) == EqualTo &&
              dir.GetInfo(info))
          rotatedFiles.insert(std::multimap<PTime, PFilePath>::value_type(info.modified, dir + name));
      } while (dir.Next() && --failsafe > 0);
    }

    if (m_maxFileCount > 0) {
      while (rotatedFiles.size() > m_maxFileCount)
        DeleteOne(*this, rotatedFiles, "excess");
    }

    if (m_maxFileAge > 0) {
      PTime then = now - m_maxFileAge;
      while (!rotatedFiles.empty() && rotatedFiles.begin()->first < then)
        DeleteOne(*this, rotatedFiles, "aged");
    }

    if (m_freeDisk != 0) {
      int64_t total, free;
      uint32_t cluster;
      while (!rotatedFiles.empty() && dir.GetVolumeSpace(total, free, cluster)) {
        int64_t limit = m_freeDisk > 0 ? m_freeDisk : (total * -m_freeDisk / 100);
        PTRACE(4, &file, "Disk:"
               " total=" << PString(PString::ScaleSI, total, 4) << "b,"
               " free="  << PString(PString::ScaleSI, free,  4) << "b,"
               " limit=" << PString(PString::ScaleSI, limit, 4) << 'b');
        if (free > limit)
          break;
        DeleteOne(*this, rotatedFiles, "low disk space");
      }
    }
  }

  return ok;
}


void PFile::RotateInfo::OnCloseFile(PFile & file, const PFilePath & rotatedTo)
{
  OnMessage(false, PSTRSTRM("File renamed from \"" << file.GetFilePath() << "\" to \"" << rotatedTo << '"'));
}


bool PFile::RotateInfo::OnOpenFile(PFile & file)
{
  if (file.Open(PFile::WriteOnly))
    return true;

  OnMessage(true, PSTRSTRM("Could not re-open file \"" << file.GetFilePath() << "\" - " << file.GetErrorText()));
  return false;
}


void PFile::RotateInfo::OnMessage(bool PTRACE_PARAM(error), const PString & PTRACE_PARAM(msg))
{
  PTRACE(error ? 2 : 3, NULL, PTraceModule(), msg);
}


// End Of File ///////////////////////////////////////////////////////////////
