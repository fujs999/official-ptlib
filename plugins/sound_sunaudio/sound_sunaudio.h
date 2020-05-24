#ifndef SUNAUDIO_H 
#define SUNAUDIO_H

#include <ptlib.h>
#include <ptlib/sound.h>
#include <ptlib/socket.h>
#include <ptlib/plugin.h>

#include <sys/audio.h>

class PSoundChannelSunAudio: public PSoundChannel
{
 public:
    PSoundChannelSunAudio();
    ~PSoundChannelSunAudio();
    static PStringArray GetDeviceNames(PSoundChannel::Directions = Player);
    static PString GetDefaultDevice(PSoundChannel::Directions);
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

  protected:
    unsigned mNumChannels;
    unsigned mSampleRate;
    unsigned mBitsPerSample;
    unsigned actualSampleRate;
    PString device;
    bool isInitialised;
    unsigned resampleRate;

    /* save the default settings for resetting */
    /* play */
    unsigned mDefaultPlayNumChannels;
    unsigned mDefaultPlaySampleRate;
    unsigned mDefaultPlayBitsPerSample;

    /* record */
    unsigned mDefaultRecordNumChannels;
    unsigned mDefaultRecordSampleRate;
    unsigned mDefaultRecordBitsPerSample;
    unsigned mDefaultRecordEncoding;
    unsigned mDefaultRecordPort;
};

#endif
