/*
 * telnet.cxx
 *
 * TELNET socket I/O channel class.
 *
 * Portable Tools Library
 *
 * Copyright (c) 1993-2002 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 */

#ifdef __GNUC__
#pragma implementation "telnet.h"
#endif

#include <ptlib.h>
#include <ptlib/sockets.h>
#include <ptclib/telnet.h>


//////////////////////////////////////////////////////////////////////////////
// PTelnetSocket

PTelnetSocket::PTelnetSocket()
  : PTCPSocket("telnet")
{
  Construct();
}


PTelnetSocket::PTelnetSocket(const PString & address)
  : PTCPSocket("telnet")
{
  Construct();
  Connect(address);
}


void PTelnetSocket::Construct()
{
  m_synchronising = 0;
  m_terminalType = "UNKNOWN";
  m_windowWidth = m_windowHeight = 0;
  m_state = StateNormal;

  memset(m_option, 0, sizeof(m_option));
  SetOurOption(TransmitBinary);
  SetOurOption(SuppressGoAhead);
  SetOurOption(StatusOption);
  SetOurOption(TimingMark);
  SetOurOption(TerminalSpeed);
  SetOurOption(TerminalType);
  SetTheirOption(TransmitBinary);
  SetTheirOption(SuppressGoAhead);
  SetTheirOption(StatusOption);
  SetTheirOption(TimingMark);
  SetTheirOption(EchoOption);
}


bool PTelnetSocket::Connect(const PString & host)
{
  PTRACE(3, "Telnet\tConnecting to " << host);

  if (!PTCPSocket::Connect(host))
    return false;

  SendDo(SuppressGoAhead);
  SendDo(StatusOption);
  SendWill(TerminalSpeed);
  return true;
}


bool PTelnetSocket::Accept(PSocket & sock)
{
  if (!PTCPSocket::Accept(sock))
    return false;

  SendDo(SuppressGoAhead);
  SendWill(StatusOption);
  return true;
}


bool PTelnetSocket::Write(void const * buffer, PINDEX length)
{
  const uint8_t * base = (const uint8_t *)buffer;
  const uint8_t * next = base;
  int count = 0;

  while (length > 0) {
    if (*next == '\r' &&
            !(length > 1 && next[1] == '\n') && !IsOurOption(TransmitBinary)) {
      // send the characters
      if (!PTCPSocket::Write(base, (next - base) + 1))
        return false;
      count += GetLastWriteCount();

      char null = '\0';
      if (!PTCPSocket::Write(&null, 1))
        return false;
      count += GetLastWriteCount();

      base = next+1;
    }

    if (*next == IAC) {
      // send the characters
      if (!PTCPSocket::Write(base, (next - base) + 1))
        return false;
      count += GetLastWriteCount();
      base = next;
    }

    next++;
    length--;
  }

  if (next > base) {
    if (!PTCPSocket::Write(base, next - base))
      return false;
    count += GetLastWriteCount();
  }

  SetLastWriteCount(count);
  return true;
}


bool PTelnetSocket::SetLocalEcho(bool localEcho)
{
  return localEcho ? SendWont(EchoOption) : SendWill(EchoOption);
}


bool PTelnetSocket::SendCommand(Command cmd, int opt)
{
  uint8_t buffer[3];
  buffer[0] = IAC;
  buffer[1] = (uint8_t)cmd;

  switch (cmd) {
    case DO :
    case DONT :
    case WILL :
    case WONT :
      buffer[2] = (uint8_t)opt;
      return PTCPSocket::Write(buffer, 3);

    case InterruptProcess :
    case Break :
    case AbortProcess :
    case SuspendProcess :
    case AbortOutput :
      if (opt) {
        // Send the command
        if (!PTCPSocket::Write(buffer, 2))
          return false;
        // Send a TimingMark for output flush.
        buffer[1] = TimingMark;
        if (!PTCPSocket::Write(buffer, 2))
          return false;
        // Send a DataMark for synchronisation.
        if (cmd != AbortOutput) {
          buffer[1] = DataMark;
          if (!PTCPSocket::Write(buffer, 2))
            return false;
          // Send the datamark character as the only out of band data byte.
          if (!WriteOutOfBand(&buffer[1], 1))
            return false;
        }
        // Then flush any waiting input data.
        PTimeInterval oldTimeout = readTimeout;
        readTimeout = 0;
        while (PTCPSocket::Read(buffer, sizeof(buffer)))
          ;
        readTimeout = oldTimeout;
      }
      break;

    default :
      return PTCPSocket::Write(buffer, 2);
  }

  return true;
}


#if PTRACING
  static PString GetTELNETOptionName(PINDEX code)
  {
    static const char * const name[] = {
      "TransmitBinary",
      "EchoOption",
      "ReconnectOption",
      "SuppressGoAhead",
      "MessageSizeOption",
      "StatusOption",
      "TimingMark",
      "RCTEOption",
      "OutputLineWidth",
      "OutputPageSize",
      "CRDisposition",
      "HorizontalTabsStops",
      "HorizTabDisposition",
      "FormFeedDisposition",
      "VerticalTabStops",
      "VertTabDisposition",
      "LineFeedDisposition",
      "ExtendedASCII",
      "ForceLogout",
      "ByteMacroOption",
      "DataEntryTerminal",
      "SupDupProtocol",
      "SupDupOutput",
      "SendLocation",
      "TerminalType",
      "EndOfRecordOption",
      "TACACSUID",
      "OutputMark",
      "TerminalLocation",
      "Use3270RegimeOption",
      "UseX3PADOption",
      "WindowSize",
      "TerminalSpeed",
      "FlowControl",
      "LineMode",
      "XDisplayLocation",
      "EnvironmentOption",
      "AuthenticateOption",
      "EncriptionOption"
    };

    if (code < PARRAYSIZE(name))
      return name[code];
    if (code == PTelnetSocket::ExtendedOptionsList)
      return "ExtendedOptionsList";
    return PString(PString::Printf, "Option #%u", code);
  }

  struct PTelnetTrace
  {
    ostream & m_strm;
    PTelnetTrace(const char * file, int line) : m_strm(PTrace::Begin(3, file, line)) { }
    ~PTelnetTrace() { m_strm << PTrace::End; }
  };
  #define SEND_OP_START(which, code) \
     PTelnetTrace traceOutput(__FILE__, __LINE__); \
     traceOutput.m_strm << which << ' ' << GetTELNETOptionName(code) << ' '; \
     if (IsOpen()) ; else { traceOutput.m_strm << "not open yet."; return SetErrorValues(NotOpen, EBADF); }
  #define ON_OP_START(which, code) \
     PTelnetTrace traceOutput(__FILE__, __LINE__); \
     traceOutput.m_strm << which << ' ' << GetTELNETOptionName(code) << ' '
  #define TELNET_TRACE(info) traceOutput.m_strm << info
#else
  #define SEND_OP_START(which, code) if ((IsOpen() || SetErrorValues(NotOpen, EBADF))) return false
  #define ON_OP_START(which, code)
  #define TELNET_TRACE(info)
#endif

bool PTelnetSocket::SendDo(uint8_t code)
{
  SEND_OP_START("SendDo", code);

  OptionInfo & opt = m_option[code];

  switch (opt.theirState) {
    case OptionInfo::IsNo :
      TELNET_TRACE("initiated.");
      SendCommand(DO, code);
      opt.theirState = OptionInfo::WantYes;
      break;

    case OptionInfo::IsYes :
      TELNET_TRACE("already enabled.");
      return false;

    case OptionInfo::WantNo :
      TELNET_TRACE("queued.");
      opt.theirState = OptionInfo::WantNoQueued;
      break;

    case OptionInfo::WantNoQueued :
      TELNET_TRACE("already queued.");
      opt.theirState = OptionInfo::IsNo;
      return false;

    case OptionInfo::WantYes :
      TELNET_TRACE("already negotiating.");
      opt.theirState = OptionInfo::IsNo;
      return false;

    case OptionInfo::WantYesQueued :
      TELNET_TRACE("dequeued.");
      opt.theirState = OptionInfo::WantYes;
      break;
  }

  return true;
}


bool PTelnetSocket::SendDont(uint8_t code)
{
  SEND_OP_START("SendDont", code);

  OptionInfo & opt = m_option[code];

  switch (opt.theirState) {
    case OptionInfo::IsNo :
      TELNET_TRACE("already disabled.");
      return false;

    case OptionInfo::IsYes :
      TELNET_TRACE("initiated.");
      SendCommand(DONT, code);
      opt.theirState = OptionInfo::WantNo;
      break;

    case OptionInfo::WantNo :
      TELNET_TRACE("already negotiating.");
      opt.theirState = OptionInfo::IsNo;
      return false;

    case OptionInfo::WantNoQueued :
      TELNET_TRACE("dequeued.");
      opt.theirState = OptionInfo::WantNo;
      break;

    case OptionInfo::WantYes :
      TELNET_TRACE("queued.");
      opt.theirState = OptionInfo::WantYesQueued;
      break;

    case OptionInfo::WantYesQueued :
      TELNET_TRACE("already queued.");
      opt.theirState = OptionInfo::IsYes;
      return false;
  }

  return true;
}


bool PTelnetSocket::SendWill(uint8_t code)
{
  SEND_OP_START("SendWill", code);

  OptionInfo & opt = m_option[code];

  switch (opt.ourState) {
    case OptionInfo::IsNo :
      TELNET_TRACE("initiated.");
      SendCommand(WILL, code);
      opt.ourState = OptionInfo::WantYes;
      break;

    case OptionInfo::IsYes :
      TELNET_TRACE("already enabled.");
      return false;

    case OptionInfo::WantNo :
      TELNET_TRACE("queued.");
      opt.ourState = OptionInfo::WantNoQueued;
      break;

    case OptionInfo::WantNoQueued :
      TELNET_TRACE("already queued.");
      opt.ourState = OptionInfo::IsNo;
      return false;

    case OptionInfo::WantYes :
      TELNET_TRACE("already negotiating.");
      opt.ourState = OptionInfo::IsNo;
      return false;

    case OptionInfo::WantYesQueued :
      TELNET_TRACE("dequeued.");
      opt.ourState = OptionInfo::WantYes;
      break;
  }

  return true;
}


bool PTelnetSocket::SendWont(uint8_t code)
{
  SEND_OP_START("SendWont", code);

  OptionInfo & opt = m_option[code];

  switch (opt.ourState) {
    case OptionInfo::IsNo :
      TELNET_TRACE("already disabled.");
      return false;

    case OptionInfo::IsYes :
      TELNET_TRACE("initiated.");
      SendCommand(WONT, code);
      opt.ourState = OptionInfo::WantNo;
      break;

    case OptionInfo::WantNo :
      TELNET_TRACE("already negotiating.");
      opt.ourState = OptionInfo::IsNo;
      return false;

    case OptionInfo::WantNoQueued :
      TELNET_TRACE("dequeued.");
      opt.ourState = OptionInfo::WantNo;
      break;

    case OptionInfo::WantYes :
      TELNET_TRACE("queued.");
      opt.ourState = OptionInfo::WantYesQueued;
      break;

    case OptionInfo::WantYesQueued :
      TELNET_TRACE("already queued.");
      opt.ourState = OptionInfo::IsYes;
      return false;
  }

  return true;
}


bool PTelnetSocket::SendSubOption(uint8_t code, const uint8_t * info, PINDEX len, int subCode)
{
  {
    SEND_OP_START("SendSubOption", code);
    TELNET_TRACE("with " << len << " bytes.");
  }

  PBYTEArray buffer(len + 6);
  buffer[0] = IAC;
  buffer[1] = SB;
  buffer[2] = code;
  PINDEX i = 3;
  if (subCode >= 0)
    buffer[i++] = (uint8_t)subCode;
  while (len-- > 0) {
    if (*info == IAC)
      buffer[i++] = IAC;
    buffer[i++] = *info++;
  }
  buffer[i++] = IAC;
  buffer[i++] = SE;

  return PTCPSocket::Write((const uint8_t *)buffer, i);
}


void PTelnetSocket::SetTerminalType(const PString & newType)
{
  m_terminalType = newType;
}


void PTelnetSocket::SetWindowSize(uint16_t width, uint16_t height)
{
  m_windowWidth = width;
  m_windowHeight = height;
  if (IsOurOption(WindowSize)) {
    uint8_t buffer[4];
    buffer[0] = (uint8_t)(width >> 8);
    buffer[1] = (uint8_t)width;
    buffer[2] = (uint8_t)(height >> 8);
    buffer[3] = (uint8_t)height;
    SendSubOption(WindowSize, buffer, sizeof(buffer));
  }
  else {
    SetOurOption(WindowSize);
    SendWill(WindowSize);
  }
}


void PTelnetSocket::GetWindowSize(uint16_t & width, uint16_t & height) const
{
  width = m_windowWidth;
  height = m_windowHeight;
}


bool PTelnetSocket::Read(void * data, PINDEX bytesToRead)
{
  PBYTEArray buffer(bytesToRead);
  PINDEX charsLeft = bytesToRead;
  uint8_t * dst = (uint8_t *)data;

  while (charsLeft > 0) {
    uint8_t * src = buffer.GetPointer(charsLeft);
    if (!PTCPSocket::Read(src, charsLeft))
      return SetLastReadCount(bytesToRead - charsLeft) > 0;

    PINDEX readCount = GetLastReadCount();
    while (readCount > 0) {
      uint8_t currentByte = *src++;
      readCount--;
      switch (m_state) {
        case StateCarriageReturn :
          m_state = StateNormal;
          if (currentByte == '\0')
            break; // Ignore \0 after CR
          // Else, fall through for normal processing

        case StateNormal :
          if (currentByte == IAC)
            m_state = StateIAC;
          else {
            if (currentByte == '\r' && !IsTheirOption(TransmitBinary))
              m_state = StateCarriageReturn;
            *dst++ = currentByte;
            charsLeft--;
          }
          break;

        case StateIAC :
          switch (currentByte) {
            case IAC :
              m_state = StateNormal;
              *dst++ = IAC;
              charsLeft--;
              break;

            case DO :
              m_state = StateDo;
              break;

            case DONT :
              m_state = StateDont;
              break;

            case WILL :
              m_state = StateWill;
              break;

            case WONT :
              m_state = StateWont;
              break;

            case DataMark :    // data stream portion of a Synch
              /* We may have missed an urgent notification, so make sure we
                 flush whatever is in the buffer currently.
               */
              PTRACE(3, "Telnet\tReceived DataMark");
              if (m_synchronising > 0)
                m_synchronising--;
              break;

            case SB :          // subnegotiation start
              m_state = StateSubNegotiations;
              m_subOption.SetSize(0);
              break;

            default:
              if (OnCommand(currentByte))
                m_state = StateNormal;
              break;
          }
          break;

        case StateDo :
          OnDo(currentByte);
          m_state = StateNormal;
          break;

        case StateDont :
          OnDont(currentByte);
          m_state = StateNormal;
          break;

        case StateWill :
          OnWill(currentByte);
          m_state = StateNormal;
          break;

        case StateWont :
          OnWont(currentByte);
          m_state = StateNormal;
          break;

        case StateSubNegotiations :
          if (currentByte == IAC)
            m_state = StateEndNegotiations;
          else
            m_subOption[m_subOption.GetSize()] = currentByte;
          break;

        case StateEndNegotiations :
          if (currentByte == SE)
            m_state = StateNormal;
          else if (currentByte != IAC) {
            /* This is an error.  We only expect to get "IAC IAC" or "IAC SE".
               Several things may have happend.  An IAC was not doubled, the
               IAC SE was left off, or another option got inserted into the
               suboption are all possibilities. If we assume that the IAC was
               not doubled, and really the IAC SE was left off, we could get
               into an infinate loop here.  So, instead, we terminate the
               suboption, and process the partial suboption if we can.
             */
            m_state = StateIAC;
            src--;  // Go back to character for IAC ccommand
          }
          else {
            m_subOption[m_subOption.GetSize()] = currentByte;
            m_state = StateSubNegotiations;
            break;  // Was IAC IAC, subnegotiation not over yet.
          }
          if (m_subOption.GetSize() > 1 && IsOurOption(m_subOption[0]))
            OnSubOption(m_subOption[0], ((const uint8_t*)m_subOption)+1, m_subOption.GetSize()-1);
          break;

        default :
          PTRACE(2, "Telnet\tIllegal state: " << (int)m_state);
          m_state = StateNormal;
      }
      if (m_synchronising > 0) {
        charsLeft = bytesToRead;    // Flush data being received.
        dst = (uint8_t *)data;
      }
    }
  }
  SetLastReadCount(bytesToRead);
  return true;
}


void PTelnetSocket::OnDo(uint8_t code)
{
  {
    ON_OP_START("OnDo", code);

  OptionInfo & opt = m_option[code];

  switch (opt.ourState) {
    case OptionInfo::IsNo :
      if (opt.weCan) {
          TELNET_TRACE("WILL.");
        SendCommand(WILL, code);
        opt.ourState = OptionInfo::IsYes;
      }
      else {
          TELNET_TRACE("WONT.");
        SendCommand(WONT, code);
      }
      break;

    case OptionInfo::IsYes :
        TELNET_TRACE("ignored.");
      break;

    case OptionInfo::WantNo :
        TELNET_TRACE("is answer to WONT.");
      opt.ourState = OptionInfo::IsNo;
      break;

    case OptionInfo::WantNoQueued :
        TELNET_TRACE("impossible answer.");
      opt.ourState = OptionInfo::IsYes;
      break;

    case OptionInfo::WantYes :
        TELNET_TRACE("accepted.");
      opt.ourState = OptionInfo::IsYes;
      break;

    case OptionInfo::WantYesQueued :
        TELNET_TRACE("refused.");
      opt.ourState = OptionInfo::WantNo;
      SendCommand(WONT, code);
      break;
  }
  }

  if (IsOurOption(code)) {
    switch (code) {
      case TerminalSpeed : {
          static uint8_t defSpeed[] = "38400,38400";
          SendSubOption(TerminalSpeed, defSpeed, sizeof(defSpeed)-1, SubOptionIs);
        }
        break;

      case TerminalType :
        SendSubOption(TerminalType, m_terminalType, m_terminalType.GetLength(), SubOptionIs);
        break;

      case WindowSize :
        SetWindowSize(m_windowWidth, m_windowHeight);
        break;
    }
  }
}


void PTelnetSocket::OnDont(uint8_t code)
{
  ON_OP_START("OnDont", code);

  OptionInfo & opt = m_option[code];

  switch (opt.ourState) {
    case OptionInfo::IsNo :
      TELNET_TRACE("ignored.");
      break;

    case OptionInfo::IsYes :
      TELNET_TRACE("WONT.");
      opt.ourState = OptionInfo::IsNo;
      SendCommand(WONT, code);
      break;

    case OptionInfo::WantNo :
      TELNET_TRACE("disabled.");
      opt.ourState = OptionInfo::IsNo;
      break;

    case OptionInfo::WantNoQueued :
      TELNET_TRACE("accepting.");
      opt.ourState = OptionInfo::WantYes;
      SendCommand(DO, code);
      break;

    case OptionInfo::WantYes :
      TELNET_TRACE("queued disable.");
      opt.ourState = OptionInfo::IsNo;
      break;

    case OptionInfo::WantYesQueued :
      TELNET_TRACE("refused.");
      opt.ourState = OptionInfo::IsNo;
      break;
  }
}


void PTelnetSocket::OnWill(uint8_t code)
{
  ON_OP_START("OnWill", code);

  OptionInfo & opt = m_option[code];

  switch (opt.theirState) {
    case OptionInfo::IsNo :
      if (opt.theyShould) {
        TELNET_TRACE("DO.");
        SendCommand(DO, code);
        opt.theirState = OptionInfo::IsYes;
      }
      else {
        TELNET_TRACE("DONT.");
        SendCommand(DONT, code);
      }
      break;

    case OptionInfo::IsYes :
      TELNET_TRACE("ignored.");
      break;

    case OptionInfo::WantNo :
      TELNET_TRACE("is answer to DONT.");
      opt.theirState = OptionInfo::IsNo;
      break;

    case OptionInfo::WantNoQueued :
      TELNET_TRACE("impossible answer.");
      opt.theirState = OptionInfo::IsYes;
      break;

    case OptionInfo::WantYes :
      TELNET_TRACE("accepted.");
      opt.theirState = OptionInfo::IsYes;
      break;

    case OptionInfo::WantYesQueued :
      TELNET_TRACE("refused.");
      opt.theirState = OptionInfo::WantNo;
      SendCommand(DONT, code);
      break;
  }
}


void PTelnetSocket::OnWont(uint8_t code)
{
  ON_OP_START("OnWont", code);

  OptionInfo & opt = m_option[code];

  switch (opt.theirState) {
    case OptionInfo::IsNo :
      TELNET_TRACE("ignored.");
      break;

    case OptionInfo::IsYes :
      TELNET_TRACE("DONT.");
      opt.theirState = OptionInfo::IsNo;
      SendCommand(DONT, code);
      break;

    case OptionInfo::WantNo :
      TELNET_TRACE("disabled.");
      opt.theirState = OptionInfo::IsNo;
      break;

    case OptionInfo::WantNoQueued :
      TELNET_TRACE("accepting.");
      opt.theirState = OptionInfo::WantYes;
      SendCommand(DO, code);
      break;

    case OptionInfo::WantYes :
      TELNET_TRACE("refused.");
      opt.theirState = OptionInfo::IsNo;
      break;

    case OptionInfo::WantYesQueued :
      TELNET_TRACE("queued refusal.");
      opt.theirState = OptionInfo::IsNo;
      break;
  }
}


void PTelnetSocket::OnSubOption(uint8_t code, const uint8_t * info, PINDEX PTRACE_PARAM(len))
{
  ON_OP_START("OnSubOption", code);

  switch (code) {
    case TerminalType :
      if (*info == SubOptionSend) {
        TELNET_TRACE("TerminalType");
        SendSubOption(TerminalType, m_terminalType, m_terminalType.GetLength(), SubOptionIs);
      }
      break;

    case TerminalSpeed :
      if (*info == SubOptionSend) {
        TELNET_TRACE("TerminalSpeed");
        static uint8_t defSpeed[] = "38400,38400";
        SendSubOption(TerminalSpeed, defSpeed, sizeof(defSpeed)-1, SubOptionIs);
      }
      break;

    default :
      TELNET_TRACE(" of " << len << " bytes.");
  }
}


bool PTelnetSocket::OnCommand(uint8_t code)
{
  if (code == NOP)
    return true;
  PTRACE(2, "Telnet\tunknown command " << (int)code);
  return true;
}


void PTelnetSocket::OnOutOfBand(const void *, PINDEX PTRACE_PARAM(length))
{
  PTRACE(3, "Telnet\tout of band data received of length " << length);
  m_synchronising++;
}


// End Of File ///////////////////////////////////////////////////////////////
