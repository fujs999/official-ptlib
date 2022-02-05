/*
 * jscript.cxx
 *
 * Interface library for JavaScript interpreter
 *
 * Portable Tools Library
 *
 * Copyright (C) 2012 by Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): Craig Southeren
 */

#ifdef __GNUC__
#pragma implementation "jscript.h"
#endif

#include <ptlib.h>

#if P_V8

#include <ptlib/pprocess.h>
#include <ptlib/notifier_ext.h>


/*
Requires Google's V8 version 6 or later

For Unix variants, follow build instructions for v8 or just use distro.
Note most distro's use a very early version of V8 (3.14 typical) so if
you need advanced features you need to build it.

For Windows the following commands was used to build V8:

Install Visual Studio 2015, note you should do a full isntallation, in
particular the "Windows SDK" component.

Install Windows 10 SDK (yes, in addition to the above) from
https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk

Download https://storage.googleapis.com/chrome-infra/depot_tools.zip and
unpack to somehere, e.g. C:\tools\depot_tools

Set up the environment in CMD:
set DEPOT_TOOLS_WIN_TOOLCHAIN=0
set vs2019_install=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community
PATH=C:\tools\depot_tools;%PATH%

or for PowerShell:
$env:DEPOT_TOOLS_WIN_TOOLCHAIN=0
$env:vs2019_install="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community"
$env:PATH="C:\tools\depot_tools;$env:PATH"

Get the source to somewhere e.g. if PTLib is in C:\Work\ptlib, C:\Work\external\v8:
mkdir <v8-dir>   ;
cd <v8-dir>
fetch v8
cd v8
gclient sync

Apply the following patch:
diff --git a/build/config/win/BUILD.gn b/config/win/BUILD.gn
index 1e76a54cc..cc92f23bf 100644
--- a/build/config/win/BUILD.gn
+++ b/build/config/win/BUILD.gn
@@ -490,7 +490,7 @@ config("default_crt") {
       configs = [ ":dynamic_crt" ]
     } else {
       # Desktop Windows: static CRT.
-      configs = [ ":static_crt" ]
+      configs = [ ":dynamic_crt" ]
     }
   }
 }

Set up the builds:
python tools/dev/v8gen.py x64.release
python tools/dev/v8gen.py x64.debug
python tools/dev/v8gen.py ia32.release
python tools/dev/v8gen.py ia32.debug

In each of the builds chosen, go to it's subdirectory(e.g. out.gn/x64-release),
and add to the existing args.gn file:
v8_use_external_startup_data = false
is_component_build = false
v8_monolithic = true
is_clang = false

in the debug directories also include:
enable_iterator_debugging = true

Then build each directory as a separate command e.g.
ninja -C out.gn/x64.release v8_monolith
ninja -C out.gn/x64.debug v8_monolith

Finally, reconfigure PTLib and rebuild.
*/

#ifdef _MSC_VER
#pragma warning(disable:4100 4127)
#endif

#include <ptclib/jscript.h>

#include <iomanip>

#define V8_DEPRECATION_WARNINGS 1
#define V8_IMMINENT_DEPRECATION_WARNINGS 1
#define V8_COMPRESS_POINTERS 1
#pragma warning(push)
#pragma warning(disable:4996)
#include <v8.h>
#pragma warning(pop)

#ifndef V8_MAJOR_VERSION
  // This is the version distributed with many Linux distros
  #define V8_MAJOR_VERSION 3
  #define V8_MINOR_VERSION 14
#endif


#if V8_MAJOR_VERSION >= 6
  #include "libplatform/libplatform.h"
#endif


#ifdef _MSC_VER
  #pragma comment(lib, "winmm.lib")
  #pragma comment(lib, "dbghelp.lib")
  #pragma comment(lib, "shlwapi.lib")
  #pragma comment(lib, P_V8_LIB)
#endif


#define PTraceModule() "JavaScript"


static PConstString const JavaName("JavaScript");
PFACTORY_CREATE(PFactory<PScriptLanguage>, PJavaScript, JavaName, false);


#if PTRACING && V8_MAJOR_VERSION > 3
static void LogEventCallback(const char * PTRACE_PARAM(name), int PTRACE_PARAM(startEnd))
{
  PTRACE(5, "V8-Log", name << ' '  << (startEnd == 0 ? "started" : "ended"));
}

static void TraceFunction(const v8::FunctionCallbackInfo<v8::Value>& args)
{
  if (args.Length() < 2)
    return;

  if (!args[0]->IsUint32())
    return;

  unsigned level = args[0]->Uint32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(UINT_MAX);
  if (!PTrace::CanTrace(level))
    return;

  ostream & trace = PTRACE_BEGIN(level);
  for (int i = 1; i < args.Length(); i++) {
    v8::HandleScope handle_scope(args.GetIsolate());
    if (i > 0)
      trace << ' ';
    v8::String::Utf8Value str(args.GetIsolate(), args[i]);
    if (*str)
      trace << *str;
  }

  trace << PTrace::End;
}
#endif // PTRACING && V8_MAJOR_VERSION > 3


struct PJavaScript::Private : PObject
{
  PCLASSINFO(PJavaScript::Private, PObject);

  PJavaScript               & m_owner;
  v8::Isolate               * m_isolate;
  v8::Persistent<v8::Context> m_context;

#if V8_MAJOR_VERSION > 3
  v8::Isolate::CreateParams   m_isolateParams;

  struct HandleScope : v8::HandleScope
  {
    HandleScope(Private * prvt) : v8::HandleScope(prvt->m_isolate) { }
  };

  struct TryCatch : v8::TryCatch
  {
    TryCatch(Private * prvt) : v8::TryCatch(prvt->m_isolate) { }
  };

  v8::Local<v8::String> NewString(const char * str) const
  {
    v8::Local<v8::String> obj;
    if (!v8::String::NewFromUtf8(m_isolate, str, v8::NewStringType::kNormal).ToLocal(&obj))
      obj = v8::String::Empty(m_isolate);
    return obj;
  }

  template <class Type>                 inline v8::Local<Type> NewObject()            const { return Type::New(m_isolate); }
  template <class Type, typename Param> inline v8::Local<Type> NewObject(Param param) const { return Type::New(m_isolate, param); }
  inline v8::Local<v8::Context> GetContext() const { return v8::Local<v8::Context>::New(m_isolate, m_context); }
  template <typename T> inline PString ToPString(const T & value) { return *v8::String::Utf8Value(m_isolate, value);  }
#else
  struct HandleScope : v8::HandleScope
  {
    HandleScope(Private *) { }
  };
  struct TryCatch : v8::TryCatch
  {
    TryCatch(Private *) { }
  };

  inline v8::Local<v8::String> NewString(const char * str) const { return v8::String::New(str); }
  template <class Type>                 inline v8::Local<Type> NewObject()            const { return Type::New(); }
  template <class Type, typename Param> inline v8::Local<Type> NewObject(Param param) const { return Type::New(param); }
  inline v8::Local<v8::Context> GetContext() const { return v8::Local<v8::Context>::New(m_context); }
  template <typename T> inline PString ToPString(const T & value) { return *v8::String::Utf8Value(value);  }
#endif

  struct Intialisation
  {
#if V8_MAJOR_VERSION > 3
    unique_ptr<v8::Platform> m_platform;
#endif
    bool m_initialised;

    Intialisation()
      : m_initialised(false)
    {
#if V8_MAJOR_VERSION > 3
      PDirectory exeDir = PProcess::Current().GetFile().GetDirectory();
#if V8_MAJOR_VERSION < 6
      if (!v8::V8::InitializeICU(exeDir)) {
#else
      if (!v8::V8::InitializeICUDefaultLocation(exeDir)) {
#endif
        PTRACE(2, PTraceModule(), "v8::V8::InitializeICUDefaultLocation() failed, continuing.");
      }

      // Start it up!
      m_platform = v8::platform::NewDefaultPlatform();
      v8::V8::InitializePlatform(m_platform.get());
#endif // V8_MAJOR_VERSION > 3

      if (!v8::V8::Initialize()) {
        PTRACE(1, PTraceModule(), "v8::V8::Initialize() failed, not loaded.");
        return;
      }

      m_initialised = true;
      PTRACE(4, PTraceModule(), "V8 initialisation successful.");
      }

    ~Intialisation()
    {
      PTRACE(4, PTraceModule(), "V8 shutdown.");
      v8::V8::Dispose();
#if V8_MAJOR_VERSION > 8
      v8::V8::DisposePlatform();
#elif V8_MAJOR_VERSION > 3
      v8::V8::ShutdownPlatform();
#endif
    }
   };


#if V8_MAJOR_VERSION > 3
#if V8_MAJOR_VERSION < 6
  class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator
  {
  public:
    virtual void* Allocate(size_t length) { return calloc(length, 1); }
    virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
    virtual void Free(void* data, size_t length) { free(data); }
  };
  __inline v8::ArrayBuffer::Allocator * NewDefaultAllocator() { return new ArrayBufferAllocator; }
#else
  __inline v8::ArrayBuffer::Allocator * NewDefaultAllocator() { return v8::ArrayBuffer::Allocator::NewDefaultAllocator(); }
#endif // V8_MAJOR_VERSION < 6
#endif // V8_MAJOR_VERSION > 3


public:
  Private(PJavaScript & owner)
    : m_owner(owner)
    , m_isolate(NULL)
  {
    if (!PSafeSingleton<Intialisation>()->m_initialised) {
      PTRACE(1, "Could not intitalise V8.");
      return;
    }

    PTRACE_CONTEXT_ID_FROM(owner);

#if V8_MAJOR_VERSION > 3
    m_isolateParams.array_buffer_allocator = NewDefaultAllocator();
    m_isolate = v8::Isolate::New(m_isolateParams);
#else
    m_isolate = v8::Isolate::New();
#endif
    if (m_isolate == NULL) {
      PTRACE(1, "Could not create context.");
      return;
    }

    v8::Isolate::Scope isolateScope(m_isolate);
    HandleScope handleScope(this);

#if V8_MAJOR_VERSION > 3
#if PTRACING
    m_isolate->SetEventLogger(LogEventCallback);

    // Bind the global 'PTRACE' function to the PTLib trace callback.
    v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(m_isolate);
    global->Set(NewString("PTRACE"), v8::FunctionTemplate::New(m_isolate, TraceFunction));

    m_context.Reset(m_isolate, v8::Context::New(m_isolate, NULL, global));
#else
    m_context.Reset(m_isolate, v8::Context::New(m_isolate));
#endif
#else
    m_context = v8::Persistent<v8::Context>::New(v8::Context::New());
#endif
    PTRACE(4, "Created context " << this);
  }


  ~Private()
  {
    PTRACE(4, "Destroying context " << this);

    if (m_isolate != NULL)
      m_isolate->Dispose();

#if V8_MAJOR_VERSION > 3
    delete m_isolateParams.array_buffer_allocator;
#endif
  }


  v8::Local<v8::Value> GetMember(v8::Local<v8::Context> context,
                                 v8::Local<v8::Object> object,
                                 const PString & name)
  {
    v8::Local<v8::Value> value;
    if (object.IsEmpty())
      return value;

#if V8_MAJOR_VERSION > 3
    v8::MaybeLocal<v8::Value> result = name[0] != '[' ? object->Get(context, NewString(name))
                                                      : object->Get(context, name.Mid(1).AsInteger());
    value = result.FromMaybe(v8::Local<v8::Value>());
#else
    value = name[0] != '[' ? object->Get(NewString(name)) : object->Get(name.Mid(1).AsInteger());
#endif
    if (value.IsEmpty())
      m_owner.OnError(101, PSTRSTRM("Cannot get member \"" << name << '"'));
    return value;
  }


  v8::Local<v8::Object> GetObjectHandle(const v8::Local<v8::Context> & context, const PString & key, PString & var, bool withError)
  {
    PStringArray tokens = key.Tokenise('.', false);
    if (tokens.GetSize() < 1) {
      m_owner.OnError(102, PSTRSTRM('"' << key << "\" is too short"));
      return v8::Local<v8::Object>();
    }

    for (PINDEX i = 0; i < tokens.GetSize(); ++i) {
      PString element = tokens[i];
      PINDEX start = element.Find('[');
      if (start != P_MAX_INDEX) {
        PINDEX end = element.Find(']', start + 1);
        if (end != P_MAX_INDEX) {
          tokens[i] = element(0, start - 1);
          tokens.InsertAt(++i, new PString(element(start, end - 1)));
          if (end < element.GetLength() - 1) {
            i++;
            tokens.InsertAt(i, new PString(element(end + 1, P_MAX_INDEX)));
          }
        }
      }
    }

    v8::Local<v8::Object> object = context->Global();

    PINDEX i;
    for (i = 0; i < tokens.GetSize()-1; ++i) {
      // get the member variable
      v8::Local<v8::Value> value = GetMember(context, object, tokens[i]);
      if (value.IsEmpty()) {
        if (withError)
          m_owner.OnError(103, PSTRSTRM("Cannot get intermediate element \"" << tokens[i] << "\" of \"" << key << '"'));
        return v8::Local<v8::Object>();
      }

      // terminals must not be composites, internal nodes must be composites
      if (!value->IsObject()) {
        if (withError)
          m_owner.OnError(104, PSTRSTRM("Non composite intermediate element \"" << tokens[i] << "\" of \"" << key << '"'));
        return v8::Local<v8::Object>();
      }

      // if path has ended, return error
      if (
#if V8_MAJOR_VERSION > 3
        !value->ToObject(context).ToLocal(&object) || object.IsEmpty()
#else
        *(object = value->ToObject()) == NULL || object->IsNull()
#endif
        ) {
        if (withError)
          m_owner.OnError(105, PSTRSTRM("Cannot get value of intermediate element \"" << tokens[i] << "\" of \"" << key << '"'));
        return v8::Local<v8::Object>();
      }
    }

    var = tokens[i];
    return object;
  }


#if V8_MAJOR_VERSION > 3
#define ToVarType(fn,dflt) PVarType(value->fn(context).FromMaybe(dflt))
#else
#define ToVarType(fn,dflt) PVarType(value->fn())
#endif

  bool GetVarValue(const v8::Local<v8::Context> & context,
                   v8::Local<v8::Value> value,
                   PVarType & var,
                   const PString & key,
                   bool withError)
  {
    if (value.IsEmpty())
      return false;

    if (value->IsInt32()) {
      var = ToVarType(Int32Value, 0);
      PTRACE(5, "Got Int32 \"" << key << "\" = " << var);
      return true;
    }

    if (value->IsUint32()) {
      var = ToVarType(Uint32Value, 0);
      PTRACE(5, "Got Uint32 \"" << key << "\" = " << var);
      return true;
    }

    if (value->IsNumber()) {
      var = ToVarType(NumberValue, 0.0);
      PTRACE(5, "Got Number \"" << key << "\" = " << var);
      return true;
    }

    if (value->IsBoolean()) {
#if V8_MAJOR_VERSION > 8
      var = PVarType(value->BooleanValue(m_isolate));
#else
      var = ToVarType(BooleanValue, false);
#endif
      PTRACE(5, "Got Boolean \"" << key << "\" = " << var);
      return true;
    }

    if (value->IsString()) {
#if V8_MAJOR_VERSION > 3
      var = PVarType(ToPString(value->ToString(context).FromMaybe(v8::Local<v8::String>())));
#else
      var = PVarType(ToPString(value->ToString()));
#endif
      PTRACE(5, "Got String \"" << key << "\" = " << var.AsString().Ellipsis(100).ToLiteral());
      return true;
    }

    if (withError)
      m_owner.OnError(106, PSTRSTRM("Unable to determine type of \"" << key << "\" = \"" << ToPString(value) << '"'));
    var.SetType(PVarType::VarNULL);
    return false;
  }


  PScriptLanguage::VarTypes GetVarType(const PString & key)
  {
    if (m_isolate == NULL) {
      m_owner.OnError(100, "Uninitialised");
      return NumVarTypes;
    }

    v8::Isolate::Scope isolateScope(m_isolate);
    HandleScope handleScope(this);
    v8::Local<v8::Context> context = GetContext();
    v8::Context::Scope contextScope(context);

    PString varName;
    v8::Local<v8::Object> object = GetObjectHandle(context, key, varName, false);
    v8::Local<v8::Value> member = GetMember(context, object, varName);
    if (member.IsEmpty() || member->IsUndefined())
      return UndefinedType;

    if (member->IsNull())
      return NullType;
    if (member->IsBoolean())
      return BooleanType;
    if (member->IsInt32())
      return IntegerType;
    if (member->IsNumber())
      return NumberType;
    if (member->IsString())
      return StringType;
    if (member->IsObject())
      return CompositeType;
    return NumVarTypes;
}


  bool GetVar(const PString & key, PVarType & var, bool withError)
  {
    if (m_isolate == NULL) {
      m_owner.OnError(100, "Uninitialised");
      return false;
    }

    v8::Isolate::Scope isolateScope(m_isolate);
    HandleScope handleScope(this);
    v8::Local<v8::Context> context = GetContext();
    v8::Context::Scope contextScope(context);

    PString varName;
    v8::Local<v8::Object> object = GetObjectHandle(context, key, varName, withError);
    if (object.IsEmpty())
      return false;

    return GetVarValue(context, GetMember(context, object, varName), var, key, withError);
  }


  v8::Local<v8::Value> SetVarValue(const PVarType & var)
  {
    switch (var.GetType()) {
      case PVarType::VarNULL:
        return v8::Local<v8::Value>();

      case PVarType::VarBoolean:
#if V8_MAJOR_VERSION > 3
        return NewObject<v8::Boolean>(var.AsBoolean());
#else
        return v8::Local<v8::Value>::New(v8::Boolean::New(var.AsBoolean()));
#endif

      case PVarType::VarChar:
      case PVarType::VarStaticString:
      case PVarType::VarFixedString:
      case PVarType::VarDynamicString:
      case PVarType::VarGUID:
        return NewString(var.AsString());

      case PVarType::VarInt8:
      case PVarType::VarInt16:
      case PVarType::VarInt32:
        return NewObject<v8::Integer>(var.AsInteger());

      case PVarType::VarUInt8:
      case PVarType::VarUInt16:
      case PVarType::VarUInt32:
        return NewObject<v8::Integer>(var.AsUnsigned());

      case PVarType::VarInt64:
      case PVarType::VarUInt64:
        // Until V8 suppies a 64 bit integer, we use double

      case PVarType::VarFloatSingle:
      case PVarType::VarFloatDouble:
      case PVarType::VarFloatExtended:
        return NewObject<v8::Number>(var.AsFloat());

      case PVarType::VarTime:
      case PVarType::VarStaticBinary:
      case PVarType::VarDynamicBinary:
      default:
        return NewObject<v8::Object>();
    }
  }


  bool SetVar(const PString & key, const PVarType & var)
  {
    if (m_isolate == NULL) {
      m_owner.OnError(100, "Uninitialised");
      return false;
    }

    v8::Isolate::Scope isolateScope(m_isolate);
    HandleScope handleScope(this);
    v8::Local<v8::Context> context = GetContext();
    v8::Context::Scope contextScope(context);

    PString varName;
    v8::Local<v8::Object> object = GetObjectHandle(context, key, varName, true);
    if (object.IsEmpty())
      return false;

    v8::Local<v8::Value> value = SetVarValue(var);
#if V8_MAJOR_VERSION > 3
    v8::Maybe<bool> result = varName[0] != '[' ? object->Set(context, NewString(varName), value)
      : object->Set(context, varName.Mid(1).AsUnsigned(), value);
    if (result.FromMaybe(false))
#else
    if (varName[0] != '[' ? object->Set(NewString(varName), value) : object->Set(varName.Mid(1).AsInteger(), value))
#endif
    {
      PTRACE(4, "Set \"" << key << "\" to " <<
             (var.GetType() == PVarType::VarStaticBinary ? PConstString("composite") : var.AsString().Left(100).ToLiteral()));
      return true;
    }

    m_owner.OnError(107, PSTRSTRM("Could not set \"" << key << "\" to " << var.AsString().Left(100).ToLiteral()));
    return false;
  }


  struct NotifierInfo {
    PJavaScript    & m_owner;
    PString          m_key;
    PNotifierListTemplate<Signature &> m_notifiers;
    NotifierInfo(PJavaScript & owner, const PString & key)
      : m_owner(owner)
      , m_key(key)
    { }
  };
  typedef std::map<std::string, NotifierInfo> NotifierMap;
  NotifierMap m_notifiers;

  template <class ARGTYPE> v8::Local<v8::Value> FunctionCallback(const ARGTYPE & callbackInfo, NotifierInfo & notifierInfo)
  {
    Signature signature;

    v8::Local<v8::Context> context = GetContext();
    int nargs = callbackInfo.Length();
    signature.m_arguments.resize(nargs);
    for (int arg = 0; arg < nargs; ++arg)
      GetVarValue(context, callbackInfo[arg], signature.m_arguments[arg], notifierInfo.m_key, true);

    notifierInfo.m_notifiers(notifierInfo.m_owner, signature);

    return SetVarValue(signature.m_results.empty() ? PVarType() : signature.m_results[0]);
  }

  template <class ARGTYPE> static NotifierInfo * GetNotifierInfo(const ARGTYPE & callbackInfo)
  {
    v8::Local<v8::Value> callbackData = callbackInfo.Data();
    if (callbackData.IsEmpty() || !callbackData->IsExternal())
      return NULL;

    return reinterpret_cast<NotifierInfo *>(v8::Local<v8::External>::Cast(callbackData)->Value());
  }

#if V8_MAJOR_VERSION > 3
  static void StaticFunctionCallback(const v8::FunctionCallbackInfo<v8::Value>& callbackInfo)
  {
    NotifierInfo * notifierInfo = GetNotifierInfo(callbackInfo);
    if (notifierInfo != NULL)
      callbackInfo.GetReturnValue().Set(notifierInfo->m_owner.m_private->FunctionCallback(callbackInfo, *notifierInfo));
  }
#else
  static v8::Handle<v8::Value> StaticFunctionCallback(const v8::Arguments& args)
  {
    NotifierInfo * notifierInfo = GetNotifierInfo(args);
    return notifierInfo != NULL ? notifierInfo->m_owner.m_private->FunctionCallback(args, *notifierInfo) : v8::Handle<v8::Value>();
  }
#endif // V8_MAJOR_VERSION > 3

  bool SetFunction(const PString & key, const FunctionNotifier & notifier)
  {
    if (m_isolate == NULL) {
      m_owner.OnError(100, "Uninitialised");
      return false;
    }

    v8::Isolate::Scope isolateScope(m_isolate);
    HandleScope handleScope(this);
    v8::Local<v8::Context> context = GetContext();
    v8::Context::Scope contextScope(context);

    PString varName;
    v8::Local<v8::Object> object = GetObjectHandle(context, key, varName, true);
    if (object.IsEmpty())
      return false;

    NotifierMap::iterator it = m_notifiers.find(key);
    if (it == m_notifiers.end())
      it = m_notifiers.insert(make_pair(key.GetPointer(), NotifierInfo(m_owner, key))).first;
    NotifierInfo & info = it->second;
    info.m_notifiers.Add(notifier);

    v8::Local<v8::External> data = NewObject<v8::External, void *>(&info);
    v8::Local<v8::Function> value;
#if V8_MAJOR_VERSION > 3
    if (v8::Function::New(context, StaticFunctionCallback, data).ToLocal(&value) &&
        object->Set(context, NewString(varName), value).FromMaybe(false))
#else
    v8::Local<v8::FunctionTemplate> fnt = v8::FunctionTemplate::New(StaticFunctionCallback, data);
    if (*fnt != NULL && *(value = fnt->GetFunction()) != NULL && object->Set(NewString(varName), value))
#endif // V8_MAJOR_VERSION > 3
    {
      PTRACE(4, "Set function \"" << key << '"');
      return true;
    }

    m_owner.OnError(110, PSTRSTRM("Could not set function \"" << varName << '"'));
    return false;
  }


  PString OnException(TryCatch & exceptionHandler)
  {
    PString err;

    v8::Local<v8::Message> msg = exceptionHandler.Message();
    if (*msg != NULL)
      err = ToPString(msg->Get());

    if (err.IsEmpty())
      err = ToPString(exceptionHandler.Exception());

    if (err.IsEmpty())
      err = "Unknown";

    return err;
  }


  bool Run(const PString & text, PString & resultText)
  {
    if (m_isolate == NULL) {
      m_owner.OnError(100, "Uninitialised");
      return false;
    }

    // create a V8 scopes
    v8::Isolate::Scope isolateScope(m_isolate);
    HandleScope handleScope(this);

    // make context scope availabke
    v8::Local<v8::Context> context = GetContext();
    v8::Context::Scope contextScope(context);

    v8::Local<v8::String> source = NewString(text);

    TryCatch exceptionHandler(this);

    // compile the source 
    v8::Local<v8::Script> script;
#if V8_MAJOR_VERSION > 3
    v8::Script::Compile(context, source).ToLocal(&script);
#else
    script = v8::Script::Compile(source);
#endif
    if (exceptionHandler.HasCaught()) {
      PTRACE(3, "Could not compile source " << text. Ellipsis(100).ToLiteral());
      m_owner.OnError(120, OnException(exceptionHandler));
      return false;
    }
    if (script.IsEmpty()) {
      PTRACE(3, "Could not compile source " << text.Left(100).ToLiteral());
      m_owner.OnError(120, "Compile ToLocal failed");
      return false;
    }

    // run the code
    v8::Local<v8::Value> result;
#if V8_MAJOR_VERSION > 3
    script->Run(context).ToLocal(&result);
#else
    result = script->Run();
#endif
    if (exceptionHandler.HasCaught()) {
      PTRACE(3, "Exception running source " << text. Ellipsis(100).ToLiteral());
      m_owner.OnError(121, OnException(exceptionHandler));
      return false;
    }

    if (result.IsEmpty()) {
      PTRACE(3, "No result running source " << text.Ellipsis(100).ToLiteral());
      m_owner.OnError(121, "Run had no result");
      return false;
    }

    resultText = ToPString(result);
    PTRACE(4, "Script execution returned " << resultText.Ellipsis(100).ToLiteral());

    return true;
  }
  };


#define new PNEW


///////////////////////////////////////////////////////////////////////////////

PJavaScript::PJavaScript()
  : m_private(new Private(*this))
{
}


PJavaScript::~PJavaScript()
{
  delete m_private;
}


PString PJavaScript::LanguageName()
{
  return JavaName;
}


PString PJavaScript::GetLanguageName() const
{
  return JavaName;
}


bool PJavaScript::IsInitialised() const
{
  return m_private->m_isolate != NULL;
}


bool PJavaScript::LoadText(const PString & text)
{
  PWaitAndSignal mutex(m_mutex);
  m_scriptToRun = text;
  return !text.empty();
}


bool PJavaScript::Run(const char * text)
{
  PWaitAndSignal mutex(m_mutex);

  PString script(text != NULL ? text : m_scriptToRun.c_str());
  if (script.empty())
    return false;

  PTextFile file(text, PFile::ReadOnly);
  if (file.IsOpen()) {
    PTRACE(4, "Reading script file: " << file.GetFilePath());
    script = file.ReadString(P_MAX_INDEX);
  }

  if (m_scopeChain.empty())
    return m_private->Run(script, m_resultText);

  PStringStream wrapped;
  for (PStringList::iterator it = m_scopeChain.begin(); it != m_scopeChain.end(); ++it)
    wrapped << "with(" << *it << "){";
  wrapped << script << std::string(m_scopeChain.size(), '}');
  return m_private->Run(wrapped, m_resultText);
}


bool PJavaScript::CreateComposite(const PString & name)
{
  PWaitAndSignal mutex(m_mutex);

  PVarType dummy;
  dummy.SetType(PVarType::VarStaticBinary);
  return m_private->SetVar(name, dummy);
}


PScriptLanguage::VarTypes PJavaScript::GetVarType(const PString & name)
{
  PWaitAndSignal mutex(m_mutex);
  return m_private->GetVarType(name);
}


bool PJavaScript::GetVar(const PString & name, PVarType & var)
{
  PWaitAndSignal mutex(m_mutex);
  for (PStringList::iterator it = m_scopeChain.rbegin(); it != m_scopeChain.rend(); --it) {
    if (m_private->GetVar(PSTRSTRM(*it << '.' << name), var, false))
      return true;
  }
  return m_private->GetVar(name, var, true);
}

bool PJavaScript::SetVar(const PString & name, const PVarType & var)
{
  PWaitAndSignal mutex(m_mutex);
  return m_private->SetVar(name, var);
}


bool PJavaScript::ReleaseVariable(const PString & name)
{
  return Run("delete " + name);
}


bool PJavaScript::Call(const PString & /*name*/, const char * /*signature*/, ...)
{
  return false;
}


bool PJavaScript::Call(const PString & /*name*/, Signature & /*signature*/)
{
  return false;
}


bool PJavaScript::SetFunction(const PString & name, const FunctionNotifier & func)
{
  PWaitAndSignal mutex(m_mutex);
  return m_private->SetFunction(name, func);
}

#endif // P_V8
