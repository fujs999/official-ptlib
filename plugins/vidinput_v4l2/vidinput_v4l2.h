/*
 * vidinput_v4l2.h
 *
 * Portable Tools Library
 *
 * Copyright (c) 1993-2000 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Tools Library (V4L plugin).
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Derek Smithies (derek@indranet.co.nz)
 *                 Mark Cooke (mpc@star.sr.bham.ac.uk)
 *                 Nicola Orru' <nigu@itadinanta.it>
 *
 */
#ifndef _PVIDEOIOV4L2
#define _PVIDEOIOV4L2


#include <sys/mman.h>
#include <sys/time.h>

#include <ptlib.h>
#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptclib/delaychan.h>

#include V4L2_HEADER

#ifndef V4L2_PIX_FMT_SBGGR8
#define V4L2_PIX_FMT_SBGGR8  v4l2_fourcc('B','A','8','1') /*  8  BGBG.. GRGR.. */
#endif

class PVideoInputDevice_V4L2: public PVideoInputDevice
{

  PCLASSINFO(PVideoInputDevice_V4L2, PVideoInputDevice);
private:
  PVideoInputDevice_V4L2(const PVideoInputDevice_V4L2& ):readyToReadMutex(0,1) {};
  PVideoInputDevice_V4L2& operator=(const PVideoInputDevice_V4L2& ){ return *this; };
public:
  PVideoInputDevice_V4L2();
  virtual ~PVideoInputDevice_V4L2();

  static PStringList GetInputDeviceNames();

  PStringArray GetDeviceNames() const { return GetInputDeviceNames(); }

  bool Open(const PString &deviceName, bool startImmediate);

  bool IsOpen();

  bool Close();

  bool Start();
  bool Stop();

  bool IsCapturing();

  PINDEX GetMaxFrameBytes();

  bool GetFrameSizeLimits(unsigned int&, unsigned int&,
			  unsigned int&, unsigned int&);

  bool SetFrameSize(unsigned int, unsigned int);
  bool SetNearestFrameSize(unsigned int, unsigned int);
  bool SetFrameRate(unsigned int);

  bool SetColourFormat(const PString&);

  bool GetAttributes(Attributes & attrib);
  bool SetAttributes(const Attributes & attrib);

  bool SetVideoChannelFormat(int, PVideoDevice::VideoFormat);
  bool SetVideoFormat(PVideoDevice::VideoFormat);
  int GetNumChannels();
  bool SetChannel(int);

  /**Retrieve a list of Device Capabilities
    */
  virtual bool GetDeviceCapabilities(
    Capabilities * capabilities          ///< List of supported capabilities
  ) const;

  /**Retrieve a list of Device Capabilities for particular device
    */
  static bool GetDeviceCapabilities(
    const PString & deviceName,           ///< Name of device
    Capabilities * capabilities,          ///< List of supported capabilities
    PPluginManager * pluginMgr = NULL     ///< Plug in manager, use default if NULL
  );

private:
  virtual bool InternalGetFrameData(uint8_t * buffer, PINDEX & bytesReturned, bool & keyFrame, bool wait);

  int GetControlCommon(unsigned int control, int *value);
  bool SetControlCommon(unsigned int control, int newValue);
  bool NormalReadProcess(uint8_t*, PINDEX*);

  void Reset();
  void ClearMapping();
  bool SetMapping();

  bool VerifyHardwareFrameSize(unsigned int & width, unsigned int & height);
  bool TryFrameSize(unsigned int& width, unsigned int& height);

  bool QueueAllBuffers();

  bool StartStreaming();
  void StopStreaming();

  bool DoIOCTL(unsigned long int r, void * s, int structSize, bool retryOnBusy=false);

  bool EnumFrameFormats(Capabilities & capabilities) const;
  bool EnumControls(Capabilities & capabilities) const;

  struct v4l2_capability videoCapability;
  struct v4l2_streamparm videoStreamParm;
  bool   canRead;
  bool   canStream;
  bool   canSelect;
  bool   canSetFrameRate;
  bool   isMapped;
#define NUM_VIDBUF 4
  uint8_t * videoBuffer[NUM_VIDBUF];
  uint   videoBufferCount;
  uint   currentVideoBuffer;

  PSemaphore readyToReadMutex;			/** Allow frame reading only from the time Start() used until Stop() */
  PMutex inCloseMutex;				/** Prevent InternalGetFrameData() to stuck on readyToReadMutex in the middle of device closing operation */
  bool isOpen;				/** Has the Video Input Device successfully been opened? */
  bool areBuffersQueued;
  bool isStreaming;

  int    videoFd;
  int    frameBytes;
  bool   started;
  PAdaptiveDelay m_pacing;
  PString userFriendlyDevName;
};

#endif
