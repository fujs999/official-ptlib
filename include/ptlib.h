/*
 * $Id: ptlib.h,v 1.14 1998/05/30 13:25:00 robertj Exp $
 *
 * Portable Windows Library
 *
 * User Interface Classes Interface Declarations
 *
 * Copyright 1993 by Robert Jongbloed and Craig Southeren
 *
 * $Log: ptlib.h,v $
 * Revision 1.14  1998/05/30 13:25:00  robertj
 * Added PSyncPointAck class.
 *
 * Revision 1.13  1998/03/20 03:16:10  robertj
 * Added special classes for specific sepahores, PMutex and PSyncPoint.
 *
 * Revision 1.12  1996/09/14 13:09:16  robertj
 * Major upgrade:
 *   rearranged sockets to help support IPX.
 *   added indirect channel class and moved all protocols to descend from it,
 *   separating the protocol from the low level byte transport.
 *
 * Revision 1.11  1996/08/08 10:08:40  robertj
 * Directory structure changes for common files.
 *
 * Revision 1.10  1996/05/23 09:57:24  robertj
 * Changed process.h to pprocess.h to avoid name conflict.
 *
 * Revision 1.9  1995/07/31 12:06:21  robertj
 * Added semaphore class.
 *
 * Revision 1.8  1995/03/12 04:44:56  robertj
 * Added dynamic link libraries.
 *
 * Revision 1.7  1994/09/25  10:43:57  robertj
 * Added pipe channel.
 *
 * Revision 1.6  1994/08/23  11:32:52  robertj
 * Oops
 *
 * Revision 1.5  1994/08/22  00:46:48  robertj
 * Added pragma fro GNU C++ compiler.
 *
 * Revision 1.4  1994/07/25  03:36:03  robertj
 * Added sockets to common, normalising to same comment standard.
 *
 * Revision 1.3  1994/07/21  12:17:41  robertj
 * Sockets.
 *
 * Revision 1.2  1994/06/25  12:27:39  robertj
 * *** empty log message ***
 *
 * Revision 1.1  1994/04/01  14:38:42  robertj
 * Initial revision
 *
 */

#ifndef _PTLIB_H
#define _PTLIB_H

#ifdef __GNUC__
#pragma interface
#endif


#include <contain.h>


///////////////////////////////////////////////////////////////////////////////
// PTime

#include <ptime.h>


///////////////////////////////////////////////////////////////////////////////
// PTimeInterval

#include <timeint.h>


///////////////////////////////////////////////////////////////////////////////
// PTimer

#include <timer.h>


///////////////////////////////////////////////////////////////////////////////
// PDirectory

#include <pdirect.h>


///////////////////////////////////////////////////////////////////////////////
// PChannel

#include <channel.h>


///////////////////////////////////////////////////////////////////////////////
// PIndirectChannel

#include <ptlib/indchan.h>


///////////////////////////////////////////////////////////////////////////////
// PFilePath

#include <filepath.h>


///////////////////////////////////////////////////////////////////////////////
// PFile

#include <file.h>


///////////////////////////////////////////////////////////////////////////////
// PTextFile

#include <textfile.h>


///////////////////////////////////////////////////////////////////////////////
// PStructuredFile

#include <sfile.h>


///////////////////////////////////////////////////////////////////////////////
// PPipeChannel

#include <pipechan.h>


///////////////////////////////////////////////////////////////////////////////
// PSerialChannel

#include <serchan.h>


///////////////////////////////////////////////////////////////////////////////
// PModem

#include <ptlib/modem.h>


///////////////////////////////////////////////////////////////////////////////
// PConfig

#include <config.h>


///////////////////////////////////////////////////////////////////////////////
// PArgList

#include <ptlib/args.h>


///////////////////////////////////////////////////////////////////////////////
// PThread

#include <thread.h>


///////////////////////////////////////////////////////////////////////////////
// PProcess

#include <pprocess.h>


///////////////////////////////////////////////////////////////////////////////
// PSemaphore

#include <semaphor.h>


///////////////////////////////////////////////////////////////////////////////
// PMutex

#include <mutex.h>


///////////////////////////////////////////////////////////////////////////////
// PSyncPoint

#include <syncpoint.h>


///////////////////////////////////////////////////////////////////////////////
// PSyncPointAck

#include <ptlib/syncptack.h>


///////////////////////////////////////////////////////////////////////////////
// PDynaLink

#include <dynalink.h>


///////////////////////////////////////////////////////////////////////////////
// PSound

#include <sound.h>


///////////////////////////////////////////////////////////////////////////////


#if defined(P_USE_INLINES)
#include <ptlib.inl>
#include <ptlib/osutil.inl>
#endif


#endif // _PTLIB_H


// End Of File ///////////////////////////////////////////////////////////////
