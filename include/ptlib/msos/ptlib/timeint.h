/*
 * $Id: timeint.h,v 1.2 1994/07/02 03:18:09 robertj Exp $
 *
 * Portable Windows Library
 *
 * Operating System Classes Interface Declarations
 *
 * Copyright 1993 Equivalence
 *
 * $Log: timeint.h,v $
 * Revision 1.2  1994/07/02 03:18:09  robertj
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

#include "../../common/timeint.h"
};

#define PMaxTimeInterval PTimeInterval((long)0x7fffffff)


#endif
