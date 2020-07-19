/*
 * socket.cxx
 *
 * Berkley sockets classes implementation
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

#ifdef __GNUC__
#pragma implementation "sockets.h"
#pragma implementation "socket.h"
#pragma implementation "ipsock.h"
#pragma implementation "udpsock.h"
#pragma implementation "tcpsock.h"
#pragma implementation "ipdsock.h"
#pragma implementation "ethsock.h"
#endif

#include <ptlib.h>
#include <ptlib/sockets.h>


// Appears to have disappeared (I hope - statics impossible in Nucleus!)
//extern PSemaphore PX_iostreamMutex;

PSocket::~PSocket()
{
  os_close();
}

int PSocket::os_close()
{
  if (os_handle < 0)
    return -1;

  // send a shutdown to the other end
  ::shutdown(os_handle, 2);

  return PXClose();
}

int PSocket::os_socket(int af, int type, int protocol)
{
  // attempt to create a socket
  int handle;
  if ((handle = ::socket(af, type, protocol)) >= 0) {

    // make the socket non-blocking and close on exec
    if (!ConvertOSError(::fcntl(handle, F_SETFD, 1))) {
      ::shutdown(handle, 2);
      return -1;
    }
//PError << "socket " << handle << " created" << endl;
  }
  return handle;
}

bool PSocket::os_connect(struct sockaddr * addr, PINDEX size)
{
  int val = ::connect(os_handle, addr, size);
  if (val == 0 || errno != EINPROGRESS)
    return ConvertOSError(val);

  if (!PXSetIOBlock(PXConnectBlock, readTimeout))
    return false;

  // A successful select() call does not necessarily mean the socket connected OK.
  int optval = -1;
  socklen_t optlen = sizeof(optval);
  getsockopt(os_handle, SOL_SOCKET, SO_ERROR, (char *)&optval, &optlen);
  if (optval != 0) {
    errno = optval;
    return ConvertOSError(-1);
  }

  return true;
}


bool PSocket::os_accept(int sock, struct sockaddr * addr, PINDEX * size,
                       const PTimeInterval & timeout)
{
  if (!listener.PXSetIOBlock(PXAcceptBlock, listener.GetReadTimeout()))
    return SetErrorValues(listener.GetErrorCode(), listener.GetErrorNumber());

  return ConvertOSError(SetNonBlocking(::accept(listener.GetHandle(), addr, (socklen_t *)size)));
}


int PSocket::os_select(int maxHandle,
                   fd_set & readBits,
                   fd_set & writeBits,
                   fd_set & exceptionBits,
          const PIntArray & ,
      const PTimeInterval & timeout)
{
  struct timeval * tptr = NULL;
  struct timeval   timeout_val;
  if (timeout != PMaxTimeInterval) {
    if (timeout.GetMilliSeconds() < 1000L*60L*60L*24L) {
      timeout_val.tv_usec = (timeout.GetMilliSeconds() % 1000) * 1000;
      timeout_val.tv_sec  = timeout.GetSeconds();
      tptr                = &timeout_val;
    }
  }

  do {
    int result = ::select(maxHandle, &readBits, &writeBits, &exceptionBits, tptr);
    if (result >= 0)
      return result;
  } while (errno == EINTR);
  return -1;
}


PIPSocket::Address::Address(uint32_t dw)
{
  s_addr = dw;
}


PIPSocket::Address & PIPSocket::Address::operator=(uint32_t dw)
{
  s_addr = dw;
  return *this;
}


PIPSocket::Address::operator uint32_t() const
{
  return (uint32_t)s_addr;
}

uint8_t PIPSocket::Address::Byte1() const
{
  return *(((uint8_t *)&s_addr)+0);
}

uint8_t PIPSocket::Address::Byte2() const
{
  return *(((uint8_t *)&s_addr)+1);
}

uint8_t PIPSocket::Address::Byte3() const
{
  return *(((uint8_t *)&s_addr)+2);
}

uint8_t PIPSocket::Address::Byte4() const
{
  return *(((uint8_t *)&s_addr)+3);
}

PIPSocket::Address::Address(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
{
  uint8_t * p = (uint8_t *)&s_addr;
  p[0] = b1;
  p[1] = b2;
  p[2] = b3;
  p[3] = b4;
}

bool PIPSocket::IsLocalHost(const PString & hostname)
{
  if (hostname.IsEmpty())
    return true;

  if (hostname *= "localhost")
    return true;

  // lookup the host address using inet_addr, assuming it is a "." address
  Address addr = hostname;
  if (addr == 16777343)  // Is 127.0.0.1
    return true;
  if (addr == (uint32_t)-1)
    return false;

  if (!GetHostAddress(hostname, addr))
    return false;

  PUDPSocket sock;

  // get number of interfaces
  int ifNum;
#ifdef SIOCGIFNUM
  PAssert(::ioctl(sock.GetHandle(), SIOCGIFNUM, &ifNum) >= 0, "could not do ioctl for ifNum");
#else
  ifNum = 100;
#endif

  PBYTEArray buffer;
  struct ifconf ifConf;
  ifConf.ifc_len  = ifNum * sizeof(ifreq);
  ifConf.ifc_req = (struct ifreq *)buffer.GetPointer(ifConf.ifc_len);
  
  if (ioctl(sock.GetHandle(), SIOCGIFCONF, &ifConf) >= 0) {
#ifndef SIOCGIFNUM
    ifNum = ifConf.ifc_len / sizeof(ifreq);
#endif

    int num = 0;
    for (num = 0; num < ifNum; num++) {

      ifreq * ifName = ifConf.ifc_req + num;
      struct ifreq ifReq;
      strcpy(ifReq.ifr_name, ifName->ifr_name);

      if (ioctl(sock.GetHandle(), SIOCGIFFLAGS, &ifReq) >= 0) {
        int flags = ifReq.ifr_flags;
        if (ioctl(sock.GetHandle(), SIOCGIFADDR, &ifReq) >= 0) {
          if ((flags & IFF_UP) && (addr == Address(((sockaddr_in *)&ifReq.ifr_addr)->sin_addr)))
            return true;
        }
      }
    }
  }

  return false;
}


////////////////////////////////////////////////////////////////
//
//  PTCPSocket
//
bool PTCPSocket::Read(void * buf, PINDEX maxLen)

{
  lastReadCount = 0;

  // wait until select indicates there is data to read, or until
  // a timeout occurs
  if (!PXSetIOBlock(PXReadBlock, readTimeout)) {
    lastError     = Timeout;
    return false;
  }

  // attempt to read out of band data
  char buffer[32];
  int ooblen;
  while ((ooblen = ::recv(os_handle, buffer, sizeof(buffer), MSG_OOB)) > 0) 
    OnOutOfBand(buffer, ooblen);

    // attempt to read non-out of band data
  if (ConvertOSError(lastReadCount = ::recv(os_handle, (char *)buf, maxLen, 0)))
    return lastReadCount > 0;

  lastReadCount = 0;
  return false;
}


int PSocket::os_recvfrom(
      void * buf,     // Data to be written as URGENT TCP data.
      PINDEX len,     // Number of bytes pointed to by <CODE>buf</CODE>.
      int    flags,
      sockaddr * addr, // Address from which the datagram was received.
      PINDEX * addrlen)
{
  if (!PXSetIOBlock(PXReadBlock, readTimeout)) {
    lastError     = Timeout;
    lastReadCount = 0;
    return 0;
  }

  // attempt to read non-out of band data
  if (ConvertOSError(lastReadCount =
        ::recvfrom(os_handle, (char *)buf, len, flags, (sockaddr *)addr, (socklen_t *)addrlen)))
    return lastReadCount > 0;

  lastReadCount = 0;
  return -1;
}


int PSocket::os_sendto(
      const void * buf,   // Data to be written as URGENT TCP data.
      PINDEX len,         // Number of bytes pointed to by <CODE>buf</CODE>.
      int flags,
      sockaddr * addr, // Address to which the datagram is sent.
      PINDEX addrlen)  
{
  lastWriteCount = 0;

  if (!IsOpen()) {
    lastError     = NotOpen;
    return 0;
  }

  // attempt to write data
  int writeResult;
  if (addr != NULL)
    writeResult = ::sendto(os_handle, (char *)buf, len, flags, (sockaddr *)addr, addrlen);
  else
    writeResult = ::send(os_handle, (char *)buf, len, flags);
  if (writeResult > 0) {
//    PThread::Yield();
    lastWriteCount = writeResult;
    return -1;
  }

  if (errno != EWOULDBLOCK)
    return ConvertOSError(-1);

  if (!PXSetIOBlock(PXWriteBlock, writeTimeout)) {
    lastError     = Timeout;
    return 0;
  }

  // attempt to write data
  if (addr != NULL)
    lastWriteCount = ::sendto(os_handle, (char *)buf, len, flags, (sockaddr *)addr, addrlen);
  else
    lastWriteCount = ::send(os_handle, (char *)buf, len, flags);
  if (ConvertOSError(lastWriteCount))
    return lastWriteCount > 0;

  return -1;
}


bool PSocket::Read(void * buf, PINDEX len)
{
  if (os_handle < 0) {
    lastError = NotOpen;
    return false;
  }

  if (!PXSetIOBlock(PXReadBlock, readTimeout)) 
    return false;

  if (ConvertOSError(lastReadCount = ::recv(os_handle, (char *)buf, len, 0)))
    return lastReadCount > 0;

  lastReadCount = 0;
  return false;
}



//////////////////////////////////////////////////////////////////
//
//  PEthSocket
//

PEthSocket::PEthSocket(PINDEX, PINDEX, PINDEX)
{
  medium = MediumUnknown;
  filterMask = FilterDirected|FilterBroadcast;
  filterType = TypeAll;
  fakeMacHeader = false;
  ipppInterface = false;
}


PEthSocket::~PEthSocket()
{
  Close();
}


bool PEthSocket::Connect(const PString & interfaceName)
{
  Close();

  fakeMacHeader = false;
  ipppInterface = false;

  if (strncmp("eth", interfaceName, 3) == 0)
    medium = Medium802_3;
  else if (strncmp("lo", interfaceName, 2) == 0)
    medium = MediumLoop;
  else if (strncmp("sl", interfaceName, 2) == 0) {
    medium = MediumWan;
    fakeMacHeader = true;
  }
  else if (strncmp("ppp", interfaceName, 3) == 0) {
    medium = MediumWan;
    fakeMacHeader = true;
  }
  else if (strncmp("ippp", interfaceName, 4) == 0) {
    medium = MediumWan;
    ipppInterface = true;
  }
  else {
    lastError = NotFound;
    osError = ENOENT;
    return false;
  }

#ifdef SIOCGIFHWADDR
  PUDPSocket ifsock;
  struct ifreq ifr;
  ifr.ifr_addr.sa_family = AF_INET;
  strcpy(ifr.ifr_name, interfaceName);
  if (!ConvertOSError(ioctl(ifsock.GetHandle(), SIOCGIFHWADDR, &ifr)))
    return false;

  memcpy(&macAddress, ifr.ifr_hwaddr.sa_data, sizeof(macAddress));
#endif

  channelName = interfaceName;
  return OpenSocket();
}


bool PEthSocket::OpenSocket()
{
#ifdef SOCK_PACKET
  if (!ConvertOSError(os_handle = os_socket(AF_INET, SOCK_PACKET, htons(filterType))))
    return false;

  struct sockaddr addr;
  memset(&addr, 0, sizeof(addr));
  addr.sa_family = AF_INET;
  strcpy(addr.sa_data, channelName);
  if (!ConvertOSError(bind(os_handle, &addr, sizeof(addr)))) {
    os_close();
    os_handle = -1;
    return false;
  }
#endif

  return true;
}


bool PEthSocket::Close()
{
  SetFilter(FilterDirected, filterType);  // Turn off promiscuous mode
  return PSocket::Close();
}


bool PEthSocket::EnumInterfaces(PINDEX idx, PString & name)
{
  PUDPSocket ifsock;

  ifreq ifreqs[20]; // Maximum of 20 interfaces
  ifconf ifc;
  ifc.ifc_len = sizeof(ifreqs);
  ifc.ifc_buf = (caddr_t)ifreqs;
  if (!ConvertOSError(ioctl(ifsock.GetHandle(), SIOCGIFCONF, &ifc)))
    return false;

  int ifcount = ifc.ifc_len/sizeof(ifreq);
  int ifidx;
  for (ifidx = 0; ifidx < ifcount; ifidx++) {
    if (strchr(ifreqs[ifidx].ifr_name, ':') == NULL) {
      ifreq ifr;
      strcpy(ifr.ifr_name, ifreqs[ifidx].ifr_name);
      if (ioctl(ifsock.GetHandle(), SIOCGIFFLAGS, &ifr) == 0 &&
          (ifr.ifr_flags & IFF_UP) != 0 &&
           idx-- == 0) {
        name = ifreqs[ifidx].ifr_name;
        return true;
      }
    }
  }

  return false;
}


bool PEthSocket::GetAddress(Address & addr)
{
  if (!IsOpen())
    return false;

  addr = macAddress;
  return true;
}


bool PEthSocket::EnumIpAddress(PINDEX idx,
                               PIPSocket::Address & addr,
                               PIPSocket::Address & net_mask)
{
  if (!IsOpen())
    return false;

  PUDPSocket ifsock;
  struct ifreq ifr;
  strstream str;
  ifr.ifr_addr.sa_family = AF_INET;
  if (idx == 0)
    strcpy(ifr.ifr_name, channelName);
  else
  {
    str<<(const char *)channelName<<':'<<(int)(idx-1)<<'\x0';
    strcpy(ifr.ifr_name, str.str());
//    sprintf(ifr.ifr_name, "%s:%u", (const char *)channelName, (int)(idx-1));
  }
  if (!ConvertOSError(ioctl(os_handle, SIOCGIFADDR, &ifr)))
    return false;

  sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
  addr = sin->sin_addr;

  if (!ConvertOSError(ioctl(os_handle, SIOCGIFNETMASK, &ifr)))
    return false;

  net_mask = sin->sin_addr;
  return true;
}


bool PEthSocket::GetFilter(unsigned & mask, uint16_t & type)
{
  if (!IsOpen())
    return false;

  ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, channelName);
  if (!ConvertOSError(ioctl(os_handle, SIOCGIFFLAGS, &ifr)))
    return false;

  if ((ifr.ifr_flags&IFF_PROMISC) != 0)
    filterMask |= FilterPromiscuous;
  else
    filterMask &= ~FilterPromiscuous;

  mask = filterMask;
  type = filterType;
  return true;
}


bool PEthSocket::SetFilter(unsigned filter, uint16_t type)
{
  if (!IsOpen())
    return false;

  if (filterType != type) {
    os_close();
    filterType = type;
    if (!OpenSocket())
      return false;
  }

  ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, channelName);
  if (!ConvertOSError(ioctl(os_handle, SIOCGIFFLAGS, &ifr)))
    return false;

  if ((filter&FilterPromiscuous) != 0)
    ifr.ifr_flags |= IFF_PROMISC;
  else
    ifr.ifr_flags &= ~IFF_PROMISC;

  if (!ConvertOSError(ioctl(os_handle, SIOCSIFFLAGS, &ifr)))
    return false;

  filterMask = filter;

  return true;
}


PEthSocket::MediumTypes PEthSocket::GetMedium()
{
  return medium;
}


bool PEthSocket::ResetAdaptor()
{
  // No implementation
  return true;
}


bool PEthSocket::Read(void * buf, PINDEX len)
{
  static const uint8_t macHeader[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 8, 0 };

  uint8_t * bufptr = (uint8_t *)buf;

  if (fakeMacHeader) {
    if (len <= sizeof(macHeader)) {
      memcpy(bufptr, macHeader, len);
      lastReadCount = len;
      return true;
    }

    memcpy(bufptr, macHeader, sizeof(macHeader));
    bufptr += sizeof(macHeader);
    len -= sizeof(macHeader);
  }

  for (;;) {
    sockaddr from;
    PINDEX fromlen = sizeof(from);
    if (!os_recvfrom(bufptr, len, 0, &from, &fromlen))
      return false;

    if (channelName != from.sa_data)
      continue;

    if (ipppInterface) {
      if (lastReadCount <= 10)
        return false;
      if (memcmp(bufptr+6, "\xff\x03\x00\x21", 4) != 0) {
        memmove(bufptr+sizeof(macHeader), bufptr, lastReadCount);
        lastReadCount += sizeof(macHeader);
      }
      else {
        memmove(bufptr+sizeof(macHeader), bufptr+10, lastReadCount);
        lastReadCount += sizeof(macHeader)-10;
      }
      memcpy(bufptr, macHeader, sizeof(macHeader));
      break;
    }

    if (fakeMacHeader) {
      lastReadCount += sizeof(macHeader);
      break;
    }

    if ((filterMask&FilterPromiscuous) != 0)
      break;

    if ((filterMask&FilterDirected) != 0 && macAddress == bufptr)
      break;

    static const Address broadcast;
    if ((filterMask&FilterBroadcast) != 0 && broadcast == bufptr)
      break;
  }

  return lastReadCount > 0;
}


bool PEthSocket::Write(const void * buf, PINDEX len)
{
  sockaddr to;
  strcpy(to.sa_data, channelName);
  return os_sendto(buf, len, 0, &to, sizeof(to)) && lastWriteCount >= len;
}


///////////////////////////////////////////////////////////////////////////////

bool PIPSocket::GetGatewayAddress(Address & addr)
{
  RouteTable table;
  if (GetRouteTable(table)) {
    for (PINDEX i = 0; i < table.GetSize(); i++) {
      if (table[i].GetNetwork() == 0) {
        addr = table[i].GetDestination();
        return true;
      }
    }
  }
  return false;
}



PString PIPSocket::GetGatewayInterface()
{
  RouteTable table;
  if (GetRouteTable(table)) {
    for (PINDEX i = 0; i < table.GetSize(); i++) {
      if (table[i].GetNetwork() == 0)
        return table[i].GetInterface();
    }
  }
  return PString();
}


bool PIPSocket::GetRouteTable(RouteTable & table)
{
#if defined(P_LINUX)

  PTextFile procfile;
  if (!procfile.Open("/proc/net/route", PFile::ReadOnly))
    return false;

  for (;;) {
    // Ignore heading line or remainder of route line
    procfile.ignore(1000, '\n');
    if (procfile.eof())
      return true;

    char iface[20];
    long net_addr, dest_addr, net_mask;
    int flags, refcnt, use, metric;
    procfile >> iface >> ::hex >> net_addr >> dest_addr >> flags 
                      >> ::dec >> refcnt >> use >> metric 
                      >> ::hex >> net_mask;
    if (procfile.bad())
      return false;

    RouteEntry * entry = new RouteEntry(net_addr);
    entry->net_mask = net_mask;
    entry->destination = dest_addr;
    entry->interfaceName = iface;
    entry->metric = metric;
    table.Append(entry);
  }

#else
  return false;
#endif
}

#include "../common/pethsock.cxx"
