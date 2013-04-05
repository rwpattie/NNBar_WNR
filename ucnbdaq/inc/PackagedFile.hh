// File: PackagedFile.hh
// Name: Leah Broussard
// Date: 2013/4/4
// Purpose: Handles output files created by MIDAS containing Fred Digitizer
//          data.
//
// Revision History:
// 2013/4/4:  LJB  file created
 
#ifndef __PACKAGED_FILE_HH__
#define __PACKAGED_FILE_HH__

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

class PackagedFile
{
private:
  bool Open();
  std::string fFilePath;
  std::string fFileName;
  std::ifstream fFileStream;
  bool fIsOpen;
public:
  PackagedFile();
  PackagedFile(std::string path, std::string name);
  PackagedFile(std::string filename);
  PackagedFile(int filenum);
  ~PackagedFile();
  bool Open(std::string path, std::string name);
  bool Open(std::string filename);
  bool Open(int filenum);
  void Close();
  inline bool IsOpen(){return fIsOpen;}
};

#endif // __PACKAGED_FILE_HH__
