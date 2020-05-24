#ifndef _PVIDEOIOBSDCAPTURE

#define _PVIDEOIOBSDCAPTURE

#ifdef __GNUC__   
#pragma interface
#endif

#include <sys/mman.h>

#include <ptlib.h>
#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptclib/delaychan.h>

#if defined(P_FREEBSD)
#include <sys/param.h>
# if __FreeBSD_version >= 502100
#include <dev/bktr/ioctl_meteor.h>
# else
#include <machine/ioctl_meteor.h>
# endif
#endif

#if defined(P_OPENBSD) || defined(P_NETBSD)
#if P_OPENBSD >= 200105
#include <dev/ic/bt8xx.h>
#elif P_NETBSD >= 105000000
#include <dev/ic/bt8xx.h>
#else
#include <i386/ioctl_meteor.h>
#endif
#endif

#if !P_USE_INLINES
#include <ptlib/contain.inl>
#endif


class PVideoInputDevice_BSDCAPTURE : public PVideoInputDevice
{

  PCLASSINFO(PVideoInputDevice_BSDCAPTURE, PVideoInputDevice);

public:
  PVideoInputDevice_BSDCAPTURE();
  ~PVideoInputDevice_BSDCAPTURE();

  bool Open(
    const PString &deviceName,
    bool startImmediate = true
  );

  bool IsOpen();

  bool Close();

  bool Start();
  bool Stop();

  bool IsCapturing();

  static PStringList GetInputDeviceNames();

  PStringArray GetDeviceNames() const
  { return GetInputDeviceNames(); }

  PINDEX GetMaxFrameBytes();

  bool GetFrameSizeLimits(unsigned int&, unsigned int&,
			  unsigned int&, unsigned int&);

  bool TestAllFormats();

  bool SetFrameSize(unsigned int, unsigned int);
  bool SetFrameRate(unsigned int);
  bool VerifyHardwareFrameSize(unsigned int, unsigned int);

  bool SetColourFormat(const PString&);

//  bool SetVideoChannelFormat(int, PVideoDevice::VideoFormat);
  bool SetVideoFormat(PVideoDevice::VideoFormat);
  int GetNumChannels();
  bool SetChannel(int);

  bool NormalReadProcess(uint8_t*, PINDEX*);

  void ClearMapping();

protected:
  virtual bool InternalGetFrameData(uint8_t * buffer, PINDEX & bytesReturned, bool & keyFrame, bool wait);

  struct video_capability
  {
      int channels;   /* Num channels */
      int maxwidth;   /* Supported width */
      int maxheight;  /* And height */
      int minwidth;   /* Supported width */
      int minheight;  /* And height */
  };

  int    videoFd;
  struct video_capability videoCapability;
  int    canMap;  // -1 = don't know, 0 = no, 1 = yes
  uint8_t * videoBuffer;
  PINDEX frameBytes;
  int    mmap_size;
  PAdaptiveDelay m_pacing;
 
};

#endif
