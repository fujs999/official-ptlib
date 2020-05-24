/*
 * osutil.inl
 *
 * Operating System Classes Inline Function Definitions
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
 *
 * $Id: osutil.inl 19426 2008-02-09 03:19:51Z rjongbloed $
 */

#include "ptlib.h"

///////////////////////////////////////////////////////////////////////////////
// PTimeInterval

PINLINE PTimeInterval::PTimeInterval(int64_t millisecs)
  : m_nanoseconds(millisecs*MilliToNano) { }

PINLINE PTimeInterval::PTimeInterval(const PTimeInterval & other)
  : m_nanoseconds(other.InternalGet()) { }

PINLINE PTimeInterval & PTimeInterval::operator=(const PTimeInterval & other)
  { InternalSet(other.InternalGet()); return *this; }

PINLINE PObject * PTimeInterval::Clone() const
  { return PNEW PTimeInterval(GetMilliSeconds()); }

PINLINE PTimeInterval PTimeInterval::Frequency(double frequency)
  { PTimeInterval t; t.SetFrequency(frequency); return t; }

PINLINE PTimeInterval PTimeInterval::NanoSeconds(int64_t nsecs, int secs)
  { PTimeInterval t; t.SetNanoSeconds(nsecs, secs); return t; }

PINLINE PTimeInterval PTimeInterval::MicroSeconds(int64_t usecs, int secs)
  { PTimeInterval t; t.SetMicroSeconds(usecs, secs); return t; }

PINLINE PTimeInterval PTimeInterval::Seconds(double secs)
  { PTimeInterval t; t.SetSeconds(secs); return t; }

PINLINE int64_t PTimeInterval::GetNanoSeconds() const
  { return InternalGet(); }

PINLINE void PTimeInterval::SetNanoSeconds(int64_t nsecs, int secs)
  { InternalSet(nsecs+secs*SecsToNano); }

PINLINE int64_t PTimeInterval::GetMicroSeconds() const
  { return InternalGet()/MicroToNano; }

PINLINE void PTimeInterval::SetMicroSeconds(int64_t usecs, int secs)
  { InternalSet(usecs*MicroToNano+secs*SecsToNano); }

PINLINE int64_t PTimeInterval::GetMilliSeconds() const
  { return InternalGet()/MilliToNano; }

PINLINE void PTimeInterval::SetMilliSeconds(int64_t msecs)
  { InternalSet(msecs*MilliToNano); }

PINLINE double PTimeInterval::GetFrequency() const
  { return 1000000000.0/InternalGet(); }

PINLINE void PTimeInterval::SetFrequency(double frequency)
  { InternalSet((int64_t)(SecsToNano/frequency)); }

PINLINE long PTimeInterval::GetSeconds() const
  { return (long)(InternalGet()/SecsToNano); }

PINLINE double PTimeInterval::GetSecondsAsDouble() const
  { return InternalGet()/(double)SecsToNano; }

PINLINE void PTimeInterval::SetSeconds(double secs)
  { InternalSet((int64_t)(secs*SecsToNano)); }

PINLINE long PTimeInterval::GetMinutes() const
  { return (long)(InternalGet()/MinsToNano); }

PINLINE int PTimeInterval::GetHours() const
  { return (int)(InternalGet()/HoursToNano); }

PINLINE int PTimeInterval::GetDays() const
  { return (int)(InternalGet()/DaysToNano); }


PINLINE PTimeInterval PTimeInterval::operator-() const
  { return PTimeInterval::NanoSeconds(-GetNanoSeconds()); }

PINLINE PTimeInterval PTimeInterval::operator+(const PTimeInterval & t) const
  { return PTimeInterval::NanoSeconds(GetNanoSeconds() + t.GetNanoSeconds()); }

PINLINE PTimeInterval operator+(int64_t left, const PTimeInterval & right)
  { return PTimeInterval(left + right.GetMilliSeconds()); }

PINLINE PTimeInterval & PTimeInterval::operator+=(const PTimeInterval & t)
  { SetNanoSeconds(GetNanoSeconds() + t.GetNanoSeconds()); return *this; }

PINLINE PTimeInterval PTimeInterval::operator-(const PTimeInterval & t) const
  { return PTimeInterval::NanoSeconds(GetNanoSeconds() - t.GetNanoSeconds()); }

PINLINE PTimeInterval operator-(int64_t left, const PTimeInterval & right)
  { return PTimeInterval(left - right.GetMilliSeconds()); }

PINLINE PTimeInterval & PTimeInterval::operator-=(const PTimeInterval & t)
  { SetNanoSeconds(GetNanoSeconds() - t.GetNanoSeconds()); return *this; }

PINLINE PTimeInterval PTimeInterval::operator*(int f) const
  { return PTimeInterval::NanoSeconds(GetNanoSeconds() * f); }

PINLINE PTimeInterval operator*(int left, const PTimeInterval & right)
  { return PTimeInterval::NanoSeconds(left * right.GetNanoSeconds()); }

PINLINE PTimeInterval & PTimeInterval::operator*=(int f)
  { SetNanoSeconds(GetNanoSeconds() * f); return *this; }

PINLINE int PTimeInterval::operator/(const PTimeInterval & t) const
  { return (int)(GetNanoSeconds() / t.GetNanoSeconds()); }

PINLINE PTimeInterval PTimeInterval::operator/(int f) const
  { return PTimeInterval::NanoSeconds(GetNanoSeconds() / f); }

PINLINE PTimeInterval operator/(int64_t left, const PTimeInterval & right)
  { return PTimeInterval::NanoSeconds(left / right.GetNanoSeconds()); }

PINLINE PTimeInterval & PTimeInterval::operator/=(int f)
  { SetNanoSeconds(GetNanoSeconds() / f); return *this; }


PINLINE bool PTimeInterval::operator==(const PTimeInterval & t) const
  { return GetNanoSeconds() == t.GetNanoSeconds(); }

PINLINE bool PTimeInterval::operator!=(const PTimeInterval & t) const
  { return GetNanoSeconds() != t.GetNanoSeconds(); }

PINLINE bool PTimeInterval::operator> (const PTimeInterval & t) const
  { return GetNanoSeconds() > t.GetNanoSeconds(); }

PINLINE bool PTimeInterval::operator>=(const PTimeInterval & t) const
  { return GetNanoSeconds() >= t.GetNanoSeconds(); }

PINLINE bool PTimeInterval::operator< (const PTimeInterval & t) const
  { return GetNanoSeconds() < t.GetNanoSeconds(); }

PINLINE bool PTimeInterval::operator<=(const PTimeInterval & t) const
  { return GetNanoSeconds() <= t.GetNanoSeconds(); }

PINLINE bool PTimeInterval::operator==(long msecs) const
  { return GetMilliSeconds() == (int64_t)msecs; }

PINLINE bool PTimeInterval::operator!=(long msecs) const
  { return GetMilliSeconds() != (int64_t)msecs; }

PINLINE bool PTimeInterval::operator> (long msecs) const
  { return GetMilliSeconds() > (int64_t)msecs; }

PINLINE bool PTimeInterval::operator>=(long msecs) const
  { return GetMilliSeconds() >= (int64_t)msecs; }

PINLINE bool PTimeInterval::operator< (long msecs) const
  { return GetMilliSeconds() < (int64_t)msecs; }

PINLINE bool PTimeInterval::operator<=(long msecs) const
  { return GetMilliSeconds() <= (int64_t)msecs; }


///////////////////////////////////////////////////////////////////////////////
// PTime

PINLINE PTime::PTime(const PTime & other)
  : m_microSecondsSinceEpoch(other.m_microSecondsSinceEpoch.load()) { }

PINLINE PTime & PTime::operator=(const PTime & other)
  { m_microSecondsSinceEpoch.store(other.m_microSecondsSinceEpoch.load()); return *this; }

PINLINE PObject * PTime::Clone() const
  { return PNEW PTime(*this); }

PINLINE bool PTime::IsValid() const
  { return m_microSecondsSinceEpoch.load() > 46800000000LL; }

PINLINE int64_t PTime::GetTimestamp() const
  { return m_microSecondsSinceEpoch.load(); }

PINLINE PTime & PTime::SetTimestamp(time_t seconds, int64_t usecs)
  { m_microSecondsSinceEpoch.store(seconds*Micro + usecs); return *this; }

PINLINE PTime & PTime::AddTimestamp(int64_t usecs)
 { m_microSecondsSinceEpoch += usecs; return *this; }

PINLINE time_t PTime::GetTimeInSeconds() const
  { return m_microSecondsSinceEpoch.load()/Micro; }

PINLINE unsigned PTime::GetMicrosecond() const
  { return m_microSecondsSinceEpoch.load()%Micro; }

PINLINE PTimeInterval PTime::GetElapsed() const
  { return IsValid() ? (PTime() - *this) : 0; }

PINLINE bool PTime::IsPast() const
  { return GetElapsed() > 0; }

PINLINE bool PTime::IsFuture() const
  { return GetElapsed() < 0; }


PINLINE PString PTime::AsString(const PString & format, int zone) const
  { return AsString((const char *)format, zone); }

PINLINE int PTime::GetTimeZone() 
  { return GetTimeZone(IsDaylightSavings() ? DaylightSavings : StandardTime); }

PINLINE PTime PTime::operator+(const PTimeInterval & t) const
  { return PTime(0, GetTimestamp() +  t.GetMicroSeconds()); }

PINLINE PTime & PTime::operator+=(const PTimeInterval & t)
  { m_microSecondsSinceEpoch += t.GetMicroSeconds(); return *this; }

PINLINE PTimeInterval PTime::operator-(const PTime & t) const
  { return PTimeInterval::MicroSeconds(GetTimestamp() - t.GetTimestamp()); }

PINLINE PTime PTime::operator-(const PTimeInterval & t) const
  { return PTime(0, GetTimestamp() - t.GetMicroSeconds()); }

PINLINE PTime & PTime::operator-=(const PTimeInterval & t)
  { m_microSecondsSinceEpoch -= t.GetMicroSeconds(); return *this; }


#if P_TIMERS
///////////////////////////////////////////////////////////////////////////////
// PSimpleTimer

PINLINE void PSimpleTimer::Stop()
  { SetInterval(0); }

PINLINE PTimeInterval PSimpleTimer::GetElapsed() const
  { return PTimer::Tick() - m_startTick; }

PINLINE bool PSimpleTimer::IsRunning() const
  { return (PTimer::Tick() - m_startTick) < *this; }

PINLINE bool PSimpleTimer::HasExpired() const
  { return (PTimer::Tick() - m_startTick) >= *this; }

PINLINE PSimpleTimer::operator bool() const
  { return HasExpired(); }


///////////////////////////////////////////////////////////////////////////////
// PTimer

PINLINE PTimeInterval PTimer::GetResetTime() const
  { return PTimeInterval::NanoSeconds(PTimeInterval::InternalGet()); }

PINLINE const PNotifier & PTimer::GetNotifier() const
  { return m_callback; }

PINLINE void PTimer::SetNotifier(const PNotifier & func, const PString & threadName)
  { m_callback = func; m_threadName = threadName; }


#endif // P_TIMERS

///////////////////////////////////////////////////////////////////////////////

PINLINE PChannelStreamBuffer::PChannelStreamBuffer(const PChannelStreamBuffer & sbuf)
  : channel(sbuf.channel) { }

PINLINE PChannelStreamBuffer &
          PChannelStreamBuffer::operator=(const PChannelStreamBuffer & sbuf)
  { channel = sbuf.channel; return *this; }

PINLINE PChannel::PChannel(const PChannel &)
  : std::iostream(cout.rdbuf())
  { PAssertAlways("Cannot copy channels"); }

PINLINE PChannel & PChannel::operator=(const PChannel &)
  { PAssertAlways("Cannot assign channels"); return *this; }

PINLINE void PChannel::SetReadTimeout(const PTimeInterval & time)
  { readTimeout = time; }

PINLINE PTimeInterval PChannel::GetReadTimeout() const
  { return readTimeout; }

PINLINE void PChannel::SetWriteTimeout(const PTimeInterval & time)
  { writeTimeout = time; }

PINLINE PTimeInterval PChannel::GetWriteTimeout() const
  { return writeTimeout; }

PINLINE P_INT_PTR PChannel::GetHandle() const
  { return os_handle; }

PINLINE void PChannel::AbortCommandString()
  { m_abortCommandString = true; }


///////////////////////////////////////////////////////////////////////////////
// PIndirectChannel

PINLINE PIndirectChannel::~PIndirectChannel()
  { Close(); }

PINLINE PChannel * PIndirectChannel::GetReadChannel() const
  { return readChannel; }

PINLINE PChannel * PIndirectChannel::GetWriteChannel() const
  { return writeChannel; }


///////////////////////////////////////////////////////////////////////////////
// PDirectory

PINLINE PDirectory & PDirectory::operator=(const PDirectory & str)
  { Close(); PFilePathString::operator=(str); return *this; }

PINLINE PDirectory & PDirectory::operator=(const PString & str)
  { Close(); PFilePathString::operator=(PFilePath::Canonicalise(str, true)); return *this; }

PINLINE PDirectory & PDirectory::operator=(const char * cstr)
  { Close(); PFilePathString::operator=(PFilePath::Canonicalise(cstr, true)); return *this; }


PINLINE bool PDirectory::Exists() const
  { return Exists(*this); }

PINLINE bool PDirectory::Change() const
  { return Change(*this); }

PINLINE bool PDirectory::Create(int perm, bool recurse) const
  { return Create(*this, perm, recurse); }

PINLINE bool PDirectory::Remove()
  { Close(); return Remove(*this); }


///////////////////////////////////////////////////////////////////////////////

PINLINE PFilePath::PFilePath()
  { }

PINLINE PFilePath::PFilePath(const PFilePath & path)
  : PFilePathString(path) { }

PINLINE PFilePath & PFilePath::operator=(const PFilePath & path)
  { PFilePathString::operator=(path); return *this; }

PINLINE PFilePath & PFilePath::operator=(const PString & str)
  { PFilePathString::operator=(Canonicalise(str, false)); return *this; }

PINLINE PFilePath & PFilePath::operator=(const char * cstr)
  { PFilePathString::operator=(Canonicalise(cstr, false)); return *this; }

PINLINE PFilePath & PFilePath::operator+=(const PString & str)
  { PFilePathString::operator=(Canonicalise(*this + str, false)); return *this; }

PINLINE PFilePath & PFilePath::operator+=(const char * cstr)
  { PFilePathString::operator=(Canonicalise(*this + cstr, false)); return *this; }


///////////////////////////////////////////////////////////////////////////////

PINLINE PFile::PFile()
  { os_handle = -1; m_removeOnClose = false; }

PINLINE PFile::PFile(OpenMode mode, OpenOptions opts)
  { os_handle = -1; m_removeOnClose = false; Open(mode, opts); }

PINLINE PFile::PFile(const PFilePath & name, OpenMode mode, OpenOptions opts)
  { os_handle = -1; m_removeOnClose = false; Open(name, mode, opts); }


PINLINE bool PFile::Exists() const
  { return Exists(m_path); }

PINLINE bool PFile::Access(OpenMode mode)
  { return ConvertOSError(Access(m_path, mode) ? 0 : -1); }

PINLINE bool PFile::Touch(const PFilePath & name, const PTime & accessTime)
  { return Touch(name, accessTime, accessTime); }

PINLINE bool PFile::Touch(const PTime & accessTime)
  { return ConvertOSError(Touch(m_path, accessTime) ? 0 : -1); }

PINLINE bool PFile::Touch(const PTime & accessTime, const PTime & modTime)
  { return ConvertOSError(Touch(m_path, accessTime, modTime) ? 0 : -1); }

PINLINE bool PFile::Remove(bool force)
  { Close(); return ConvertOSError(Remove(m_path, force) ? 0 : -1); }

PINLINE bool PFile::Copy(const PFilePath & newname, bool force, bool recurse)
  { return ConvertOSError(Copy(m_path, newname, force, recurse) ? 0 : -1); }

PINLINE bool PFile::GetInfo(PFileInfo & info)
  { return ConvertOSError(GetInfo(m_path, info) ? 0 : -1); }

PINLINE bool PFile::SetPermissions(PFileInfo::Permissions permissions)
  { return ConvertOSError(SetPermissions(m_path, permissions) ? 0 : -1); }


PINLINE const PFilePath & PFile::GetFilePath() const
  { return m_path; }
      

PINLINE PString PFile::GetName() const
  { return m_path; }

PINLINE off_t PFile::GetPosition() const
  { return _lseek(GetOSHandleAsInt(), 0, SEEK_CUR); }


///////////////////////////////////////////////////////////////////////////////

PINLINE PTextFile::PTextFile()
  { }

PINLINE PTextFile::PTextFile(OpenMode mode, OpenOptions opts)
  { Open(mode, opts); }

PINLINE PTextFile::PTextFile(const PFilePath & name, OpenMode mode, OpenOptions opts)
  { Open(name, mode, opts); }


///////////////////////////////////////////////////////////////////////////////
// PConfig

#ifdef P_CONFIG_FILE

PINLINE PConfig::PConfig(Source src)
  : defaultSection(DefaultSectionName()) { Construct(src, PString::Empty(), PString::Empty()); }

PINLINE PConfig::PConfig(Source src, const PString & appname)
  : defaultSection(DefaultSectionName()) { Construct(src, appname, PString::Empty()); }

PINLINE PConfig::PConfig(Source src, const PString & appname, const PString & manuf)
  : defaultSection(DefaultSectionName()) { Construct(src, appname, manuf); }

PINLINE PConfig::PConfig(const PString & section, Source src)
  : defaultSection(section) { Construct(src, PString::Empty(), PString::Empty()); }

PINLINE PConfig::PConfig(const PString & section, Source src, const PString & appname)
  : defaultSection(section) { Construct(src, appname, PString::Empty()); }

PINLINE PConfig::PConfig(const PString & section,
                         Source src,
                         const PString & appname,
                         const PString & manuf)
  : defaultSection(section) { Construct(src, appname, manuf); }

PINLINE PConfig::PConfig(const PFilePath & filename, const PString & section)
  : defaultSection(section) { Construct(filename); }

PINLINE void PConfig::SetDefaultSection(const PString & section)
  { defaultSection = section; }

PINLINE PString PConfig::GetDefaultSection() const
  { return defaultSection; }

PINLINE PStringArray PConfig::GetKeys() const
  { return GetKeys(defaultSection); }

PINLINE PStringToString PConfig::GetAllKeyValues() const
  { return GetAllKeyValues(defaultSection); }

PINLINE void PConfig::DeleteSection()
  { DeleteSection(defaultSection); }

PINLINE void PConfig::DeleteKey(const PString & key)
  { DeleteKey(defaultSection, key); }

PINLINE bool PConfig::HasKey(const PString & key) const
  { return HasKey(defaultSection, key); }

PINLINE PString PConfig::GetString(const PString & key) const
  { return GetString(defaultSection, key, PString()); }

PINLINE PString PConfig::GetString(const PString & key, const PString & dflt) const
  { return GetString(defaultSection, key, dflt); }

PINLINE void PConfig::SetString(const PString & key, const PString & value)
  { SetString(defaultSection, key, value); }

PINLINE bool PConfig::GetBoolean(const PString & key, bool dflt) const
  { return GetBoolean(defaultSection, key, dflt); }

PINLINE void PConfig::SetBoolean(const PString & key, bool value)
  { SetBoolean(defaultSection, key, value); }

PINLINE long PConfig::GetInteger(const PString & key, long dflt) const
  { return GetInteger(defaultSection, key, dflt); }

PINLINE void PConfig::SetInteger(const PString & key, long value)
  { SetInteger(defaultSection, key, value); }

PINLINE int64_t PConfig::GetInt64(const PString & key, int64_t dflt) const
  { return GetInt64(defaultSection, key, dflt); }

PINLINE void PConfig::SetInt64(const PString & key, int64_t value)
  { SetInt64(defaultSection, key, value); }

PINLINE double PConfig::GetReal(const PString & key, double dflt) const
  { return GetReal(defaultSection, key, dflt); }

PINLINE void PConfig::SetReal(const PString & key, double value)
  { SetReal(defaultSection, key, value); }

PINLINE PTime PConfig::GetTime(const PString & key) const
  { return GetTime(defaultSection, key); }

PINLINE PTime PConfig::GetTime(const PString & key, const PTime & dflt) const
  { return GetTime(defaultSection, key, dflt); }

PINLINE void PConfig::SetTime(const PString & key, const PTime & value)
  { SetTime(defaultSection, key, value); }


#endif // P_CONFIG_FILE


///////////////////////////////////////////////////////////////////////////////
// PArgList

PINLINE void PArgList::SetArgs(int argc, char ** argv)
  { SetArgs(PStringArray(argc, argv)); }

PINLINE bool PArgList::Parse(const PString & theArgumentSpec, bool optionsBeforeParams)
  { return Parse((const char *)theArgumentSpec, optionsBeforeParams); }

PINLINE bool PArgList::HasOption(char option) const
  { return GetOptionCount(option) != 0; }

PINLINE bool PArgList::HasOption(const char * option) const
  { return GetOptionCount(option) != 0; }

PINLINE bool PArgList::HasOption(const PString & option) const
  { return GetOptionCount(option) != 0; }

PINLINE PINDEX PArgList::GetCount() const
  { return m_parameterIndex.GetSize()-m_shift; }

PINLINE PString PArgList::operator[](PINDEX num) const
  { return GetParameter(num); }

PINLINE PArgList & PArgList::operator<<(int sh)
  { Shift(sh); return *this; }

PINLINE PArgList & PArgList::operator>>(int sh)
  { Shift(-sh); return *this; }


///////////////////////////////////////////////////////////////////////////////
// PThread

PINLINE PThreadIdentifier PThread::GetThreadId() const
  { return m_threadId; }

PINLINE PUniqueThreadIdentifier PThread::GetUniqueIdentifier() const
  { return m_uniqueId; }


///////////////////////////////////////////////////////////////////////////////
// PProcess

PINLINE PArgList & PProcess::GetArguments()
  { return m_arguments; }

PINLINE const PString & PProcess::GetManufacturer() const
  { return m_manufacturer; }

PINLINE const PString & PProcess::GetName() const
  { return m_productName; }

PINLINE const PFilePath & PProcess::GetFile() const
  { return m_executableFile; }

PINLINE int PProcess::GetMaxHandles() const
  { return m_maxHandles; }

PINLINE void PProcess::SetTerminationValue(int value)
  { m_terminationValue = value; }

PINLINE int PProcess::GetTerminationValue() const
  { return m_terminationValue; }

PINLINE void PThreadYield()
  { PThread::Yield(); }


// End Of File ///////////////////////////////////////////////////////////////
