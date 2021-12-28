/*
 * aws_sdk.h
 *
 * AWS SDK interface wrapper
 *
 * Portable Windows Library
 *
 * Copyright (c) 2021 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): Robert Jongbloed.
 */

#include <ptlib/object.h>

#ifndef PTLIB_AWS_SDK_H
#define PTLIB_AWS_SDK_H

#if P_AWS_SDK

/* Note, all the AWS code requires C++11 or better. So, we can use things like
   std::shared_ptr<> in this header too. Some compilers (MSVC) allow use of
   C++11 classes even if not explicitly stated C++11 is in use. Others (GCC) do
   not allow this, so you must configure them for C++11 or later to use AWS.
*/
#include <memory>


namespace Aws {
  namespace Client {
    struct ClientConfiguration;
  }
};


class PAwsClientBase
{
protected:
  PString m_region;
  PString m_accessKeyId;
  PString m_secretAccessKey;
  std::shared_ptr<Aws::Client::ClientConfiguration> m_clientConfig;
public:
  PAwsClientBase();
  ~PAwsClientBase();
};


template <class Client> class PAwsClient : public PAwsClientBase
{
protected:
  std::shared_ptr<Client> m_client;
public:
  PAwsClient()
    : m_client(std::make_shared<Client>(*m_clientConfig))
  { }
};


#endif //P_AWS_SDK
#endif // PTLIB_AWS_SDK_H