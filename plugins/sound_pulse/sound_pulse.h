
#include <ptlib.h>
#include <ptlib/sound.h>
#include <ptlib/socket.h>

//#if !P_USE_INLINES
//#include <ptlib/contain.inl>
//#endif

#ifdef P_LINUX
#include <sys/soundcard.h>
#endif

#ifdef P_FREEBSD
#if P_FREEBSD >= 500000
#include <sys/soundcard.h>
#else
#include <machine/soundcard.h>
#endif
#endif

#if defined(P_OPENBSD) || defined(P_NETBSD)
#include <soundcard.h>
#endif

#include <pulse/stream.h>

class PSoundChannelPulse: public PSoundChannel
{
 public:
    PSoundChannelPulse();
    ~PSoundChannelPulse();
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
    bool HasPlayCompleted();
    bool WaitForPlayCompletion();
    bool StartRecording();
    bool IsRecordBufferFull();
    bool AreAllRecordBuffersFull();
    bool WaitForRecordBufferFull();
    bool WaitForAllRecordBuffersFull();
    bool Abort()   { return true; }
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

    PINDEX bufferSize;
    PINDEX bufferCount;

  pa_sample_spec ss;

  pa_stream *s;
  const void* record_data;
  size_t record_len;

  // This locks all access of this channel to this device. Thus, there
  // is only one activity hitting this instance of sound. if this
  // sound channel instance is used for read, it does not interfere
  // with anything related to write
  PMutex deviceMutex;
};
