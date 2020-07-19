/*
 * platform.h
 *
 * Unix machine dependencies
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
 */

#ifndef PTLIB_PALTFORM_H
#define PTLIB_PALTFORM_H

///////////////////////////////////////////////////////////////////////////////

#ifdef __NUCLEUS_MNT__
#define P_PLATFORM_HAS_THREADS
// The windows version of errno.h, which this will find, should do for us -
// it contains lots of things from Unix!!!
#include <errno.h>
#else
#ifdef __NUCLEUS_PLUS__
#define P_PLATFORM_HAS_THREADS
#endif
#ifndef __NUCLEUS_PLUS__
#include <netdb.h>
#endif
#endif


#define P_PLATFORM_HAS_THREADS
#ifndef __NUCLEUS_PLUS__
#include <pthread.h>
#endif

// If we're running effectively 'doze, then it's little endian.  If PoserPC,
// big endian.
#ifdef __NUCLEUS_MNT__
#define PBYTE_ORDER PLITTLE_ENDIAN
#else
//#define PBYTE_ORDER PBIG_ENDIAN
#endif

#if 0
#ifdef __NUCLEUS_PLUS__
// Other things
#define	INADDR_NONE	-1
#endif
#endif

#ifdef _DEBUG
  __inline void PBreakToDebugger() { kill(taskIdSelf(), SIGABRT); }
#else
  __inline void PBreakToDebugger() { }
#endif


// Wos ere.  Moved so that types are defined properly in the right order.
//#include "pmachdep.h"

#ifdef __NUCLEUS_MNT__
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_     /* _WINSOCKAPI_ must be defined to keep the      */
#endif                   /* winsock.h file from being included.  If it is */
                         /* included some of the typedefs will conflict   */
                         /* with those in Nucleus NET.                    */

extern "C"
  {
#include "net\inc\externs.h"  // Nucleus (V)NET unofficial
#include "control.h"  // Nucleus (V)NET official
#include "net\inc\Sockext.h"
  }

// This really is for VNET only, because they've used INT16 in NET and not
// even bothered to define it in NET.
//typedef int16 INT16;

#else
#include <unistd.h>
#endif
#include <ctype.h>


///////////////////////////////////////////
//
//  define TRUE and FALSE for environments that don't have them
//

#ifndef	TRUE
#define	TRUE		1
#define	FALSE		0
#endif

#include <istream>
#include <ostream>

ostream & operator<<(ostream & s, int64_t v);
ostream & operator<<(ostream & s, uint64_t v);

istream & operator>>(istream & s, int64_t & v);
istream & operator>>(istream & s, uint64_t & v);
#endif
#ifdef __DIAB
typedef   signed long long int int64_t;
typedef unsigned long long int uint64_t;	// 64 bit
#endif

// MSVC problem requires a PINDEX to be #defined (specifically) to an int
// (specifically)
#ifdef __NUCLEUS_MNT__
#define PINDEX int
#else
typedef size_t PINDEX;
#endif

#define P_MAX_INDEX 		0x7fffffff
#define PABSINDEX(idx) 		(idx)		// careful - size_t may be signed!
#define PASSERTINDEX(idx)


#endif // PTLIB_PALTFORM_H

// End of file
