/*
 * serial.cxx
 *
 * Asynchronous Serial I/O channel class.
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

#include <ctype.h>


///////////////////////////////////////////////////////////////////////////////
// PSerialChannel

PSerialChannel::PSerialChannel()
{
  Construct();
}


PSerialChannel::PSerialChannel(const PString & port, uint32_t speed, uint8_t data,
       Parity parity, uint8_t stop, FlowControl inputFlow, FlowControl outputFlow)
{
  Construct();
  Open(port, speed, data, parity, stop, inputFlow, outputFlow);
}


#if P_CONFIG_FILE
PSerialChannel::PSerialChannel(PConfig & cfg)
{
  Construct();
  Open(cfg);
}
#endif // P_CONFIG_FILE


PSerialChannel::~PSerialChannel()
{
  Close();
}


#if P_CONFIG_FILE

static const char PortName[] = "PortName";
static const char PortSpeed[] = "PortSpeed";
static const char PortDataBits[] = "PortDataBits";
static const char PortParity[] = "PortParity";
static const char PortStopBits[] = "PortStopBits";
static const char PortInputFlow[] = "PortInputFlow";
static const char PortOutputFlow[] = "PortOutputFlow";


bool PSerialChannel::Open(PConfig & cfg)
{
  PStringList ports = GetPortNames();
  return Open(cfg.GetString(PortName, ports.front()),
              cfg.GetInteger(PortSpeed, 9600),
              (uint8_t)cfg.GetInteger(PortDataBits, 8),
              (PSerialChannel::Parity)cfg.GetInteger(PortParity, 1),
              (uint8_t)cfg.GetInteger(PortStopBits, 1),
              (PSerialChannel::FlowControl)cfg.GetInteger(PortInputFlow, 1),
              (PSerialChannel::FlowControl)cfg.GetInteger(PortOutputFlow, 1));
}


void PSerialChannel::SaveSettings(PConfig & cfg)
{
  cfg.SetString(PortName, GetName());
  cfg.SetInteger(PortSpeed, GetSpeed());
  cfg.SetInteger(PortDataBits, GetDataBits());
  cfg.SetInteger(PortParity, GetParity());
  cfg.SetInteger(PortStopBits, GetStopBits());
  cfg.SetInteger(PortInputFlow, GetInputFlowControl());
  cfg.SetInteger(PortOutputFlow, GetOutputFlowControl());
}

#endif // P_CONFIG_FILE


PString PSerialChannel::GetName() const
{
  return m_portName;
}


void PSerialChannel::ClearDTR()
{
  SetDTR(false);
}


void PSerialChannel::ClearRTS()
{
  SetRTS(false);
}


void PSerialChannel::ClearBreak()
{
  SetBreak(false);
}


// End Of File ///////////////////////////////////////////////////////////////
