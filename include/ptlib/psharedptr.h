/*
 * psharedptr.h
 *
 * SharedPtr template
 *
 * Portable Tools Library
 *
 * Copyright (C) 2004 Post Increment
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
 * Contributor(s): ______________________________________.
 *
 */

#ifndef PTLIB_SHAREDPTR_H
#define PTLIB_SHAREDPTR_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>
#include <memory>

/** This class is simple wrapper around std::shared_ptr<> for backward compatibility
 */
template <class T>
class PSharedPtr : public PObject, public std::shared_ptr<T>
{
  PCLASSINFO_WITH_CLONE(PSharedPtr, PObject);
  public:
    typedef T element_type;

    PSharedPtr(element_type * p = nullptr) : std::shared_ptr<T>(p) { }
    element_type * Get() const { return get(); }
    void Reset() const { reset(); }
};


#endif // PTLIB_SHAREDPTR_H


// End Of File ///////////////////////////////////////////////////////////////
