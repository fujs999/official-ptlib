/*
 * dummyvideo.cxx
 *
 * Classes to support streaming video input (grabbing) and output.
 *
 * Portable Tools Library
 *
 * Copyright (c) 1993-2001 Equivalence Pty. Ltd.
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
 * Contributor(s): Roger Hardiman <roger@freebsd.org>
 *
 */

#pragma implementation "videoio.h"

#include <ptlib.h>
#include <ptlib/videoio.h>
#include <ptlib/vfakeio.h>
#include <ptlib/vconvert.h>

///////////////////////////////////////////////////////////////////////////////
// PVideoInputDevice

PVideoInputDevice::PVideoInputDevice()
{
}


bool PVideoInputDevice::Open(const PString & devName, bool startImmediate)
{
  return false;    
}


bool PVideoInputDevice::IsOpen() 
{
  return false;    
}


bool PVideoInputDevice::Close()
{
  return false;    
}


bool PVideoInputDevice::Start()
{
  return false;
}


bool PVideoInputDevice::Stop()
{
  return false;
}


bool PVideoInputDevice::IsCapturing()
{
  return false;
}


bool PVideoInputDevice::SetVideoFormat(VideoFormat newFormat)
{
  return false;
}


int PVideoInputDevice::GetBrightness()
{
  return -1;
}


bool PVideoInputDevice::SetBrightness(unsigned newBrightness)
{
  return false;
}


int PVideoInputDevice::GetHue()
{
  return -1;
}


bool PVideoInputDevice::SetHue(unsigned newHue)
{
  return false;
}


int PVideoInputDevice::GetContrast()
{
  return -1;
}


bool PVideoInputDevice::SetContrast(unsigned newContrast)
{
  return false;
}


bool PVideoInputDevice::GetParameters (int *whiteness, int *brightness,
                                       int *colour, int *contrast, int *hue)
{
  return false;
}


int PVideoInputDevice::GetNumChannels() 
{
  return 0;
}


bool PVideoInputDevice::SetChannel(int newChannel)
{
  return false;
}


bool PVideoInputDevice::SetColourFormat(const PString & newFormat)
{
  return false;
}


bool PVideoInputDevice::SetFrameRate(unsigned rate)
{
  return false;
}


bool PVideoInputDevice::GetFrameSizeLimits(unsigned & minWidth,
                                           unsigned & minHeight,
                                           unsigned & maxWidth,
                                           unsigned & maxHeight) 
{
  return false;
}


bool PVideoInputDevice::SetFrameSize(unsigned width, unsigned height)
{
  return false;
}


PINDEX PVideoInputDevice::GetMaxFrameBytes()
{
  return 0;
}



bool PVideoInputDevice::InternalGetFrameData(uint8_t * buffer, PINDEX & bytesReturned, bool & keyFrame, bool wait)
{
  return false;
}


void PVideoInputDevice::ClearMapping()
{
}

bool PVideoInputDevice::VerifyHardwareFrameSize(unsigned width,
                                                unsigned height)
{
	// Assume the size is valid
	return true;
}

bool PVideoInputDevice::TestAllFormats()
{
  return true;
}
    
// End Of File ///////////////////////////////////////////////////////////////
