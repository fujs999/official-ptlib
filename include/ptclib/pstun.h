/*
 * pstun.h
 *
 * STUN client
 *
 * Portable Windows Library
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: pstun.h,v $
 * Revision 1.7  2004/02/24 11:15:48  rjongbloed
 * Added function to get external router address, also did a bunch of documentation.
 *
 * Revision 1.6  2004/01/17 17:54:02  rjongbloed
 * Added function to get server name from STUN client.
 *
 * Revision 1.5  2003/10/05 00:56:25  rjongbloed
 * Rewrite of STUN to not to use imported code with undesirable license.
 *
 * Revision 1.4  2003/02/05 06:26:49  robertj
 * More work in making the STUN usable for Symmetric NAT systems.
 *
 * Revision 1.3  2003/02/04 07:01:02  robertj
 * Added ip/port version of constructor.
 *
 * Revision 1.2  2003/02/04 05:05:55  craigs
 * Added new functions
 *
 * Revision 1.1  2003/02/04 03:31:04  robertj
 * Added STUN
 *
 */

#ifndef _PSTUN_H
#define _PSTUN_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>
#include <ptlib/sockets.h>


/**UDP socket that has been created by the STUN client.
  */
class PSTUNUDPSocket : public PUDPSocket
{
  PCLASSINFO(PSTUNUDPSocket, PUDPSocket);
  public:
    PSTUNUDPSocket();

    virtual BOOL GetLocalAddress(
      Address & addr    /// Variable to receive hosts IP address
    );
    virtual BOOL GetLocalAddress(
      Address & addr,    /// Variable to receive peer hosts IP address
      WORD & port        /// Variable to receive peer hosts port number
    );

  protected:
    PIPSocket::Address externalIP;

  friend class PSTUNClient;
};


/**STUN client.
  */
class PSTUNClient : public PObject
{
  PCLASSINFO(PSTUNClient, PObject);
  public:
    enum {
      DefaultPort = 3478
    };

    PSTUNClient(
      const PString & server,
      WORD portBase = 0,
      WORD portMax = 0,
      WORD portPairBase = 0,
      WORD portPairMax = 0
    );
    PSTUNClient(
      const PIPSocket::Address & serverAddress,
      WORD serverPort = DefaultPort,
      WORD portBase = 0,
      WORD portMax = 0,
      WORD portPairBase = 0,
      WORD portPairMax = 0
    );


    /**Get the current STUN server address and port being used.
      */
    PString GetServer() const;

    /**Set the STUN server to use.
       The server string may be of the form host:port. If :port is absent
       then the default port 3478 is used. The substring port can also be
       a service name as found in /etc/services. The host substring may be
       a DNS name or explicit IP address.
      */
    BOOL SetServer(
      const PString & server
    );

    /**Set the STUN server to use by IP address and port.
       If serverPort is zero then the default port of 3478 is used.
      */
    BOOL SetServer(
      const PIPSocket::Address & serverAddress,
      WORD serverPort = 0
    );

    /**Set the port ranges to be used on local machine.
       Note that the ports used on the NAT router may not be the same unless
       some form of port forwarding is present.

       If the port base is zero then standard operating system port allocation
       method is used.

       If the max port is zero then it will be automatically set to the port
       base + 99.
      */
    void SetPortRanges(
      WORD portBase,          /// Single socket port number base
      WORD portMax = 0,       /// Single socket port number max
      WORD portPairBase = 0,  /// Socket pair port number base
      WORD portPairMax = 0    /// Socket pair port number max
    );


    enum NatTypes {
      UnknownNat,
      OpenNat,
      ConeNat,
      RestrictedNat,
      PortRestrictedNat,
      SymmetricNat,
      SymmetricFirewall,
      BlockedNat,
      NumNatTypes
    };

    /**Determine via the STUN protocol the NAT type for the router.
       This will cache the last determine NAT type. Use the force variable to
       guarantee an up to date value.
      */
    NatTypes GetNatType(
      BOOL force = FALSE    /// Force a new check
    );

    /**Determine via the STUN protocol the NAT type for the router.
       As for GetNatType() but returns an English string for the type.
      */
    PString GetNatTypeName(
      BOOL force = FALSE    /// Force a new check
    );

    /**Determine the external router address.
       This will send UDP packets out using the STUN protocol to determine
       the intervening routers external IP address.

       A cached address is returned provided it is no older than the time
       specified.
      */
    BOOL GetExternalAddress(
      PIPSocket::Address & externalAddress, // External address of router
      const PTimeInterval & maxAge = 1000   // Maximum age for caching
    );

    /**Create a single socket.
       The STUN protocol is used to create a socket for which the external IP
       address and port numbers are known. A PUDPSocket descendant is returned
       which will, in response to GetLocalAddress() return the externally
       visible IP and port rather than the local machines IP and socket.

       The will create a new socket pointer. It is up to the caller to make
       sure the socket is deleted to avoid memory leaks.

       The socket pointer is set to NULL if the function fails and returns
       FALSE.
      */
    BOOL CreateSocket(
      PUDPSocket * & socket
    );

    /**Create a socket pair.
       The STUN protocol is used to create a pair of sockets with adjacent
       port numbers for which the external IP address and port numbers are
       known. PUDPSocket descendants are returned which will, in response
       to GetLocalAddress() return the externally visible IP and port rather
       than the local machines IP and socket.

       The will create new socket pointers. It is up to the caller to make
       sure the sockets are deleted to avoid memory leaks.

       The socket pointers are set to NULL if the function fails and returns
       FALSE.
      */
    BOOL CreateSocketPair(
      PUDPSocket * & socket1,
      PUDPSocket * & socket2
    );

  protected:
    void Construct();

    PIPSocket::Address serverAddress;
    WORD               serverPort;

    struct PortInfo {
      PMutex mutex;
      WORD   basePort;
      WORD   maxPort;
      WORD   currentPort;
    } singlePortInfo, pairedPortInfo;
    bool OpenSocket(PUDPSocket & socket, PortInfo & portInfo) const;

    int  numSocketsForPairing;

    NatTypes natType;
    PIPSocket::Address cachedExternalAddress;
    PTime              timeAddressObtained;
};



#endif // _PSTUN_H


// End of file ////////////////////////////////////////////////////////////////
