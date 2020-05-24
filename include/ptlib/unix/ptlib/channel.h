/*
 * channel.h
 *
 * I/O channel ancestor class.
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
 */

  public:
    enum PXBlockType {
      PXReadBlock,
      PXWriteBlock,
      PXAcceptBlock,
      PXConnectBlock
    };

  protected:
    bool PXSetIOBlock(PXBlockType type, const PTimeInterval & timeout);
    P_INT_PTR GetOSHandleAsInt() const { return os_handle; }
    int  PXClose();

    PDECLARE_MUTEX(  px_threadMutex);
    PXBlockType      px_lastBlockType;
    PThread        * px_readThread;
    PThread        * px_writeThread;
    PDECLARE_MUTEX(  px_writeMutex);
    PThread        * px_selectThread[3];
    PCriticalSection px_selectMutex[3];


// End Of File ////////////////////////////////////////////////////////////////
