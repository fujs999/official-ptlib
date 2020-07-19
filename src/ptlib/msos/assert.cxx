/*
 * assert.cxx
 *
 * Function to implement assert clauses.
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

#define P_DISABLE_FACTORY_INSTANCES

#include <ptlib.h>
#include <ptlib/svcproc.h>


///////////////////////////////////////////////////////////////////////////////
// PProcess

static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM thisProcess)
{
  char wndClassName[100];
  GetClassName(hWnd, wndClassName, sizeof(wndClassName));
  if (strcmp(wndClassName, "ConsoleWindowClass") != 0)
    return true;

  DWORD wndProcess;
  GetWindowThreadProcessId(hWnd, &wndProcess);
  if (wndProcess != (uint32_t)thisProcess)
    return true;

  PTRACE(2, "PTLib", "Awaiting key press on exit.");
  cerr << "\nPress a key to exit . . .";
  cerr.flush();

  HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
  SetConsoleMode(in, ENABLE_PROCESSED_INPUT);
  FlushConsoleInputBuffer(in);
  char dummy;
  DWORD readBytes;
  ReadConsole(in, &dummy, 1, &readBytes, NULL);
  return false;
}


void PProcess::WaitOnExitConsoleWindow()
{
  if (m_waitOnExitConsoleWindow && !m_library)
    EnumWindows(EnumWindowsProc, GetCurrentProcessId());
}


///////////////////////////////////////////////////////////////////////

#pragma warning(disable:4091)
#include <DbgHelp.h>
#include <Psapi.h>
#pragma warning(default:4091)

#if PTRACING
  #define InternalMaxStackWalk PTrace::MaxStackWalk
#else
  #define InternalMaxStackWalk 20
#endif // PTRACING

class PDebugDLL : public PDynaLink
{
  PCLASSINFO(PDebugDLL, PDynaLink)
  public:
    BOOL (__stdcall *m_SymInitialize)(
      __in HANDLE hProcess,
      __in_opt PCSTR UserSearchPath,
      __in BOOL fInvadeProcess
    );
    BOOL (__stdcall *m_SymCleanup)(
      __in HANDLE hProcess
    );
    uint32_t (__stdcall *m_SymGetOptions)(
      VOID
    );
    uint32_t (__stdcall *m_SymSetOptions)(
      __in uint32_t   SymOptions
    );
    BOOL (__stdcall *m_StackWalk64)(
      __in uint32_t MachineType,
      __in HANDLE hProcess,
      __in HANDLE hThread,
      __inout LPSTACKFRAME64 StackFrame,
      __inout PVOID ContextRecord,
      __in_opt PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
      __in_opt PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
      __in_opt PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
      __in_opt PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
    );
    BOOL (__stdcall *m_SymGetSymFromAddr64)(
      __in HANDLE hProcess,
      __in DWORD64 qwAddr,
      __out_opt PDWORD64 pdwDisplacement,
      __inout PIMAGEHLP_SYMBOL64  Symbol
    );
    BOOL (__stdcall *m_SymGetLineFromAddr64)(
      _In_  HANDLE           hProcess,
      _In_  DWORD64          dwAddr,
      _Out_ PDWORD           pdwDisplacement,
      _Out_ PIMAGEHLP_LINE64 Line
    );

    PFUNCTION_TABLE_ACCESS_ROUTINE64 m_SymFunctionTableAccess64;
    PGET_MODULE_BASE_ROUTINE64       m_SymGetModuleBase64;

    HANDLE m_hProcess;


    PDebugDLL()
      : PDynaLink("DBGHELP.DLL")
      , m_hProcess(GetCurrentProcess())
    {
    }

    ~PDebugDLL()
    {
      if (IsLoaded())
        m_SymCleanup(GetCurrentProcess());
    }

    void WalkStack(ostream & strm, PThreadIdentifier id, unsigned framesToSkip, PCONTEXT pThreadContext)
    {
      if (!GetFunction("SymInitialize", (Function &)m_SymInitialize) ||
          !GetFunction("SymCleanup", (Function &)m_SymCleanup) ||
          !GetFunction("SymGetOptions", (Function &)m_SymGetOptions) ||
          !GetFunction("SymSetOptions", (Function &)m_SymSetOptions) ||
          !GetFunction("StackWalk64", (Function &)m_StackWalk64) ||
          !GetFunction("SymGetSymFromAddr64", (Function &)m_SymGetSymFromAddr64) ||
          !GetFunction("SymFunctionTableAccess64", (Function &)m_SymFunctionTableAccess64) ||
          !GetFunction("SymGetModuleBase64", (Function &)m_SymGetModuleBase64)) {
        strm << "\n    Invalid stack walk DLL: " << GetName() << " not all functions present.";
        return;
      }

      if (!GetFunction("SymGetLineFromAddr64", (Function &)m_SymGetLineFromAddr64))
        m_SymGetLineFromAddr64 = NULL;

      // Get the directory the .exe file is in
      char exename[_MAX_PATH];
      if (GetModuleFileNameEx(m_hProcess, NULL, exename, sizeof(exename)) == 0) {
        uint32_t err = ::GetLastError();
        strm << "\n    GetModuleFileNameEx failed, error=" << err;
        return;
      }

      ostringstream path;

      // Always look in same place as the .exe file for .pdb file
      char * ptr = strrchr(exename, '\\');
      if (ptr == NULL)
        path << exename;
      else {
        *ptr = '\0';
        path << exename;
        *ptr = '\\';
      }

      /* Add in the environment variables as per default, but do not add in
          current directory (as default does), as if that is C:\, it searches
          the entire disk. Slooooow. */
      auto dir = PConfig::GetEnv("_NT_SYMBOL_PATH");
      if (!dir.empty())
        path << ';' << dir;
      dir = PConfig::GetEnv("_NT_ALTERNATE_SYMBOL_PATH");
      if (!dir.empty())
        path << ';' << dir;

      // Initialise the symbols with path for PDB files.
      if (!m_SymInitialize(m_hProcess, path.str().c_str(), TRUE)) {
        uint32_t err = ::GetLastError();
        strm << "\n    SymInitialize failed, error=" << err;
        return;
      }

      // See if PDB file exists
      std::string pdbname = exename;
      auto dot = pdbname.rfind('.');
      if (dot == string::npos)
        pdbname += ".pdb";
      else {
        pdbname.erase(dot, 4);
        pdbname.insert(dot, ".pdb");
      }

      if (_access(pdbname.c_str(), 4) != 0)
        strm << "\n    Stack walk could not find symbols file \"" << pdbname << "\",  path=\"" << path.str() << '"';
      else
        strm << "\n    Stack walk symbols initialised, path=\"" << path.str() << '"';

      m_SymSetOptions(m_SymGetOptions()|SYMOPT_LOAD_LINES|SYMOPT_FAIL_CRITICAL_ERRORS|SYMOPT_NO_PROMPTS);

      // The thread information.
      HANDLE hThread;
      if (id == PNullThreadIdentifier || id == PThread::GetCurrentThreadId())
        hThread = GetCurrentThread();
      else {
        hThread = OpenThread(THREAD_QUERY_INFORMATION|THREAD_GET_CONTEXT|THREAD_SUSPEND_RESUME, FALSE, *reinterpret_cast<DWORD *>(&id));
        if (hThread == NULL) {
          uint32_t err = ::GetLastError();
          strm << "\n    No thread: id=" << id << " (0x" << std::hex << id << std::dec << "), error=" << err;
          return;
        }
      }

      int resumeCount = -1;
      CONTEXT threadContext;

      if (!pThreadContext) {
        pThreadContext = &threadContext;
        memset(pThreadContext, 0, sizeof(threadContext));
        pThreadContext->ContextFlags = CONTEXT_FULL;

        if (id == PNullThreadIdentifier || id == PThread::GetCurrentThreadId()) {
          RtlCaptureContext(pThreadContext);
          #if P_64BIT
          framesToSkip += 2;
          #else
          ++framesToSkip;
          #endif
        }
        else {
          resumeCount = SuspendThread(hThread);
          if (!GetThreadContext(hThread, pThreadContext)) {
            if (resumeCount >= 0)
              ResumeThread(hThread);
            uint32_t err = ::GetLastError();
            strm << "\n    No context for thread: id=" << id << " (0x" << std::hex << id << std::dec << "), error=" << err;
            return;
          }
        }
      }

      STACKFRAME64 frame;
      memset(&frame, 0, sizeof(frame));
      frame.AddrPC.Mode = AddrModeFlat;
      frame.AddrStack.Mode = AddrModeFlat;
      frame.AddrFrame.Mode = AddrModeFlat;
      #ifdef _M_IX86
        // normally, call ImageNtHeader() and use machine info from PE header
      uint32_t imageType = IMAGE_FILE_MACHINE_I386;
      frame.AddrPC.Offset = pThreadContext->Eip;
      frame.AddrFrame.Offset = frame.AddrStack.Offset = pThreadContext->Esp;
      #elif _M_X64
      uint32_t imageType = IMAGE_FILE_MACHINE_AMD64;
      frame.AddrPC.Offset = pThreadContext->Rip;
      frame.AddrFrame.Offset = frame.AddrStack.Offset = pThreadContext->Rsp;
      #elif _M_IA64
      uint32_t imageType = IMAGE_FILE_MACHINE_IA64;
      frame.AddrPC.Offset = pThreadContext->StIIP;
      frame.AddrFrame.Offset = pThreadContext->IntSp;
      frame.AddrBStore.Offset = pThreadContext->RsBSP;
      frame.AddrBStore.Mode = AddrModeFlat;
      frame.AddrStack.Offset = pThreadContext->IntSp;
      #else
      #error "Platform not supported!"
      #endif

      unsigned frameCount = 0;
      while (frameCount++ < InternalMaxStackWalk) {
        if (!m_StackWalk64(imageType,
                           m_hProcess,
                           hThread,
                           &frame,
                           pThreadContext,
                           NULL,
                           m_SymFunctionTableAccess64,
                           m_SymGetModuleBase64,
                           NULL)) {
          uint32_t err = ::GetLastError();
          strm << "\n    StackWalk64 failed: error=" << err;
          break;
        }

        if (frame.AddrPC.Offset == frame.AddrReturn.Offset) {
          strm << "\n    StackWalk64 returned recursive stack! PC=0x" << hex << frame.AddrPC.Offset << dec;
          break;
        }

        if (frameCount <= framesToSkip || frame.AddrPC.Offset == 0)
          continue;

        strm << "\n    " << hex << setfill('0');

        char buffer[sizeof(IMAGEHLP_SYMBOL64) + 200];
        PIMAGEHLP_SYMBOL64 symbol = (PIMAGEHLP_SYMBOL64)buffer;
        symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
        symbol->MaxNameLength = sizeof(buffer) - sizeof(IMAGEHLP_SYMBOL64);
        DWORD64 displacement = 0;
        uint32_t error = 0;
        if (m_SymGetSymFromAddr64(m_hProcess, frame.AddrReturn.Offset, &displacement, symbol))
          strm << symbol->Name;
        else {
          error = ::GetLastError();
          strm << setw(8) << frame.AddrReturn.Offset;
        }

        if (frame.Params) {
          strm << '(';
          for (PINDEX i = 0; i < 4; i++) {
            if (i > 0)
              strm << ", ";
            if (frame.Params[i] != 0)
              strm << "0x";
            strm << frame.Params[i];
          }
          strm << setfill(' ') << ')';
        }

        if (displacement != 0)
          strm << " + 0x" << displacement;

        strm << dec << setfill(' ');

        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(line);
        DWORD dwDisplacement;
        if (m_SymGetLineFromAddr64 != NULL && m_SymGetLineFromAddr64(m_hProcess, frame.AddrReturn.Offset, &dwDisplacement, &line))
          strm << ' ' << line.FileName << '(' << line.LineNumber << ')';

        if (error != 0)
          strm << " - symbol lookup error=" << error;

        if (frame.AddrReturn.Offset == 0)
          break;
      }

      if (resumeCount >= 0)
        ResumeThread(hThread);
    }
};


void PPlatformWalkStack(ostream & strm, PThreadIdentifier id, PUniqueThreadIdentifier, unsigned framesToSkip, bool)
{
  PDebugDLL debughelp;
  debughelp.WalkStack(strm, id, framesToSkip, nullptr);
}


static const char ActionMessage[] = "<A>bort, <B>reak, <I>gnore";

static bool AssertAction(int c)
{
  switch (c) {
    case 'A':
    case 'a':
      cerr << "Aborted" << endl;
    case IDABORT :
      PTRACE(0, "Assert abort, application exiting immediately");
      abort(); // Never returns

    case 'B':
    case 'b':
      cerr << "Break" << endl;
    case IDRETRY :
      PBreakToDebugger();
      return false; // Then ignore it

    case 'I':
    case 'i':
      cerr << "Ignored" << endl;

    case IDIGNORE :
    case EOF:
      return false;
  }

  return true;
}


void PPlatformAssertFunc(const PDebugLocation & PTRACE_PARAM(location), const char * msg, char defaultAction)
{
#if PTRACING
  if (PTrace::GetStream() != &PError) \
    PTrace::Begin(0, location.m_file, location.m_line, NULL, "PAssert") << msg << PTrace::End;
#endif

  if (defaultAction != '\0' && !AssertAction(defaultAction))
      return;

  else if (PProcess::Current().IsGUIProcess()) {
    PVarString boxMsg = msg;
    PVarString boxTitle = PProcess::Current().GetName();
    AssertAction(MessageBox(NULL, boxMsg, boxTitle, MB_ABORTRETRYIGNORE|MB_ICONHAND|MB_TASKMODAL));
  }
  else {
    do {
      cerr << msg << '\n' << ActionMessage << "? " << flush;
    } while (AssertAction(cin.get()));
  }
}


// End Of File ///////////////////////////////////////////////////////////////
