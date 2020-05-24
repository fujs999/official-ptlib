
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

class PSoundChannelESD: public PSoundChannel
{
 public:
    PSoundChannelESD();
    ~PSoundChannelESD();
    static PStringArray GetDeviceNames(PSoundChannel::Directions = Player);
    static PString GetDefaultDevice(PSoundChannel::Directions);
    bool Open(const Params & params);
    bool Close();
    PString GetName() const { return GetDefaultDevice(m_activeDirection); }
    bool Write(const void * buf, PINDEX len);
    bool Read(void * buf, PINDEX len);
    bool SetFormat(unsigned numChannels,
                   unsigned sampleRate,
                   unsigned bitsPerSample);
    unsigned GetChannels() const { return mNumChannels; }
    unsigned GetSampleRate() const { return mSampleRate; }
    unsigned GetSampleSize() const { return mBitsPerSample; }
    bool SetBuffers(PINDEX size, PINDEX count);
    bool GetBuffers(PINDEX & size, PINDEX & count);
    bool HasPlayCompleted();
    bool WaitForPlayCompletion();
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
};
