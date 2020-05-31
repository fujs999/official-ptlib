/*
 * osutil.inl
 *
 * Operating System classes inline function implementation
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
 *
 * $Id$
 */

#ifdef P_USE_LANGINFO
#include <langinfo.h>
#endif

PINLINE uint32_t PProcess::GetProcessID() const
{
#ifdef __NUCLEUS_PLUS__
// Only one process
  return 0;
#else
  return (uint32_t)getpid();
#endif
}

///////////////////////////////////////////////////////////////////////////////

PINLINE unsigned PTimer::Resolution()
#ifdef __NUCLEUS_PLUS__
  {
// Returns number of milliseconds per tick
  return 10;
  }
#elif defined(P_SUN4)
  { return 1000; }
#else
  { return (unsigned)(1000/CLOCKS_PER_SEC); }
#endif

///////////////////////////////////////////////////////////////////////////////

PINLINE bool PDirectory::IsRoot() const
#ifdef WOT_NO_FILESYSTEM
  { return true;}
#else
  { return IsSeparator((*this)[0]) && ((*this)[1] == '\0'); }
#endif

PINLINE bool PDirectory::IsSeparator(char ch)
  { return ch == PDIR_SEPARATOR; }

#ifdef WOT_NO_FILESYSTEM
PINLINE bool PDirectory::Change(const PString &)
  { return true;}

PINLINE bool PDirectory::Exists(const PString & p)
  { return false; }
#else
PINLINE bool PDirectory::Change(const PString & p)
  { return chdir(p) == 0; }

PINLINE bool PDirectory::Exists(const PString & p)
  { return access((const char *)p, 0) == 0; }
#endif

///////////////////////////////////////////////////////////////////////////////

PINLINE PString PFilePath::GetVolume() const
  { return PString::Empty(); }

///////////////////////////////////////////////////////////////////////////////

PINLINE bool PFile::Exists(const PFilePath & name)
#ifdef WOT_NO_FILESYSTEM
  { return false; }
#else
  { return access(name, 0) == 0; }
#endif

PINLINE bool PFile::Remove(const PFilePath & name, bool)
  { return unlink(name) == 0; }

///////////////////////////////////////////////////////////////////////////////

PINLINE void PChannel::Construct()
  { os_handle = -1; }

// End Of File ///////////////////////////////////////////////////////////////
