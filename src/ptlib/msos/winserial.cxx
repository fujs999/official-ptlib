/*
 * winserial.cxx
 *
 * Miscellaneous implementation of classes for Win32
 *
 * Portable Tools Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
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
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 */

#include <ptlib.h>
#include <ptlib/serchan.h>


#define QUEUE_SIZE 2048


///////////////////////////////////////////////////////////////////////////////
// PSerialChannel

void PSerialChannel::Construct()
{
  m_commsResource = INVALID_HANDLE_VALUE;

  char str[50] = "com1";
  GetProfileString("ports", str, "9600,n,8,1,x", &str[5], sizeof(str)-6);
  str[4] = ':';
  memset(&m_deviceControlBlock, 0, sizeof(m_deviceControlBlock));
  m_deviceControlBlock.DCBlength = sizeof(m_deviceControlBlock);
  BuildCommDCB(str, &m_deviceControlBlock);

  // These values are not set by BuildCommDCB
  m_deviceControlBlock.XoffChar = 19;
  m_deviceControlBlock.XonChar = 17;
  m_deviceControlBlock.XoffLim = (QUEUE_SIZE * 7)/8;  // upper limit before XOFF is sent to stop reception
  m_deviceControlBlock.XonLim = (QUEUE_SIZE * 3)/4;   // lower limit before XON is sent to re-enabled reception
}


bool PSerialChannel::Read(void * buf, PINDEX len)
{
  SetLastReadCount(0);

  if (CheckNotOpen())
    return false;

  COMMTIMEOUTS cto;
  PAssertOS(GetCommTimeouts(m_commsResource, &cto));
  cto.ReadIntervalTimeout = 0;
  cto.ReadTotalTimeoutMultiplier = 0;
  cto.ReadTotalTimeoutConstant = 0;
  cto.ReadIntervalTimeout = MAXDWORD; // Immediate timeout
  PAssertOS(SetCommTimeouts(m_commsResource, &cto));

  DWORD eventMask;
  PAssertOS(GetCommMask(m_commsResource, &eventMask));
  if (eventMask != (EV_RXCHAR|EV_TXEMPTY))
    PAssertOS(SetCommMask(m_commsResource, EV_RXCHAR|EV_TXEMPTY));

  DWORD timeToGo = readTimeout.GetInterval();
  DWORD bytesToGo = len;
  char * bufferPtr = (char *)buf;

  for (;;) {
    PWin32Overlapped overlap;
    DWORD readCount = 0;
    if (!ReadFile(m_commsResource, bufferPtr, bytesToGo, &readCount, &overlap)) {
      if (::GetLastError() != ERROR_IO_PENDING)
        return ConvertOSError(-2, LastReadError);
      if (!::GetOverlappedResult(m_commsResource, &overlap, &readCount, false))
        return ConvertOSError(-2, LastReadError);
    }

    bytesToGo -= readCount;
    bufferPtr += readCount;
    if (SetLastReadCount(GetLastReadCount() + readCount) >= len || timeToGo == 0)
      return GetLastReadCount() > 0;

    if (!::WaitCommEvent(m_commsResource, &eventMask, &overlap)) {
      if (::GetLastError()!= ERROR_IO_PENDING)
        return ConvertOSError(-2, LastReadError);
      DWORD err = ::WaitForSingleObject(overlap.hEvent, timeToGo);
      if (err == WAIT_TIMEOUT) {
        SetErrorValues(Timeout, ETIMEDOUT, LastReadError);
        ::CancelIo(m_commsResource);
        return GetLastReadCount() > 0;
      }
      else if (err == WAIT_FAILED)
        return ConvertOSError(-2, LastReadError);
    }
  }
}


bool PSerialChannel::Write(const void * buf, PINDEX len)
{
  SetLastWriteCount(0);

  if (CheckNotOpen())
    return false;

  COMMTIMEOUTS cto;
  PAssertOS(GetCommTimeouts(m_commsResource, &cto));
  cto.WriteTotalTimeoutMultiplier = 0;
  if (writeTimeout == PMaxTimeInterval)
    cto.WriteTotalTimeoutConstant = 0;
  else if (writeTimeout <= PTimeInterval(0))
    cto.WriteTotalTimeoutConstant = 1;
  else
    cto.WriteTotalTimeoutConstant = writeTimeout.GetInterval();
  PAssertOS(SetCommTimeouts(m_commsResource, &cto));

  PWin32Overlapped overlap;
  DWORD written;
  if (WriteFile(m_commsResource, buf, len, &written, &overlap)) 
    return SetLastWriteCount(written) == len;

  if (GetLastError() == ERROR_IO_PENDING)
    if (GetOverlappedResult(m_commsResource, &overlap, &written, true)) {
      return SetLastWriteCount(written) == len;
    }

  ConvertOSError(-2, LastWriteError);

  return false;
}


bool PSerialChannel::Close()
{
  if (CheckNotOpen())
    return false;

  CloseHandle(m_commsResource);
  m_commsResource = INVALID_HANDLE_VALUE;
  os_handle = -1;
  return ConvertOSError(-2);
}


bool PSerialChannel::SetCommsParam(uint32_t speed, uint8_t data, Parity parity,
                     uint8_t stop, FlowControl inputFlow, FlowControl outputFlow)
{
  if (speed > 0)
    m_deviceControlBlock.BaudRate = speed;

  if (data > 0)
    m_deviceControlBlock.ByteSize = data;

  switch (parity) {
    case NoParity :
      m_deviceControlBlock.Parity = NOPARITY;
      break;
    case OddParity :
      m_deviceControlBlock.Parity = ODDPARITY;
      break;
    case EvenParity :
      m_deviceControlBlock.Parity = EVENPARITY;
      break;
    case MarkParity :
      m_deviceControlBlock.Parity = MARKPARITY;
      break;
    case SpaceParity :
      m_deviceControlBlock.Parity = SPACEPARITY;
      break;
    case DefaultParity:
      break;
  }

  switch (stop) {
    case 1 :
      m_deviceControlBlock.StopBits = ONESTOPBIT;
      break;
    case 2 :
      m_deviceControlBlock.StopBits = TWOSTOPBITS;
      break;
  }

  switch (inputFlow) {
    case NoFlowControl :
      m_deviceControlBlock.fRtsControl = RTS_CONTROL_DISABLE;
      m_deviceControlBlock.fInX = false;
      break;
    case XonXoff :
      m_deviceControlBlock.fRtsControl = RTS_CONTROL_DISABLE;
      m_deviceControlBlock.fInX = true;
      break;
    case RtsCts :
      m_deviceControlBlock.fRtsControl = RTS_CONTROL_HANDSHAKE;
      m_deviceControlBlock.fInX = false;
      break;
    case DefaultFlowControl:
      break;
  }

  switch (outputFlow) {
    case NoFlowControl :
      m_deviceControlBlock.fOutxCtsFlow = false;
      m_deviceControlBlock.fOutxDsrFlow = false;
      m_deviceControlBlock.fOutX = false;
      break;
    case XonXoff :
      m_deviceControlBlock.fOutxCtsFlow = false;
      m_deviceControlBlock.fOutxDsrFlow = false;
      m_deviceControlBlock.fOutX = true;
      break;
    case RtsCts :
      m_deviceControlBlock.fOutxCtsFlow = true;
      m_deviceControlBlock.fOutxDsrFlow = false;
      m_deviceControlBlock.fOutX = false;
      break;
    case DefaultFlowControl:
      break;
  }

  if (CheckNotOpen())
    return false;

  return ConvertOSError(SetCommState(m_commsResource, &m_deviceControlBlock) ? 0 : -2);
}


bool PSerialChannel::Open(const PString & port, uint32_t speed, uint8_t data,
               Parity parity, uint8_t stop, FlowControl inputFlow, FlowControl outputFlow)
{
  Close();

  m_portName = port;
  if (m_portName.Find(PDIR_SEPARATOR) == P_MAX_INDEX)
    m_portName = "\\\\.\\" + port;
  m_commsResource = CreateFile(m_portName,
                             GENERIC_READ|GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_OVERLAPPED,
                             NULL);
  if (m_commsResource == INVALID_HANDLE_VALUE)
    return ConvertOSError(-2);

  os_handle = 0;

  SetupComm(m_commsResource, QUEUE_SIZE, QUEUE_SIZE);

  if (SetCommsParam(speed, data, parity, stop, inputFlow, outputFlow))
    return true;

  ConvertOSError(-2);
  CloseHandle(m_commsResource);
  os_handle = -1;
  return false;
}


bool PSerialChannel::SetSpeed(uint32_t speed)
{
  return SetCommsParam(speed,
                  0, DefaultParity, 0, DefaultFlowControl, DefaultFlowControl);
}


uint32_t PSerialChannel::GetSpeed() const
{
  return m_deviceControlBlock.BaudRate;
}


bool PSerialChannel::SetDataBits(uint8_t data)
{
  return SetCommsParam(0,
               data, DefaultParity, 0, DefaultFlowControl, DefaultFlowControl);
}


uint8_t PSerialChannel::GetDataBits() const
{
  return m_deviceControlBlock.ByteSize;
}


bool PSerialChannel::SetParity(Parity parity)
{
  return SetCommsParam(0,0,parity,0,DefaultFlowControl,DefaultFlowControl);
}


PSerialChannel::Parity PSerialChannel::GetParity() const
{
  switch (m_deviceControlBlock.Parity) {
    case ODDPARITY :
      return OddParity;
    case EVENPARITY :
      return EvenParity;
    case MARKPARITY :
      return MarkParity;
    case SPACEPARITY :
      return SpaceParity;
  }
  return NoParity;
}


bool PSerialChannel::SetStopBits(uint8_t stop)
{
  return SetCommsParam(0,
               0, DefaultParity, stop, DefaultFlowControl, DefaultFlowControl);
}


uint8_t PSerialChannel::GetStopBits() const
{
  return (uint8_t)(m_deviceControlBlock.StopBits == ONESTOPBIT ? 1 : 2);
}


bool PSerialChannel::SetInputFlowControl(FlowControl flowControl)
{
  return SetCommsParam(0,0,DefaultParity,0,flowControl,DefaultFlowControl);
}


PSerialChannel::FlowControl PSerialChannel::GetInputFlowControl() const
{
  if (m_deviceControlBlock.fRtsControl == RTS_CONTROL_HANDSHAKE)
    return RtsCts;
  if (m_deviceControlBlock.fInX != 0)
    return XonXoff;
  return NoFlowControl;
}


bool PSerialChannel::SetOutputFlowControl(FlowControl flowControl)
{
  return SetCommsParam(0,0,DefaultParity,0,DefaultFlowControl,flowControl);
}


PSerialChannel::FlowControl PSerialChannel::GetOutputFlowControl() const
{
  if (m_deviceControlBlock.fOutxCtsFlow != 0)
    return RtsCts;
  if (m_deviceControlBlock.fOutX != 0)
    return XonXoff;
  return NoFlowControl;
}


void PSerialChannel::SetDTR(bool state)
{
  if (IsOpen())
    PAssertOS(EscapeCommFunction(m_commsResource, state ? SETDTR : CLRDTR));
  else
    SetErrorValues(NotOpen, EBADF);
}


void PSerialChannel::SetRTS(bool state)
{
  if (IsOpen())
    PAssertOS(EscapeCommFunction(m_commsResource, state ? SETRTS : CLRRTS));
  else
    SetErrorValues(NotOpen, EBADF);
}


void PSerialChannel::SetBreak(bool state)
{
  if (IsOpen())
    if (state)
      PAssertOS(SetCommBreak(m_commsResource));
    else
      PAssertOS(ClearCommBreak(m_commsResource));
  else
    SetErrorValues(NotOpen, EBADF);
}


bool PSerialChannel::GetCTS()
{
  if (CheckNotOpen())
    return false;

  DWORD stat;
  PAssertOS(GetCommModemStatus(m_commsResource, &stat));
  return (stat&MS_CTS_ON) != 0;
}


bool PSerialChannel::GetDSR()
{
  if (CheckNotOpen())
    return false;

  DWORD stat;
  PAssertOS(GetCommModemStatus(m_commsResource, &stat));
  return (stat&MS_DSR_ON) != 0;
}


bool PSerialChannel::GetDCD()
{
  if (CheckNotOpen())
    return false;

  DWORD stat;
  PAssertOS(GetCommModemStatus(m_commsResource, &stat));
  return (stat&MS_RLSD_ON) != 0;
}


bool PSerialChannel::GetRing()
{
  if (CheckNotOpen())
    return false;

  DWORD stat;
  PAssertOS(GetCommModemStatus(m_commsResource, &stat));
  return (stat&MS_RING_ON) != 0;
}


PStringList PSerialChannel::GetPortNames()
{
  PStringList ports;
  for (char p = 1; p <= 9; p++)
    ports.AppendString(psprintf("\\\\.\\COM%u", p));
  return ports;
}
