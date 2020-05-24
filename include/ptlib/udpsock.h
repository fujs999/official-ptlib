/*
 * udpsock.h
 *
 * User Datagram Protocol socket I/O channel class.
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

#ifndef PTLIB_UDPSOCKET_H
#define PTLIB_UDPSOCKET_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/ipdsock.h>
 
/**
   A socket channel that uses the UDP transport on the Internet Protocol.
 */
class PUDPSocket : public PIPDatagramSocket
{
  PCLASSINFO(PUDPSocket, PIPDatagramSocket);

  public:
  /**@name Construction */
  //@{
    /** Create a UDP socket. If a remote machine address or
       a "listening" socket is specified then the channel is also opened.
     */
    PUDPSocket(
      uint16_t port = 0,             ///< Port number to use for the connection.
      int iAddressFamily = AF_INET ///< Family
    );
    PUDPSocket(
      const PString & service,   ///< Service name to use for the connection.
      int iAddressFamily = AF_INET ///< Family
    );
    PUDPSocket(
      const PString & address,  ///< Address of remote machine to connect to.
      uint16_t port                 ///< Port number to use for the connection.
    );
    PUDPSocket(
      const PString & address,  ///< Address of remote machine to connect to.
      const PString & service   ///< Service name to use for the connection.
    );
  //@}

  /**@name Overrides from class PSocket */
  //@{
    /** Override of PChannel functions to allow connectionless reads
     */
    bool Read(
      void * buf,   ///< Pointer to a block of memory to read.
      PINDEX len    ///< Number of bytes to read.
    );

    /** Override of PChannel functions to allow connectionless writes
     */
    bool Write(
      const void * buf, ///< Pointer to a block of memory to write.
      PINDEX len        ///< Number of bytes to write.
    );

    /** Override of PSocket functions to allow connectionless writes
     */
    bool Connect(
      const PString & address   ///< Address of remote machine to connect to.
    );
  //@}

  /**@name New functions for class */
  //@{
    /** Set the address to use for connectionless Write() or Windows QoS.
        If the \p mtuDiscovery parameter is >= 0, then the socket MT discovery
        mode is set. See IP_MTU_DISCOVER socket option for mopre infor. Note
        the connect() function is used for exclusive use of this socket to one
        destination, thus WriteTo() will only work with this send address.
     */
    bool SetSendAddress(
      const Address & address,    ///< IP address to send packets.
      uint16_t port,                  ///< Port to send packets.
      int mtuDiscovery = -1       ///< MTU discovery mode
    );
    bool SetSendAddress(
      const PIPSocketAddressAndPort & addressAndPort, ///< IP address and port to send packets.
      int mtuDiscovery = -1                           ///< MTU discovery mode
    );

    /** Get the address to use for connectionless Write().
     */
    void GetSendAddress(
      Address & address,    ///< IP address to send packets.
      uint16_t & port           ///< Port to send packets.
    ) const;
    void GetSendAddress(
      PIPSocketAddressAndPort & addressAndPort
    ) const;
    PString GetSendAddress() const;

    /** Get the address of the sender in the last connectionless Read().
        Note that thsi only applies to the Read() and not the ReadFrom()
        function.
     */
    void GetLastReceiveAddress(
      Address & address,    ///< IP address to send packets.
      uint16_t & port           ///< Port to send packets.
    ) const;
    void GetLastReceiveAddress(
      PIPSocketAddressAndPort & addressAndPort    ///< IP address and port to send packets.
    ) const;
    PString GetLastReceiveAddress() const;

    /**Get the current MTU size.
       Note this usually needs to be enabled with SetSendAddress()
     */
    int GetCurrentMTU();
  //@}

    // Normally, one would expect these to be protected, but they are just so darn
    // useful that it's just easier if they are public
    virtual bool InternalReadFrom(Slice * slices, size_t sliceCount, PIPSocketAddressAndPort & ipAndPort);
    virtual bool InternalSetSendAddress(const PIPSocketAddressAndPort & addr, int mtuDiscovery = -1);
    virtual void InternalGetSendAddress(PIPSocketAddressAndPort & addr) const;
    virtual void InternalSetLastReceiveAddress(const PIPSocketAddressAndPort & addr);
    virtual void InternalGetLastReceiveAddress(PIPSocketAddressAndPort & addr) const;

  protected:
    // Open an IPv4 socket (for backward compatibility)
    virtual bool OpenSocket();

    // Open an IPv4 or IPv6 socket
    virtual bool OpenSocket(
      int ipAdressFamily
    );

    virtual bool InternalListen(const Address & bind, unsigned queueSize, uint16_t port, Reusability reuse);

    virtual const char * GetProtocolName() const;

// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/udpsock.h"
#else
#include "unix/ptlib/udpsock.h"
#endif

    private:
      AddressAndPort m_sendAddressAndPort;
      AddressAndPort m_lastReceiveAddressAndPort;
};


#endif // PTLIB_UDPSOCKET_H


// End Of File ///////////////////////////////////////////////////////////////
