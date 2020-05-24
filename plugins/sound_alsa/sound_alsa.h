#include <ptlib.h>
#include <ptlib/sound.h>

#define ALSA_PCM_NEW_HW_PARAMS_API 1
#include <alsa/asoundlib.h>


class PSoundChannelALSA : public PSoundChannel {
 public:
  PSoundChannelALSA();
  ~PSoundChannelALSA();
  static PStringArray GetDeviceNames(PSoundChannel::Directions);
  static PString GetDefaultDevice(PSoundChannel::Directions);
  bool Open(const Params & params);
  PString GetName() const { return device; }
  bool Close();
  bool Write(const void * buf, PINDEX len);
  bool Read(void * buf, PINDEX len);
  bool SetFormat(unsigned numChannels, unsigned sampleRate, unsigned bitsPerSample);
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
  bool Abort();
  bool SetVolume(unsigned);
  bool GetVolume(unsigned &);
  bool IsOpen() const;

 private:
  static void UpdateDictionary(PSoundChannel::Directions);
  bool SetHardwareParams();
  bool Volume(bool, unsigned, unsigned &);

  PString device;
  unsigned mNumChannels;
  unsigned mSampleRate;
  unsigned mBitsPerSample;
  bool isInitialised;

  snd_pcm_t *pcm_handle; /* Handle, different from the PChannel handle */
  int card_nr;

  PMutex device_mutex;

  PINDEX m_bufferSize;
  PINDEX m_bufferCount;

  /** Number of bytes in a ALSA frame. a frame may only be 4ms long*/
  int frameBytes; 
};
