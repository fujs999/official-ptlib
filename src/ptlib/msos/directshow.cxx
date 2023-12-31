/*
 * directshow.cxx
 *
 * DirectShow Implementation for the H323Plus/OPAL Project.
 *
 * Copyright (c) 2009 ISVO (Asia) Pte Ltd. All Rights Reserved.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the General Public License (the  "GNU License"), in which case the
 * provisions of GNU License are applicable instead of those
 * above. If you wish to allow use of your version of this file only
 * under the terms of the GNU License and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GNU License. If you do not delete
 * the provisions above, a recipient may use your version of this file
 * under either the MPL or the GNU License."
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * Initial work sponsored by Requestec Ltd.
 *
 * Contributor(s): ______________________________________.
 */


#include <ptlib.h>

#ifdef P_DIRECTSHOW

#define P_FORCE_STATIC_PLUGIN 1

#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptclib/guid.h>

#include <ptlib/msos/ptlib/pt_atl.h>

#include <functional>


#define PTraceModule() "DShow"


/* workaround a compile error with mingw-w64 on sprintf member function
    below. Even though the member function is not the same thing as the
    global function which _is_ deprecated.

    Also applies to a warning in MSVC.

    Need to define this before dshow.h inclusion.
  */
#define STRSAFE_NO_DEPRECATE

#include <dshow.h>
#include <ks.h>
#include <ksmedia.h>


#ifdef P_DIRECTSHOW_QEDIT_H

  // Use this to avoid compile error in Qedit.h with DirectX SDK
  #define __IDxtCompositor_INTERFACE_DEFINED__
  #define __IDxtAlphaSetter_INTERFACE_DEFINED__
  #define __IDxtJpeg_INTERFACE_DEFINED__
  #define __IDxtKey_INTERFACE_DEFINED__

  #pragma include_alias("dxtrans.h", "ptlib/msos/dxtrans.h")

  #include <rpcsal.h>
  #include P_DIRECTSHOW_QEDIT_H

#else

  extern "C" {
    extern const CLSID CLSID_SampleGrabber;
    extern const IID IID_ISampleGrabber;
    extern const IID IID_ISampleGrabberCB;
    extern const CLSID CLSID_NullRenderer;
    extern const CLSID CLSID_CameraName;
  };


  #undef INTERFACE
  #define INTERFACE ISampleGrabberCB
  DECLARE_INTERFACE_(ISampleGrabberCB, IUnknown)
  {
    STDMETHOD_(HRESULT, SampleCB)(THIS_ double, IMediaSample *) PURE;
    STDMETHOD_(HRESULT, BufferCB)(THIS_ double, BYTE *, long) PURE;
  };

  #undef INTERFACE
  #define INTERFACE ISampleGrabber

  DECLARE_INTERFACE_(ISampleGrabber,IUnknown)
  {
    STDMETHOD_(HRESULT, SetOneShot)(THIS_ BOOL) PURE;
    STDMETHOD_(HRESULT, SetMediaType)(THIS_ AM_MEDIA_TYPE *) PURE;
    STDMETHOD_(HRESULT, GetConnectedMediaType)(THIS_ AM_MEDIA_TYPE *) PURE;
    STDMETHOD_(HRESULT, SetBufferSamples)(THIS_ BOOL) PURE;
    STDMETHOD_(HRESULT, GetCurrentBuffer)(THIS_ long *, long *) PURE;
    STDMETHOD_(HRESULT, GetCurrentSample)(THIS_ IMediaSample *) PURE;
    STDMETHOD_(HRESULT, SetCallback)(THIS_ ISampleGrabberCB *, long) PURE;
  };

#endif // P_DIRECTSHOW_QEDIT_H

#ifdef _MSC_VER
  #pragma comment(lib, "quartz.lib")
#endif

class PSampleGrabberCB : public ISampleGrabberCB 
{
  private:
    PBYTEArray m_buffer;
    bool       m_stopped;
    PINDEX     m_actualSize;
    PDECLARE_MUTEX(m_mutex);
    PSyncPoint m_frameReady;
    PINDEX     m_skipInitialGrabs;
#if PTRACING
    unsigned     m_totalFrames;
    PSimpleTimer m_totalTime;
#endif

  public:
    PSampleGrabberCB();
    ~PSampleGrabberCB();

    void Start();
    void Stop();

    // Fake out any COM ref counting
    STDMETHODIMP_(ULONG) AddRef() { return 2; }
    STDMETHODIMP_(ULONG) Release() { return 1; }

    // Fake out any COM QI'ing
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);

    // We don't implement this one
    STDMETHODIMP SampleCB( double /*SampleTime*/, IMediaSample * /*pSample*/) { return 0; }

    // The sample grabber is calling us back on its deliver thread.
    STDMETHODIMP BufferCB(double PTRACE_PARAM(dblSampleTime), BYTE * buffer, long size);

    bool GetData(BYTE * data, PINDEX maxSize, PINDEX & actualSize, bool wait);
};


#ifdef _MSC_VER
  #pragma comment(lib, "strmiids.lib")
#endif


static long const InputControlPropertyCode[PVideoControlInfo::NumTypes] = {
  CameraControl_Pan, CameraControl_Tilt, CameraControl_Zoom, CameraControl_Focus
};


//////////////////////////////////////////////////////////////////////
// Video Input device

class PVideoInputDevice_DirectShow : public PVideoInputDevice
{
    PCLASSINFO(PVideoInputDevice_DirectShow, PVideoInputDevice);

  public:
    PVideoInputDevice_DirectShow();
    ~PVideoInputDevice_DirectShow();


    static PStringArray GetInputDeviceNames();
    virtual PStringArray GetDeviceNames() const;
    static PBoolean GetInputDeviceCapabilities(const PString & deviceName, Capabilities * capabilities);
    virtual bool GetDeviceCapabilities(Capabilities * capabilities) const;
    virtual bool SetControl(PVideoControlInfo::Types type, int value, ControlMode mode);

    virtual PBoolean Open(const PString & deviceName, PBoolean startImmediate);
    virtual PBoolean IsOpen();
    virtual PBoolean Close();
    virtual PBoolean Start();
    virtual PBoolean Stop();
    virtual PBoolean IsCapturing();
    virtual PBoolean SetColourFormat(const PString & colourFormat);
    virtual PBoolean SetFrameRate(unsigned rate);
    virtual PBoolean SetFrameSize(unsigned width, unsigned height);
    virtual PINDEX GetMaxFrameBytes();
    virtual bool FlowControl(const void * flowData);
    virtual bool GetAttributes(Attributes & attributes);
    virtual bool SetAttributes(const Attributes & attributes);


  protected:
    virtual bool InternalGetFrameData(BYTE * buffer, PINDEX & bytesReturned, bool & keyFrame, bool wait);
    bool BindCaptureDevice(const PString & devName);
    bool PlatformOpen();
    PINDEX GetCurrentBufferSize();
    bool GetCurrentBufferData(BYTE * data, PINDEX & bufferSize, bool wait);
    bool SetPinFormat(unsigned useDefaultColourOrSize = 0);
    bool SetAttributeCommon(long control, int newValue);
    int GetAttributeCommon(long control) const;


    // DirectShow 
    CComPtr<IGraphBuilder>         m_pGraphBuilder;
    CComPtr<ICaptureGraphBuilder2> m_pCaptureBuilder;
    CComPtr<IBaseFilter>           m_pCaptureFilter;
    CComPtr<IPin>                  m_pCameraOutPin; // Camera output out -> Transform Input pin
    GUID                           m_selectedGUID;

    CComPtr<ISampleGrabber>        m_pSampleGrabber;
    CComPtr<PSampleGrabberCB>      m_pSampleGrabberCB;
    CComPtr<IAMCameraControl>      m_pCameraControls;
    CComPtr<IBaseFilter>           m_pNullRenderer;
    CComPtr<IMediaControl>         m_pMediaControl;

    PINDEX     m_maxFrameBytes;
    bool       m_fixedSizeFrames; // Not JPEG
    PBYTEArray m_tempFrame;
    PDECLARE_MUTEX(m_lastFrameMutex);
};


PCREATE_VIDINPUT_PLUGIN(DirectShow);


////////////////////////////////////////////////////////////////////

#ifndef __MINGW32__
static const GUID MEDIASUBTYPE_I420 = { 0x30323449, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };
#endif

static PString GUID2Format(GUID guid)
{
  static struct {
      const char * m_colourFormat;
      GUID         m_guid;
  } const ColourFormat2GUID[] =
  {
    { "Grey",    MEDIASUBTYPE_RGB8   },
    { "BGR32",   MEDIASUBTYPE_RGB32  }, /* Microsoft assumes that we are in little endian */
    { "BGR24",   MEDIASUBTYPE_RGB24  },
    { "RGB565",  MEDIASUBTYPE_RGB565 },
    { "RGB555",  MEDIASUBTYPE_RGB555 },
    { PTLIB_VIDEO_YUV420P, MEDIASUBTYPE_IYUV   },  // aka I420
    { PTLIB_VIDEO_YUV420P, MEDIASUBTYPE_I420   },
    { PTLIB_VIDEO_YUV420P, MEDIASUBTYPE_YV12   },
    { "YUV411",  MEDIASUBTYPE_Y411   },
    { "YUV411P", MEDIASUBTYPE_Y41P   },
    { "YUV410P", MEDIASUBTYPE_YVU9   },
    { "YUY2",    MEDIASUBTYPE_YUY2   },
    { "MJPEG",   MEDIASUBTYPE_MJPG   },
    { "UYVY422", MEDIASUBTYPE_UYVY   }
  };

  for (int j = 0; j < sizeof(ColourFormat2GUID)/sizeof(ColourFormat2GUID[0]); j++) {
    if (guid == ColourFormat2GUID[j].m_guid)
      return ColourFormat2GUID[j].m_colourFormat;
  }

  return PGloballyUniqueID(guid).AsString();
}


//////////////////////////////////////////////////////////////////////////////////////////////////

class MediaTypePtr : PNonCopyable
{
    AM_MEDIA_TYPE * pointer;
  public:
    MediaTypePtr()
      : pointer(NULL)
    {
    }

    ~MediaTypePtr()
    {
      Release();
    }

    void Release()
    {
      if (pointer == NULL)
        return;

      if (pointer->cbFormat != 0) {
        CoTaskMemFree(pointer->pbFormat);
        pointer->cbFormat = 0;
        pointer->pbFormat = NULL;
      }

      if (pointer->pUnk != NULL) {
        // Uncessessary because pUnk should not be used, but safest.
        pointer->pUnk->Release();
        pointer->pUnk = NULL;
      }

      CoTaskMemFree(pointer);
    }

    operator AM_MEDIA_TYPE *()  const { return  pointer; }
    AM_MEDIA_TYPE & operator*() const { return *pointer; }
    AM_MEDIA_TYPE** operator&()       { return &pointer; }
    AM_MEDIA_TYPE* operator->() const { return  pointer; }
};


//////////////////////////////////////////////////////////////////////////////////////////////////
// Input device

PVideoInputDevice_DirectShow::PVideoInputDevice_DirectShow()
  : m_maxFrameBytes(0)
  , m_fixedSizeFrames(true)
{
  PTRACE(5, "Constructed " << this);

  PThread::Current()->CoInitialise();
}


PVideoInputDevice_DirectShow::~PVideoInputDevice_DirectShow()
{
  Close();
  PTRACE(5, "Destroyed " << this);
}


PStringArray PVideoInputDevice_DirectShow::GetInputDeviceNames()
{
  PVideoInputDevice_DirectShow instance;
  return instance.GetDeviceNames();
}


PBoolean PVideoInputDevice_DirectShow::GetInputDeviceCapabilities(const PString & deviceName,
                                                             Capabilities * capabilities)
{
  PVideoInputDevice_DirectShow instance;
  return instance.Open(deviceName, false) && instance.GetDeviceCapabilities(capabilities);
}


bool PVideoInputDevice_DirectShow::GetDeviceCapabilities(Capabilities * caps) const
{
  if (m_pCameraOutPin == NULL)
    return false;

  CComPtr<IAMStreamConfig> pStreamConfig;
#ifdef __MINGW32__
  PCOM_RETURN_ON_FAILED(m_pCameraOutPin->QueryInterface,(IID_IAMStreamConfig, (void**)&pStreamConfig));
#else
  PCOM_RETURN_ON_FAILED(m_pCameraOutPin->QueryInterface,(&pStreamConfig));
#endif

  int iCount, iSize;
  PCOM_RETURN_ON_FAILED(pStreamConfig->GetNumberOfCapabilities,(&iCount, &iSize));

  /* Sanity check: just to be sure that the Streamcaps is a VIDEOSTREAM and not AUDIOSTREAM */
  VIDEO_STREAM_CONFIG_CAPS scc;
  if (sizeof(scc) != iSize) {
    PTRACE(1, "Bad Capapabilities (not a VIDEO_STREAM_CONFIG_CAPS)");
    return false;
  }

  // Set is ordered large to small, and unique
  std::set<PVideoFrameInfo, std::greater<PVideoFrameInfo> > fsizes;
  for (int iFormat = 0; iFormat < iCount; iFormat++) {
    MediaTypePtr pMediaFormat;
    if (SUCCEEDED(pStreamConfig->GetStreamCaps(iFormat, &pMediaFormat, (BYTE *)&scc)) &&
        pMediaFormat->majortype == MEDIATYPE_Video &&
        pMediaFormat->formattype == FORMAT_VideoInfo &&
        pMediaFormat->pbFormat != NULL &&
        pMediaFormat->cbFormat >= sizeof(VIDEOINFOHEADER))
      fsizes.insert(PVideoFrameInfo(scc.MaxOutputSize.cx,
                                    scc.MaxOutputSize.cy,
                                    GUID2Format(pMediaFormat->subtype),
                                    10000000/(unsigned)scc.MinFrameInterval,
                                    PVideoFrameInfo::eMaxResizeMode));
  }

  if (fsizes.empty())
    return false;

  // Sort so we have unique sizes from largest to smallest
  for (std::set<PVideoFrameInfo, std::greater<PVideoFrameInfo> >::iterator it = fsizes.begin(); it != fsizes.end(); ++it) {
    PTRACE(5, "Format["<< caps->m_frameSizes.size() << "] = " << *it);
    caps->m_frameSizes.push_back(*it);
  }

  for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type) {
    if (m_controlInfo[type].IsValid())
      caps->m_controls.push_back(m_controlInfo[type]);
  }

  caps->m_brightness = GetAttributeCommon(VideoProcAmp_Brightness) >= 0;
  caps->m_contrast   = GetAttributeCommon(VideoProcAmp_Contrast) >= 0;
  caps->m_saturation = GetAttributeCommon(VideoProcAmp_Saturation) >= 0;
  caps->m_hue        = GetAttributeCommon(VideoProcAmp_Hue) >= 0;
  caps->m_gamma      = GetAttributeCommon(VideoProcAmp_Gamma) >= 0;

  return true;
}


bool PVideoInputDevice_DirectShow::SetPinFormat(unsigned useDefaultColourOrSize)
{
  if (m_pCameraOutPin == NULL) {
    PTRACE(2, "Camera output pin is NULL!");
    return false;
  }

  CComPtr<IAMStreamConfig> pStreamConfig;
#ifdef __MINGW32__
  PCOM_RETURN_ON_FAILED(m_pCameraOutPin->QueryInterface,(IID_IAMStreamConfig, (void**)&pStreamConfig));
#else
  PCOM_RETURN_ON_FAILED(m_pCameraOutPin->QueryInterface,(&pStreamConfig));
#endif

  int iCount = 0, iSize=0;
  PCOM_RETURN_ON_FAILED(pStreamConfig->GetNumberOfCapabilities,(&iCount, &iSize));

  if (iSize != sizeof(VIDEO_STREAM_CONFIG_CAPS))
    return false;

  for (int iFormat = 0; iFormat < iCount; iFormat++) {
    VIDEO_STREAM_CONFIG_CAPS scc;
    MediaTypePtr pMediaFormat;
    if (SUCCEEDED(pStreamConfig->GetStreamCaps(iFormat, &pMediaFormat, (BYTE *)&scc)) &&
        pMediaFormat->majortype == MEDIATYPE_Video &&
        pMediaFormat->formattype == FORMAT_VideoInfo &&
        pMediaFormat->pbFormat != NULL &&
        pMediaFormat->cbFormat >= sizeof(VIDEOINFOHEADER) &&
        (
          useDefaultColourOrSize >= 2 ||
          (
            scc.MinOutputSize.cx <= (LONG)m_frameWidth  && (LONG)m_frameWidth  <= scc.MaxOutputSize.cx &&
            scc.MinOutputSize.cy <= (LONG)m_frameHeight && (LONG)m_frameHeight <= scc.MaxOutputSize.cy &&
            (
              useDefaultColourOrSize >= 1 ||
              GUID2Format(pMediaFormat->subtype) == m_colourFormat
            )
          )
        )) {
      VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER *)pMediaFormat->pbFormat;

      if (useDefaultColourOrSize >= 1) {
        m_colourFormat = GUID2Format(pMediaFormat->subtype);
        if (useDefaultColourOrSize >= 2) {
          m_frameWidth = scc.MaxOutputSize.cx;
          m_frameHeight = scc.MaxOutputSize.cy;
          PTRACE(3, "Camera format forced resolution to " << *this);
        }
        else
          PTRACE(3, "Camera format forced colour to " << *this);
      }

      m_selectedGUID = pMediaFormat->subtype;
      m_maxFrameBytes = CalculateFrameBytes(m_frameWidth, m_frameHeight, m_colourFormat);
      m_fixedSizeFrames = m_colourFormat.Find("JPEG") == P_MAX_INDEX;

      if (pVih->bmiHeader.biHeight > 0 && m_colourFormat.NumCompare("BGR") == EqualTo) {
        m_nativeVerticalFlip = true;
        PTRACE(3, "Image up side down");
      }

      bool running = IsCapturing();
      if (running)
        PCOM_RETURN_ON_FAILED(m_pMediaControl->Stop,());

      pVih->AvgTimePerFrame = 10000000 / m_frameRate;
      pVih->bmiHeader.biWidth = m_frameWidth;
      pVih->bmiHeader.biHeight = pVih->bmiHeader.biHeight < 0 ? -(LONG)m_frameHeight : m_frameHeight;
      pVih->bmiHeader.biSizeImage = m_maxFrameBytes;
      PCOM_RETURN_ON_FAILED(pStreamConfig->SetFormat,(pMediaFormat));

      if (running)
        PCOM_RETURN_ON_FAILED(m_pMediaControl->Run,());

      PTRACE(4, "Camera format set to " << *this);
      return true;
    }
    else {
      PTRACE(6, "Tested format: "
             << GUID2Format(pMediaFormat->subtype) << ' '
             << scc.MinOutputSize.cx << 'x' << scc.MinOutputSize.cy
             << ".."
             << scc.MaxOutputSize.cx << 'x' << scc.MaxOutputSize.cy << ' '
             << scc.MinFrameInterval << ".." << scc.MaxFrameInterval);
    }
  }

  return false;
}


PBoolean PVideoInputDevice_DirectShow::Open(const PString & devName,
                                            PBoolean        startImmediate)
{
  Close();

  if (devName.IsEmpty()) {
    PTRACE(2, "Unable to Bind to empty device");
    return false;
  }

  // Get the interface for DirectShow's GraphBuilder
  PCOM_RETURN_ON_FAILED(CoCreateInstance,(CLSID_FilterGraph,
                                          NULL,
                                          CLSCTX_INPROC_SERVER,
                                          IID_IGraphBuilder,
                                          (LPVOID *)&m_pGraphBuilder));

  // Create the capture graph builder
  PCOM_RETURN_ON_FAILED(CoCreateInstance,(CLSID_CaptureGraphBuilder2,
                                          NULL,
                                          CLSCTX_INPROC_SERVER,
                                          IID_ICaptureGraphBuilder2,
                                          (LPVOID *)&m_pCaptureBuilder));

  // Bind the Camera Input
  if (!BindCaptureDevice(devName))
    return false;

  // Add Capture filter to our graph.
  PCOM_RETURN_ON_FAILED(m_pGraphBuilder->AddFilter,(m_pCaptureFilter, L"Video Capture"));

  // Attach the filter graph to the capture graph
  PCOM_RETURN_ON_FAILED(m_pCaptureBuilder->SetFiltergraph,(m_pGraphBuilder));

  // Obtain interfaces for media control (start/stop capture)
  PCOM_RETURN_ON_FAILED(m_pGraphBuilder->QueryInterface,(IID_IMediaControl, (void **)&m_pMediaControl));

  // Get the camera output Pin 
  CComPtr<IEnumPins> pEnum;
  PCOM_RETURN_ON_FAILED(m_pCaptureFilter->EnumPins,(&pEnum));

  PCOM_RETURN_ON_FAILED(pEnum->Reset,());
  PCOM_RETURN_ON_FAILED(pEnum->Next,(1, &m_pCameraOutPin, NULL));

  PTRACE(4, "Attempting to use " << *this);
  // Set the format of the Output pin of the camera
  if (!(SetPinFormat(0) || SetPinFormat(1) || SetPinFormat(2))) {
    PTRACE(2, "Camera formats available could not be matched to " << *this);
    return false;
  }

  if (!PlatformOpen())
    return false;

  if (startImmediate) 
    Start();

  PTRACE(4, "Device " << devName << " opened as " << this);
  m_deviceName = devName;
  return true;
}


PBoolean PVideoInputDevice_DirectShow::IsOpen()
{
  return !m_deviceName.empty();
}


PBoolean PVideoInputDevice_DirectShow::Close()
{
  if (!IsOpen())
    return false;

  PTRACE(4, "Closing " << this);

  m_deviceName.MakeEmpty();

  // Stop Camera Graph
  m_pMediaControl->Pause();
  m_pSampleGrabberCB->Stop();

  m_lastFrameMutex.Wait();

  // Release the Camera and interfaces
  m_pMediaControl.Release();
  m_pCameraOutPin.Release(); 
  m_pCaptureBuilder.Release();
  m_pCaptureFilter.Release();

  // Relase DirectShow Graph
  m_pGraphBuilder.Release(); 

  // Release filters
  m_pNullRenderer.Release();
  m_pCameraControls.Release();
  m_pSampleGrabber.Release();
  delete m_pSampleGrabberCB.Detach();

  m_lastFrameMutex.Signal();

  return true;
}


PBoolean PVideoInputDevice_DirectShow::Start()
{
  if (!IsOpen()) {
    PTRACE(3, "Not open.");
    return false;
  }

  if (IsCapturing())
    return true;

  PCOM_RETURN_ON_FAILED(m_pMediaControl->Run,());

  m_pSampleGrabberCB->Start();

  PTRACE(4, "Video Started.");
  return true;
}


PBoolean PVideoInputDevice_DirectShow::Stop()
{
  if (!IsOpen()) {
    PTRACE(3, "Not open.");
    return false;
  }

  if (!IsCapturing())
    return true;

  // Use Pause() not Stop() as the latter is to much of a stop and takes too long to restart
  PCOM_RETURN_ON_FAILED(m_pMediaControl->Pause,());

  m_pSampleGrabberCB->Stop();
  return true;
}


PBoolean PVideoInputDevice_DirectShow::IsCapturing()
{
  if (!IsOpen())
    return false;

  OAFilterState state;
  PCOM_RETURN_ON_FAILED(m_pMediaControl->GetState,(0, &state));
  return state != State_Stopped;
}


PBoolean PVideoInputDevice_DirectShow::SetColourFormat(const PString & newColourFormat)
{
  if (m_colourFormat == newColourFormat)
    return true;

  PString oldColourFormat = m_colourFormat;

  if (!PVideoDevice::SetColourFormat(newColourFormat))
    return false;

  if (SetPinFormat())
    return true;

  PVideoDevice::SetColourFormat(oldColourFormat);
  return false;
}


PBoolean PVideoInputDevice_DirectShow::SetFrameRate(unsigned newRate)
{
  if (m_frameRate == newRate)
    return true;

  unsigned oldRate = m_frameRate;

  if (!PVideoDevice::SetFrameRate(newRate))
    return false;

  if (SetPinFormat())
    return true;

  PVideoDevice::SetFrameRate(oldRate);
  return false;
}


PBoolean PVideoInputDevice_DirectShow::SetFrameSize(unsigned width, unsigned height)
{
  if (m_frameWidth == width && m_frameHeight == height)
    return true;

  unsigned oldWidth = m_frameWidth;
  unsigned oldHeight = m_frameHeight;

  if (!PVideoDevice::SetFrameSize(width, height))
    return false;

  if (SetPinFormat())
    return true;

  PVideoDevice::SetFrameSize(oldWidth, oldHeight);
  return false;
}


bool PVideoInputDevice_DirectShow::FlowControl(const void * flowData)
{
    const PStringArray & options = *(const PStringArray *)flowData;
    
    int w=0; int h=0; int r=0;
    for (PINDEX i=0; i < options.GetSize(); i+=2) {
      if (options[i] == "Frame Width")
            w = options[i+1].AsInteger();
      else if (options[i] == "Frame Height")
            h = options[i+1].AsInteger();
      else if (options[i] ==  "Frame Time")
            r =  90000/options[i+1].AsInteger();
    }

    PTRACE(4, "Adjusting to new H: " << h << " W: " << w << " R: " << r);
    m_lastFrameMutex.Wait();
    SetFrameSize(w,h);
    SetFrameRate(r);
    m_lastFrameMutex.Signal();

    return true;
}

PINDEX PVideoInputDevice_DirectShow::GetMaxFrameBytes()
{
  return GetMaxFrameBytesConverted(CalculateFrameBytes(m_frameWidth, m_frameHeight, m_colourFormat));
}

bool PVideoInputDevice_DirectShow::InternalGetFrameData(BYTE * buffer, PINDEX & bytesReturned, bool & keyFrame, bool wait)
{
  if (!IsOpen())
    return false;

  keyFrame = true;

  PWaitAndSignal mutex(m_lastFrameMutex);

  PINDEX bufferSize = GetCurrentBufferSize();
  PTRACE(6, "Grabbing Frame " << m_frameWidth << 'x' << m_frameHeight << " (" << bufferSize << ')');

  if (m_converter != NULL) {
    if (!GetCurrentBufferData(m_tempFrame.GetPointer(bufferSize), bufferSize, wait))
      return false;
    m_converter->SetSrcFrameBytes(bufferSize);
    if (!m_converter->Convert(m_tempFrame, buffer, &bytesReturned))
      return false;
  }
  else {
    if (!PAssert(bufferSize <= m_maxFrameBytes, PLogicError))
      return false;
    if (!GetCurrentBufferData(buffer, bufferSize, wait))
      return false;
    bytesReturned = bufferSize;
  }

  return true;
}


int PVideoInputDevice_DirectShow::GetAttributeCommon(long control) const
{
  CComPtr<IAMVideoProcAmp> pVideoProcAmp;
  if (PCOM_FAILED(m_pCaptureFilter->QueryInterface,(IID_IAMVideoProcAmp, (void **)&pVideoProcAmp)))
    return -1;

  long minimum, maximum, stepping, def, flags;
  if (PCOM_FAILED(pVideoProcAmp->GetRange,(control, &minimum, &maximum, &stepping, &def, &flags)))
    return -1;

  long value;
  if (PCOM_FAILED(pVideoProcAmp->Get,(control, &value, &flags)))
    return -1;

  if (flags == VideoProcAmp_Flags_Auto)
    return -1;

  return ((value - minimum) * 65536) / (maximum-minimum);
}


bool PVideoInputDevice_DirectShow::GetAttributes(Attributes & attrib)
{
  if (!IsOpen())
    return false;

  attrib.m_brightness = GetAttributeCommon(VideoProcAmp_Brightness);
  attrib.m_contrast   = GetAttributeCommon(VideoProcAmp_Contrast);
  attrib.m_saturation = GetAttributeCommon(VideoProcAmp_Saturation);
  attrib.m_hue        = GetAttributeCommon(VideoProcAmp_Hue);
  attrib.m_gamma      = GetAttributeCommon(VideoProcAmp_Gamma);

  return true;
}


PBoolean PVideoInputDevice_DirectShow::SetAttributeCommon(long control, int newValue)
{
  CComPtr<IAMVideoProcAmp> pVideoProcAmp;
  PCOM_RETURN_ON_FAILED(m_pCaptureFilter->QueryInterface,(IID_IAMVideoProcAmp, (void **)&pVideoProcAmp));

  long minimum, maximum, stepping, def, flags;
  PCOM_RETURN_ON_FAILED(pVideoProcAmp->GetRange,(control, &minimum, &maximum, &stepping, &def, &flags));

  if (newValue == -1) {
    if ((flags&VideoProcAmp_Flags_Auto) == 0) {
      PTRACE(2, "Automatic control for element " << control << " not supported");
      return false;
    }
    PCOM_RETURN_ON_FAILED(pVideoProcAmp->Set,(control, 0, VideoProcAmp_Flags_Auto));
  }
  else {
    if ((flags&VideoProcAmp_Flags_Manual) == 0) {
      PTRACE(2, "Manual control for element " << control << " not supported");
      return false;
    }

    long scaled = minimum + ((maximum-minimum) * newValue) / 65536;
    PCOM_RETURN_ON_FAILED(pVideoProcAmp->Set,(control, scaled, VideoProcAmp_Flags_Manual));
  }

  PTRACE(4, "SetControl: element=" << control << ", value=" << newValue);
  return true;
}

PBoolean PVideoInputDevice_DirectShow::SetAttributes(const Attributes & attrib)
{
  return SetAttributeCommon(VideoProcAmp_Brightness, attrib.m_brightness) &&
         SetAttributeCommon(VideoProcAmp_Saturation, attrib.m_saturation) &&
         SetAttributeCommon(VideoProcAmp_Contrast, attrib.m_contrast) &&
         SetAttributeCommon(VideoProcAmp_Hue, attrib.m_hue) &&
         SetAttributeCommon(VideoProcAmp_Gamma, attrib.m_gamma);
}


///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////


class PComEnumerator
{
    CComPtr<ICreateDevEnum> m_pDevEnum;
    CComPtr<IEnumMoniker>   m_pClassEnum;
    CComPtr<IMoniker>       m_pMoniker;
  public:
    PComEnumerator()
    {
      // Create the system device enumerator
      if (PCOM_FAILED(CoCreateInstance,(CLSID_SystemDeviceEnum,
                                        NULL,
                                        CLSCTX_INPROC,
                                        IID_ICreateDevEnum,
                                        (LPVOID *)&m_pDevEnum)))
        return;

      // Create an enumerator for the video capture devices
      if (PCOM_FAILED(m_pDevEnum->CreateClassEnumerator,(CLSID_VideoInputDeviceCategory, &m_pClassEnum, 0)))
        return;

      PTRACE_IF(2, m_pClassEnum == NULL, "No video capture device was detected.");
    }

    bool Next()
    {
      if (m_pClassEnum == NULL)
        return false;

      m_pMoniker.Release();

      ULONG cFetched;
      return m_pClassEnum->Next(1, &m_pMoniker, &cFetched) == S_OK;
    }

    IMoniker * GetMoniker() const { return m_pMoniker; }

    PString GetMonikerName()
    {
      CComPtr<IPropertyBag> pPropBag;
      if (PCOM_FAILED(m_pMoniker->BindToStorage,(0, 0, IID_IPropertyBag, (void **)&pPropBag)))
        return PString::Empty();

      // Find the description or friendly name.
      PComVariant varName;
      if (PCOM_FAILED(pPropBag->Read,(L"Description", &varName, NULL), ERROR_FILE_NOT_FOUND) &&
          PCOM_FAILED(pPropBag->Read,(L"FriendlyName", &varName, NULL), ERROR_FILE_NOT_FOUND))
        return PString::Empty();

      PString name = varName.AsString();

      PINDEX i = name.GetLength();
      while ((i > 0) && !::isprint(name[i-1]))
        name = name.Left(--i);

      return name;
    }
};

PStringArray PVideoInputDevice_DirectShow::GetDeviceNames() const
{
  PStringArray devices;
  unsigned duplicate = 0;

  PComEnumerator enumerator;
  while (enumerator.Next()) {
    PString name = enumerator.GetMonikerName();
    if (!name.IsEmpty()) {
      if (devices.GetValuesIndex(name) != P_MAX_INDEX)
        name.sprintf("{%u}", ++duplicate);
      PTRACE(3, "Found capture device \""<< name <<'"');
      devices.AppendString(name);
    }
  }

  PTRACE_IF(3, devices.IsEmpty(), "No video capture devices available.");

  return devices;
}


bool PVideoInputDevice_DirectShow::SetControl(PVideoControlInfo::Types type, int value, ControlMode mode)
{
  if (m_pCameraControls == NULL) {
    PTRACE(2, "No controls available to " << type);
    return false;
  }

  long flags = KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
  switch (mode) {
    case AutomaticControl :
      flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
      break;

    case AbsoluteControl :
      value = m_controlInfo[type].SetCurrent(value);
      break;

    case RelativeControl :
      value = m_controlInfo[type].SetCurrent(m_controlInfo[type].GetCurrent() + value);
      break;

    case ResetControl :
      value = m_controlInfo[type].Reset();
      break;
  }

  PComResult hr;
  if (!hr.Succeeded(m_pCameraControls->Set(InputControlPropertyCode[type], value, flags))) {
    PTRACE(2, "Failed to set " << type << " to " << value << ": " << hr);
    return false;
  }

  PTRACE(4, "Set " << type << " to " << value);
  return true;
}


// Implementation of PSampleGrabberCB object
PSampleGrabberCB::PSampleGrabberCB()
  : m_stopped(true)
  , m_actualSize(0)
  , m_skipInitialGrabs(4)
#if PTRACING
  , m_totalFrames(0)
#endif
{
}


PSampleGrabberCB::~PSampleGrabberCB()
{
  m_frameReady.Signal();
}


void PSampleGrabberCB::Start()
{
#if PTRACING
  m_totalTime = 0;
#endif
  m_stopped = false;
}


void PSampleGrabberCB::Stop()
{
#if PTRACING
  static const int Level = 3;
  if (PTrace::CanTrace(Level)) {
    int64_t ms = m_totalTime.GetElapsed().GetMilliSeconds();
    ostream & trace = PTRACE_BEGIN(Level);
    trace << "Grabber Stopped";
    if (ms > 0)
      trace << ", frames/second=" << fixed << setprecision(2) << m_totalFrames*1000.0/ms;
    trace << PTrace::End;
  }
#endif

  m_stopped = true;
  m_frameReady.Signal();
}


// Fake out any COM QI'ing
//
STDMETHODIMP PSampleGrabberCB::QueryInterface(REFIID riid, void ** ppv)
{
  if( riid == IID_ISampleGrabberCB || riid == IID_IUnknown ) {
    *ppv = (void *) static_cast<ISampleGrabberCB*> ( this );
    return NOERROR;
  }    

  return E_NOINTERFACE;
}


// The sample grabber is calling us back on its deliver thread.
// This is NOT the main app thread!
//
STDMETHODIMP PSampleGrabberCB::BufferCB(double PTRACE_PARAM(dblSampleTime), BYTE * buffer, long size)
{
  PTRACE(6, "Buffer callback: time=" << dblSampleTime
          << ", buf=" << (void *)buffer << ", size=" << size);

  if (size == 0 || m_stopped)
    return S_OK;

  if (m_skipInitialGrabs > 0) {
    --m_skipInitialGrabs;
    return S_OK;
  }

  PWaitAndSignal mutex(m_mutex);

  if (m_buffer.GetSize() != (PINDEX)size) {
    if (!m_buffer.SetMinSize(size))
      return E_POINTER;
  }

  memcpy(m_buffer.GetPointer(), buffer, size);
  m_actualSize = size;

  m_frameReady.Signal();

#if PTRACING
  ++m_totalFrames;
#endif
  return S_OK;
}


bool PSampleGrabberCB::GetData(BYTE * data, PINDEX maxSize, PINDEX & actualSize, bool wait)
{
  // Live! Cam Optia AF (VC0100) webcam took 3.1 sec.
  if (!m_frameReady.Wait(wait ? 5000 : 0)) {
    PTRACE(1, "Timeout awaiting next frame");
    return !m_stopped && !wait;
  }

  PWaitAndSignal mutex(m_mutex);

  if (m_stopped) {
    PTRACE(4, "Stopped");
    return false;
  }

  if (m_actualSize <= maxSize)
    memcpy(data, m_buffer, m_actualSize);

  actualSize = m_actualSize;
  return true;
}


static HRESULT GetUnconnectedPin(IBaseFilter *pFilter,   // Pointer to the filter.
                                 PIN_DIRECTION PinDir,   // Direction of the pin to find.
                                 IPin **ppPin)           // Receives a pointer to the pin.
{
  *ppPin = 0;
  IEnumPins *pEnum = 0;
  IPin *pPin = 0;
  HRESULT hr = pFilter->EnumPins(&pEnum);
  if (FAILED(hr))
    return hr;

  while (pEnum->Next(1, &pPin, NULL) == S_OK)
  {
    PIN_DIRECTION ThisPinDir;
    pPin->QueryDirection(&ThisPinDir);
    if (ThisPinDir == PinDir)
    {
      IPin *pTmp = 0;
      hr = pPin->ConnectedTo(&pTmp);
      if (SUCCEEDED(hr))  // Already connected, not the pin we want.
      {
        pTmp->Release();
      }
      else  // Unconnected, this is the pin we want.
      {
        pEnum->Release();
        *ppPin = pPin;
        return S_OK;
      }
    }
    pPin->Release();
  }

  pEnum->Release();

  // Did not find a matching pin.
  return E_FAIL;
}


static HRESULT ConnectFilters(IGraphBuilder *pGraph, // Filter Graph Manager.
                              IPin *pOut,            // Output pin on the upstream filter.
                              IBaseFilter *pDest)    // Downstream filter.
{
  if ((pGraph == NULL) || (pOut == NULL) || (pDest == NULL))
    return E_POINTER;

  // Find an input pin on the downstream filter.
  IPin *pIn = 0;
  HRESULT hr = GetUnconnectedPin(pDest, PINDIR_INPUT, &pIn);
  if (FAILED(hr))
    return hr;

  // Try to connect them.
  hr = pGraph->Connect(pOut, pIn);
  pIn->Release();
  return hr;
}


static HRESULT ConnectFilters(IGraphBuilder *pGraph, 
                              IBaseFilter *pSrc, 
                              IBaseFilter *pDest)
{
  if ((pGraph == NULL) || (pSrc == NULL) || (pDest == NULL))
    return E_POINTER;

  // Find an output pin on the first filter.
  IPin *pOut = 0;
  HRESULT hr = GetUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
  if (FAILED(hr)) 
    return hr;

  hr = ConnectFilters(pGraph, pOut, pDest);
  pOut->Release();
  return hr;
}


bool PVideoInputDevice_DirectShow::BindCaptureDevice(const PString & devName)
{
  // Bind Device Filter.  We know the device because the id was passed in
  int deviceNumber = 0;
  PString devSearch = devName;
  PINDEX brace = devName.Find('{');
  if (brace != P_MAX_INDEX) {
    deviceNumber = devName.Mid(brace+1).AsInteger();
    devSearch = devName.Left(brace).Trim();
  }

  PComEnumerator enumerator;
  while (enumerator.Next()) {
    PString name = enumerator.GetMonikerName();
    if (name == devSearch && deviceNumber-- == 0) {
      enumerator.GetMoniker()->BindToObject(0, 0, IID_IBaseFilter, (void**)&m_pCaptureFilter);
      return true;
    }
  }

  return false;
}


bool PVideoInputDevice_DirectShow::PlatformOpen()
{
  // Buid the Camera Sample Grabber
  PTRACE(5, "Building Sample Grabber");

  CComPtr<IBaseFilter> pGrab; 
  PCOM_RETURN_ON_FAILED(CoCreateInstance,(CLSID_SampleGrabber,
                                          NULL,
                                          CLSCTX_INPROC_SERVER,
                                          IID_IBaseFilter,
                                          (LPVOID *)&pGrab));
  PCOM_RETURN_ON_FAILED(m_pGraphBuilder->AddFilter,(pGrab, L"Sample Grabber"));
  PCOM_RETURN_ON_FAILED(pGrab->QueryInterface,(IID_ISampleGrabber, (void **)&m_pSampleGrabber));

  AM_MEDIA_TYPE mt;
  ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
  mt.majortype = MEDIATYPE_Video;
  mt.subtype = m_selectedGUID;
  PCOM_RETURN_ON_FAILED(m_pSampleGrabber->SetMediaType,(&mt));

  PCOM_RETURN_ON_FAILED(m_pSampleGrabber->SetBufferSamples,(true));

  PTRACE(5, "Setting Sample Grabber Callback");
  m_pSampleGrabberCB = new PSampleGrabberCB();
  PCOM_RETURN_ON_FAILED(m_pSampleGrabber->SetCallback,(m_pSampleGrabberCB, 1));

  PTRACE(5, "Connect sample grabber to camera");
  PCOM_RETURN_ON_FAILED(ConnectFilters,(m_pGraphBuilder, m_pCameraOutPin, pGrab));

  // Set the NULL Renderer
  PTRACE(5, "Building NULL output filter");
  PCOM_RETURN_ON_FAILED(CoCreateInstance,(CLSID_NullRenderer,
                                          NULL,
                                          CLSCTX_INPROC_SERVER,
                                          IID_IBaseFilter,
                                          (LPVOID *)&m_pNullRenderer));

  PCOM_RETURN_ON_FAILED(m_pGraphBuilder->AddFilter,(m_pNullRenderer, L"NULL Output"));

  PTRACE(5, "Connect Null Render to Grab filter Pin");
  PCOM_RETURN_ON_FAILED(ConnectFilters,(m_pGraphBuilder, pGrab, m_pNullRenderer));

  PTRACE(5, "Checking image flip");
  PCOM_RETURN_ON_FAILED(m_pSampleGrabber->GetConnectedMediaType,(&mt));

  // Query for camera controls
  if (FAILED(m_pCaptureFilter->QueryInterface(IID_IAMCameraControl, (void **)&m_pCameraControls))) {
    PTRACE(3, "Camera " << m_deviceName << " does not support Camera Controls.");
    m_pCameraControls = NULL;
  }
  else {
    for (PVideoControlInfo::Types type = PVideoControlInfo::BeginTypes; type < PVideoControlInfo::EndTypes; ++type) {
      PComResult hr;
      long minimum, maximum, step, reset, flags;
      if (hr.Succeeded(m_pCameraControls->GetRange(InputControlPropertyCode[type], &minimum, &maximum, &step, &reset, &flags)))
        m_controlInfo[type] = PVideoControlInfo(type, minimum, maximum, step, reset);
      else {
        PTRACE(4, "Camera does not support " << type << ": " << hr);
      }
    }
  }

  return true;
}


PINDEX PVideoInputDevice_DirectShow::GetCurrentBufferSize()
{
  return m_maxFrameBytes;
}


bool PVideoInputDevice_DirectShow::GetCurrentBufferData(BYTE * data, PINDEX & bufferSize, bool wait)
{
  if (m_pSampleGrabberCB == NULL)
    return false;

  while (IsCapturing() && m_pSampleGrabberCB->GetData(data, m_maxFrameBytes, bufferSize, wait)) {
    if (m_fixedSizeFrames ? (bufferSize == m_maxFrameBytes) : (bufferSize <= m_maxFrameBytes))
      return true;

    PTRACE(4, "Camera resolution changed, ignoring received frame at old size.");
  }

  return false;
}

#endif  // P_DIRECTSHOW
