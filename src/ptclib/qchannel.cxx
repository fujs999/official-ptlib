/*
 * qchannel.cxx
 *
 * Class for implementing a serial queue channel in memory.
 *
 * Portable Tools Library
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 */

#ifdef __GNUC__
#pragma implementation "qchannel.h"
#endif

#include <ptlib.h>
#include <ptclib/qchannel.h>


#define new PNEW


/////////////////////////////////////////////////////////

PQueueChannel::PQueueChannel(PINDEX size)
{
  if (size > 0) {
    queueBuffer = new uint8_t[size];
    os_handle = 1;
  }
  else {
    queueBuffer = NULL;
    os_handle = -1;
  }
  queueSize = size;
  queueLength = enqueuePos = dequeuePos = 0;
}


PQueueChannel::~PQueueChannel()
{
  Close();
}


bool PQueueChannel::Open(PINDEX size)
{
  Close();

  if (size == 0)
    return false;

  mutex.Wait();

  if ((queueBuffer = new uint8_t[size]) == NULL)
    return false;

  queueSize = size;
  queueLength = enqueuePos = dequeuePos = 0;
  os_handle = 1;

  mutex.Signal();

  return true;
}


bool PQueueChannel::Close()
{
  if (CheckNotOpen())
    return false;

  mutex.Wait();
  if (queueBuffer != NULL)
    delete [] queueBuffer;
  queueBuffer = NULL;
  os_handle = -1;
  mutex.Signal();
  unempty.Signal();
  unfull.Signal();
  return true;
}


bool PQueueChannel::Read(void * buf, PINDEX count)
{
  mutex.Wait();

  SetLastReadCount(0);

  if (CheckNotOpen()) {
    mutex.Signal();
    return false;
  }

  uint8_t * buffer = (uint8_t *)buf;

  /* If queue is empty then we should block for the time specifed in the
      read timeout.
    */
  while (queueLength == 0) {

    // unlock the data
    mutex.Signal();

    // block until data has arrived
    PTRACE_IF(6, readTimeout > 0, "QChan\tBlocking on empty queue");
    if (!unempty.Wait(readTimeout)) {
      PTRACE(6, "QChan\tRead timeout on empty queue");
      return SetErrorValues(Timeout, ETIMEDOUT, LastReadError);
    }

    // relock the data
    mutex.Wait();

    // check if the channel is still open
    if (CheckNotOpen()) {
      mutex.Signal();
      return SetErrorValues(NotOpen, EBADF, LastReadError);
    }
  }

  // should always have data now
  PAssert(queueLength > 0, "read queue signalled without data");

  while (queueLength > 0 && count > 0) {
    // To make things simpler, limit to amount to copy out of queue to till
    // the end of the linear part of memory. Another loop around will get
    // rest of data to dequeue
    PINDEX copyLen = queueSize - dequeuePos;

    // But do not copy more than has actually been queued
    if (copyLen > queueLength)
      copyLen = queueLength;

    // Or more than has been requested
    if (copyLen > count)
      copyLen = count;

    PAssert(copyLen > 0, "zero copy length");

    // Copy data out and increment pointer, decrement bytes yet to dequeue
    memcpy(buffer, queueBuffer+dequeuePos, copyLen);
    SetLastReadCount(GetLastReadCount() + copyLen);
    buffer += copyLen;
    count -= copyLen;

    // Move the queue pointer along, wrapping to beginning
    dequeuePos += copyLen;
    if (dequeuePos >= queueSize)
      dequeuePos = 0;

    // If buffer was full, signal possibly blocked write of data to queue
    // that it can write to queue now.
    if (queueLength == queueSize) {
      PTRACE(6, "QChan\tSignalling queue no longer full");
      unfull.Signal();
    }

    // Now decrement queue length by the amount we copied
    queueLength -= copyLen;
  }

  // unlock the buffer
  mutex.Signal();

  return true;
}


bool PQueueChannel::Write(const void * buf, PINDEX count)
{
  mutex.Wait();

  SetLastWriteCount(0);

  if (CheckNotOpen()) {
    mutex.Signal();
    return false;
  }

  const uint8_t * buffer = (uint8_t *)buf;
  PINDEX written = 0;
  while (count > 0) {
    /* If queue is full then we should block for the time specifed in the
        write timeout.
      */
    while (queueLength == queueSize) {
      mutex.Signal();

      PTRACE_IF(6, writeTimeout > 0, "QChan\tBlocking on full queue");
      if (!unfull.Wait(writeTimeout)) {
        PTRACE(6, "QChan\tWrite timeout on full queue");
        return SetErrorValues(Timeout, ETIMEDOUT, LastWriteError);
      }

      mutex.Wait();

      if (!IsOpen()) {
        mutex.Signal();
        return SetErrorValues(NotOpen, EBADF, LastWriteError);
      }
    }

    // Calculate number of bytes to copy
    PINDEX copyLen = count;

    // First don't copy more than are availble in queue
    PINDEX bytesLeftInQueue = queueSize - queueLength;
    if (copyLen > bytesLeftInQueue)
      copyLen = bytesLeftInQueue;

    // Then to make things simpler, limit to amount left till the end of the
    // linear part of memory. Another loop around will get rest of data to queue
    PINDEX bytesLeftInUnwrapped = queueSize - enqueuePos;
    if (copyLen > bytesLeftInUnwrapped)
      copyLen = bytesLeftInUnwrapped;

    PAssert(copyLen > 0, "attempt to write zero bytes");

    // Move the data in and increment pointer, decrement bytes yet to queue
    memcpy(queueBuffer + enqueuePos, buffer, copyLen);
    written += copyLen;
    buffer += copyLen;
    count -= copyLen;

    // Move the queue pointer along, wrapping to beginning
    enqueuePos += copyLen;
    if (enqueuePos >= queueSize)
      enqueuePos = 0;

    // see if we need to signal reader that queue was empty
    bool queueWasEmpty = queueLength == 0;

    // increment queue length by the amount we copied
    queueLength += copyLen;

    // signal reader if necessary
    if (queueWasEmpty) {
      PTRACE(6, "QChan\tSignalling queue no longer empty");
      unempty.Signal();
    }
  }

  mutex.Signal();

  SetLastWriteCount(written);
  return true;
}


// End of File ///////////////////////////////////////////////////////////////
