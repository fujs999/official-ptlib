/*
 * shmvideo.cxx
 *
 * This file contains the class hierarchy for both shared memory video
 * input and output devices.
 *
 * Copyright (c) 2003 Pai-Hsiang Hsiao
 * Copyright (c) 1998-2003 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 */

#include <ptlib.h>

#define P_FORCE_STATIC_PLUGIN 1

#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptlib/unix/ptlib/shmvideo.h>


class PColourConverter;

static const char *
ShmKeyFileName()
{
  return "/dev/null";
}

bool
PVideoOutputDevice_Shm::shmInit()
{
  semLock = sem_open(SEM_NAME_OF_OUTPUT_DEVICE,
		     O_RDWR, S_IRUSR|S_IWUSR, 0);

  if (semLock != (sem_t *)SEM_FAILED) {
    shmKey = ftok(ShmKeyFileName(), 0);
    if (shmKey >= 0) {
      shmId = shmget(shmKey, SHMVIDEO_BUFSIZE, 0666);
      if (shmId >= 0) {
        shmPtr = shmat(shmId, NULL, 0);
        if (shmPtr) {
          return true;
        }
        else {
          PTRACE(1, "SHMV\t shmInit can not attach shared memory" << endl);
          shmctl(shmId, IPC_RMID, NULL);
          sem_close(semLock);
        }
      }
      else {
        PTRACE(1, "SHMV\t shmInit can not find the shared memory" << endl);
        sem_close(semLock);
      }
    }
    else {
      PTRACE(1, "SHMV\t shmInit can not create key for shared memory" << endl);
      sem_close(semLock);
    }
  }
  else {
    PTRACE(1, "SHMV\t shmInit can not create semaphore" << endl);
  }

  semLock = (sem_t *)SEM_FAILED;
  shmKey = -1;
  shmId = -1;
  shmPtr = NULL;

  return false;
}

PVideoOutputDevice_Shm::PVideoOutputDevice_Shm()
{
	colourFormat = "RGB24";
	bytesPerPixel = 3;
	frameStore.SetSize(frameWidth * frameHeight * bytesPerPixel);
	
  semLock = (sem_t *)SEM_FAILED;
  shmKey = -1;
  shmId = -1;
  shmPtr = NULL;

  PTRACE(6, "SHMV\t Constructor of PVideoOutputDevice_Shm");
}

bool PVideoOutputDevice_Shm::SetColourFormat(const PString & colourFormat)
{
	if( colourFormat == "RGB32")
		bytesPerPixel = 4;
	else if(colourFormat == "RGB24")
		bytesPerPixel = 3;
	else
		return false;
	
	return PVideoOutputDevice::SetColourFormat(colourFormat) && SetFrameSize(frameWidth, frameHeight);
}

bool PVideoOutputDevice_Shm::SetFrameSize(unsigned width, unsigned height)
{
	if (!PVideoOutputDevice::SetFrameSize(width, height))
		return false;
	
	return frameStore.SetSize(frameWidth*frameHeight*bytesPerPixel);
}

PINDEX PVideoOutputDevice_Shm::GetMaxFrameBytes()
{
	return frameStore.GetSize();
}

bool PVideoOutputDevice_Shm::SetFrameData(unsigned x, unsigned y,
                                         unsigned width, unsigned height,
                                         const uint8_t * data,
                                         bool endFrame)
{
	if (x+width > frameWidth || y+height > frameHeight)
		return false;
	
	if (x == 0 && width == frameWidth && y == 0 && height == frameHeight) {
		if (converter != NULL)
			converter->Convert(data, frameStore.GetPointer());
		else
			memcpy(frameStore.GetPointer(), data, height*width*bytesPerPixel);
	}
	else {
		if (converter != NULL) {
			PAssertAlways("Converted output of partial RGB frame not supported");
			return false;
		}
		
		if (x == 0 && width == frameWidth)
			memcpy(frameStore.GetPointer() + y*width*bytesPerPixel, data, height*width*bytesPerPixel);
		else {
			for (unsigned dy = 0; dy < height; dy++)
				memcpy(frameStore.GetPointer() + ((y+dy)*width + x)*bytesPerPixel,
					   data + dy*width*bytesPerPixel, width*bytesPerPixel);
		}
	}
	
	if (endFrame)
		return EndFrame();
	
	return true;
}

bool
PVideoOutputDevice_Shm::Open(const PString & name,
			   bool /*startImmediate*/)
{
  PTRACE(1, "SHMV\t Open of PVideoOutputDevice_Shm");

  Close();

  if (shmInit() == true) {
    deviceName = name;
    return true;
  }
  else {
    return false;
  }
}

bool
PVideoOutputDevice_Shm::IsOpen()
{
  if (semLock != (sem_t *)SEM_FAILED) {
    return true;
  }
  else {
    return false;
  }
}

bool
PVideoOutputDevice_Shm::Close()
{
  if (semLock != (sem_t *)SEM_FAILED) {
    shmdt(shmPtr);
    sem_close(semLock);
    shmPtr = NULL;
  }
  return true;
}

PStringArray
PVideoOutputDevice_Shm::GetDeviceNames() const
{
  return PString("shm");
}

bool
PVideoOutputDevice_Shm::EndFrame()
{
  long *ptr = (long *)shmPtr;

  if (semLock == (sem_t *)SEM_FAILED) {
    return false;
  }

  if (bytesPerPixel != 3 && bytesPerPixel != 4) {
    PTRACE(1, "SHMV\t EndFrame() does not handle bytesPerPixel!={3,4}"<<endl);
    return false;
  }

  if (frameWidth*frameHeight*bytesPerPixel > SHMVIDEO_FRAMESIZE) {
    return false;
  }

  // write header info so the consumer knows what to expect
  ptr[0] = frameWidth;
  ptr[1] = frameHeight;
  ptr[2] = bytesPerPixel;

  PTRACE(1, "writing " << frameStore.GetSize() << " bytes" << endl);
  if (memcpy((char *)shmPtr+sizeof(long)*3,
             frameStore, frameStore.GetSize()) == NULL) {
    return false;
  }

  sem_post(semLock);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

PCREATE_VIDINPUT_PLUGIN(Shm);

bool
PVideoInputDevice_Shm::shmInit()
{
  semLock = sem_open(SEM_NAME_OF_INPUT_DEVICE,
		     O_RDWR, S_IRUSR|S_IWUSR, 0);

  if (semLock != (sem_t *)SEM_FAILED) {
    shmKey = ftok(ShmKeyFileName(), 100);
    if (shmKey >= 0) {
      shmId = shmget(shmKey, SHMVIDEO_BUFSIZE, 0666);
      if (shmId >= 0) {
        shmPtr = shmat(shmId, NULL, 0);
        if (shmPtr) {
          return true;
        }
        else {
          PTRACE(1, "SHMV\t shmInit can not attach shared memory" << endl);
          shmctl(shmId, IPC_RMID, NULL);
          sem_close(semLock);
        }
      }
      else {
        PTRACE(1, "SHMV\t shmInit can not find the shared memory" << endl);
        sem_close(semLock);
      }
    }
    else {
      PTRACE(1, "SHMV\t shmInit can not create key for shared memory" << endl);
      sem_close(semLock);
    }
  }
  else {
    PTRACE(1, "SHMV\t shmInit can not create semaphore" << endl);
  }

  semLock = (sem_t *)SEM_FAILED;
  shmKey = -1;
  shmId = -1;
  shmPtr = NULL;

  return false;
}

PVideoInputDevice_Shm::PVideoInputDevice_Shm()
{
  semLock = (sem_t *)SEM_FAILED;
  shmKey = -1;
  shmId = -1;
  shmPtr = NULL;

  PTRACE(4, "SHMV\t Constructor of PVideoInputDevice_Shm");
}

bool
PVideoInputDevice_Shm::Open(const PString & name,
			  bool /*startImmediate*/)
{
  PTRACE(1, "SHMV\t Open of PVideoInputDevice_Shm");

  Close();

  if (shmInit() == true) {
    deviceName = name;
    return true;
  }
  else {
    return false;
  }
}

bool
PVideoInputDevice_Shm::IsOpen()
{
  if (semLock != (sem_t *)SEM_FAILED) {
    return true;
  }
  else {
    return false;
  }
}

bool
PVideoInputDevice_Shm::Close()
{
  if (semLock != (sem_t *)SEM_FAILED) {
    shmdt(shmPtr);
    sem_close(semLock);
    shmPtr = NULL;
  }
  return true;
}

bool PVideoInputDevice_Shm::IsCapturing()
{
	return true;
}

PINDEX PVideoInputDevice_Shm::GetMaxFrameBytes()
{
	return videoFrameSize;
}

PStringArray
PVideoInputDevice_Shm::GetInputDeviceNames()
{
  return PString("shm");
}

bool
PVideoInputDevice_Shm::GetFrameSizeLimits(unsigned & minWidth,
					unsigned & minHeight,
					unsigned & maxWidth,
					unsigned & maxHeight) 
{
  minWidth  = 176;
  minHeight = 144;
  maxWidth  = 352;
  maxHeight =  288;

  return true;
}

static void RGBtoYUV420PSameSize (const uint8_t *, uint8_t *, unsigned, bool, 
                                  int, int);


#define rgbtoyuv(r, g, b, y, u, v) \
  y=(uint8_t)(((int)30*r  +(int)59*g +(int)11*b)/100); \
  u=(uint8_t)(((int)-17*r  -(int)33*g +(int)50*b+12800)/100); \
  v=(uint8_t)(((int)50*r  -(int)42*g -(int)8*b+12800)/100); \



static void RGBtoYUV420PSameSize (const uint8_t * rgb,
                                  uint8_t * yuv,
                                  unsigned rgbIncrement,
                                  bool flip, 
                                  int srcFrameWidth, int srcFrameHeight) 
{
  const unsigned planeSize = srcFrameWidth*srcFrameHeight;
  const unsigned halfWidth = srcFrameWidth >> 1;
  
  // get pointers to the data
  uint8_t * yplane  = yuv;
  uint8_t * uplane  = yuv + planeSize;
  uint8_t * vplane  = yuv + planeSize + (planeSize >> 2);
  const uint8_t * rgbIndex = rgb;

  for (int y = 0; y < (int) srcFrameHeight; y++) {
    uint8_t * yline  = yplane + (y * srcFrameWidth);
    uint8_t * uline  = uplane + ((y >> 1) * halfWidth);
    uint8_t * vline  = vplane + ((y >> 1) * halfWidth);

    if (flip)
      rgbIndex = rgb + (srcFrameWidth*(srcFrameHeight-1-y)*rgbIncrement);

    for (int x = 0; x < (int) srcFrameWidth; x+=2) {
      rgbtoyuv(rgbIndex[0], rgbIndex[1], rgbIndex[2],*yline, *uline, *vline);
      rgbIndex += rgbIncrement;
      yline++;
      rgbtoyuv(rgbIndex[0], rgbIndex[1], rgbIndex[2],*yline, *uline, *vline);
      rgbIndex += rgbIncrement;
      yline++;
      uline++;
      vline++;
    }
  }
}


bool PVideoInputDevice_Shm::InternalGetFrameData(uint8_t * buffer, PINDEX & bytesReturned, bool & keyFrame, bool wait)
{
  if (wait)
    m_pacing.Delay(1000/GetFrameRate());

  long *bufPtr = (long *)shmPtr;

  unsigned width = 0;
  unsigned height = 0;
  unsigned rgbIncrement = 4;

  GetFrameSize (width, height);

  bufPtr[0] = width;
  bufPtr[1] = height;

  if (semLock != (sem_t *)SEM_FAILED && sem_trywait(semLock) == 0) {
    if (bufPtr[0] == (long)width && bufPtr[1] == (long)height) {
      rgbIncrement = bufPtr[2];
      RGBtoYUV420PSameSize ((uint8_t *)(bufPtr+3), buffer, rgbIncrement, false, 
			    width, height);
	  
	  bytesReturned = videoFrameSize;
      return true;
    }
  }

  return false;
}

bool PVideoInputDevice_Shm::TestAllFormats()
{
	return true;
}

bool PVideoInputDevice_Shm::Start()
{
	return true;
}

bool PVideoInputDevice_Shm::Stop()
{
	return true;
}

#endif
