/*
 * $Id: timeint.h,v 1.6 1996/08/08 10:09:20 robertj Exp $
 *
 * Portable Windows Library
 *
 * Operating System Classes Interface Declarations
 *
 * Copyright 1993 Equivalence
 *
 * $Log: timeint.h,v $
 * Revision 1.6  1996/08/08 10:09:20  robertj
 * Directory structure changes for common files.
 *
 * Revision 1.5  1995/12/10 11:49:26  robertj
 * Fixed bug in time interfval constant variable initialisation. Not guarenteed to work.
 *
 * Revision 1.4  1995/04/25 11:20:53  robertj
 * Moved const variable to .cxx file to better compiler portability.
 *
 * Revision 1.3  1995/03/12 05:00:03  robertj
 * Re-organisation of DOS/WIN16 and WIN32 platforms to maximise common code.
 * Used built-in equate for WIN32 API (_WIN32).
 *
 * Revision 1.2  1994/07/02  03:18:09  robertj
 * Fixed bug in time intervals being signed.
 *
 * Revision 1.1  1994/06/25  12:13:01  robertj
 * Initial revision
 *
 * Revision 1.1  1994/04/12  08:31:05  robertj
 * Initial revision
 *
 */

#ifndef _PTIMEINTERVAL

///////////////////////////////////////////////////////////////////////////////
// PTimeInterval

#include "../../common/ptlib/timeint.h"
};

#define PMaxTimeInterval PTimeInterval(0x7fffffff)


#endif
