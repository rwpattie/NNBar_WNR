// File: PackagedFile.cpp
// Name: Leah Broussard
// Date: 2013/4/4
// Purpose: Handles output files created by MIDAS containing Fred Digitizer
//          data.
//
// Revision History:
// 2013/4/4:  LJB  file created, can open files in various ways
// 2013/4/5:  LJB  constructors open files

#ifndef __PACKAGED_FILE_CPP__
#define __PACKAGED_FILE_CPP__

#include "PackagedFile.hh"
#include "UCNBConfig.hh"

/*************************************************************************/
//                           Constructors
/*************************************************************************/
PackagedFile::PackagedFile() {
}

PackagedFile::PackagedFile(std::string path, std::string name) {
  Open(path,name);
}

PackagedFile::PackagedFile(std::string filename) {
  Open(filename);
}

PackagedFile::PackagedFile(int filenum) {
  Open(filenum);
}

/*************************************************************************/
//                             Destructor
/*************************************************************************/
PackagedFile::~PackagedFile() {
  if (fIsOpen)
    Close(); 
}

/*************************************************************************/
//                                Open
/*************************************************************************/

bool PackagedFile::Open() {
  if (fIsOpen)
    Close(); 
  std::string filename = fFilePath;
  filename.append("/");
  filename.append(fFileName);
  fFileStream.open(filename.c_str());
  fIsOpen = fFileStream.is_open();
  if (!fIsOpen) {
    fFileName = "";
    fFilePath = "";
  }
  return fIsOpen;
}

bool PackagedFile::Open(std::string path, std::string name) {
  fFilePath = path;
  fFileName = name;
  return Open();
}

bool PackagedFile::Open(std::string filename) {
  std::size_t pos = filename.rfind("/");
  if (pos!=std::string::npos) {
    fFilePath = filename.substr(0,pos+1);
    fFileName = filename.substr(pos+1,filename.length());
  }
  else {
    //1. Check local directory
    //2. Check Files/ directory
    //3. Check INPUT_PATH defined in UCNBConfig.hh
    //4. Check environmental variables (getenv("WNR_RAW_DATA"))
    const int ntrypath = 7;
    std::string trypath[] = {".","file","File","files","Files",
			      INPUT_PATH,getenv("WNR_RAW_DATA")};
    int tp = 0;
    do {
      fFileName = filename;
      fFilePath = trypath[tp];
    }while (++tp<ntrypath && !Open());
  }
  return fIsOpen;
}

bool PackagedFile::Open(int filenum) {
  char tempstr[255];
  sprintf(tempstr,"run%05d.fat",filenum);
  std::string filename = tempstr;
  return Open(filename);
}

/*************************************************************************/
//                               Close
/*************************************************************************/
void PackagedFile::Close() {
  if (fIsOpen) {
    fFileStream.close();
    fIsOpen = false;
  }
}


#endif // __PACKAGED_FILE_CPP__
