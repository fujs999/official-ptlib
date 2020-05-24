/*
 * socket.h
 *
 * Berkley Socket channel ancestor class.
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

#ifndef PTLIB_SOCKET_H
#define PTLIB_SOCKET_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/channel.h>

#ifdef P_OPENBSD
#include <sys/uio.h>
#endif

#ifdef __NUCLEUS_PLUS__
#include <sys/socket.h>
#endif


class PSocket;

typedef PList<PSocket> PSocketList;


/**A network communications channel. This is based on the concepts in the
   Berkley Sockets library.

   A socket represents a bidirectional communications channel to a <i>port</i>
   at a remote <i>host</i>.
 */
class PSocket : public PChannel
{
  PCLASSINFO(PSocket, PChannel);

  protected:
    PSocket();

  public:
  /**@name Socket establishment functions */
  //@{
    /**Connect a socket to a remote host on the specified port number. This is
       typically used by the client or initiator of a communications channel.
       This connects to a "listening" socket at the other end of the
       communications channel.

       Use the SetReadTimeout() function to set a maximum time for the Connect().

       @return
       true if the channel was successfully connected to the remote host.
     */
    virtual bool Connect(
      const PString & address   ///< Address of remote machine to connect to.
    );


    /// Flags to reuse of port numbers in Listen() function.
    enum Reusability {
      CanReuseAddress,
      AddressIsExclusive
    };

    /**Listen on a socket for a remote host on the specified port number. This
       may be used for server based applications. A "connecting" socket begins
       a connection by initiating a connection to this socket. An active socket
       of this type is then used to generate other "accepting" sockets which
       establish a two way communications channel with the "connecting" socket.

       If the \p port parameter is zero then the port number as
       defined by the object instance construction or the descendent classes
       SetPort() or SetService() function.

       @return
       true if the channel was successfully opened.
     */
    virtual bool Listen(
      unsigned queueSize = 5,  ///< Number of pending accepts that may be queued.
      uint16_t port = 0,           ///< Port number to use for the connection.
      Reusability reuse = AddressIsExclusive ///< Can/Cant listen more than once.
    );


    /**Open a socket to a remote host on the specified port number. This is an
       "accepting" socket. When a "listening" socket has a pending connection
       to make, this will accept a connection made by the "connecting" socket
       created to establish a link.

       The port that the socket uses is the one used in the Listen()
       command of the \p socket parameter. Note an error occurs if
       the \p socket parameter has not had the Listen()
       function called on it.

       Note that this function will block until a remote system connects to the
       port number specified in the "listening" socket. The time that the
       function will block is determined by the read timeout of the
       \p socket parameter. This will normally be
       PMaxTimeInterval which indicates an infinite time.

       The default behaviour is to assert.

       @return
       true if the channel was successfully opened.
     */
    virtual bool Accept(
      PSocket & socket          ///< Listening socket making the connection.
    );

    /**Close one or both of the data streams associated with a socket.

       @return
       true if the shutdown was performed
     */
    virtual bool Shutdown(
      ShutdownValue option   ///< Flag for shutdown of read, write or both.
    );
  //@}

  /**@name Socket options functions */
  //@{
    /**Set options on the socket. These options are defined as Berkeley socket
       options of the class SOL_SOCKET.

       @return
       true if the option was successfully set.
     */
    bool SetOption(
      int option,             ///< Option to set.
      int value,              ///< New value for option.
      int level = SOL_SOCKET  ///< Level for option.
    );

    /**Set options on the socket. These options are defined as Berkeley socket
       options of the class SOL_SOCKET.

       @return
       true if the option was successfully set.
     */
    bool SetOption(
      int option,             ///< Option to set.
      const void * valuePtr,  ///< Pointer to new value for option.
      PINDEX valueSize,       ///< Size of new value.
      int level = SOL_SOCKET  ///< Level for option.
    );

    /**Get options on the socket. These options are defined as Berkeley socket
       options of the class SOL_SOCKET.

       @return
       true if the option was successfully retrieved.
     */
    bool GetOption(
      int option,             ///< Option to get.
      int & value,            ///< Integer to receive value.
      int level = SOL_SOCKET  ///< Level for option.
    );

    /**Get options on the socket. These options are defined as Berkeley socket
       options of the class SOL_SOCKET.

       @return
       true if the option was successfully retrieved.
     */
    bool GetOption(
      int option,             ///< Option to get.
      void * valuePtr,        ///< Pointer to buffer for value.
      PINDEX valueSize,       ///< Size of buffer to receive value.
      int level = SOL_SOCKET  ///< Level for option
    );
  //@}

  /**@name Port/Service database functions */
  //@{
    /**Get the number of the protocol associated with the specified name.

       @return
       Number of protocol or 0 if the protocol was not found.
     */
    static uint16_t GetProtocolByName(
      const PString & name      ///< Name of protocol.
    );

    /**Get the name of the protocol number specified.

       @return
       Name of protocol or the number if the protocol was not found.
     */
    static PString GetNameByProtocol(
      uint16_t proto                ///< Number of protocol.
    );


    /**Get the port number for the specified service name. */
    virtual uint16_t GetPortByService(
      const PString & service   ///< Name of service to get port number for.
    ) const;
    /**Get the port number for the specified service name.

       A name is a unique string contained in a system database. The parameter
       here may be either this unique name, an integer value or both separated
       by a space (name then integer). In the latter case the integer value is
       used if the name cannot be found in the database.

       The exact behviour of this function is dependent on whether TCP or UDP
       transport is being used. The PTCPSocket and PUDPSocket
       classes will implement this function.

       The static version of the function is independent of the socket type as
       its first parameter may be "tcp" or "udp".

       @return
       Port number for service name, or 0 if service cannot be found.
     */
    static uint16_t GetPortByService(
      const char * protocol,     ///< Protocol type for port lookup.
      const PString & service    ///< Name of service to get port number for.
    );

    /**Get the service name from the port number. */
    virtual PString GetServiceByPort(
      uint16_t port   ///< Number for service to find name of.
    ) const;
    /**Get the service name from the port number.

       A service name is a unique string contained in a system database. The
       parameter here may be either this unique name, an integer value or both
       separated by a space (name then integer). In the latter case the
       integer value is used if the name cannot be found in the database.

       The exact behviour of this function is dependent on whether TCP or UDP
       transport is being used. The PTCPSocket and PUDPSocket
       classes will implement this function.

       The static version of the function is independent of the socket type as
       its first parameter may be "tcp" or "udp".

       @return
       Service name for port number.
     */
    static PString GetServiceByPort(
      const char * protocol,  ///< Protocol type for port lookup
      uint16_t port   ///< Number for service to find name of.
    );


    /**Set the port number for the channel. */
    void SetPort(
      uint16_t port   ///< New port number for the channel.
    );
    /**Set the port number for the channel. This a 16 bit number representing
       an agreed high level protocol type. The string version looks up a
       database of names to find the number for the string name.

       A service name is a unique string contained in a system database. The
       parameter here may be either this unique name, an integer value or both
       separated by a space (name then integer). In the latter case the
       integer value is used if the name cannot be found in the database.

       The port number may not be changed while the port is open and the
       function will assert if an attempt is made to do so.
     */
    void SetPort(
      const PString & service   ///< Service name to describe the port number.
    );

    /**Get the port the TCP socket channel object instance is using.

       @return
       Port number.
     */
    uint16_t GetPort() const;

    /**Get a service name for the port number the TCP socket channel object
       instance is using.

       @return
       String service name or a string representation of the port number if no
       service with that number can be found.
     */
    PString GetService() const;
  //@}

  /**@name Multiple socket selection functions */
  //@{
    /// List of sockets used for Select() function.
    class SelectList : public PSocketList
    {
      PCLASSINFO(SelectList, PSocketList)
      public:
        SelectList()
          { DisallowDeleteObjects(); }
        /** Add a socket to list .*/
        void operator+=(PSocket & sock /** Socket to add. */)
          { Append(&sock); }
        /** Remove a socket from list .*/
        void operator-=(PSocket & sock /** Socket to remove. */)
          { Remove(&sock); }
    };

    /**Select a socket with available data. */
    static int Select(
      PSocket & sock1,        ///< First socket to check for readability.
      PSocket & sock2         ///< Second socket to check for readability.
    );
    /**Select a socket with available data. */
    static int Select(
      PSocket & sock1,        ///< First socket to check for readability.
      PSocket & sock2,        ///< Second socket to check for readability.
      const PTimeInterval & timeout ///< Timeout for wait on read/write data.
    );
    /**Select a socket with available data. */
    static Errors Select(
      SelectList & read       ///< List of sockets to check for readability.
    );
    /**Select a socket with available data. */
    static Errors Select(
      SelectList & read,      ///< List of sockets to check for readability.
      const PTimeInterval & timeout ///< Timeout for wait on read/write data.
    );
    /**Select a socket with available data. */
    static Errors Select(
      SelectList & read,      ///< List of sockets to check for readability.
      SelectList & write      ///< List of sockets to check for writability.
    );
    /**Select a socket with available data. */
    static Errors Select(
      SelectList & read,      ///< List of sockets to check for readability.
      SelectList & write,     ///< List of sockets to check for writability.
      const PTimeInterval & timeout ///< Timeout for wait on read/write data.
    );
    /**Select a socket with available data. */
    static Errors Select(
      SelectList & read,      ///< List of sockets to check for readability.
      SelectList & write,     ///< List of sockets to check for writability.
      SelectList & except     ///< List of sockets to check for exceptions.
    );
    /**Select a socket with available data. This function will block until the
       timeout or data is available to be read or written to the specified
       sockets.

       The read, write and except lists
       are modified by the call so that only the sockets that have data
       available are present. If the call timed out then all of these lists
       will be empty.

       If no timeout is specified then the call will block until a socket
       has data available.

       @return
       true if the select was successful or timed out, false if an error
       occurred. If a timeout occurred then the lists returned will be empty.

       For the versions taking sockets directly instead of lists the integer
       returned is >0 for an error being a value from the PChannel::Errors
       enum, 0 for a timeout, -1 for the first socket having read data,
       -2 for the second socket and -3 for both.
     */
    static Errors Select(
      SelectList & read,      ///< List of sockets to check for readability.
      SelectList & write,     ///< List of sockets to check for writability.
      SelectList & except,    ///< List of sockets to check for exceptions.
      const PTimeInterval & timeout ///< Timeout for wait on read/write data.
    );
  //@}

  /**@name Integer conversion functions */
  //@{
    /// Convert from host to network byte order
    inline static uint16_t  Host2Net(uint16_t  v) { return htons(v); }
    /// Convert from host to network byte order
    inline static uint32_t Host2Net(uint32_t v) { return htonl(v); }

    /// Convert from network to host byte order
    inline static uint16_t  Net2Host(uint16_t  v) { return ntohs(v); }
    /// Convert from network to host byte order
    inline static uint32_t Net2Host(uint32_t v) { return ntohl(v); }
  //@}


  /**@name Scattered read/write functions */
  //@{
    /** Structure that defines a "slice" of memory to be written to
     */
#if _WIN32
    struct Slice : public WSABUF 
    {
      Slice()
      { buf = (char *)0; len = 0; }

      Slice(void * v, const size_t len)
      { SetBase(v); SetLength(len); }

      Slice(const void * v, const size_t len)
      { SetBase(v); SetLength(len); }

      void SetBase(const void * v)   { buf = (char *)v; }
      void SetBase(void * v)         { buf = (char *)v; }
      void * GetBase() const         { return buf; }

      void SetLength(size_t v)  { len = (ULONG)v; }
      size_t GetLength() const  { return len; }
    };
#else
#if P_HAS_RECVMSG
    struct Slice : public iovec
    {
#else
    struct Slice 
    {
      protected:
        void * iov_base;
        size_t iov_len;
      public:
#endif
      Slice()
      { SetBase(NULL); SetLength(0); }

      Slice(void * v, size_t len)
      { SetBase(v); SetLength(len); }

      void SetBase(void * v) { iov_base = v; }
      void * GetBase() const   { return iov_base; }
      void SetLength(size_t v) { iov_len = v; }
      size_t GetLength() const  { return iov_len; }
    };
#endif

    /** Low level scattered read from the channel. This is identical to Read except 
        that the data will be read into a series of scattered memory slices. By default,
        this call will default to calling Read multiple times, but this may be 
        implemented by operating systems to do a real scattered read

       @return
       true indicates that at least one character was read from the channel.
       false means no bytes were read due to timeout or some other I/O error.
     */
    virtual bool Read(
      Slice * slices,    // slices to read to
      size_t sliceCount
    );

    /** Low level scattered write to the channel. This is identical to Write except 
        that the data will be written from a series of scattered memory slices. By default,
        this call will default to calling Write multiple times, but this can be actually
        implemented by operating systems to do a real scattered write

       @return
       true indicates that at least one character was read from the channel.
       false means no bytes were read due to timeout or some other I/O error.
     */
    virtual bool Write(
      const Slice * slices,  // slices to write from
      size_t sliceCount
    );
  //@}

  protected:
    virtual bool ConvertOSError(P_INT_PTR libcReturnValue, ErrorGroup group = LastGeneralError);

    /*This function calls os_socket() with the correct parameters for the
       socket protocol type.
     */
    virtual bool OpenSocket() = 0;

    /**This function returns the protocol name for the socket type.
     */
    virtual const char * GetProtocolName() const = 0;


    int os_close();
    int os_socket(int af, int type, int proto);
    bool os_connect(
      struct sockaddr * sin,
      socklen_t size
    );

    bool os_vread(
      Slice * slices,
      size_t sliceCount,
      int flags,
      struct sockaddr * from,
      socklen_t * fromlen
    );

    bool os_vwrite(
      const Slice * slices,
      size_t sliceCount,
      int flags,
      struct sockaddr * to,
      socklen_t tolen
    );

    bool os_accept(
      PSocket & listener,
      struct sockaddr * addr,
      socklen_t * size
    );

    virtual int os_errno() const;


  // Member variables
    /// Port to be used by the socket when opening the channel.
    uint16_t m_port;

// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/socket.h"
#else
#include "unix/ptlib/socket.h"
#endif
};


#endif // PTLIB_SOCKET_H


// End Of File ///////////////////////////////////////////////////////////////
