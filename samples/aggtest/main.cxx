/*
 * main.cxx
 *
 * PWLib application source file for aggtest
 *
 * Main program entry point.
 *
 * Copyright (C) 2004 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 */

#include "precompile.h"
#include "main.h"

#include <ptclib/sockagg.h>

PCREATE_PROCESS(AggTest);

AggTest::AggTest()
  : PProcess("Post Increment", "AggTest")
{
}

class MyUDPSocket : public PUDPSocket
{
  public:
    bool OnRead()
    {
      uint8_t buffer[1024];
      Read(buffer, 1024);
      return true;
    }

};

void AggTest::Main()
{
  PArgList & args = GetArguments();

  args.Parse(
             "-server:"
             "-to:"
             "-from:"
             "-re:"
             "-attachment:"

#if PTRACING
             "o-output:"             "-no-output."
             "t-trace."              "-no-trace."
#endif
  );

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  PSocketAggregator<MyUDPSocket> socketHandler;

  MyUDPSocket * sockets[100];
  memset(sockets, 0, sizeof(sockets));
  const unsigned count = sizeof(sockets) / sizeof(sockets[0]);

  for (PINDEX i = 0; i < 1000000; ++i) {
    int num = rand() % count;
    if (sockets[num] == NULL) {
      sockets[num] = new MyUDPSocket();
      sockets[num]->Listen();
      socketHandler.AddSocket(sockets[num]);
    }
    else
    {
      socketHandler.RemoveSocket(sockets[num]);
      delete sockets[num];
      sockets[num] = NULL;
    }
  }

  cout << "handler finished with " << socketHandler.workers.size() << " threads" << endl;
}


// End of File ///////////////////////////////////////////////////////////////
