/*
 * beaudio.h
 *
 * BeOS Sound driver class definitions.
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
 * Contributor(s): 
 * Yuri Kiryanov, ykiryanov at users.sourceforge.net
 * Bits by Jac Goudsmit
 *
 */

#include <ptlib.h>
#include <ptlib/dynalink.h>
#include <ptlib/sound.h>
#include <ptlib/socket.h>
#include <ptlib/plugin.h>


// Storage kit
#include <storage/Directory.h>
#include <storage/Entry.h>
#include <storage/File.h>

// Media kit 
#include <media/MediaDefs.h>
#include <media/MediaFormats.h>
#include <media/MediaFile.h>
#include <media/MediaTrack.h>
#include <media/SoundPlayer.h>
#include <media/PlaySound.h>

// Kernel kit
#include <kernel/OS.h>

// Beep kit :-)
#include <support/Beep.h>

#include "resampler.h"

#ifdef B_ZETA_VERSION
#define MEDIA_KIT_UPDATE
#endif

#ifdef MEDIA_KIT_UPDATE
#include <media/MediaRecorder.h>
#else
#include "beaudio/MediaRecorder.h"
#endif

////////////////////////////////////////////////////////////////////////////////
// PSoundChannelBeOS declaration

class CircularBuffer;

class PSoundChannelBeOS: public PSoundChannel
{
 public:
    PSoundChannelBeOS();
    void Construct();
    PSoundChannelBeOS(const PString &device,
                     PSoundChannel::Directions dir,
                     unsigned numChannels,
                     unsigned sampleRate,
                     unsigned bitsPerSample);
    ~PSoundChannelBeOS();
    static PStringArray GetDeviceNames(PSoundChannel::Directions = Player);
    static PString GetDefaultDevice(PSoundChannel::Directions);
    bool Open(
      const PString & device,
      Directions dir,
      unsigned numChannels,
      unsigned sampleRate,
      unsigned bitsPerSample
    );
    bool Setup();
    bool Close();
    bool IsOpen() const;
    bool Write(const void * buf, PINDEX len);
    bool Read(void * buf, PINDEX len);
    bool SetFormat(unsigned numChannels,
                   unsigned sampleRate,
                   unsigned bitsPerSample);
    unsigned GetChannels() const;
    unsigned GetSampleRate() const;
    unsigned GetSampleSize() const;
    bool SetBuffers(PINDEX size, PINDEX count);
    bool GetBuffers(PINDEX & size, PINDEX & count);
    bool PlaySound(const PSound & sound, bool wait);
    bool PlayFile(const PFilePath & filename, bool wait);
    bool HasPlayCompleted();
    bool WaitForPlayCompletion();
    bool RecordSound(PSound & sound);
    bool RecordFile(const PFilePath & filename);
    bool StartRecording();
    bool IsRecordBufferFull();
    bool AreAllRecordBuffersFull();
    bool WaitForRecordBufferFull();
    bool WaitForAllRecordBuffersFull();
    bool Abort();
    bool SetVolume(unsigned newVal);
    bool GetVolume(unsigned &devVol);

  public:
    // Overrides from class PChannel
    virtual PString GetName() const { return deviceName; }
    // Return the name of the channel.
  protected:
    PString     deviceName;
    Directions  direction;   

  private:
    // Only one of the following pointers can be non-NULL at a time.
    BMediaRecorder *mRecorder;
    BSoundPlayer   *mPlayer;

    // Raw media format specifier used for sound player.
    // It also stores the parameters (number of channels, sample rate etc) so
    // no need to store them separately here.
    // For the recorder, a media_format struct is created temporarily with
    // the data from this raw format spec.
    media_raw_audio_format    mFormat;

    // The class holds a circular buffer whose size is set with SetBuffers.
    // We only need one buffer for BeOS. The number of buffers that was set
    // is only kept for reference.
    friend class CircularBuffer;
    CircularBuffer *mBuffer;    // The internal buffer
    PINDEX mNumBuffers;       // for reference only!

    // Just some helpers so that the Open function doesn't get too big
    bool OpenPlayer(void);
    bool OpenRecorder(const PString &dev);

    // internal buffer setting function so we can disable the SetBuffers
    // function for debug purposes
    // size is the total size, threshold is the fill/drain threshold on
    // the buffer
    bool InternalSetBuffers(PINDEX size, PINDEX threshold);

    // Input resampler
    Resampler *mResampler;
};
