/*
 * sound.h
 *
 * Sound class.
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
// PSound

#ifndef _PSOUND_WIN32
#define _PSOUND_WIN32

#include <mmsystem.h>
#ifndef _WIN32_WCE
#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif
#endif

class PWaveFormat : public PObject
{
  PCLASSINFO(PWaveFormat, PObject)
  public:
    PWaveFormat();
    ~PWaveFormat();
    PWaveFormat(const PWaveFormat & fmt);
    PWaveFormat & operator=(const PWaveFormat & fmt);

    void PrintOn(ostream &) const;
    void ReadFrom(istream &);

    void SetFormat(unsigned numChannels, unsigned sampleRate, unsigned bitsPerSample);
    void SetFormat(const void * data, PINDEX size);

    bool           SetSize   (PINDEX sz);
    PINDEX         GetSize   () const { return  size;       }
    void         * GetPointer() const { return  waveFormat; }
    WAVEFORMATEX * operator->() const { return  waveFormat; }
    WAVEFORMATEX & operator *() const { return *waveFormat; }
    operator   WAVEFORMATEX *() const { return  waveFormat; }

  protected:
    PINDEX         size;
    WAVEFORMATEX * waveFormat;
};


class PMultiMediaFile
{
  public:
    PMultiMediaFile();
    ~PMultiMediaFile();

    bool CreateWaveFile(const PFilePath & filename,
                        const PWaveFormat & waveFormat,
                        uint32_t dataSize);
    bool OpenWaveFile(const PFilePath & filename,
                      PWaveFormat & waveFormat,
                      uint32_t & dataSize);

    bool Open(const PFilePath & filename, uint32_t dwOpenFlags, LPMMIOINFO lpmmioinfo = NULL);
    bool Close(UINT wFlags = 0);
    bool Ascend(MMCKINFO & ckinfo, UINT wFlags = 0);
    bool Descend(UINT wFlags, MMCKINFO & ckinfo, LPMMCKINFO lpckParent = NULL);
    bool Read(void * data, PINDEX len);
    bool CreateChunk(MMCKINFO & ckinfo, UINT wFlags = 0);
    bool Write(const void * data, PINDEX len);

    uint32_t GetLastError() const { return dwLastError; }

  protected:
    HMMIO hmmio;
    uint32_t dwLastError;
};


class PWaveBuffer : public PBYTEArray
{
  PCLASSINFO(PWaveBuffer, PBYTEArray);
  private:
    PWaveBuffer(PINDEX sz = 0);
    ~PWaveBuffer();

    PWaveBuffer & operator=(const PSound & sound);

    uint32_t Prepare(HWAVEOUT hWaveOut, PINDEX & count);
    uint32_t Prepare(HWAVEIN hWaveIn);
    uint32_t Release();

    void PrepareCommon(PINDEX count);

    HWAVEOUT hWaveOut;
    HWAVEIN  hWaveIn;
    WAVEHDR  header;

  friend class PSoundChannelWin32;
};

typedef PArray<PWaveBuffer> PWaveBufferArray;


class PSoundChannelWin32: public PSoundChannel
{
 public:
    static const char * GetDriverName();
    PSoundChannelWin32();
    ~PSoundChannelWin32();
    static PString GetDefaultDevice(Directions dir);
    static PStringArray GetDeviceNames(PSoundChannel::Directions = Player);
    bool Open(const Params & params);
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
    bool SetMute(bool mute);
    bool GetMute(bool & mute);

  public:
    // Overrides from class PChannel
    virtual PString GetName() const;
      // Return the name of the channel.

      
    PString GetErrorText(ErrorGroup group = NumErrorGroups) const;
    // Get a text form of the last error encountered.

    bool SetFormat(const PWaveFormat & format);

    bool Open(const PString & device, Directions dir,const PWaveFormat & format);
    // Open with format other than PCM

  protected:
    PString      deviceName;
    HWAVEIN      hWaveIn;
    HWAVEOUT     hWaveOut;
    HMIXER       hMixer;
    MIXERCONTROL volumeControl;
    MIXERCONTROL muteControl;
    HANDLE       hEventDone;
    PWaveFormat  waveFormat;
    bool         opened;
    bool         m_reopened;

    PWaveBufferArray buffers;
    PINDEX           bufferIndex;
    PINDEX           bufferByteOffset;
    PDECLARE_MUTEX(  bufferMutex);

  private:
    bool OpenDevice(P_INT_PTR id);
    bool GetDeviceID(const PString & device, Directions dir, unsigned& id);
    int WaitEvent(ErrorGroup group);
};


#endif

// End Of File ///////////////////////////////////////////////////////////////
