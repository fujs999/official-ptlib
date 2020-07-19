/*
 * semaphor.h
 *
 * Thread synchronisation semaphore class.
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

  protected:
  #if defined(P_HAS_SEMAPHORES)
    mutable sem_t m_semaphore;
    #if defined(P_HAS_NAMED_SEMAPHORES)
      mutable struct SemPtr {
        SemPtr() : ptr(NULL) { }
        sem_t * ptr;
      } m_namedSemaphore;
      __inline sem_t * GetSemPtr() const { return m_namedSemaphore.ptr != NULL ? m_namedSemaphore.ptr : &m_semaphore; }
    #else
      __inline sem_t * GetSemPtr() const { return &m_semaphore; }
    #endif
  #else
    mutable pthread_mutex_t mutex;
    mutable pthread_cond_t  condVar;
    mutable unsigned currentCount;
    mutable unsigned queuedLocks;
    friend class PThread;
  #endif

// End Of File ////////////////////////////////////////////////////////////////
