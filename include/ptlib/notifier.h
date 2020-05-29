/*
 * notifier.h
 *
 * Notified type safe function pointer class.
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
 * Contributor(s): ______________________________________.
 */

#ifndef PTLIB_NOTIFIER_H
#define PTLIB_NOTIFIER_H

#include <functional>


///////////////////////////////////////////////////////////////////////////////
// General notification mechanism from one object to another

template <typename ParamType, class NotifierType = PObject, class TargetType = PObject>
class PNotifierTemplate : public PObject, public std::function<void(NotifierType &, ParamType)>
{
    PCLASSINFO(PNotifierTemplate, PObject);
  protected:
    TargetType * m_target;
  public:
    typedef std::function<void(NotifierType &, ParamType)> Function;

    PNotifierTemplate() : m_target(nullptr) { }
    PNotifierTemplate(nullptr_t *) : m_target(nullptr) { }
    PNotifierTemplate(Function fn, TargetType * target) : Function(std::move(fn)), m_target(target) { }

    P_DEPRECATED bool IsNULL() const { return Function::operator bool(); }
    TargetType * GetTarget() const { return m_target; }
};

/** \class PNotifier
    Class specialisation for PNotifierTemplate<intptr_t>
  */
typedef PNotifierTemplate<intptr_t> PNotifier;


#define PDECLARE_NOTIFIER_EXT(NotifierType, notifierArg, TargetType, func, ParamType, paramArg, FunctionType, ...) \
  struct func##_PNotifier { \
    static FunctionType Create(TargetType * target) { return FunctionType(std::bind(&TargetType::func##_PObject,  target, std::placeholders::_1, std::placeholders::_2),  target, ##__VA_ARGS__); } \
    static FunctionType Create(TargetType & target) { return FunctionType(std::bind(&TargetType::func##_PObject, &target, std::placeholders::_1, std::placeholders::_2), &target, ##__VA_ARGS__); } \
  }; \
  void func##_PObject(PObject & n, ParamType p) { func(dynamic_cast<NotifierType &>(n), p); } \
  virtual void func(NotifierType & notifierArg, ParamType paramArg)

/// Declare PNotifier derived class with intptr_t parameter. Uses PDECLARE_NOTIFIER_EXT macro.
#define PDECLARE_NOTIFIER2(NotifierType, TargetType, func, ParamType) \
     PDECLARE_NOTIFIER_EXT(NotifierType, , TargetType, func, ParamType, , PNotifierTemplate<ParamType>)

/// Declare PNotifier derived class with intptr_t parameter. Uses PDECLARE_NOTIFIER_EXT macro.
#define PDECLARE_NOTIFIER(NotifierType, TargetType, func) \
       PDECLARE_NOTIFIER2(NotifierType, TargetType, func, intptr_t)


/** Create a PNotifier object instance.
  This macro creates an instance of the particular <code>PNotifier</code> class using
  the \p func parameter as the member function to call.

  The \p obj parameter is the instance to call the function against.
  If the instance to be called is the current instance, ie if \p obj is
  \p this then the <code>PCREATE_NOTIFIER</code> macro should be used.
 */
#define PCREATE_NOTIFIER2_EXT(obj, TargetType, func, type) TargetType::func##_PNotifier::Create(obj)

/// Create PNotifier object instance with intptr_t parameter. Uses PCREATE_NOTIFIER2_EXT macro.
#define PCREATE_NOTIFIER_EXT( obj, TargetType, func) TargetType::func##_PNotifier::Create(obj)


/** Create a PNotifier object instance.
  This macro creates an instance of the particular <code>PNotifier</code> class using
  the \p func parameter as the member function to call.

  The \p this object is used as the instance to call the function
  against. The <code>PCREATE_NOTIFIER_EXT</code> macro may be used if the instance to be
  called is not the current object instance.
 */
#define PCREATE_NOTIFIER2(func, type) P_DISABLE_MSVC_WARNINGS(4355, func##_PNotifier::Create(this))

/// Create PNotifier object instance with intptr_t parameter. Uses PCREATE_NOTIFIER2 macro.
#define PCREATE_NOTIFIER(func) P_DISABLE_MSVC_WARNINGS(4355, func##_PNotifier::Create(this))


#endif // PTLIB_NOTIFIER_H


// End Of File ///////////////////////////////////////////////////////////////
