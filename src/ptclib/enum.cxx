/*
 * enum.cxx
 *
 * Portable Windows Library
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: enum.cxx,v $
 * Revision 1.3  2004/06/05 01:58:37  rjongbloed
 * Fixed MSVC 6 compatibility
 *
 * Revision 1.2  2004/05/31 23:14:17  csoutheren
 * Fixed warnings under VS.net and fixed problem with SRV records when returning multiple records
 *
 * Revision 1.1  2004/05/31 13:56:37  csoutheren
 * Added implementation of ENUM resolution of E.164 numbers by DNS
 *
 */

#ifdef __GNUC__
#pragma implementation "enum.h"
#endif

#include <ptlib.h>
#include <ptclib/pdns.h>
#include <ptclib/enum.h>

#if P_DNS

///////////////////////////////////////////////////////////////////////

PObject::Comparison PDNS::NAPTRRecord::Compare(const PObject & obj) const
{
  const NAPTRRecord * other = dynamic_cast<const NAPTRRecord *>(&obj);

  if (other == NULL)
    return LessThan;

  if (order < other->order)
    return LessThan;
  else if (order > other->order)
    return GreaterThan;

  if (preference < other->preference)
    return LessThan;
  else if (preference > other->preference)
    return GreaterThan;

  return EqualTo;
}

void PDNS::NAPTRRecord::PrintOn(ostream & strm) const
{
  strm << "order=" << order << ", "
       << "preference=" << preference << ", "
       << "flags=" << flags << ", "
       << "service=" << service << ", "
       << "regex=" << regex << ", "
       << "replacement=" << replacement;
}

///////////////////////////////////////////////////////////////////////

struct NAPTR_DNS {
  PUInt16b order;
  PUInt16b preference;

  char info[1];

  char * GetFlagsBase() const       { return (char *)&info; }
  int GetFlagsLen() const           { return (int)GetFlagsBase()[0]; }

  char * GetServiceBase() const     { return GetFlagsBase() + 1 + GetFlagsLen(); }
  int GetServiceLen() const         { return (int)GetServiceBase()[0]; }

  char * GetRegexBase() const       { return GetServiceBase() + 1 + GetServiceLen(); }
  int GetRegexLen() const           { return (int)GetRegexBase()[0]; }

  char * GetReplacementBase() const { return GetRegexBase() + 1 + GetRegexLen(); }
  int GetReplacementLen() const     { return (int)GetReplacementBase()[0]; }

  PString GetFlags() const          { return PString(GetFlagsBase()+1,       GetFlagsLen()); }
  PString GetService() const        { return PString(GetServiceBase()+1,     GetServiceLen()); }
  PString GetRegex() const          { return PString(GetRegexBase()+1,       GetRegexLen()); }
  PString GetReplacement() const    { return PString(GetReplacementBase()+1, GetReplacementLen()); }
};

PDNS::NAPTRRecord * PDNS::NAPTRRecordList::HandleDNSRecord(PDNS_RECORD dnsRecord, PDNS_RECORD /*results*/)
{
  PDNS::NAPTRRecord * record = NULL;

  if (
      (dnsRecord->Flags.S.Section == DnsSectionAnswer) && 
      (dnsRecord->wType == DNS_TYPE_NAPTR)
      ) {
    record = new NAPTRRecord();

    NAPTR_DNS * naptr = (NAPTR_DNS *)&dnsRecord->Data;

    record->order       = naptr->order;
    record->preference  = naptr->preference;
    record->flags       = naptr->GetFlags();
    record->service     = naptr->GetService();
    record->regex       = naptr->GetRegex();
    record->replacement = naptr->GetReplacement();
  }

  return record;
}


void PDNS::NAPTRRecordList::PrintOn(ostream & strm) const
{
  PINDEX i;
  for (i = 0; i < GetSize(); i++) 
    strm << (*this)[i] << endl;
}

PDNS::NAPTRRecord * PDNS::NAPTRRecordList::GetFirst(const char * service)
{
  if (GetSize() == 0)
    return NULL;

  currentPos   = 0;
  lastOrder = operator[](0).order;
  orderLocked = FALSE;

  return GetNext(service);
}

PDNS::NAPTRRecord * PDNS::NAPTRRecordList::GetNext(const char * service)
{
  if (GetSize() == 0)
    return NULL;

  while (currentPos < GetSize()) {

    NAPTRRecord & record = operator[](currentPos);

    // once we have a match, we cannot look at higher order records
    // and note that the list is already sorted by preference
    if (orderLocked && lastOrder != record.order)
      return NULL;

    else {
      currentPos++;
      lastOrder   = record.order;
      if (record.order == lastOrder) {
        if (service == NULL || record.service == service) {
          orderLocked = TRUE;
          return &record;
        }
      }
    }
  }

  return NULL;
}


static PString ApplyRegex(const PString & orig, const PString & regexStr)
{
  // must have at least 3 delimiters and two chars of text
  if (regexStr.GetLength() < 5) { 
    PTRACE(1, "ENUM regex is too short: " << regexStr);
    return PString::Empty();
  }

  // first char in the regex is always the delimiter
  char delimiter = regexStr[0];

  // break the string into match and replace strings by looking for non-escaped delimiters
  PString strings[2];
  PINDEX strNum = 0;
  PINDEX pos = 1;
  PINDEX start = pos;
  for (pos = 1; strNum < 2 && pos < regexStr.GetLength(); pos++) {
    if (regexStr[pos] == '\\')
      pos++;
    else if (regexStr[pos] == delimiter) {
      strings[strNum] = regexStr(start, pos-1);
      strNum++;
      pos++;
      start = pos;
    }
  }

  // make sure we have some strings
  if (strings[0].IsEmpty() || strings[1].IsEmpty()) {
    PTRACE(1, "ENUM regex does not parse into two string: " << regexStr);
    return PString::Empty();
  }

  // get the flags
  PString flags;
  if (strNum == 2 && pos < regexStr.GetLength()-1) {
    pos++;
    flags = regexStr.Mid(pos+1).ToLower();
  }

  // construct the regular expression
  PRegularExpression regex;
  int regexFlags = PRegularExpression::Extended;
  if (flags.Find('i') != P_MAX_INDEX)
    regexFlags += PRegularExpression::IgnoreCase;
  if (!regex.Compile(strings[0], regexFlags)) {
    PTRACE(1, "ENUM regex does not compile : " << regexStr);
    return PString();
  }

  // apply the regular expression to the original string
  PIntArray starts(10), ends(10);
  if (!regex.Execute(orig, starts, ends)) {
    PTRACE(1, "ENUM regex does not execute : " << regexStr);
    return PString();
  }

  // replace variables in the second string
  PString value = strings[1];
  for (pos = 0; pos < value.GetLength(); pos++) {
    if (value[pos] == '\\' && pos < value.GetLength()-1) {
      int var = value[pos+1]-'1'+1;   // not sure if the extra -1 is correct
      PString str;
      if (var >= 0 && var < starts.GetSize() && var < ends.GetSize())
        str = orig(starts[var], ends[var]);
      value = value.Left(pos) + str + value.Mid(pos+2);
    }
  }

  return value;
}

BOOL PDNS::ENUMLookup(
   const PString & e164,
   const PString & service,
         PString & dn
)
{
  const char * domains[] = { "e164.voxgratia.org", "e164.org", "e164.arpa" };
  return PDNS::ENUMLookup(e164, service, PStringArray(sizeof(domains)/sizeof(domains[0]), domains), dn);
}

static BOOL InternalENUMLookup(const PString & e164, const PString & service, PDNS::NAPTRRecordList & records, PString & returnStr)
{
  BOOL result = FALSE;

  // get the first record that matches the service. 
  PDNS::NAPTRRecord * rec = records.GetFirst(service);

  do {

    // if no more records that match this service, then fail
    if (rec == NULL)
      break;

    // process the flags
    BOOL handled  = FALSE;
    BOOL terminal = TRUE;

    for (PINDEX f = 0; !handled && f < rec->flags.GetLength(); ++f) {
      switch (tolower(rec->flags[f])) {

        // do an SRV lookup
        case 's':
          terminal = TRUE;
          handled = FALSE;
          break;

        // do an A lookup
        case 'a':
          terminal = TRUE;
          handled = FALSE;
          break;

        // apply regex and do the lookup
        case 'u':
          returnStr = ApplyRegex(e164, rec->regex);
          result   = TRUE;
          terminal = TRUE;
          handled  = TRUE;
          break;

        // handle in a protocol specific way - not supported
        case 'p':
          handled = FALSE;
          break;
  
        default:
          handled = FALSE;
      }
    }

    // if no flags were accepted, then unlock the order on the record and get the next record
    if (!handled) {
      records.UnlockOrder();
      rec = records.GetNext(service);
      continue;
    }

    // if this was a terminal lookup, finish now
    if (terminal)
      break;

  } while (!result);

  return result;
}

BOOL PDNS::ENUMLookup(
        const PString & _e164,
        const PString & service,
   const PStringArray & enumSpaces,
              PString & returnStr
)
{
  PString e164 = _e164;

  if (e164[0] != '+')
    e164 = PString('+') + e164;

  ////////////////////////////////////////////////////////
  // convert to domain name as per RFC 2916

  // remove all non-digits
  PINDEX pos = 1;
  while (pos < e164.GetLength()) {
    if (isdigit(e164[pos]))
      pos++;
    else
      e164 = e164.Left(pos) + e164.Mid(pos+1);
  }

  // reverse the order of the digits, and add "." in between each digit
  PString domain;
  for (pos = 1; pos < e164.GetLength(); pos++) {
    if (!domain.IsEmpty())
      domain = PString('.') + domain;
    domain = PString(e164[pos]) + domain;
  }

  for (PINDEX i = 0; i < enumSpaces.GetSize(); i++) {

    PDNS::NAPTRRecordList records;

    // do the initial lookup - if no answer then the lookup failed
    if (!PDNS::GetRecords(domain + "." + enumSpaces[i], records))
      continue;

    if (InternalENUMLookup(e164, service, records, returnStr))
      return TRUE;
  }

  return FALSE;
}

#endif

// End of File ///////////////////////////////////////////////////////////////
