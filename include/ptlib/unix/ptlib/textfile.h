/*
 * textfile.h
 *
 * Text file I/O channel class.
 *
 * Portable Windows Library
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: textfile.h,v $
 * Revision 1.6  2002/09/16 01:08:59  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 1.5  2001/05/22 12:49:32  robertj
 * Did some seriously wierd rewrite of platform headers to eliminate the
 *   stupid GNU compiler warning about braces not matching.
 *
 * Revision 1.4  1998/11/30 22:07:21  robertj
 * New directory structure.
 *
 * Revision 1.3  1998/09/24 04:12:02  robertj
 * Added open software license.
 *
 * Revision 1.2  1996/08/03 12:09:51  craigs
 * Changed for new common directories
 *
 * Revision 1.1  1995/01/23 18:43:27  craigs
 * Initial revision
 *
 * Revision 1.1  1994/04/12  08:31:05  robertj
 * Initial revision
 *
 */

#ifndef _PTEXTFILE

#ifdef P_USE_PRAGMA
#pragma interface
#endif

///////////////////////////////////////////////////////////////////////////////
// PTextFile

#define _PTEXTFILE_PLATFORM_INCLUDE
#include "../../textfile.h"

#endif
#ifdef _PTEXTFILE_PLATFORM_INCLUDE
#undef _PTEXTFILE_PLATFORM_INCLUDE


#endif


// End Of File ////////////////////////////////////////////////////////////////
