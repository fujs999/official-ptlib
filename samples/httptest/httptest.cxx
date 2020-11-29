//
// httptest.cxx
//
// Copyright 2011 Vox Lucida Pty. Ltd.
//

#include <ptlib.h>
#include <ptlib/pprocess.h>
#include <ptlib/sockets.h>
#include <ptclib/pssl.h>
#include <ptclib/http.h>
#include <ptclib/threadpool.h>


class HTTPConnection
{
  public:
    HTTPConnection(
      PHTTPSpace & httpNameSpace
#if P_SSL
      , PSSLContext * context
#endif
    ) : m_httpNameSpace(httpNameSpace)
#if P_SSL
      , m_context(context)
#endif
    {
    }

    void Work()
    {
      PTRACE(3, "HTTPTest\tStarted work on " << m_socket.GetPeerAddress());

      PHTTPServer httpServer(m_httpNameSpace);

#if P_SSL
      if (m_context != NULL) {
        PSSLChannel * ssl = new PSSLChannel(m_context);
        if (!ssl->Open(m_socket))
          return;
        if (!ssl->Accept())
          return;
        if (!httpServer.Open(ssl))
          return;
      }
      else
#endif
        if (!httpServer.Open(m_socket))
          return;

      unsigned count = 0;
      while (httpServer.ProcessCommand())
        ++count;

      PTRACE(3, "HTTPTest\tEnded work on " << m_socket.GetPeerAddress() << ", " << count << " transactions.");
    }

    PHTTPSpace  & m_httpNameSpace;
#if P_SSL
    PSSLContext * m_context;
#endif
    PTCPSocket    m_socket;
};


class HTTPTest : public PProcess
{
  PCLASSINFO(HTTPTest, PProcess)
  PSyncPoint m_done;
public:
  P_DISABLE_MSVC_WARNINGS(4100, 
  PDECLARE_HttpPoolNotifier(HTTPTest, OnPoolDone)
  {
    m_done.Signal();
  })

  void ClientPool(PHTTP::Commands cmd, const PArgList & args)
  {
    PHTTPClientPool pool;
#if P_SSL
    pool.SetSSLCredentials(args.GetOptionString("ca"),
                           args.GetOptionString("certificate"),
                           args.GetOptionString("private-key"));
#endif

    pool.QueueRequest(PHTTPClientPool::Request(cmd, args[0], PCREATE_NOTIFIER(OnPoolDone)));
    m_done.Wait();
  }


  void Client(PHTTP::Commands cmd, const PArgList & args)
  {
    PHTTPClient client;
#if P_SSL
    client.SetSSLCredentials(args.GetOptionString("ca"),
                             args.GetOptionString("certificate"),
                             args.GetOptionString("private-key"));
#endif

    bool ok;
    switch (cmd) {
      case PHTTP::GET :
      {
        PString str;
        ok = client.GetTextDocument(args[0], str);
        if (ok)
          cout << "Response body: \"" << str << '"' << endl;
        break;
      }

      case PHTTP::DELETE :
        ok = client.DeleteDocument(args[0]);
        break;

      case PHTTP::PUT :
        ok = client.PutDocument(args[0], PFilePath(args[1]));
        break;

      case PHTTP::POST :
      {
        PMIMEInfo outMIME;
        ok = client.PostData(args[0], outMIME, args[1]);
        break;
      }

      default :
        cerr << args.Usage("[ url [ file ] ]");
        return;
    }
    if (ok)
      cout << client.GetNameFromCommand(cmd) << " sucessful.";
    else
      cout << "Error in " << client.GetNameFromCommand(cmd)
      << " code=" << client.GetLastResponseCode()
      << " info=\"" << client.GetLastResponseInfo() << '"';
    cout << endl;
  }


  void Server(const PArgList & args)
  {
    PQueuedThreadPool<HTTPConnection> pool;
    pool.SetMaxWorkers(args.GetOptionString('T', "10").AsUnsigned());

#if P_SSL
    PSSLContext * sslContext = args.HasOption('s') ? new PSSLContext : NULL;
    if (sslContext != NULL) {
      if (!sslContext->SetCredentials(args.GetOptionString("ca", "."),
                                      args.GetOptionString("certificate", "certificate.pem"),
                                      args.GetOptionString("private-key", "privatekey.pem"),
                                      true)) {
        cerr << "Could not set credentials for SSL" << endl;
        return;
      }
    }

    PTCPSocket listener(args.GetOptionAs('p', (uint16_t)(sslContext != NULL ? 443 : 80)));
#else
    PTCPSocket listener(args.GetOptionAs('p', 80));
#endif

    if (!listener.Listen(args.GetOptionString('Q', "100").AsUnsigned())) {
      cerr << "Could not listen on port " << listener.GetPort() << endl;
      return;
    }

    PHTTPSpace httpNameSpace;
    httpNameSpace.AddResource(new PHTTPString("index.html", "Hello", "text/plain"));

    cout << "Listening for "
#if P_SSL
      << (sslContext != NULL ? "https" : "http") <<
#else
      "http"
#endif
      " on port " << listener.GetPort() << endl;

    for (;;) {
#if P_SSL
      HTTPConnection * connection = new HTTPConnection(httpNameSpace, sslContext);
#else
      HTTPConnection * connection = new HTTPConnection(httpNameSpace);
#endif
      if (connection->m_socket.Accept(listener))
        pool.AddWork(connection);
      else {
        delete connection;
        cerr << "Error in accept: " << listener.GetErrorText() << endl;
        break;
      }
    }

    cout << "Exiting HTTP test" << endl;
  }


  void Main()
  {
    PArgList & args = GetArguments();
    args.Parse("h-help.       print this help message.\n"
               "O-operation:  do a GET/POST/PUT/DELETE, if absent then acts as server\n"
               "P-pool.       do above operation using client pool"
               "p-port:       port number to listen on (default 80 or 443).\n"
#if P_SSL
               "s-secure.     SSL/TLS mode for server.\n"
               "-ca:          SSL/TLS client certificate authority file/directory.\n"
               "-certificate: SSL/TLS server certificate.\n"
               "-private-key: SSL/TLS server private key.\n"
#endif
               "T-theads:  max number of threads in pool(default 10)\n"
               "Q-queue:   max queue size for listening sockets(default 100).\n"
               PTRACE_ARGLIST
    );

    PTRACE_INITIALISE(args);

    if (!args.IsParsed() || args.HasOption('h')) {
      cerr << args.Usage("[ options] [ url [ file ] ]");
      return;
    }

    if (args.HasOption('O')) {
      PINDEX cmd = PHTTPClient().GetCommandFromName(args.GetOptionString('O'));
      if (cmd == P_MAX_INDEX) {
        cerr << args.Usage("[ options] [ url [ file ] ]");
        return;
      }

      if (args.GetCount() < (cmd == PHTTP::PUT || cmd == PHTTP::POST ? 2 : 1)) {
        cerr << args.Usage("[ options] [ url [ file ] ]");
        return;
      }

      if (args.HasOption('P'))
        ClientPool((PHTTP::Commands)cmd, args);
      else
        Client((PHTTP::Commands)cmd, args);
    }
    else
      Server(args);
  }
};


PCREATE_PROCESS(HTTPTest)

// End of hello.cxx
