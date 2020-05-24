/*
 * dummyaudio.cxx
 *
 * Sound driver implementation.
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

#pragma implementation "sound.h"

#include <ptlib.h>

PSound::PSound(unsigned channels,
               unsigned samplesPerSecond,
               unsigned bitsPerSample,
               PINDEX   bufferSize,
               const uint8_t * buffer)
{
  encoding = 0;
  numChannels = channels;
  sampleRate = samplesPerSecond;
  sampleSize = bitsPerSample;
  SetSize(bufferSize);
  if (buffer != NULL)
    memcpy(GetPointer(), buffer, bufferSize);
}


PSound::PSound(const PFilePath & filename)
{
  encoding = 0;
  numChannels = 1;
  sampleRate = 8000;
  sampleSize = 16;
  Load(filename);
}


PSound & PSound::operator=(const PBYTEArray & data)
{
  PBYTEArray::operator=(data);
  return *this;
}


void PSound::SetFormat(unsigned channels,
                       unsigned samplesPerSecond,
                       unsigned bitsPerSample)
{
  encoding = 0;
  numChannels = channels;
  sampleRate = samplesPerSecond;
  sampleSize = bitsPerSample;
  formatInfo.SetSize(0);
}


bool PSound::Load(const PFilePath & /*filename*/)
{
  return false;
}


bool PSound::Save(const PFilePath & /*filename*/)
{
  return false;
}

bool PSound::Play(const PString & device)
{

  PSoundChannel channel(device,
                       PSoundChannel::Player);
  if (!channel.IsOpen())
    return false;

  return channel.PlaySound(*this, true);
}

///////////////////////////////////////////////////////////////////////////////

PSoundChannel::PSoundChannel()
{
  Construct();
}


PSoundChannel::PSoundChannel(const PString & device,
                             Directions dir,
                             unsigned numChannels,
                             unsigned sampleRate,
                             unsigned bitsPerSample)
{
  Construct();
  Open(device, dir, numChannels, sampleRate, bitsPerSample);
}


void PSoundChannel::Construct()
{
}


PSoundChannel::~PSoundChannel()
{
  Close();
}


PStringArray PSoundChannel::GetDeviceNames(Directions /*dir*/)
{
  PStringArray array;

  array[0] = "/dev/audio";
  array[1] = "/dev/dsp";

  return array;
}


PString PSoundChannel::GetDefaultDevice(Directions /*dir*/)
{
  return "/dev/audio";
}


bool PSoundChannel::Open(const PString & device,
                         Directions dir,
                         unsigned numChannels,
                         unsigned sampleRate,
                         unsigned bitsPerSample)
{
  Close();

  if (!ConvertOSError(os_handle = ::open(device, dir == Player ? O_RDONLY : O_WRONLY)))
    return false;

  return SetFormat(numChannels, sampleRate, bitsPerSample);
}


bool PSoundChannel::Close()
{
  return PChannel::Close();
}


bool PSoundChannel::SetFormat(unsigned numChannels,
                              unsigned sampleRate,
                              unsigned bitsPerSample)
{
  Abort();

  PAssert(numChannels >= 1 && numChannels <= 2, PInvalidParameter);
  PAssert(bitsPerSample == 8 || bitsPerSample == 16, PInvalidParameter);

  return true;
}


bool PSoundChannel::SetBuffers(PINDEX size, PINDEX count)
{
  Abort();

  PAssert(size > 0 && count > 0 && count < 65536, PInvalidParameter);

  return true;
}


bool PSoundChannel::GetBuffers(PINDEX & size, PINDEX & count)
{
  return true;
}


bool PSoundChannel::Write(const void * buffer, PINDEX length)
{
  return PChannel::Write(buffer, length);
}


bool PSoundChannel::PlaySound(const PSound & sound, bool wait)
{
  Abort();

  if (!Write((const uint8_t *)sound, sound.GetSize()))
    return false;

  if (wait)
    return WaitForPlayCompletion();

  return true;
}


bool PSoundChannel::PlayFile(const PFilePath & filename, bool wait)
{
  return true;
}


bool PSoundChannel::HasPlayCompleted()
{
  return true;
}


bool PSoundChannel::WaitForPlayCompletion()
{
  return true;
}


bool PSoundChannel::Read(void * buffer, PINDEX length)
{
  return PChannel::Read(buffer, length);
}


bool PSoundChannel::RecordSound(PSound & sound)
{
  return true;
}


bool PSoundChannel::RecordFile(const PFilePath & filename)
{
  return true;
}


bool PSoundChannel::StartRecording()
{
  return true;
}


bool PSoundChannel::IsRecordBufferFull()
{
  return true;
}


bool PSoundChannel::AreAllRecordBuffersFull()
{
  return true;
}


bool PSoundChannel::WaitForRecordBufferFull()
{
  if (os_handle < 0) {
    return false;
  }

  return PXSetIOBlock(PXReadBlock, readTimeout);
}


bool PSoundChannel::WaitForAllRecordBuffersFull()
{
  return false;
}


bool PSoundChannel::Abort()
{
  return true;
}

bool PSoundChannel::SetVolume(unsigned newVolume)
{
  return false;
}

bool  PSoundChannel::GetVolume(unsigned & volume)
{
  return false;
}


// End of file

