#include <ptlib.h>
#include <ptlib/sound.h>
#include <ptclib/delaychan.h>

class PAudioDelay : public PObject
{
  PCLASSINFO(PAudioDelay, PObject);

  public:
    PAudioDelay();
    bool Delay(int time);
    void Restart();
    int  GetError();

  protected:
    PTime  previousTime;
    bool   firstTime;
    int    error;
};

#define MIN_HEADROOM    30
#define MAX_HEADROOM    60

class SoundHandleEntry : public PObject {

  PCLASSINFO(SoundHandleEntry, PObject)

  public:
    SoundHandleEntry();

    int handle;
    int direction;

    unsigned numChannels;
    unsigned sampleRate;
    unsigned bitsPerSample;
    unsigned fragmentValue;
    bool isInitialised;
};

#define LOOPBACK_BUFFER_SIZE 5000
#define BYTESINBUF ((startptr<endptr)?(endptr-startptr):(LOOPBACK_BUFFER_SIZE+endptr-startptr))



class PSoundChannelSHM : public PSoundChannel {
 public:
  PSoundChannelSHM();
  ~PSoundChannelSHM();
  static PStringArray GetDeviceNames(PSoundChannel::Directions);
  static PString GetDefaultDevice(PSoundChannel::Directions);
  bool Open(const Params & params);
  bool Setup();
  bool Close();
  bool Write(const void * buf, PINDEX len);
  int writeSample( void *aData, int aLen );
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
  bool SetVolume (unsigned);
  bool GetVolume (unsigned &);
  bool IsOpen() const;

 private:

  static void UpdateDictionary(PSoundChannel::Directions);
  bool Volume (bool, unsigned, unsigned &);
  PString device;
  unsigned mNumChannels;
  unsigned mSampleRate;
  unsigned mBitsPerSample;
  bool isInitialised;

  void *os_handle;
  int card_nr;

  PMutex device_mutex;
  PAdaptiveDelay m_Pacing;


  /**number of 30 (or 20) ms long sound intervals stored by ALSA. Typically, 2.*/
  PINDEX storedPeriods;

  /**Total number of bytes of audio stored by ALSA.  Typically, 2*480 or 960.*/
  PINDEX storedSize;

  /** Number of bytes in a ALSA frame. a frame may only be 4ms long*/
  int frameBytes; 

};
