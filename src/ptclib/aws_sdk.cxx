/*
 * aws_sdk.cxx
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

#include <ptlib.h>

#if P_AWS_SDK

#include <ptlib/pprocess.h>
#include <ptclib/aws_sdk.h>

#define USE_IMPORT_EXPORT
#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/FormattedLogSystem.h>
#include <aws/core/client/ClientConfiguration.h>


#define PTraceModule() "AWS-SDK"

#ifdef _MSC_VER
  #pragma comment(lib, P_AWS_SDK_LIBDIR(core))
#endif


class PAwsSdkWrap : public PProcessStartup
{
private:
  PString m_profile;
  PString m_accessKeyId;
  PString m_secretAccessKey;
  Aws::SDKOptions m_options;
  std::atomic<bool> m_inititalised;

#if PTRACING

  class Logger : public Aws::Utils::Logging::LogSystemInterface
  {
    const PRegularExpression m_suppression;
  public:
    Logger()
      : m_suppression(getenv("PTLIB_AWS_LOG_SUPPRESSION"), PRegularExpression::IgnoreCase|PRegularExpression::Extended)
    { }

  private:
    virtual Aws::Utils::Logging::LogLevel GetLogLevel() const override
    {
      return static_cast<Aws::Utils::Logging::LogLevel>(PTrace::GetLevel()+1);
    }

    virtual void Log(Aws::Utils::Logging::LogLevel logLevel, const char */*tag*/, const char *formatStr, ...) override
    {
      auto level = static_cast<unsigned>(logLevel)-1;
      if (PTrace::CanTrace(level)) {
        va_list args;
        va_start(args, formatStr);
        PTRACE_BEGIN(level, NULL, PTraceModule()) << pvsprintf(formatStr, args) << PTrace::End;
        va_end(args);
      }
    }

    virtual void LogStream(Aws::Utils::Logging::LogLevel logLevel, const char */*tag*/, const Aws::OStringStream &messageStream) override
    {
      auto level = static_cast<unsigned>(logLevel)-1;
      if (PTrace::CanTrace(level)) {
        auto msg = messageStream.str();
        PINDEX dummy;
        if (!m_suppression.Execute(msg.c_str(), dummy))
          PTRACE_BEGIN(level, NULL, PTraceModule()) << msg << PTrace::End;
      }
    }

    virtual void Flush()
    { }
  };

  static std::shared_ptr<Aws::Utils::Logging::LogSystemInterface> CreateLogger() { return std::make_shared<Logger>(); }

#endif // PTRACING

public:
  PFACTORY_GET_SINGLETON(PProcessStartupFactory, PAwsSdkWrap);

  void Initialise()
  {
    if (m_inititalised.exchange(true))
      return;

    PTRACE(3, "Initialising API");
#if PTRACING
    m_options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Debug; // Just needs to be something other than Off
    m_options.loggingOptions.logger_create_fn = PAwsSdkWrap::CreateLogger;
#endif // PTRACING
    Aws::InitAPI(m_options);

    m_profile = getenv("PTLIB_AWS_PROFILE");
    m_accessKeyId = getenv("PTLIB_AWS_ACCESS_KEY_ID");
    m_secretAccessKey = getenv("PTLIB_AWS_SECRET_ACCESS_KEY");

    if (m_accessKeyId.empty() || m_secretAccessKey.empty()) {
      PConfig cfg(PProcess::Current().GetHomeDirectory() + ".aws/credentials", m_profile.empty() ? PConstString("default") : m_profile);
      m_accessKeyId = cfg.GetString("aws_access_key_id");
      m_secretAccessKey = cfg.GetString("aws_secret_access_key");
    }
  }

  virtual void OnShutdown()
  {
    if (m_inititalised) {
      PTRACE(3, "Shutting down API");
      Aws::ShutdownAPI(m_options);
    }
  }

  const PString & GetProfile() const { return m_profile; }
  const PString & GetAccessKeyId() const { return m_accessKeyId; }
  const PString & GetSecretAccessKey() const { return m_secretAccessKey; }
};


PFACTORY_CREATE_SINGLETON(PProcessStartupFactory, PAwsSdkWrap);


PAwsClientBase::PAwsClientBase()
{
  PAwsSdkWrap & wrap = PAwsSdkWrap::GetInstance();
  wrap.Initialise();

  m_accessKeyId = wrap.GetAccessKeyId();
  m_secretAccessKey = wrap.GetSecretAccessKey();

  if (wrap.GetProfile().empty())
    m_clientConfig = std::make_shared<Aws::Client::ClientConfiguration>();
  else
    m_clientConfig = std::make_shared<Aws::Client::ClientConfiguration>(wrap.GetProfile());
#ifdef _WIN32
  m_clientConfig->httpLibOverride = Aws::Http::TransferLibType::WIN_INET_CLIENT;
#endif
  // Maybe add more stuff here
  PTRACE(4, NULL, PTraceModule(), "Client config: " << m_clientConfig.get());
  m_region = m_clientConfig->region.empty() ? "us-east-1" : m_clientConfig->region.c_str();
}


PAwsClientBase::~PAwsClientBase()
{
}


#endif // P_AWS_SDK
