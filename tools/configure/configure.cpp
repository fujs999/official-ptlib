/*
 * configure.cxx
 *
 * Build options generated by the configure script.
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
 * $Log: configure.cpp,v $
 * Revision 1.10  2003/11/25 08:21:37  rjongbloed
 * Fixed display of configured items
 *
 * Revision 1.9  2003/11/06 09:13:20  rjongbloed
 * Improved the Windows configure system to allow multiple defines based on file existence. Needed for SDL support of two different distros.
 *
 * Revision 1.8  2003/10/30 01:17:15  dereksmithies
 * Add fix from Jose Luis Urien. Many thanks.
 *
 * Revision 1.7  2003/10/23 21:49:51  dereksmithies
 * Add very sensible fix to limit extent of search. Thanks Ben Lear.
 *
 * Revision 1.6  2003/08/04 05:13:17  dereksmithies
 * Reinforce the disablement if the command lines specifies --no-XXXX to a feature.
 *
 * Revision 1.5  2003/08/04 05:07:08  dereksmithies
 * Command line option now disables feature when feature found on disk.
 *
 * Revision 1.4  2003/05/16 02:03:07  rjongbloed
 * Fixed being able to manually disable a "feature" when does a full disk search.
 *
 * Revision 1.3  2003/05/05 08:39:52  robertj
 * Added ability to explicitly disable a feature, or tell configure exactly
 *   where features library is so it does not need to search for it.
 *
 * Revision 1.2  2003/04/17 03:32:06  robertj
 * Improved windows configure program to use lines out of configure.in
 *
 * Revision 1.1  2003/04/16 08:00:19  robertj
 * Windoes psuedo autoconf support
 *
 */

#pragma warning(disable:4786)

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <stdlib.h>
#include <windows.h>


using namespace std;


class Feature
{
  public:
    Feature() : enabled(true) { }
    Feature(const string & featureName, const string & optionName, const string & optionValue);

    void Parse(const string & optionName, const string & optionValue);
    void Adjust(string & line);
    bool Locate(const char * testDir);

    string featureName;
    string displayName;
    string directorySymbol;
    string simpleDefineName;
    string simpleDefineValue;

    struct CheckFileInfo {
      CheckFileInfo() : found(false), defineName("1") { }

      bool Locate(const string & testDir);

      bool   found;
      string fileName;
      string fileText;
      string defineName;
      string defineValue;
    };
    list<CheckFileInfo> checkFiles;

    string directory;
    bool   enabled;
};


list<Feature> features;


///////////////////////////////////////////////////////////////////////

Feature::Feature(const string & featureNameParam,
                 const string & optionName,
                 const string & optionValue)
  : featureName(featureNameParam),
    enabled(true)
{
  Parse(optionName, optionValue);
}


void Feature::Parse(const string & optionName, const string & optionValue)
{
  if (optionName == "DISPLAY")
    displayName = optionValue;
  else if (optionName == "DEFINE") {
    int equal = optionValue.find('=');
    if (equal == string::npos)
      simpleDefineName = optionValue;
    else {
      simpleDefineName.assign(optionValue, 0, equal);
      simpleDefineValue.assign(optionValue, equal+1, INT_MAX);
    }
  }
  else if (optionName == "CHECK_FILE") {
    int comma = optionValue.find(',');
    if (comma == string::npos)
      return;

    CheckFileInfo check;
    int pipe = optionValue.find('|');
    if (pipe < 0 || pipe > comma)
      check.fileName.assign(optionValue, 0, comma);
    else {
      check.fileName.assign(optionValue, 0, pipe);
      check.fileText.assign(optionValue, pipe+1, comma-pipe-1);
    }

    int equal = optionValue.find('=', comma);
    if (equal == string::npos)
      check.defineName.assign(optionValue, comma+1, INT_MAX);
    else {
      check.defineName.assign(optionValue, comma+1, equal-comma-1);
      check.defineValue.assign(optionValue, equal+1, INT_MAX);
    }

    checkFiles.push_back(check);
  }
  else if (optionName == "DIR_SYMBOL")
    directorySymbol = '@' + optionValue + '@';
  else if (optionName == "CHECK_DIR")
    Locate(optionValue.c_str());
}


static bool CompareName(const string & line, const string & name)
{
  int pos = line.find(name);
  if (pos == string::npos)
    return false;

  pos += name.length();
  return !isalnum(line[pos]) && line[pos] != '_';
}

void Feature::Adjust(string & line)
{
  if (enabled && line.find("#undef") != string::npos) {
    if (!simpleDefineName.empty() && CompareName(line, simpleDefineName))
      line = "#define " + simpleDefineName + ' ' + simpleDefineValue;

    for (list<CheckFileInfo>::iterator file = checkFiles.begin(); file != checkFiles.end(); file++) {
      if (file->found && CompareName(line, file->defineName)) {
        line = "#define " + file->defineName + ' ' + file->defineValue;
        break;
      }
    }
  }

  if (directorySymbol[0] != '\0') {
    int pos = line.find(directorySymbol);
    if (pos != string::npos)
      line.replace(pos, directorySymbol.length(), directory);
  }
}


bool Feature::Locate(const char * testDir)
{
  if (!directory.empty())
    return true;

  if (checkFiles.empty())
    return true;

  string testDirectory = testDir;
  if (testDirectory[testDirectory.length()-1] != '\\')
    testDirectory += '\\';

  list<CheckFileInfo>::iterator file = checkFiles.begin();
  if (!file->Locate(testDirectory))
    return false;

  while (++file != checkFiles.end())
    file->Locate(testDirectory);

  char buf[_MAX_PATH];
  _fullpath(buf, testDirectory.c_str(), _MAX_PATH);
  directory = buf;

  cout << "Located " << displayName << " at " << directory << endl;

  int pos;
  while ((pos = directory.find('\\')) != string::npos)
    directory[pos] = '/';
  pos = directory.length()-1;
  if (directory[pos] == '/')
    directory.erase(pos);

  return true;
}


bool Feature::CheckFileInfo::Locate(const string & testDirectory)
{
  string filename = testDirectory + fileName;
  ifstream file(filename.c_str(), ios::in);
  if (!file.is_open())
    return false;

  if (fileText.empty())
    found = true;
  else {
    while (file.good()) {
      string line;
      getline(file, line);
      if (line.find(fileText) != string::npos) {
        found = true;
        break;
      }
    }
  }

  return found;
}


bool TreeWalk(const string & directory)
{
  bool foundAll = false;

  string wildcard = directory;
  wildcard += "*.*";

  WIN32_FIND_DATA fileinfo;
  HANDLE hFindFile = FindFirstFile(wildcard.c_str(), &fileinfo);
  if (hFindFile != NULL) {
    do {
      string subdir = directory;
      subdir += fileinfo.cFileName;

      if ((fileinfo.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) != 0 &&
                                       fileinfo.cFileName[0] != '.' &&
                                       stricmp(fileinfo.cFileName, "RECYCLER") != 0) {
        subdir += '\\';

        foundAll = true;
        list<Feature>::iterator feature;
        for (feature = features.begin(); feature != features.end(); feature++) {
          if (!feature->Locate(subdir.c_str()))
            foundAll = false;
        }

        if (foundAll)
          break;

        TreeWalk(subdir);
      }
    } while (FindNextFile(hFindFile, &fileinfo));

    FindClose(hFindFile);
  }

  return foundAll;
}


bool ProcessHeader(const string & headerFilename)
{
  string inFilename = headerFilename;
  inFilename += ".in";

  ifstream in(inFilename.c_str(), ios::in);
  if (!in.is_open()) {
    cerr << "Could not open " << inFilename << endl;
    return false;
  }

  ofstream out(headerFilename.c_str(), ios::out);
  if (!out.is_open()) {
    cerr << "Could not open " << headerFilename << endl;
    return false;
  }

  while (in.good()) {
    string line;
    getline(in, line);

    list<Feature>::iterator feature;
    for (feature = features.begin(); feature != features.end(); feature++)
      feature->Adjust(line);

    out << line << '\n';
  }

  return !in.bad() && !out.bad();
}


int main(int argc, char* argv[])
{
  ifstream conf("configure.in", ios::in);
  if (!conf.is_open()) {
    cerr << "Could not open configure.in file" << endl;
    return 1;
  }

  list<string> headers;
  list<Feature>::iterator feature;

  while (conf.good()) {
    string line;
    getline(conf, line);
    int pos;
    if ((pos = line.find("AC_CONFIG_HEADERS")) != string::npos) {
      pos = line.find('(', pos);
      if (pos != string::npos) {
        int end = line.find(')', pos);
        if (pos != string::npos)
          headers.push_back(line.substr(pos+1, end-pos-1));
      }
    }
    else if (line.find("dnl MSWIN_") == 0) {
      int space = line.find(' ', 10);
      if (space != string::npos) {
        string optionName(line, 10, space-10);
        while (line[space] == ' ')
          space++;
        int comma = line.find(',', space);
        if (comma != string::npos) {
          string optionValue(line, comma+1, INT_MAX);
          if (!optionValue.empty()) {
            string featureName(line, space, comma-space);
            bool found = false;
            for (feature = features.begin(); feature != features.end(); feature++) {
              if (feature->featureName == featureName) {
                found = true;
                break;
              }
            }
            if (found)
              feature->Parse(optionName, optionValue);
            else
              features.push_back(Feature(featureName, optionName, optionValue));
          }
        }
      }
    }
  }

  const char EXTERN_DIR[] = "--extern-dir=";
  
  bool searchDisk = true;
  char *externDir = 0;
  for (int i = 1; i < argc; i++) {
    if (stricmp(argv[i], "--no-search") == 0)
      searchDisk = false;
    else if (strnicmp(argv[i], EXTERN_DIR, sizeof(EXTERN_DIR) - 1) == 0){
	    externDir = argv[i] + sizeof(EXTERN_DIR) - 1; 	
    }
    else if (stricmp(argv[i], "-h") == 0 || stricmp(argv[i], "--help") == 0) {
      cout << "usage: configure args\n"
              "  --no-search\t\tDo not search disk for libraries.\n"
	"  --extern-dir\t\t specify where to search disk for libraries.\n";
      for (feature = features.begin(); feature != features.end(); feature++) {
        if (feature->featureName[0] != '\0') {
          cout << "  --no-" << feature->featureName
               << "\t\tDisable " << feature->displayName << '\n';
          if (!feature->checkFiles.empty())
            cout << "  --" << feature->featureName << "-dir=dir"
                    "\tSet directory for " << feature->displayName << '\n';
        }
      }
      return 1;
    }
    else {
      for (feature = features.begin(); feature != features.end(); feature++) {
        if (stricmp(argv[i], ("--no-"+feature->featureName).c_str()) == 0)
          feature->enabled = false;
	else if (strstr(argv[i], ("--" + feature->featureName+"-dir=").c_str()) == argv[i])
	  if (!feature->Locate(argv[i] + strlen(("--" + feature->featureName + "-dir=").c_str())))
	    cerr << feature->displayName << " not found in "
		 << argv[i] + strlen(("--" + feature->featureName+"-dir=").c_str()) << endl;
      }
    }
  }

  if (searchDisk) {
    bool foundAll = true;
    for (feature = features.begin(); feature != features.end(); feature++) {
      if (feature->enabled && !feature->checkFiles.empty() && !feature->checkFiles.begin()->found)
        foundAll = false;
    }

    if (!foundAll) {
      // Do search of entire system
      char drives[100];
      if (!externDir){
	if (!GetLogicalDriveStrings(sizeof(drives), drives))
	  strcpy(drives, "C:\\");
      } else {
	strcpy(drives, externDir);	
      }

      const char * drive = drives;
      while (*drive != '\0') {
        if (GetDriveType(drive) == DRIVE_FIXED) {
          cout << "Searching " << drive << endl;
          if (TreeWalk(drive))
            break;
        }
        drive += strlen(drive)+1;
      }
    }
  }

  for (feature = features.begin(); feature != features.end(); feature++) {
    cout << "  " << feature->displayName << ' ';
    if (feature->checkFiles.empty() && !feature->simpleDefineValue.empty())
      cout << "set to " << feature->simpleDefineValue;
    else if (feature->enabled && (feature->checkFiles.empty() || feature->checkFiles.begin()->found))
      cout << "enabled";
    else
      cout << "disabled";
    cout << '\n';
  }
  cout << endl;

  for (list<string>::iterator hdr = headers.begin(); hdr != headers.end(); hdr++)
    ProcessHeader(*hdr);

  cout << "Configuration completed.\n";

  return 0;
}
