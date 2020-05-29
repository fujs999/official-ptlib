/*
 * wxWinVidDev.h
 *
 * Classes to support video output via wxWindows
 *
 * Copyright (c) 2017 Equivalence Pty. Ltd.
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
 * Note, is expected the application include this header in precisely
 * one source file compiled.
 */

#include <ptlib/videoio.h>
#include <wx/rawbmp.h>


#ifndef P_WXWINDOWS_IMPLEMENTATION_ONLY

#define P_WXWINDOWS_DRIVER_NAME "wxWindows"
#define P_WXWINDOWS_DEVICE_NAME "wxWindows"
#define P_WXWINDOWS_DEVICE_CLASS  PVideoOutputDevice_##wxWindows


/**Display data to the wxWindows window.
  */
class P_WXWINDOWS_DEVICE_CLASS : public PVideoOutputDeviceRGB, public wxFrame
{
    PCLASSINFO(P_WXWINDOWS_DEVICE_CLASS, PVideoOutputDeviceRGB);
  public:
    /**Constructor. Does not make a window.
      */
    P_WXWINDOWS_DEVICE_CLASS();

    /// Destroy the device, make sure it is closed
    ~P_WXWINDOWS_DEVICE_CLASS();

    /**Get a list of all of the devices available.
    */
    virtual PStringArray GetDeviceNames() const;
    static PStringArray GetOutputDeviceNames();
  
    /**Open the device given the device name.
    */
    virtual bool Open(
      const PString & deviceName,   ///< Device name to open
      bool /*startImmediate*/ = true    ///< Immediately start device
    );
  
    /**Synonymous with the destructor.
    */
    virtual bool Close();

    /**Stop the video device I/O display.
    */
    virtual bool Stop();

    /**Indicate if this video rendering class is open.*/
    virtual bool IsOpen();
  
    /**Set the colour format to be used.
       Note that this function does not do any conversion. If it returns true
       then the video device does the colour format in native mode.

       To utilise an internal converter use the SetColourFormatConverter()
       function.

       Default behaviour sets the value of the colourFormat variable and then
       returns true.
    */
    virtual bool SetColourFormat(
      const PString & colourFormat ///< New colour format for device.
    );

    /**Set the frame size to be used.

       Note that devices may not be able to produce the requested size, and
       this function will fail.  See SetFrameSizeConverter().

       Default behaviour sets the frameWidth and frameHeight variables and
       returns true.
    */
    virtual bool SetFrameSize(
      unsigned width,   ///< New width of frame
      unsigned height   ///< New height of frame
    );

    /**Set a section of the output frame buffer.
      */
    virtual bool FrameComplete();

  protected:
    void InternalOpen();
    void InternalClose();
    void InternalFrameComplete();

    void OnClose(wxCloseEvent &);
    void OnPaint(wxPaintEvent &);

    enum { e_Closing, e_Closed, e_Open, e_Opening } m_state; // Note order important
    wxBitmap m_bitmap;

  wxDECLARE_DYNAMIC_CLASS(P_WXWINDOWS_DEVICE_CLASS);
  wxDECLARE_EVENT_TABLE();
};

#endif // P_WXWINDOWS_IMPLEMENTATION_ONLY

#ifndef P_WXWINDOWS_DECLARATION_ONLY

wxIMPLEMENT_DYNAMIC_CLASS(P_WXWINDOWS_DEVICE_CLASS, wxFrame);

BEGIN_EVENT_TABLE(P_WXWINDOWS_DEVICE_CLASS, wxFrame)
  EVT_CLOSE(P_WXWINDOWS_DEVICE_CLASS::OnClose)
  EVT_PAINT(P_WXWINDOWS_DEVICE_CLASS::OnPaint)
END_EVENT_TABLE()

PCREATE_VIDOUTPUT_PLUGIN_EX(wxWindows,
  virtual bool ValidateDeviceName(const PString & deviceName, intptr_t /*userData*/) const
  {
    return deviceName.NumCompare(GetServiceName()) == PObject::EqualTo;
  }
);


P_WXWINDOWS_DEVICE_CLASS::P_WXWINDOWS_DEVICE_CLASS()
  : m_state(e_Closed)
#ifdef _WIN32
  , m_bitmap(m_frameWidth, m_frameHeight, 24)
#else
  , m_bitmap(m_frameWidth, m_frameHeight, 32)
#endif
{
  wxNativePixelData bmdata(m_bitmap);
  m_nativeVerticalFlip = bmdata.GetRowStride() < 0;
}
 

P_WXWINDOWS_DEVICE_CLASS::~P_WXWINDOWS_DEVICE_CLASS()
{
  Close();
}


PStringArray P_WXWINDOWS_DEVICE_CLASS::GetDeviceNames() const
{
  return GetOutputDeviceNames();
}


PStringArray P_WXWINDOWS_DEVICE_CLASS::GetOutputDeviceNames()
{
  return PString(P_WXWINDOWS_DEVICE_NAME);
}
  

bool P_WXWINDOWS_DEVICE_CLASS::Open(const PString & deviceName, bool /*startImmediate*/)
{
  m_state = e_Opening;
  m_deviceName = deviceName;
  if (wxThread::IsMain())
    InternalOpen();
  else {
    PTRACE(4, P_WXWINDOWS_DEVICE_NAME, "Opening asynchronously " << *this);
    CallAfter(&P_WXWINDOWS_DEVICE_CLASS::InternalOpen);
  }

  return m_state >= e_Open;
}


void P_WXWINDOWS_DEVICE_CLASS::InternalOpen()
{
  PWaitAndSignal lock(m_mutex);

  if (PAssert(m_state == e_Opening, PLogicError)) {
    m_state = Create(NULL, 0,
                     PwxString(ParseDeviceNameTokenString("TITLE", "Video Output")),
                     wxPoint(ParseDeviceNameTokenInt("X", -1),
                             ParseDeviceNameTokenInt("Y", -1)),
                     wxSize(GetFrameWidth(), GetFrameHeight())) ? e_Open : e_Closed;
    PTRACE(m_state != e_Open ? 2 : 4, P_WXWINDOWS_DEVICE_NAME, "Open " << (m_state == e_Open ? "successful" : "failed") << " on " << *this);
  }
}
  

bool P_WXWINDOWS_DEVICE_CLASS::Close()
{
  if (!IsOpen())
    return false;

  m_state = e_Closing;
  if (wxThread::IsMain())
    InternalClose();
  else {
    PTRACE(4, P_WXWINDOWS_DEVICE_NAME, "Closing asynchronously " << *this);
    CallAfter(&P_WXWINDOWS_DEVICE_CLASS::InternalClose);
  }

  return true;
}
  

void P_WXWINDOWS_DEVICE_CLASS::InternalClose()
{
  PTRACE(4, P_WXWINDOWS_DEVICE_NAME, "Closing " << *this);
  PWaitAndSignal lock(m_mutex);
  wxFrame::Close(true);
}


void P_WXWINDOWS_DEVICE_CLASS::OnClose(wxCloseEvent & evt)
{
  PWaitAndSignal lock(m_mutex);

  if (m_state == e_Closed)
    return;

  if (evt.CanVeto())
    evt.Veto();
  else {
    Destroy();
    m_state = e_Closed;
    PTRACE(4, P_WXWINDOWS_DEVICE_NAME, "Closed " << *this);
  }
}


bool P_WXWINDOWS_DEVICE_CLASS::Stop()
{
  return Close();
}


bool P_WXWINDOWS_DEVICE_CLASS::IsOpen()
{
  return m_state >= e_Open;
}
  

bool P_WXWINDOWS_DEVICE_CLASS::SetColourFormat(const PString & colourFormat)
{
  PWaitAndSignal lock(m_mutex);
  return (colourFormat *= psprintf("BGR%u", m_bitmap.GetDepth()))
            && PVideoOutputDeviceRGB::SetColourFormat(colourFormat);
}


bool P_WXWINDOWS_DEVICE_CLASS::SetFrameSize(unsigned width, unsigned height)
{
  PWaitAndSignal lock(m_mutex);

  if (width == m_frameWidth && height == m_frameHeight)
    return true;

  if (!PVideoOutputDeviceRGB::SetFrameSize(width, height))
    return false;

  if (IsOpen()) {
    if (!m_bitmap.Create(m_frameWidth, m_frameHeight, m_bitmap.GetDepth())) {
      PTRACE(1, P_WXWINDOWS_DEVICE_NAME, "Cannot copy frame, wxBitmap create failed on " << *this);
      return false;
    }

    PTRACE(4, P_WXWINDOWS_DEVICE_NAME, "Resized wxBitmap to " << m_frameWidth << 'x' << m_frameHeight << " on " << *this);
  }

  return true;
}


bool P_WXWINDOWS_DEVICE_CLASS::FrameComplete()
{
  PWaitAndSignal lock(m_mutex);

  if (!IsOpen())
    return false;

  wxNativePixelData bmdata(m_bitmap);
  wxNativePixelData::Iterator it = bmdata.GetPixels();
  if (!it.IsOk()) {
    PTRACE(1, P_WXWINDOWS_DEVICE_NAME, "Cannot copy frame, wxBitmap invalid on " << *this);
    return false;
  }

  if (bmdata.GetRowStride() < 0)
    it.Offset(bmdata, 0, m_frameHeight - 1);
  memcpy((uint8_t *)&it.Data(), m_frameStore.GetPointer(), m_frameStore.GetSize());

  CallAfter(&P_WXWINDOWS_DEVICE_CLASS::InternalFrameComplete);
  return true;
}


void P_WXWINDOWS_DEVICE_CLASS::InternalFrameComplete()
{
  PWaitAndSignal lock(m_mutex);

  if (IsOpen()) {
    wxSize sz = GetClientSize();
    if (sz.GetWidth() != m_bitmap.GetWidth() || sz.GetHeight() != m_bitmap.GetHeight()) {
      PTRACE(4, P_WXWINDOWS_DEVICE_NAME, "Resized window to " << m_bitmap.GetWidth() << 'x' << m_bitmap.GetHeight() << " on " << *this);
      SetClientSize(m_bitmap.GetWidth(), m_bitmap.GetHeight());
    }
    Show(true);
    Refresh(false);
  }
}


void P_WXWINDOWS_DEVICE_CLASS::OnPaint(wxPaintEvent &)
{
  PWaitAndSignal lock(m_mutex);

  if (IsOpen() && PAssert(m_bitmap.IsOk(), PLogicError)) {
    wxPaintDC pDC(this);
    wxMemoryDC bmDC;
    bmDC.SelectObject(m_bitmap);
    if (pDC.Blit(0, 0, m_bitmap.GetWidth(), m_bitmap.GetHeight(), &bmDC, 0, 0))
      PTRACE(5, P_WXWINDOWS_DEVICE_NAME, "Updated screen on " << *this);
    else
      PTRACE(1, P_WXWINDOWS_DEVICE_NAME, "Cannot update screen, Blit failed on " << *this);
  }
}


#endif // P_WXWINDOWS_DECLARATION_ONLY

// End Of File ///////////////////////////////////////////////////////////////
