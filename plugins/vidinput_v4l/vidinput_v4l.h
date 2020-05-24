
#include <sys/mman.h>
#include <sys/time.h>

#include <ptlib.h>
#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptclib/delaychan.h>

#include <linux/videodev.h>

class PVideoInputDevice_V4L: public PVideoInputDevice
{

public:
  PVideoInputDevice_V4L();
  ~PVideoInputDevice_V4L();

  static PStringList GetInputDeviceNames();

  PStringArray GetDeviceNames() const
  { return GetInputDeviceNames(); }

  bool Open(const PString &deviceName, bool startImmediate);

  bool IsOpen();

  bool Close();

  bool Start();
  bool Stop();

  bool IsCapturing();

  PINDEX GetMaxFrameBytes();

  bool GetFrameSizeLimits(unsigned int&, unsigned int&,
			  unsigned int&, unsigned int&);

  bool TestAllFormats();

  bool SetFrameSize(unsigned int, unsigned int);
  bool SetFrameRate(unsigned int);
  bool VerifyHardwareFrameSize(unsigned int, unsigned int);

  bool GetAttributes(Attributes & attrib);
  bool SetAttributes(const Attributes & attrib);

  bool SetColourFormat(const PString&);

  bool SetVideoChannelFormat(int, PVideoDevice::VideoFormat);
  bool SetVideoFormat(PVideoDevice::VideoFormat);
  int GetNumChannels();
  bool SetChannel(int);

  bool NormalReadProcess(uint8_t*, PINDEX*);

  void ClearMapping();
  bool RefreshCapabilities();

protected:
  virtual bool InternalGetFrameData(uint8_t * buffer, PINDEX & bytesReturned, bool & keyFrame, bool wait);

  PAdaptiveDelay m_pacing;

  int    videoFd;
  struct video_capability videoCapability;
  int    canMap;  // -1 = don't know, 0 = no, 1 = yes
  int    colourFormatCode;
  PINDEX hint_index;
  uint8_t *videoBuffer;
  PINDEX frameBytes;
  
  bool   pendingSync[2];
  
  int    currentFrame;
  struct video_mbuf frame;
  struct video_mmap frameBuffer[2];
  
};
