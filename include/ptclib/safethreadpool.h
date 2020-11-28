/*
 * safethreadpool.h
 *
 * Safe Thread Pooling functions
 *
 * Portable Tools Library
 *
 * Copyright (C) 2009 Post Increment
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
 * Portions of this code were written with the financial assistance of 
 * Metreos Corporation (http://www.metros.com).
 *
 * Contributor(s): ______________________________________.
 */


#ifndef PTLIB_SAFETHREADPOOL_H
#define PTLIB_SAFETHREADPOOL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <ptclib/threadpool.h>
#include <ptlib/safecoll.h>


/**A PThreadPool work item template that uses PSafePtr to execute callback
   function.
  */
class PSafeWork : public PSafePtrBase {
  public:
    PSafeWork(
      PSafeObject * ptr
    ) : PSafePtrBase(ptr) { }

    virtual void Work()
    {
      PSafeObject * ptr = this->GetObject();
      if (ptr != NULL) {
        PTRACE_CONTEXT_ID_PUSH_THREAD(ptr);
        CallFunction(*ptr);
      }
    }

    virtual void CallFunction(PSafeObject & obj) = 0;
};


/// The thread pool for PSafeWork items.
typedef PQueuedThreadPool<PSafeWork> PSafeThreadPool;


/// A PSafeWork thread pool item where call back has no arguments.
template <class PtrClass, typename FuncRet = void>
class PSafeWorkNoArg : public PSafeWork
{
  PCLASSINFO_ALIGNED(PSafeWorkNoArg, PSafeWork, 16);

  public:
    typedef FuncRet (PtrClass::*Function)();

  protected:
    P_ALIGN_FIELD(Function,m_function,16);

  public:
    PSafeWorkNoArg(
      PtrClass * ptr,
      Function function
    ) : PSafeWork(ptr)
      , m_function(function)
    { }

    virtual void CallFunction(PSafeObject & obj)
    {
      (dynamic_cast<PtrClass&>(obj).*(this->m_function))();
    }
};


/// A PSafeWork thread pool item where call back has 1 argument.
template <
  class PtrClass,
  typename Arg1Type,
  typename FuncRet = void
>
class PSafeWorkArg1 : public PSafeWork
{
  PCLASSINFO_ALIGNED(PSafeWorkArg1, PSafeWork, 16);

  public:
    typedef FuncRet (PtrClass::*Function)(Arg1Type arg1);

  protected:
    P_ALIGN_FIELD(Function,m_function,16);
    Arg1Type m_arg1;

  public:
    PSafeWorkArg1(
      PtrClass * ptr,
      Arg1Type arg1,
      Function function
    ) : PSafeWork(ptr)
      , m_function(function)
      , m_arg1(arg1)
    { }

    virtual void CallFunction(PSafeObject & obj)
    {
      (dynamic_cast<PtrClass&>(obj).*(this->m_function))(m_arg1);
    }
};


/// A PSafeWork thread pool item where call back has 2 arguments.
template <
  class PtrClass,
  typename Arg1Type,
  typename Arg2Type,
  typename FuncRet = void
>
class PSafeWorkArg2 : public PSafeWork
{
  PCLASSINFO_ALIGNED(PSafeWorkArg2, PSafeWork, 16);

  public:
    typedef FuncRet (PtrClass::*Function)(Arg1Type arg1, Arg2Type arg2);

  protected:
    P_ALIGN_FIELD(Function,m_function,16);
    Arg1Type m_arg1;
    Arg2Type m_arg2;

  public:
    PSafeWorkArg2(
      PtrClass * ptr,
      Arg1Type arg1,
      Arg2Type arg2,
      Function function
    ) : PSafeWork(ptr)
      , m_function(function)
      , m_arg1(arg1)
      , m_arg2(arg2)
    { }

    virtual void CallFunction(PSafeObject & obj)
    {
      (dynamic_cast<PtrClass&>(obj).*(this->m_function))(m_arg1, m_arg2);
    }
};


/// A PSafeWork thread pool item where call back has 3 arguments.
template <
  class PtrClass,
  typename Arg1Type,
  typename Arg2Type,
  typename Arg3Type,
  typename FuncRet = void
>
class PSafeWorkArg3 : public PSafeWork
{
  PCLASSINFO_ALIGNED(PSafeWorkArg3, PSafeWork, 16);

  public:
    typedef FuncRet (PtrClass::*Function)(Arg1Type arg1, Arg2Type arg2, Arg3Type arg3);

  protected:
    P_ALIGN_FIELD(Function,m_function,16);
    Arg1Type m_arg1;
    Arg2Type m_arg2;
    Arg3Type m_arg3;

  public:
    PSafeWorkArg3(
      PtrClass * ptr,
      Arg1Type arg1,
      Arg2Type arg2,
      Arg3Type arg3,
      Function function
    ) : PSafeWork(ptr)
      , m_function(function)
      , m_arg1(arg1)
      , m_arg2(arg2)
      , m_arg3(arg3)
    { }

    virtual void CallFunction(PSafeObject & obj)
    {
      (dynamic_cast<PtrClass&>(obj).*(this->m_function))(m_arg1, m_arg2, m_arg3);
    }
};


#endif // PTLIB_THREADPOOL_H


// End Of File ///////////////////////////////////////////////////////////////
