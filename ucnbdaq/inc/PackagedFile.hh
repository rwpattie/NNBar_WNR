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

#include <fstream>
#include <iostream>
#include <string>

class PackagedFile
{
private:
  std::string fFilePath;
  std::string fFileName;
  std::ifstream fFileStream;
public:
  PackagedFile();
  ~PackagedFile();
  
};

#endif // __PACKAGED_FILE_HH__
