/*
 * remconn.cxx
 *
 * Remote Networking Connection class implmentation for Win32 RAS.
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

#include <ptlib.h>
#include <ptlib/remconn.h>
#include <ptlib/pprocess.h>


#if P_REMCONN

class PRASDLL : public PDynaLink
{
  PCLASSINFO(PRASDLL, PDynaLink)
  public:
    PRASDLL();

  uint32_t (FAR PASCAL *Dial)(LPRASDIALEXTENSIONS,LPTSTR,LPRASDIALPARAMS,uint32_t,LPVOID,LPHRASCONN);
  uint32_t (FAR PASCAL *HangUp)(HRASCONN);
  uint32_t (FAR PASCAL *GetConnectStatus)(HRASCONN,LPRASCONNSTATUS);
  uint32_t (FAR PASCAL *EnumConnections)(LPRASCONN,LPDWORD,LPDWORD);
  uint32_t (FAR PASCAL *EnumEntries)(LPTSTR,LPTSTR,LPRASENTRYNAME,LPDWORD,LPDWORD);
  uint32_t (FAR PASCAL *GetEntryProperties)(LPTSTR, LPTSTR, LPRASENTRY, LPDWORD, LPBYTE, LPDWORD);
  uint32_t (FAR PASCAL *SetEntryProperties)(LPTSTR, LPTSTR, LPRASENTRY, uint32_t, LPBYTE, uint32_t);
  uint32_t (FAR PASCAL *DeleteEntry)(LPTSTR, LPTSTR);
  uint32_t (FAR PASCAL *ValidateEntryName)(LPTSTR, LPTSTR);
  uint32_t (FAR PASCAL *GetProjectionInfo)(HRASCONN, RASPROJECTION, LPVOID, LPDWORD);
} Ras;


PRASDLL::PRASDLL()
#ifdef _WIN32
  : PDynaLink("RASAPI32.DLL")
#else
  : PDynaLink("RASAPI16.DLL")
#endif
{
  if (!GetFunction("RasDialA",             (Function &)Dial) ||
      !GetFunction("RasHangUpA",           (Function &)HangUp) ||
      !GetFunction("RasGetConnectStatusA", (Function &)GetConnectStatus) ||
      !GetFunction("RasEnumConnectionsA",  (Function &)EnumConnections) ||
      !GetFunction("RasEnumEntriesA",      (Function &)EnumEntries))
    Close();

  GetFunction("RasGetEntryPropertiesA", (Function &)GetEntryProperties);
  GetFunction("RasSetEntryPropertiesA", (Function &)SetEntryProperties);
  GetFunction("RasDeleteEntryA",        (Function &)DeleteEntry);
  GetFunction("RasValidateEntryNameA",  (Function &)ValidateEntryName);
  GetFunction("RasGetProjectionInfoA",  (Function &)GetProjectionInfo);
}


#define new PNEW


PRemoteConnection::PRemoteConnection()
{
  Construct();
}


PRemoteConnection::PRemoteConnection(const PString & name)
  : remoteName(name)
{
  Construct();
}


PRemoteConnection::~PRemoteConnection()
{
  Close();
}


bool PRemoteConnection::Open(const PString & name,
                             const PString & user,
                             const PString & pass,
                             bool existing)
{
  if (name != remoteName) {
    Close();
    remoteName = name;
  }
  userName = user;
  password = pass;
  return Open(existing);
}


bool PRemoteConnection::Open(const PString & name, bool existing)
{
  if (name != remoteName) {
    Close();
    remoteName = name;
  }
  return Open(existing);
}


PObject::Comparison PRemoteConnection::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PRemoteConnection), PInvalidCast);
  return remoteName.Compare(((const PRemoteConnection &)obj).remoteName);
}


PINDEX PRemoteConnection::HashFunction() const
{
  return remoteName.HashFunction();
}


void PRemoteConnection::Construct()
{
  rasConnection = NULL;
  osError = SUCCESS;
}


bool PRemoteConnection::Open(bool existing)
{
  Close();
  if (!Ras.IsLoaded())
    return false;

  RASCONN connection;
  connection.dwSize = sizeof(RASCONN);

  LPRASCONN connections = &connection;
  DWORD size = sizeof(connection);
  DWORD numConnections;

  osError = Ras.EnumConnections(connections, &size, &numConnections);
  if (osError == ERROR_BUFFER_TOO_SMALL) {
    connections = new RASCONN[size/connection.dwSize];
    connections[0].dwSize = connection.dwSize;
    osError = Ras.EnumConnections(connections, &size, &numConnections);
  }

  if (osError == 0) {
    for (uint32_t i = 0; i < numConnections; i++) {
      if (remoteName == connections[i].szEntryName) {
        rasConnection = connections[i].hrasconn;
        break;
      }
    }
  }

  if (connections != &connection)
    delete [] connections;

  if (rasConnection != NULL && GetStatus() == Connected) {
    osError = 0;
    return true;
  }
  rasConnection = NULL;

  if (existing)
    return false;

  RASDIALPARAMS params;
  memset(&params, 0, sizeof(params));
  params.dwSize = sizeof(params);

  if (remoteName[0] != '.') {
    PAssert(remoteName.GetLength() < (PINDEX)sizeof(params.szEntryName)-1, PInvalidParameter);
    strcpy_s(params.szEntryName, sizeof(params.szEntryName), remoteName);
  }
  else {
    PAssert(remoteName.GetLength() < (PINDEX)sizeof(params.szPhoneNumber), PInvalidParameter);
    strcpy_s(params.szPhoneNumber, sizeof(params.szPhoneNumber), remoteName(1, P_MAX_INDEX));
  }
  strcpy_s(params.szUserName, sizeof(params.szUserName), userName);
  strcpy_s(params.szPassword, sizeof(params.szPassword), password);

  osError = Ras.Dial(NULL, NULL, &params, 0, NULL, &rasConnection);
  if (osError == 0)
    return true;

  if (rasConnection != NULL) {
    Ras.HangUp(rasConnection);
    rasConnection = NULL;
  }

  SetLastError(osError);
  return false;
}


void PRemoteConnection::Close()
{
  if (rasConnection != NULL) {
    Ras.HangUp(rasConnection);
    rasConnection = NULL;
  }
}


static int GetRasStatus(HRASCONN rasConnection, uint32_t & rasError)
{
  RASCONNSTATUS status;
  status.dwSize = sizeof(status);

  rasError = Ras.GetConnectStatus(rasConnection, &status);
  if (rasError == ERROR_INVALID_HANDLE) {
    PError << "RAS Connection Status invalid handle, retrying.";
    rasError = Ras.GetConnectStatus(rasConnection, &status);
  }

  if (rasError == 0) {
    rasError = status.dwError;
    SetLastError(rasError);
    return status.rasconnstate;
  }

  PError << "RAS Connection Status failed (" << rasError << "), retrying.";
  rasError = Ras.GetConnectStatus(rasConnection, &status);
  if (rasError == 0)
    rasError = status.dwError;
  SetLastError(rasError);

  return -1;
}


PRemoteConnection::Status PRemoteConnection::GetStatus() const
{
  if (!Ras.IsLoaded())
    return NotInstalled;

  if (rasConnection == NULL) {
    switch (osError) {
      case SUCCESS :
        return Idle;
      case ERROR_CANNOT_FIND_PHONEBOOK_ENTRY :
        return NoNameOrNumber;
      case ERROR_LINE_BUSY :
        return LineBusy;
      case ERROR_NO_DIALTONE :
        return NoDialTone;
      case ERROR_NO_ANSWER :
      case ERROR_NO_CARRIER :
        return NoAnswer;
      case ERROR_PORT_ALREADY_OPEN :
      case ERROR_PORT_NOT_AVAILABLE :
        return PortInUse;
      case ERROR_ACCESS_DENIED :
      case ERROR_NO_DIALIN_PERMISSION :
      case ERROR_AUTHENTICATION_FAILURE :
        return AccessDenied;
      case ERROR_HARDWARE_FAILURE :
      case ERROR_PORT_OR_DEVICE :
        return HardwareFailure;
    }
    return GeneralFailure;
  }

  switch (GetRasStatus(rasConnection, ((PRemoteConnection*)this)->osError)) {
    case RASCS_Connected :
      return Connected;
    case RASCS_Disconnected :
      break;
    case -1 :
      return ConnectionLost;
    default :
      return InProgress;
  }

  PError << "RAS Connection Status disconnected, retrying.";
  switch (GetRasStatus(rasConnection, ((PRemoteConnection*)this)->osError)) {
    case RASCS_Connected :
      return Connected;
    case RASCS_Disconnected :
      return Idle;
    case -1 :
      return ConnectionLost;
  }
  return InProgress;
}


PString PRemoteConnection::GetAddress()
{
  if (Ras.GetProjectionInfo == NULL) {
    osError = ERROR_CALL_NOT_IMPLEMENTED;
    return PString();
  }

  if (rasConnection == NULL) {
    osError = ERROR_INVALID_HANDLE;
    return PString();
  }

  RASPPPIP ip;
  ip.dwSize = sizeof(ip);
  DWORD size = sizeof(ip);
  osError = Ras.GetProjectionInfo(rasConnection, RASP_PppIp, &ip, &size);
  if (osError != ERROR_SUCCESS)
    return PString();

  osError = ip.dwError;
  if (osError != ERROR_SUCCESS)
    return PString();

  return ip.szIpAddress;
}


PStringArray PRemoteConnection::GetAvailableNames()
{
  PStringArray array;
  if (!Ras.IsLoaded())
    return array;

  RASENTRYNAME entry;
  entry.dwSize = sizeof(RASENTRYNAME);

  LPRASENTRYNAME entries = &entry;
  DWORD size = sizeof(entry);
  DWORD numEntries;

  uint32_t rasError = Ras.EnumEntries(NULL, NULL, entries, &size, &numEntries);

  if (rasError == ERROR_BUFFER_TOO_SMALL) {
    entries = new RASENTRYNAME[size/sizeof(RASENTRYNAME)];
    entries[0].dwSize = sizeof(RASENTRYNAME);
    rasError = Ras.EnumEntries(NULL, NULL, entries, &size, &numEntries);
  }

  if (rasError == 0) {
    array.SetSize(numEntries);
    for (uint32_t i = 0; i < numEntries; i++)
      array[i] = entries[i].szEntryName;
  }

  if (entries != &entry)
    delete [] entries;

  return array;
}


PRemoteConnection::Status
      PRemoteConnection::GetConfiguration(Configuration & config)
{
  return GetConfiguration(remoteName, config);
}


static uint32_t MyRasGetEntryProperties(const char * name, PBYTEArray & entrybuf)
{
  LPRASENTRY entry = (LPRASENTRY)entrybuf.GetPointer(sizeof(RASENTRY));
  entry->dwSize = sizeof(RASENTRY);

  DWORD entrySize = sizeof(RASENTRY);
  DWORD error = Ras.GetEntryProperties(NULL, (char *)name, entry, &entrySize, NULL, 0);
  if (error == ERROR_BUFFER_TOO_SMALL) {
    entry = (LPRASENTRY)entrybuf.GetPointer(entrySize);
    error = Ras.GetEntryProperties(NULL, (char *)name, entry, &entrySize, NULL, 0);
  }

  return error;
}


PRemoteConnection::Status
      PRemoteConnection::GetConfiguration(const PString & name,
                                          Configuration & config)
{
  if (!Ras.IsLoaded() || Ras.GetEntryProperties == NULL)
    return NotInstalled;

  PBYTEArray entrybuf;
  switch (MyRasGetEntryProperties(name, entrybuf)) {
    case 0 :
      break;

    case ERROR_CANNOT_FIND_PHONEBOOK_ENTRY :
      return NoNameOrNumber;

    default :
      return GeneralFailure;
  }

  LPRASENTRY entry = (LPRASENTRY)(const uint8_t *)entrybuf;

  config.device = entry->szDeviceType + PString("/") + entry->szDeviceName;

  if ((entry->dwfOptions&RASEO_UseCountryAndAreaCodes) == 0)
    config.phoneNumber = entry->szLocalPhoneNumber;
  else
    config.phoneNumber = psprintf("+%u %s %s",
                       entry->dwCountryCode, entry->szAreaCode, entry->szLocalPhoneNumber);

  if ((entry->dwfOptions&RASEO_SpecificIpAddr) == 0)
    config.ipAddress = "";
  else
    config.ipAddress = psprintf("%u.%u.%u.%u",
                                entry->ipaddr.a, entry->ipaddr.b,
                                entry->ipaddr.c, entry->ipaddr.d);
  if ((entry->dwfOptions&RASEO_SpecificNameServers) == 0)
    config.dnsAddress = "";
  else
    config.dnsAddress = psprintf("%u.%u.%u.%u",
                                 entry->ipaddrDns.a, entry->ipaddrDns.b,
                                 entry->ipaddrDns.c, entry->ipaddrDns.d);

  config.script = entry->szScript;
  
  config.subEntries = entry->dwSubEntries;
  config.dialAllSubEntries = entry->dwDialMode == RASEDM_DialAll;

  return Connected;
}


PRemoteConnection::Status
      PRemoteConnection::SetConfiguration(const Configuration & config, bool create)
{
  return SetConfiguration(remoteName, config, create);
}


PRemoteConnection::Status
      PRemoteConnection::SetConfiguration(const PString & name,
                                          const Configuration & config,
                                          bool create)
{
  if (!Ras.IsLoaded() || Ras.SetEntryProperties == NULL || Ras.ValidateEntryName == NULL)
    return NotInstalled;

  PBYTEArray entrybuf;
  switch (MyRasGetEntryProperties(name, entrybuf)) {
    case 0 :
      break;

    case ERROR_CANNOT_FIND_PHONEBOOK_ENTRY :
      if (!create)
        return NoNameOrNumber;
      if (Ras.ValidateEntryName(NULL, (char *)(const char *)name) != 0)
        return GeneralFailure;
      break;

    default :
      return GeneralFailure;
  }

  LPRASENTRY entry = (LPRASENTRY)(const uint8_t *)entrybuf;

  PINDEX barpos = config.device.Find('/');
  if (barpos == P_MAX_INDEX)
    strncpy_s(entry->szDeviceName, sizeof(entry->szDeviceName), config.device, sizeof(entry->szDeviceName)-1);
  else {
    strncpy_s(entry->szDeviceType, sizeof(entry->szDeviceType), config.device.Left(barpos),  sizeof(entry->szDeviceType)-1);
    strncpy_s(entry->szDeviceName, sizeof(entry->szDeviceName), config.device.Mid(barpos+1), sizeof(entry->szDeviceName)-1);
  }

  strncpy_s(entry->szLocalPhoneNumber, sizeof(entry->szLocalPhoneNumber), config.phoneNumber, sizeof(entry->szLocalPhoneNumber)-1);

  PStringArray dots = config.ipAddress.Tokenise('.');
  if (dots.GetSize() != 4)
    entry->dwfOptions &= ~RASEO_SpecificIpAddr;
  else {
    entry->dwfOptions |= RASEO_SpecificIpAddr;
    entry->ipaddr.a = (uint8_t)dots[0].AsInteger();
    entry->ipaddr.b = (uint8_t)dots[1].AsInteger();
    entry->ipaddr.c = (uint8_t)dots[2].AsInteger();
    entry->ipaddr.d = (uint8_t)dots[3].AsInteger();
  }
  dots = config.dnsAddress.Tokenise('.');
  if (dots.GetSize() != 4)
    entry->dwfOptions &= ~RASEO_SpecificNameServers;
  else {
    entry->dwfOptions |= RASEO_SpecificNameServers;
    entry->ipaddrDns.a = (uint8_t)dots[0].AsInteger();
    entry->ipaddrDns.b = (uint8_t)dots[1].AsInteger();
    entry->ipaddrDns.c = (uint8_t)dots[2].AsInteger();
    entry->ipaddrDns.d = (uint8_t)dots[3].AsInteger();
  }

  strncpy_s(entry->szScript, sizeof(entry->szScript), config.script, sizeof(entry->szScript-1));

  entry->dwDialMode = config.dialAllSubEntries ? RASEDM_DialAll : RASEDM_DialAsNeeded;

  if (Ras.SetEntryProperties(NULL, (char *)(const char *)name,
                             entry, entrybuf.GetSize(), NULL, 0) != 0)
    return GeneralFailure;

  return Connected;
}


PRemoteConnection::Status
                  PRemoteConnection::RemoveConfiguration(const PString & name)
{
  if (!Ras.IsLoaded() || Ras.SetEntryProperties == NULL || Ras.ValidateEntryName == NULL)
    return NotInstalled;

  switch (Ras.DeleteEntry(NULL, (char *)(const char *)name)) {
    case 0 :
      return Connected;

    case ERROR_INVALID_NAME :
      return NoNameOrNumber;
  }

  return GeneralFailure;
}


#endif // P_REMCONN


// End of File ////////////////////////////////////////////////////////////////
