/*
 * thread.h
 *
 * Thread of execution control class.
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

///////////////////////////////////////////////////////////////////////////////
// PThread

  public:
    int PXBlockOnIO(
      int handle,
      int type,
      const PTimeInterval & timeout
    );

    void PXAbortBlock() const;

#ifndef P_HAS_SEMAPHORES
    void PXSetWaitingSemaphore(PSemaphore * sem);
#endif
    static bool PlatformKill(PThreadIdentifier tid, PUniqueThreadIdentifier uid, int sig);

  protected:
#if defined(P_LINUX)
    PTimeInterval     PX_startTick;
    PTimeInterval     PX_endTick;
#endif
    std::unique_ptr<PSyncPoint> PX_synchroniseThreadFinish;

#ifndef P_HAS_SEMAPHORES
    PSemaphore      * PX_waitingSemaphore;
    pthread_mutex_t   PX_WaitSemMutex;
#endif

    int m_pxUnblockPipe[2];
    friend class PSocket;



// End Of File ////////////////////////////////////////////////////////////////
