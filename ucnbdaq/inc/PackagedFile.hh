// File: PackagedFile.hh
// Name: Leah Broussard
// Date: 2013/4/4
// Purpose: Handles output files created by MIDAS containing Fred Digitizer
//          data.
//
// Revision History:
// 2013/4/4:  LJB  can open .fat files
// 2013/4/5:  LJB  added multi-file support, read routines
// 2013/4/8:  LJB  added print, filestream checks
 
#ifndef PACKAGED_FILE_HH__
#define PACKAGED_FILE_HH__

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

#include "FredDigitizer.hh"

using std::cout;
using std::endl;


class PackagedFile
{

private:

  const static int PF_ntypes = 3;
  enum PF_type{FAT, TS, MID};
  struct PF_file_t{
    std::string fFilePath;
    std::string fFileName;
    std::ifstream fFileStream;
  };
  PF_file_t fFileList[PF_ntypes];
  void Close(PF_type ext);
  bool CheckExtension(std::string name, PF_type& ext);
  void FillDataBlocks(UChar_t buffer[RAWDATA_LENGTH],int size,std::vector<Data_Block_t> &blck);
  bool CheckStream(std::ifstream& stream);
public:

  PackagedFile();
  PackagedFile(std::string path, std::string name);
  PackagedFile(std::string pathname);
  PackagedFile(int filenum);
  ~PackagedFile();
  bool Open(std::string path, std::string name);
  bool Open(std::string pathname);
  bool Open(int filenum);
  void Close();
  bool ReadEvent(output_header& header, std::vector<Data_Block_t> &datablck);
  bool ReadHeader(output_header& header);
  bool ReadData(Int_t datasize, std::vector<Data_Block_t> &datablck);
  void PrintHeader(output_header header);
  inline bool IsFatOpen(){return fFileList[FAT].fFileStream.is_open();}
  inline bool IsTsOpen(){return fFileList[TS].fFileStream.is_open();}
  inline bool IsMidOpen(){return fFileList[MID].fFileStream.is_open();}

};

#endif // __PACKAGED_FILE_HH__
