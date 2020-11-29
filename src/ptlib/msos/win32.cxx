/*
 * win32.cxx
 *
 * Miscellaneous implementation of classes for Win32
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
#include <ptlib/pprocess.h>

#include <ptlib/msos/ptlib/debstrm.h>
#include <ptlib/msos/ptlib/pt_atl.h>

#include <process.h>
#include <errors.h>
#include <shlobj.h>
#include <Psapi.h>

#ifdef _MSC_VER
  #pragma comment(lib, "mpr.lib")
  #pragma comment(lib, "Shell32.lib")

  #ifdef P_WIN_COM
    #pragma comment(lib, "ole32.lib")
  #endif

  #if P_DIRECTSHOW
    #pragma comment(lib, "quartz.lib")
  #endif

  #pragma comment(lib, "Psapi.lib")
#endif

#if P_VERSION_HELPERS
  #include <versionhelpers.h>
#endif

#define new PNEW


///////////////////////////////////////////////////////////////////////////////
// PTime

#ifdef UNICODE
static void PWIN32GetLocaleInfo(LCID Locale,LCTYPE LCType,LPSTR lpLCData,int cchData)
{
  TCHAR* pw = new TCHAR[cchData+1];
  GetLocaleInfo(Locale,LCType,pw,cchData);
  lpLCData[0]=0;
  WideCharToMultiByte(GetACP(), 0, pw, -1, lpLCData, cchData, NULL, NULL);
}
#else

#define PWIN32GetLocaleInfo GetLocaleInfo

#endif



PString PTime::GetTimeSeparator()
{
  char str[100];
  PWIN32GetLocaleInfo(GetUserDefaultLCID(), LOCALE_STIME, str, sizeof(str));
  return str;
}


bool PTime::GetTimeAMPM()
{
  char str[2];
  PWIN32GetLocaleInfo(GetUserDefaultLCID(), LOCALE_ITIME, str, sizeof(str));
  return str[0] == '0';
}


PString PTime::GetTimeAM()
{
  char str[100];
  PWIN32GetLocaleInfo(GetUserDefaultLCID(), LOCALE_S1159, str, sizeof(str));
  return str;
}


PString PTime::GetTimePM()
{
  char str[100];
  PWIN32GetLocaleInfo(GetUserDefaultLCID(), LOCALE_S2359, str, sizeof(str));
  return str;
}


PString PTime::GetDayName(Weekdays dayOfWeek, NameType type)
{
  char str[100];
  // Of course Sunday is 6 and Monday is 1...
  PWIN32GetLocaleInfo(GetUserDefaultLCID(),
                      (dayOfWeek+6)%7 + (type == Abbreviated ? LOCALE_SABBREVDAYNAME1 : LOCALE_SDAYNAME1),
                      str, sizeof(str));
  return str;
}


PString PTime::GetDateSeparator()
{
  char str[100];
  PWIN32GetLocaleInfo(GetUserDefaultLCID(), LOCALE_SDATE, str, sizeof(str));
  return str;
}


PString PTime::GetMonthName(Months month, NameType type)
{
  char str[100];
  PWIN32GetLocaleInfo(GetUserDefaultLCID(),
                      month-1 + (type == Abbreviated ? LOCALE_SABBREVMONTHNAME1 : LOCALE_SMONTHNAME1),
                      str, sizeof(str));
  return str;
}


PTime::DateOrder PTime::GetDateOrder()
{
  char str[2];
  PWIN32GetLocaleInfo(GetUserDefaultLCID(), LOCALE_IDATE, str, sizeof(str));
  return (DateOrder)(str[0] - '0');
}


bool PTime::IsDaylightSavings()
{
  TIME_ZONE_INFORMATION tz;
  uint32_t result = GetTimeZoneInformation(&tz);
  PAssertOS(result != 0xffffffff);
  return result == TIME_ZONE_ID_DAYLIGHT;
}


int PTime::GetTimeZone(TimeZoneType type)
{
  TIME_ZONE_INFORMATION tz;
  PAssertOS(GetTimeZoneInformation(&tz) != 0xffffffff);
  if (type == DaylightSavings)
    tz.Bias += tz.DaylightBias;
  return -tz.Bias;
}


PString PTime::GetTimeZoneString(TimeZoneType type)
{
  TIME_ZONE_INFORMATION tz;
  PAssertOS(GetTimeZoneInformation(&tz) != 0xffffffff);
  return (const wchar_t *)(type == StandardTime ? tz.StandardName : tz.DaylightName);
}


///////////////////////////////////////////////////////////////////////////////
// Directories

void PDirectory::Construct()
{
  m_hFindFile = INVALID_HANDLE_VALUE;
  m_fileInfo.cFileName[0] = '\0';
}


bool PDirectory::Open(PFileInfo::FileTypes newScanMask)
{
  m_scanMask = newScanMask;
  PVarString wildcard = *this + "*.*";

  m_hFindFile = FindFirstFile(wildcard, &m_fileInfo);
  if (m_hFindFile == INVALID_HANDLE_VALUE) {
    PTRACE_IF(2, GetLastError() != ERROR_PATH_NOT_FOUND && GetLastError() != ERROR_DIRECTORY,
              "PTLib", "Could not open directory \"" << *this << "\", error=" << GetLastError());
    return false;
  }

  return InternalEntryCheck() || Next();
}


bool PDirectory::Next()
{
  if (m_hFindFile == INVALID_HANDLE_VALUE)
    return false;

  do {
    if (!FindNextFile(m_hFindFile, &m_fileInfo))
      return false;
  } while (!InternalEntryCheck());

  return true;
}


PCaselessString PDirectory::GetEntryName() const
{
  return m_fileInfo.cFileName;
}


bool PDirectory::IsSubDir() const
{
  return (m_fileInfo.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) != 0;
}


PCaselessString PDirectory::GetVolume() const
{
  char volName[100];
  PAssertOS(GetVolumeInformation(NULL, volName, sizeof(volName), NULL, NULL, NULL, NULL, 0));
  return PCaselessString(volName);
}


void PDirectory::Close()
{
  if (m_hFindFile != INVALID_HANDLE_VALUE) {
    FindClose(m_hFindFile);
    m_hFindFile = INVALID_HANDLE_VALUE;
  }
}


PFilePathString PFilePath::Canonicalise(const PFilePathString & path, bool isDirectory)
{
  PString partialpath = path;
  if (partialpath.IsEmpty()) {
      if (!isDirectory)
        return path;
      partialpath = ".";
  }
  else {
    // Look for special case of "\c:\" at start of string as some generalised
    // directory processing algorithms have a habit of adding a leading
    // PDIR_SEPARATOR as it would be for Unix.
    if (partialpath.NumCompare("\\\\\\") == EqualTo ||
      (partialpath.GetLength() > 3 &&
       partialpath[0] == PDIR_SEPARATOR &&
       partialpath[2] == ':'))
      partialpath.Delete(0, 1);
  }

  LPSTR dummy;
  uint32_t len = (PINDEX)GetFullPathName(partialpath, 0, NULL, &dummy);
  if (len-- == 0)
     return PString::Empty();
   PString fullpath;
   GetFullPathName(partialpath, len+1, fullpath.GetPointerAndSetLength(len), &dummy);

  if (isDirectory && len > 0 && fullpath[len-1] != PDIR_SEPARATOR)
    fullpath += PDIR_SEPARATOR;

  PINDEX pos = 0;
  while ((pos = fullpath.Find('/', pos)) != P_MAX_INDEX)
    fullpath[pos] = PDIR_SEPARATOR;
  return fullpath;
}


typedef bool (WINAPI *GetDiskFreeSpaceExType)(LPCTSTR lpDirectoryName,
                                              PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                                              PULARGE_INTEGER lpTotalNumberOfBytes,
                                              PULARGE_INTEGER lpTotalNumberOfFreeBytes);


bool PDirectory::GetVolumeSpace(int64_t & total, int64_t & free, uint32_t & clusterSize) const
{
  clusterSize = 512;
  total = free = ULONG_MAX;

  PString root;
  if ((*this)[1] == ':')
    root = Left(3);
  else if (GetAt(0) == '\\' && GetAt(1) == '\\') {
    PINDEX backslash = Find('\\', 2);
    if (backslash != P_MAX_INDEX) {
      backslash = Find('\\', backslash+1);
      if (backslash != P_MAX_INDEX)
        root = Left(backslash+1);
    }
  }

  if (root.IsEmpty())
    return false;

  bool needTotalAndFree = true;

  static GetDiskFreeSpaceExType GetDiskFreeSpaceEx =
        (GetDiskFreeSpaceExType)GetProcAddress(LoadLibrary("KERNEL32.DLL"), "GetDiskFreeSpaceExA");
  if (GetDiskFreeSpaceEx != NULL) {
    ULARGE_INTEGER freeBytesAvailableToCaller;
    ULARGE_INTEGER totalNumberOfBytes; 
    ULARGE_INTEGER totalNumberOfFreeBytes;
    if (GetDiskFreeSpaceEx(root,
                           &freeBytesAvailableToCaller,
                           &totalNumberOfBytes,
                           &totalNumberOfFreeBytes)) {
      total = totalNumberOfBytes.QuadPart;
      free = totalNumberOfFreeBytes.QuadPart;
      needTotalAndFree = false;
    }
  }

  clusterSize = 0;
  char fsName[100];
  if (GetVolumeInformation(root, NULL, 0, NULL, NULL, NULL, fsName, sizeof(fsName))) {
    if (strcasecmp(fsName, "FAT32") == 0) {
      clusterSize = 4096; // Cannot use GetDiskFreeSpace() results for FAT32
      if (!needTotalAndFree)
        return true;
    }
  }

  DWORD sectorsPerCluster;      // address of sectors per cluster 
  DWORD bytesPerSector;         // address of bytes per sector 
  DWORD numberOfFreeClusters;   // address of number of free clusters  
  DWORD totalNumberOfClusters;  // address of total number of clusters 

  if (!GetDiskFreeSpace(root,
                        &sectorsPerCluster,
                        &bytesPerSector,
                        &numberOfFreeClusters,
                        &totalNumberOfClusters)) 
{
    if (root[0] != '\\' || ::GetLastError() != ERROR_NOT_SUPPORTED)
      return false;

    PString drive = "A:";
    while (WNetAddConnection(root, NULL, drive) != NO_ERROR) {
      if (::GetLastError() != ERROR_ALREADY_ASSIGNED)
        return false;
      drive[0]++;
    }
    bool ok = GetDiskFreeSpace(drive+'\\',
                               &sectorsPerCluster,
                               &bytesPerSector,
                               &numberOfFreeClusters,
                               &totalNumberOfClusters);
    WNetCancelConnection(drive, true);
    if (!ok)
      return false;
  }

  if (needTotalAndFree) {
    free = numberOfFreeClusters*sectorsPerCluster*bytesPerSector;
    total = totalNumberOfClusters*sectorsPerCluster*bytesPerSector;
  }

  if (clusterSize == 0)
    clusterSize = bytesPerSector*sectorsPerCluster;

  return true;
}


///////////////////////////////////////////////////////////////////////////////
// PFilePath

static char const IllegalFilenameCharacters[] =
  "\\/:*?\"<>|"
  "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\0x10"
  "\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f";

bool PFilePath::IsValid(char c)
{
  return strchr(IllegalFilenameCharacters, c) == NULL;
}


bool PFilePath::IsValid(const PString & str)
{
  return str != "." && str != ".." &&
         str.FindOneOf(IllegalFilenameCharacters) == P_MAX_INDEX;
}


bool PFilePath::IsAbsolutePath(const PString & path)
{
  return path.GetLength() > 2 && (path[1] == ':' || (path[0] == '\\' && path[1] == '\\'));
}


///////////////////////////////////////////////////////////////////////////////
// PFile

bool PFile::Touch(const PFilePath & name, const PTime & accessTime, const PTime & modTime)
{
  PWin32Handle hFile(::CreateFile(name,FILE_WRITE_ATTRIBUTES,FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL));
  if (!hFile.IsValid())
    return false;

  FILETIME now;
  GetSystemTimeAsFileTime(&now);

  // Magic constant to convert epoch from 1601 to 1970
  static const ULONGLONG Win332FileTimeDelta = ((int64_t)369*365+(369/4)-3)*24*60*60U*1000000;

  ULARGE_INTEGER uli;
  FILETIME acc;
  if (!accessTime.IsValid())
    acc = now;
  else {
    uli.QuadPart = (accessTime.GetTimestamp()+Win332FileTimeDelta)*10;
    acc.dwHighDateTime = uli.HighPart;
    acc.dwLowDateTime = uli.LowPart;
  }

  FILETIME mod;
  if (!modTime.IsValid())
    mod = now;
  else {
    uli.QuadPart = (modTime.GetTimestamp()+Win332FileTimeDelta)*10;
    acc.dwHighDateTime = uli.HighPart;
    acc.dwLowDateTime = uli.LowPart;
  }

  return ::SetFileTime(hFile, NULL, &acc, &mod);
}


///////////////////////////////////////////////////////////////////////////////
// PChannel

HANDLE PChannel::GetAsyncReadHandle() const
{
  return INVALID_HANDLE_VALUE;
}


HANDLE PChannel::GetAsyncWriteHandle() const
{
  return INVALID_HANDLE_VALUE;
}


static VOID CALLBACK StaticOnIOComplete(DWORD dwErrorCode,
                                        DWORD dwNumberOfBytesTransfered,
                                        LPOVERLAPPED lpOverlapped)
{
  ((PChannel::AsyncContext *)lpOverlapped)->OnIOComplete(dwNumberOfBytesTransfered, dwErrorCode);
}


void PChannel::AsyncContext::SetOffset(off_t offset)
{
  Offset = (uint32_t)offset;
#if P_64BIT
  OffsetHigh = (uint32_t)((int64_t)offset>>32);
#endif
}


bool PChannel::AsyncContext::Initialise(PChannel * channel, CompletionFunction onComplete)
{
  if (m_channel != NULL)
    return false;

  m_channel = channel;
  m_onComplete = onComplete;
  return true;
}


bool PChannel::ReadAsync(AsyncContext & context)
{
  if (CheckNotOpen())
    return false;

  HANDLE handle = GetAsyncReadHandle();
  if (handle == INVALID_HANDLE_VALUE)
    return SetErrorValues(ProtocolFailure, EFAULT);

  if (!PAssert(context.Initialise(this, &PChannel::OnReadComplete),
               "Multiple async read with same context!"))
    return SetErrorValues(ProtocolFailure, EINVAL);

  return ConvertOSError(ReadFileEx(handle,
                                   context.m_buffer,
                                   context.m_length,
                                   &context,
                                   StaticOnIOComplete) && GetLastError() == 0 ? 0 : -2, LastReadError);

}


bool PChannel::WriteAsync(AsyncContext & context)
{
  if (CheckNotOpen())
    return false;

  HANDLE handle = GetAsyncWriteHandle();
  if (handle == INVALID_HANDLE_VALUE)
    return SetErrorValues(ProtocolFailure, EFAULT);

  if (!PAssert(context.Initialise(this, &PChannel::OnWriteComplete),
               "Multiple async write with same context!"))
    return SetErrorValues(ProtocolFailure, EINVAL);

  return ConvertOSError(WriteFileEx(handle,
                                    context.m_buffer,
                                    context.m_length,
                                    &context,
                                    StaticOnIOComplete) ? 0 : -2, LastWriteError);
}


PString PChannel::GetErrorText(Errors lastError, int osError)
{
  if (osError == 0) {
    if (lastError == NoError)
      return PString();

    static int const errors[NumNormalisedErrors] = {
      0, ENOENT, EEXIST, ENOSPC, EACCES, EBUSY, EINVAL, ENOMEM, EBADF, EAGAIN, ECANCELED, WSAEMSGSIZE|PWIN32ErrorFlag, EIO
    };
    osError = errors[lastError];
    if (osError == 0)
      return psprintf("High level error %u", lastError);
  }

  if (osError > 0) {
    char errStr[100];
    strerror_s(errStr, sizeof(errStr), osError);
    if (*errStr != '\0')
      return errStr;
  }

  if ((osError&PWIN32ErrorFlag) == 0)
    return psprintf("C runtime error %u", osError);

  osError &= ~PWIN32ErrorFlag;

  LPVOID lpMsgBuf;
  uint32_t count = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL, osError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

  PString str((LPCSTR)lpMsgBuf, count);
  LocalFree(lpMsgBuf);

  return str.IsEmpty() ? psprintf("WIN32 error %u", osError & ~PWIN32ErrorFlag) : str.Trim();
}


bool PChannel::ConvertOSError(intptr_t libcReturnValue, ErrorGroup group)
{
  int osError = GetErrorNumber(group);

  switch (libcReturnValue) {
    case -1 :
      if ((osError = os_errno()) != 0)
        break;
      // Do next case

    case -2 :
      osError = ::GetLastError();
      switch (osError) {
        case ERROR_INVALID_HANDLE :
          osError = EBADF;
          break;
        case ERROR_INVALID_PARAMETER :
          osError = EINVAL;
          break;
        case ERROR_ACCESS_DENIED :
          osError = EACCES;
          break;
        case ERROR_NOT_ENOUGH_MEMORY :
          osError = ENOMEM;
          break;
        default :
          osError |= PWIN32ErrorFlag;
      }
      break;

    case -3 :
      break;

    default :
      osError = 0;
  }

  Errors lastError;
  switch (osError) {
    case 0 :
      lastError = NoError;
      break;
    case ENOENT :
      lastError = NotFound;
      break;
    case EEXIST :
      lastError = FileExists;
      break;
    case EACCES :
      lastError = AccessDenied;
      break;
    case ENOMEM :
      lastError = NoMemory;
      break;
    case ENOSPC :
      lastError = DiskFull;
      break;
    case EINVAL :
      lastError = BadParameter;
      break;
    case EBADF :
    case ENOTSOCK :
      lastError = NotOpen;
      break;
    case EAGAIN :
    case ETIMEDOUT :
    case EWOULDBLOCK :
      lastError = Timeout;
      break;
    case EMSGSIZE :
      lastError = BufferTooSmall;
      break;
    case EINTR :
    case ECANCELED :
      lastError = Interrupted;
      break;
    default :
      lastError = Miscellaneous;
  }

  return SetErrorValues(lastError, osError, group);
}


///////////////////////////////////////////////////////////////////////////////
// PWin32Overlapped

PWin32Overlapped::PWin32Overlapped()
{
  memset(this, 0, sizeof(*this));
  hEvent = CreateEvent(NULL, true, false, NULL);
}

PWin32Overlapped::~PWin32Overlapped()
{
  if (hEvent != NULL)
    CloseHandle(hEvent);
}

void PWin32Overlapped::Reset()
{
  Offset = OffsetHigh = 0;
  if (hEvent != NULL)
    ResetEvent(hEvent);
}


///////////////////////////////////////////////////////////////////////////////
// Threads

void PThread::PlatformConstruct()
{
  #if defined(P_WIN_COM)
    m_comInitialised = false;
  #endif
}


void PThread::PlatformDestroy()
{
  #if defined(P_WIN_COM)
  if (m_comInitialised)
    ::CoUninitialize();
  #endif
}


void PThread::PlatformSetThreadName(const char* name)
{
  struct THREADNAME_INFO
  {
    uint32_t dwType;      // must be 0x1000
    LPCSTR szName;     // pointer to name (in user addr space)
    uint32_t dwThreadID;  // thread ID (-1=caller thread, but seems to set more than one thread's name)
    uint32_t dwFlags;     // reserved for future use, must be zero
  } threadInfo = { 0x1000, name, GetUniqueIdentifier(), 0 };

  __try {
    RaiseException(0x406D1388, 0, sizeof(threadInfo) / sizeof(uint32_t), (const ULONG_PTR*)&threadInfo);
    // if not running under debugger exception comes back
  }
  __except (EXCEPTION_CONTINUE_EXECUTION) {
    // just keep on truckin'
  }
}


void PThread::Terminate()
{
  if (PAssert(m_type != Type::IsProcess, "Cannot terminate the process!") && m_nativeHandle != NULL)
  {
    PTRACE(2, "PTLib\tTerminating thread " << *this);
    TerminateThread(m_nativeHandle, 1);
  }
}


void PThread::SetPriority(Priority priorityLevel)
{
  static int const priorities[NumPriority] = {
    THREAD_PRIORITY_LOWEST,
    THREAD_PRIORITY_BELOW_NORMAL,
    THREAD_PRIORITY_NORMAL,
    THREAD_PRIORITY_ABOVE_NORMAL,
    THREAD_PRIORITY_HIGHEST
  };
  if (PAssert(m_nativeHandle != NULL, "Operation on terminated thread"))
    SetThreadPriority(m_nativeHandle, priorities[priorityLevel]);
}


PThread::Priority PThread::GetPriority() const
{
  switch (GetThreadPriority(m_nativeHandle)) {
    case THREAD_PRIORITY_LOWEST:
      return LowestPriority;
    case THREAD_PRIORITY_BELOW_NORMAL:
      return LowPriority;
    case THREAD_PRIORITY_NORMAL:
      return NormalPriority;
    case THREAD_PRIORITY_ABOVE_NORMAL:
      return HighPriority;
    case THREAD_PRIORITY_HIGHEST:
      return HighestPriority;
  }
  PAssertAlways(POperatingSystemError);
  return LowestPriority;
}


#ifdef P_WIN_COM
static atomic<bool> s_securityInitialised(false);

bool PThread::CoInitialise()
{
  if (m_comInitialised)
    return true;

  HRESULT result = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
  if (FAILED(result)) {
    PTRACE_IF(1, result != RPC_E_CHANGED_MODE, "PTLib", "Could not initialise COM: error=0x" << hex << result);
    return false;
  }

  if (!s_securityInitialised.exchange(true)) {
    result = ::CoInitializeSecurity(NULL, 
                                    -1,                          // COM authentication
                                    NULL,                        // Authentication services
                                    NULL,                        // Reserved
                                    RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
                                    RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
                                    NULL,                        // Authentication info
                                    EOAC_NONE,                   // Additional capabilities 
                                    NULL                         // Reserved
                                    );
    PTRACE_IF(2, result, "PTLib", "Could not initialise COM security: error=0x" << hex << result);
  }

  m_comInitialised = true;
  return true;
}


std::ostream & operator<<(std::ostream & strm, const PComResult & result)
{
  TCHAR msg[MAX_ERROR_TEXT_LEN+1];
  uint32_t dwMsgLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL,
                                 result.m_result,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                 msg, sizeof(msg),
                                 NULL);
  while (dwMsgLen > 0 && isspace(msg[dwMsgLen-1]))
    msg[--dwMsgLen] = '\0';
  if (dwMsgLen > 0)
    return strm << msg;

#if P_DIRECTSHOW
  dwMsgLen = AMGetErrorText(result.m_result, msg, sizeof(msg));
  while (dwMsgLen > 0 && isspace(msg[dwMsgLen-1]))
    msg[--dwMsgLen] = '\0';
  if (dwMsgLen > 0)
    return strm << msg;
#endif

  return strm << "0x" << hex << result.m_result << dec;
}


#if PTRACING
bool PComResult::Succeeded(HRESULT result, const char * func, const char * file, int line, HRESULT nomsg1, HRESULT nomsg2)
{
  if (Succeeded(result))
    return true;

  // Don't look at "Facility" code when filterring errors.
  if (nomsg1 >= 0 && LOWORD(result) == LOWORD(nomsg1))
    return false;

  if (nomsg2 >= 0 && LOWORD(result) == LOWORD(nomsg2))
    return false;

  static const int Level = 2;
  if (PTrace::CanTrace(Level))
    PTrace::Begin(Level, file, line) << "Function \"" << func << "\" failed, "
           "error=0x" << hex << result << dec << " : " << *this << PTrace::End;
  return false;
}
#endif // PTRACING


std::ostream & operator<<(std::ostream & strm, const PComVariant & var)
{
  switch (var.vt) {
    case VT_BSTR :
      return strm << PString(var.bstrVal);
    case VT_I2 :
      return strm << var.iVal;
    case VT_I4 :
      return strm << var.lVal;
    case VT_I8 :
      return strm << var.llVal;
    case VT_R4 :
      return strm << var.fltVal;
    case VT_R8 :
      return strm << var.dblVal;
  }
  return strm;
}

#endif // P_WIN_COM


__inline static PNanoSeconds GetNanoFromFileTime(const FILETIME & ft)
{
  return reinterpret_cast<const ULARGE_INTEGER *>(&ft)->QuadPart*100;
}


struct PWindowsTimes
{
  FILETIME m_created;
  FILETIME m_exit;
  FILETIME m_kernel;
  FILETIME m_user;
  FILETIME m_idle;

  PWindowsTimes()
  {
    m_exit.dwHighDateTime = m_exit.dwLowDateTime = m_idle.dwHighDateTime = m_idle.dwLowDateTime = 0;
  }

  bool FromThread(HANDLE handle)  { return GetThreadTimes(handle, &m_created, &m_exit, &m_kernel, &m_user); }
  bool FromProcess(HANDLE handle) { return GetProcessTimes(handle, &m_created, &m_exit, &m_kernel, &m_user); }
  bool FromSystem()               { return GetSystemTimes(&m_idle, &m_kernel, &m_user); }

  void ToTimes(PThread::Times & times)
  {
    times.m_kernel = GetNanoFromFileTime(m_kernel);
    times.m_user = GetNanoFromFileTime(m_user);
    if (m_idle.dwHighDateTime != 0 || m_idle.dwLowDateTime != 0)
      times.m_real = GetNanoFromFileTime(m_kernel) + GetNanoFromFileTime(m_user) + GetNanoFromFileTime(m_idle);
    else {
      if (m_exit.dwHighDateTime == 0 && m_exit.dwLowDateTime == 0)
        GetSystemTimeAsFileTime(&m_exit);
      times.m_real = GetNanoFromFileTime(m_exit) - GetNanoFromFileTime(m_created);
    }
    times.m_sample.SetCurrentTime();
  }
};

bool PThread::GetTimes(Times & times)
{
  // Do not use any PTLib functions in here as they could to a PTRACE, and this deadlock
  times.m_name = GetThreadName();
  times.m_threadId = GetThreadId();
  times.m_uniqueId = GetUniqueIdentifier();

  PWindowsTimes wt;
  if (!wt.FromThread(m_nativeHandle))
    return false;

  wt.ToTimes(times);
  return true;
}


///////////////////////////////////////////////////////////////////////////////
// PProcess

PString PProcess::GetOSClass()
{
  return "Windows";
}


static bool RealGetVersion(POSVERSIONINFOW info)
{
  memset(info, 0, sizeof(*info));
  info->dwOSVersionInfoSize = sizeof(*info);

  HMODULE hNTDLL = ::GetModuleHandleW(L"ntdll.dll");
  if (hNTDLL != NULL) {
    typedef LONG (WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hNTDLL, "RtlGetVersion");
    if (fxPtr != NULL && fxPtr(info) == 0)
      return true;
  }

#ifdef P_VERSION_HELPERS
  info->dwPlatformId = VER_PLATFORM_WIN32_NT;
  if (IsWindows8Point1OrGreater()) {
    info->dwMajorVersion = 8;
    info->dwMinorVersion = 1;
    return true;
  }
  if (IsWindows8OrGreater()) {
    info->dwMajorVersion = 8;
    info->dwMinorVersion = 0;
    return true;
  }
  if (IsWindows7SP1OrGreater()) {
    info->dwMajorVersion = 7;
    info->dwMinorVersion = 1;
    return true;
  }
  if (IsWindows7OrGreater()) {
    info->dwMajorVersion = 7;
    info->dwMinorVersion = 0;
    return true;
  }
  if (IsWindowsVistaOrGreater()) {
    info->dwMajorVersion = 6;
    info->dwMinorVersion = 0;
    return true;
  }
  if (IsWindowsServer()) {
    info->dwMajorVersion = 5;
    info->dwMinorVersion = 2;
    return true;
  }
  if (IsWindowsXPOrGreater()) {
    info->dwMajorVersion = 5;
    info->dwMinorVersion = 1;
    return true;
  }
  return false;
#else
  return GetVersionEx(&info);
#endif
}


PString PProcess::GetOSName()
{
  OSVERSIONINFOW info;
  RealGetVersion(&info);

  switch (info.dwPlatformId) {
    case VER_PLATFORM_WIN32s :
      return "32s";

#ifdef VER_PLATFORM_WIN32_CE
    case VER_PLATFORM_WIN32_CE :
      return "CE";
#endif

    case VER_PLATFORM_WIN32_WINDOWS :
      if (info.dwMinorVersion < 10)
        return "95";
      if (info.dwMinorVersion < 90)
        return "98";
      return "ME";

    case VER_PLATFORM_WIN32_NT :
      switch (info.dwMajorVersion) {
        case 4 :
          return "NT";
        case 5:
          switch (info.dwMinorVersion) {
            case 0 :
              return "2000";
            case 1 :
              return "XP";
          }
          return "Server 2003";

        case 6 :
          switch (info.dwMinorVersion) {
            case 0 :
              return "Vista";
            case 1 :
              return "7";
            case 2 :
              return info.dwBuildNumber < 9200 ? "8" : "8.1";
          }
          break;

        default :
          if (info.dwMinorVersion == 0)
            return info.dwMajorVersion;
          else
            return psprintf("%u.%u", info.dwMajorVersion, info.dwMinorVersion);
      }
  }
  return "?";
}


PString PProcess::GetOSHardware()
{
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  switch (info.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_INTEL :
      switch (info.dwProcessorType) {
        case PROCESSOR_INTEL_386 :
          return "i386";
        case PROCESSOR_INTEL_486 :
          return "i486";
        case PROCESSOR_INTEL_PENTIUM :
          return psprintf("i586 (Model=%u Stepping=%u)", info.wProcessorRevision>>8, info.wProcessorRevision&0xff);
      }
      return "iX86";

    case PROCESSOR_ARCHITECTURE_MIPS :
      return "mips";

    case PROCESSOR_ARCHITECTURE_ALPHA :
      return "alpha";

    case PROCESSOR_ARCHITECTURE_PPC :
      return "ppc";

    case PROCESSOR_ARCHITECTURE_ARM :
      return "arm";

    case PROCESSOR_ARCHITECTURE_AMD64:
      return "x64";

    case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64 :
      return "x86/x64";
  }
  return "?";
}


PString PProcess::GetOSVersion()
{
  OSVERSIONINFOW info;
  RealGetVersion(&info);
  uint16_t wBuildNumber = (uint16_t)info.dwBuildNumber;
  return psprintf(wBuildNumber > 0 ? "v%u.%u.%u" : "v%u.%u",
                  info.dwMajorVersion, info.dwMinorVersion, wBuildNumber);
}


bool PProcess::IsOSVersion(unsigned major, unsigned minor, unsigned build)
{
#if P_VERSION_HELPERS
  OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, { 0 }, 0, 0 };
  DWORDLONG        const dwlConditionMask = VerSetConditionMask(
    VerSetConditionMask(
    VerSetConditionMask(
    0, VER_MAJORVERSION, VER_GREATER_EQUAL),
    VER_MINORVERSION, VER_GREATER_EQUAL),
    VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);

  osvi.dwMajorVersion = major;
  osvi.dwMinorVersion = minor;
  osvi.wServicePackMajor = (uint16_t)build;

  return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) != FALSE;
#else
  OSVERSIONINFO info;
  info.dwOSVersionInfoSize = sizeof(info);
  GetVersionEx(&info);

  if (info.dwMajorVersion < major)
    return false;
  if (info.dwMajorVersion > major)
    return true;

  if (info.dwMinorVersion < minor)
    return false;
  if (info.dwMinorVersion > minor)
    return true;

  return info.dwBuildNumber >= build;
#endif // P_VERSION_HELPERS
}


PDirectory PProcess::GetOSConfigDir()
{
  char dir[_MAX_PATH];

  PAssertOS(GetSystemDirectory(dir, sizeof(dir)) != 0);
  PDirectory sysdir = dir;
  return sysdir;  //+ "drivers\\etc";
}

PString PProcess::GetUserName() const
{
  char username[100];
  DWORD size = sizeof(username);
  return ::GetUserName(username, &size) ? PString(username, size-1) : PString::Empty();
}


bool PProcess::SetUserName(const PString & username, bool)
{
  if (username.IsEmpty())
    return false;

  if (username == GetUserName())
    return true;

  PAssertAlways(PUnimplementedFunction);
  return false;
}


PDirectory PProcess::GetHomeDirectory() const
{
  auto dir = PConfig::GetEnv("HOME");
  if (!dir.empty())
    return dir;

  TCHAR szPath[MAX_PATH];
  if (SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, szPath) == S_OK)
    return szPath;

  dir = PConfig::GetEnv("USERPROFILE");
  if (!dir.empty())
    return dir;

  return ".";
}


PString PProcess::GetGroupName() const
{
  return "Users";
}


bool PProcess::SetGroupName(const PString & groupname, bool)
{
  if (groupname.IsEmpty())
    return false;

  PAssertAlways(PUnimplementedFunction);
  return false;
}


PProcessIdentifier PProcess::GetCurrentProcessID()
{
  return ::GetCurrentProcessId();
}


void PProcess::GetMemoryUsage(MemoryUsage & usage)
{
  usage = MemoryUsage();

  PROCESS_MEMORY_COUNTERS info;
  info.cb = sizeof(info);
  if (GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info))) {
    usage.m_virtual = info.PeakWorkingSetSize;
    usage.m_resident = info.WorkingSetSize;
    usage.m_blocks = info.WorkingSetSize;
  }

  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  if (GlobalMemoryStatusEx(&status)) {
    usage.m_total = status.ullTotalPhys;
    usage.m_available = status.ullAvailPhys;
  }
}


bool PProcess::GetProcessTimes(Times & times) const
{
  times.m_name = GetName();

  PWindowsTimes wt;
  if (!wt.FromProcess(GetCurrentProcess()))
    return false;

  wt.ToTimes(times);
  return true;
}


bool PProcess::GetSystemTimes(Times & times)
{
  times.m_name = "SYSTEM";

  PWindowsTimes wt;
  if (!wt.FromSystem())
    return false;

  wt.ToTimes(times);
  return true;
}


unsigned PThread::GetNumProcessors()
{
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwNumberOfProcessors;
}


bool PProcess::IsGUIProcess() const
{
  USEROBJECTFLAGS uof = { FALSE, FALSE, 0 };     
  if (GetUserObjectInformation(GetProcessWindowStation(), UOI_FLAGS, &uof, sizeof(USEROBJECTFLAGS), NULL) && ((uof.dwFlags & WSF_VISIBLE) == 0))
    return false;

  return GetConsoleWindow() == NULL;
}


///////////////////////////////////////////////////////////////////////////////

void PProcess::HostSystemURLHandlerInfo::SetIcon(const PString & _icon)
{
  PString icon(_icon);
  PFilePath exe(PProcess::Current().GetFile());
  icon.Replace("%exe",  exe, true);
  icon.Replace("%base", exe.GetFileName(), true);
  iconFileName = icon;
}

PString PProcess::HostSystemURLHandlerInfo::GetIcon() const 
{
  return iconFileName;
}

void PProcess::HostSystemURLHandlerInfo::SetCommand(const PString & key, const PString & _cmd)
{
  PString cmd(_cmd);

  // do substitutions
  PFilePath exe(PProcess::Current().GetFile());
  cmd.Replace("%exe", "\"" + exe + "\"", true);
  cmd.Replace("%1",   "\"%1\"", true);

  // save command
  cmds.SetAt(key, cmd);
}

PString PProcess::HostSystemURLHandlerInfo::GetCommand(const PString & key) const
{
  return cmds(key);
}

bool PProcess::HostSystemURLHandlerInfo::GetFromSystem()
{
  if (type.IsEmpty())
    return false;

  // get icon file
  {
    RegistryKey key("HKEY_CLASSES_ROOT\\" + type + "\\DefaultIcon", RegistryKey::ReadOnly);
    key.QueryValue("", iconFileName);
  }

  // enumerate the commands
  {
    PString keyRoot("HKEY_CLASSES_ROOT\\" + type + "\\");
    RegistryKey key(keyRoot + "shell", RegistryKey::ReadOnly);
    PString str;
    for (PINDEX idx = 0; key.EnumKey(idx, str); ++idx) {
      RegistryKey cmd(keyRoot + "shell\\" + str + "\\command", RegistryKey::ReadOnly);
      PString value;
      if (cmd.QueryValue("", value)) 
        cmds.SetAt(str, value);
    }
  }

  return true;
}

bool PProcess::HostSystemURLHandlerInfo::CheckIfRegistered()
{
  // if no type information in system, definitely not registered
  HostSystemURLHandlerInfo currentInfo(type);
  if (!currentInfo.GetFromSystem()) 
    return false;

  // check icon file
  if (!iconFileName.IsEmpty() && !(iconFileName *= currentInfo.GetIcon()))
    return false;

  // check all of the commands
  return (currentInfo.cmds.GetSize() != 0) && (currentInfo.cmds == cmds);
}

bool PProcess::HostSystemURLHandlerInfo::Register()
{
  if (type.IsEmpty())
    return false;

  // delete any existing icon name
  {
    RegistryKey key("HKEY_CLASSES_ROOT\\" + type, RegistryKey::ReadOnly);
    key.DeleteKey("DefaultIcon");
  }

  // set icon file
  if (!iconFileName.IsEmpty()) {
    RegistryKey key("HKEY_CLASSES_ROOT\\" + type + "\\DefaultIcon", RegistryKey::Create);
    key.SetValue("", iconFileName);
  }

  // delete existing commands
  PString keyRoot("HKEY_CLASSES_ROOT\\" + type);
  {
    RegistryKey keyShell(keyRoot + "\\shell", RegistryKey::ReadOnly);
    PString str;
    for (PINDEX idx = 0; keyShell.EnumKey(idx, str); ++idx) {
      {
        RegistryKey key(keyRoot + "\\shell\\" + str, RegistryKey::ReadOnly);
        key.DeleteKey("command");
      }
      {
        RegistryKey key(keyRoot + "\\shell", RegistryKey::ReadOnly);
        key.DeleteKey(str);
      }
    }
  }

  // create new commands
  {
    RegistryKey key3(keyRoot, RegistryKey::Create);
    key3.SetValue("", type & "protocol");
    key3.SetValue("URL Protocol", "");

    RegistryKey key2(keyRoot + "\\shell",  RegistryKey::Create);

    for (PStringToString::iterator it = cmds.begin(); it != cmds.end(); ++it) {
      RegistryKey key1(keyRoot + "\\shell\\" + it->first,              RegistryKey::Create);
      RegistryKey key(keyRoot + "\\shell\\" + it->first + "\\command", RegistryKey::Create);
      key.SetValue("", it->second);
    }
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
// PSemaphore

void PSemaphore::Reset(unsigned initial, unsigned maximum)
{
  m_maximum = std::min(maximum, (unsigned)INT_MAX);
  m_initial = std::min(initial, m_maximum);

  m_handle.Close();
  m_handle = CreateSemaphore(NULL, m_initial, m_maximum, m_name.IsEmpty() ? NULL : m_name.GetPointer());
  PAssertOS(m_handle.IsValid());
}


PSemaphore::~PSemaphore()
{
}


void PSemaphore::Wait()
{
  m_handle.Wait(INFINITE);
}


bool PSemaphore::Wait(const PTimeInterval & timeout)
{
  return m_handle.Wait(timeout.GetInterval());
}


void PSemaphore::Signal()
{
  if (PAssertOS(ReleaseSemaphore(m_handle, 1, NULL) || ::GetLastError() == ERROR_INVALID_HANDLE))
    SetLastError(ERROR_SUCCESS);
}


void PWin32Handle::Close()
{
  if (IsValid()) {
    HANDLE h = m_handle; // This is a little more thread safe
    m_handle = NULL;
    PAssertOS(CloseHandle(h));
  }
}


HANDLE PWin32Handle::Detach()
{
  HANDLE h = m_handle;
  m_handle = NULL;
  return h;
}


HANDLE * PWin32Handle::GetPointer()
{
  Close();
  return &m_handle;
}


PWin32Handle & PWin32Handle::operator=(HANDLE h)
{
  Close();
  m_handle = h;
  return *this;
}


bool PWin32Handle::Wait(uint32_t timeout) const
{
  int retry = 0;
  while (retry < 10) {
    uint32_t tick = ::GetTickCount();
    switch (::WaitForSingleObjectEx(m_handle, timeout, TRUE)) {
      case WAIT_OBJECT_0 :
        return true;

      case WAIT_TIMEOUT :
        return false;

      case WAIT_IO_COMPLETION :
        timeout -= (::GetTickCount() - tick);
        break;

      default :
        if (::GetLastError() != ERROR_INVALID_HANDLE) {
          PAssertAlways(POperatingSystemError);
          return true;
        }
        ++retry;
    }
  }

  return false;
}


bool PWin32Handle::Duplicate(HANDLE h, uint32_t flags, uint32_t access)
{
  Close();
  return DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &m_handle, access, 0, flags);
}


///////////////////////////////////////////////////////////////////////////////
// PDynaLink

PDynaLink::PDynaLink()
  : m_hDLL(nullptr)
{
}


PDynaLink::PDynaLink(const PString & name)
  : m_hDLL(nullptr)
{
  Open(name);
}


PDynaLink::PDynaLink(PDynaLink&& other)
  : m_hDLL(std::exchange(other.m_hDLL, nullptr))
{
}


PDynaLink::~PDynaLink()
{
  Close();
}


PString PDynaLink::GetExtension()
{
  return ".DLL";
}


bool PDynaLink::Open(const PString & names)
{
  m_lastError.MakeEmpty();

  PStringArray filenames = names.Lines();
  for (PINDEX i = 0; i < filenames.GetSize(); ++i) {
    PVarString filename = filenames[i];
    m_hDLL = LoadLibraryEx(filename, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (m_hDLL != NULL)
      return true;
  }

  m_lastError.sprintf("0x%x", ::GetLastError());
  PTRACE(1, "DLL\tError loading DLL (" << setfill(',') << filenames << "), error=" << m_lastError);
  return false;
}


void PDynaLink::Close()
{
  if (m_hDLL != NULL) {
    FreeLibrary(m_hDLL);
    m_hDLL = NULL;
  }
}


bool PDynaLink::IsLoaded() const
{
  return m_hDLL != NULL;
}


PString PDynaLink::GetName(bool full) const
{
  PFilePathString str;
  if (m_hDLL != NULL) {
    TCHAR path[_MAX_PATH];
    GetModuleFileName(m_hDLL, path, _MAX_PATH-1);
    str = PString(path);
    if (!full) {
      str.Delete(0, str.FindLast('\\')+1);
      PINDEX pos = str.Find(".DLL");
      if (pos != P_MAX_INDEX)
        str.Delete(pos, P_MAX_INDEX);
    }
  }
  return str;
}


bool PDynaLink::GetFunction(PINDEX index, Function & func, bool compulsory)
{
  m_lastError.MakeEmpty();
  func = NULL;

  if (m_hDLL == NULL)
    return false;

  LPCSTR ordinal = (LPCSTR)(DWORD_PTR)LOWORD(index);
  func = (Function)GetProcAddress(m_hDLL, ordinal);
  if (func != NULL)
    return true;

  if (compulsory)
    Close();

  m_lastError.sprintf("0x%x", ::GetLastError());
  return false;
}


bool PDynaLink::GetFunction(const PString & name, Function & func, bool compulsory)
{
  m_lastError.MakeEmpty();
  func = NULL;

  if (m_hDLL == NULL)
    return false;

  PVarString funcname = name;
  func = (Function)GetProcAddress(m_hDLL, funcname);
  if (func != NULL)
    return true;

  if (compulsory)
    Close();

  m_lastError.sprintf("0x%x", ::GetLastError());
  return false;
}


///////////////////////////////////////////////////////////////////////////////
// PDebugStream

PDebugStream::PDebugStream()
  : ostream(&buffer)
{
}


PDebugStream::Buffer::Buffer()
{
  setg(buffer, buffer, &buffer[sizeof(buffer)-2]);
  setp(buffer, &buffer[sizeof(buffer)-2]);
}


int PDebugStream::Buffer::overflow(int c)
{
  size_t bufSize = pptr() - pbase();

  if (c != EOF) {
    *pptr() = (char)c;
    bufSize++;
  }

  if (bufSize != 0) {
    char * p = pbase();
    setp(p, epptr());
    p[bufSize] = '\0';

    // Note we do NOT use PWideString here as it could cause infinitely
    // recursive calls if there is an error!
    PINDEX length = strlen(p);
    std::vector<wchar_t> unicode(length+1);
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, p, length, unicode.data(), length+1);
    OutputDebugStringW(unicode.data());
  }

  return 0;
}


int PDebugStream::Buffer::underflow()
{
  return EOF;
}


int PDebugStream::Buffer::sync()
{
  return overflow(EOF);
}

// End Of File ///////////////////////////////////////////////////////////////
