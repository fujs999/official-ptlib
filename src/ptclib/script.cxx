/*
 * script.cxx
 *
 * Abstract class for script language interfaces
 *
 * Portable Tools Library
 *
 * Copyright (C) 2010-2012 by Post Increment
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
 * Contributor(s): Craig Southeren
 *                 Robert Jongbloed
 */

#ifdef __GNUC__
#pragma implementation "script.h"
#endif

#include <ptlib.h>

#include <ptclib/script.h>

#define new PNEW

PScriptLanguage::PScriptLanguage()
  : m_loaded(false)
  , m_lastErrorCode(0)
{
}


PScriptLanguage::~PScriptLanguage()
{
}


bool PScriptLanguage::Load(const PString & script)
{
  PFilePath filename = script;
  if (PFile::Exists(filename)) {
    if (!LoadFile(filename))
      return false;
  }
  else {
    if (!LoadText(script))
      return false;
  }

  return true;
}


void PScriptLanguage::OnError(int code, const PString & str)
{
  m_mutex.Wait();
  m_lastErrorCode = code;
  m_lastErrorText = str.IsEmpty() && code != 0 ? psprintf("Unknown error %i", code) : str;
  PTRACE(2, this, GetLanguageName(), "Error " << code << ": " << m_lastErrorText);
  m_mutex.Signal();
}


bool PScriptLanguage::InternalSetFunction(const PString & name, const FunctionNotifier & func)
{
  FunctionMap::iterator it = m_functions.find(PString(name));
  if (it == m_functions.end())
    return !func;

  if (!func)
    m_functions.erase(it);
  else
    it->second = func;
  return true;
}


void PScriptLanguage::InternalRemoveFunction(const PString & prefix)
{
  FunctionMap::iterator it = m_functions.lower_bound(PString(prefix));
  while (it != m_functions.end() && it->first.NumCompare(prefix) == EqualTo) {
    if (isalnum(it->first[prefix.GetLength()]))
      ++it;
    else
      m_functions.erase(it++);
  }
}


PScriptLanguage * PScriptLanguage::Create(const PString & language)
{
  PScriptLanguage * script = PFactory<PScriptLanguage>::CreateInstance(language);
  if (script != NULL && script->IsInitialised())
    return script;

  delete script;
  return NULL;
}


PStringArray PScriptLanguage::GetLanguages()
{
  PStringArray names = PFactory<PScriptLanguage>::GetKeyList();
  for (PINDEX i = 0; i < names.GetSize(); ) {
    PScriptLanguage * script = Create(names[i]);
    if (script == NULL)
      names.RemoveAt(i);
    else {
      delete script;
      ++i;
    }
  }
  return names;
}
