// File: PackagedFile.cpp
// Name: Leah Broussard
// Date: 2013/4/4
// Purpose: Handles output files created by MIDAS containing Fred Digitizer
//          data.
//
// Revision History:
// 2013/4/4:  LJB  file created, can open .fat files in various ways
// 2013/4/5:  LJB  added open constructors, ts.txt and .mid file support,
//                 data read routines

#ifndef PACKAGED_FILE_CPP__
#define PACKAGED_FILE_CPP__

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
  Close();
}

/*************************************************************************/
//                                Open
/*************************************************************************/

bool PackagedFile::Open(std::string path, std::string name) {
  PF_type ext;
  if (!CheckExtension(name,ext))
    return false;
  if (fFileList[ext].fFileStream.is_open())
    Close(ext);
  std::string filename = path;
  filename.append("/");
  filename.append(name);
  fFileList[ext].fFileStream.open(filename.c_str());
  if (!fFileList[ext].fFileStream.is_open()) {
    fFileList[ext].fFilePath = path;
    fFileList[ext].fFileName = name;
  }
  return fFileList[ext].fFileStream.is_open();
}

bool PackagedFile::Open(std::string filename) {
  std::size_t pos = filename.rfind("/");
  if (pos!=std::string::npos) {
    std::string path = filename.substr(0,pos+1);
    std::string name = filename.substr(pos+1,filename.length());
    return Open(path, name);
  }
  else { //no path in filename
    //1. Check local directory
    //2. Check Files/ directory
    //3. Check INPUT_PATH defined in UCNBConfig.hh
    //4. Check environmental variables (getenv("WNR_RAW_DATA"))
    std::string name = filename; 
    const int ntrypath = 7;
    std::string trypath[] = {".","file","File","files","Files",
			      INPUT_PATH,getenv("WNR_RAW_DATA")};
    int tp = 0;
    bool success = false;
    do {
      success = Open(trypath[tp],name);
    }while (++tp<ntrypath && !success);
    return success;
  }
}

bool PackagedFile::Open(int filenum) {
  //hardcode file name until I get a handle of our conventions
  bool openany = false;
  char tempstr[255];
  sprintf(tempstr,"run%05d.fat",filenum);
  std::string filename = tempstr;
  openany = openany || Open(filename);
  sprintf(tempstr,"run%05dts.txt",filenum);
  filename = tempstr;
  openany = openany || Open(filename);
  return openany;
}

/*************************************************************************/
//                               Close
/*************************************************************************/
void PackagedFile::Close() {
  for (int pf=0;pf<PF_ntypes;pf++)
    if (fFileList[pf].fFileStream.is_open()) {
      fFileList[pf].fFileName = "";
      fFileList[pf].fFilePath = "";
      fFileList[pf].fFileStream.close();
    }
}

void PackagedFile::Close(PF_type ext) {
  if (fFileList[ext].fFileStream.is_open()) {
    fFileList[ext].fFileName = "";
    fFileList[ext].fFilePath = "";
    fFileList[ext].fFileStream.close();
  }
}

/*************************************************************************/
//                           CheckExtension
/*************************************************************************/
bool PackagedFile::CheckExtension(std::string name, PF_type& ext) {
  std::size_t pos = name.rfind(".");
  std::string extstr = name.substr(pos,name.length());
  if (extstr.compare(".fat")==0)
    ext = FAT;
  else if (extstr.compare(".txt")==0)
    ext = TS;
  else if (extstr.compare(".mid")==0)
    ext = MID;
  else
    return false;
  return true;
}

/*************************************************************************/
//                              ReadEvent
/*************************************************************************/
bool PackagedFile::ReadEvent(output_header& header, std::vector<Data_Block_t> &datablck) {
  if (!ReadHeader(header))
    return false;
  //Check board number?
  bool bnfound;
  int board_index = 0;
  do {
    bnfound = (header.board_number == boardnum[board_index])? true : false;
  } while (!bnfound && ++board_index < NUMBOARDS);
  if (!bnfound) {
    printf("Board number does not match record.\n");
    //return false;   do we need this? LJB
  }
  //<Check last serial number?>
  if (!ReadData(header.data_size,datablck))
    return false;
}

/*************************************************************************/
//                             ReadHeader
/*************************************************************************/
bool PackagedFile::ReadHeader(output_header& header) {
  if (!IsFatOpen())
    return false;
  if (fFileList[FAT].fFileStream.eof())
    return false;
  fFileList[FAT].fFileStream >> header.board_number >> header.packet_serial
			     >> header.fadc_number >> header.data_size
			     >> header.tv_usec >> header.tv_sec
			     >> header.admin_message >> header.buffer_number;
  if (!fFileList[FAT].fFileStream) {
    printf("Didnt read full header\n");
    return false;
  }
  return true;
}

/*************************************************************************/
//                              ReadData
/*************************************************************************/
bool PackagedFile::ReadData(Int_t datasize, std::vector<Data_Block_t> &datablck) {
  if (datasize <= 0)
    return false;
  if (datasize%10 != 0) {
    printf("Bad data size\n");
    return false;
  }
  char buffer[RAWDATA_LENGTH];
  fFileList[FAT].fFileStream.read(buffer,datasize);
  if (!fFileList[FAT].fFileStream) {
    printf("Didnt read all data\n");
    return false;
  }
  FillDataBlocks(buffer,datasize/10,datablck);
  return true;
}

/*************************************************************************/
//                            FillDataBlocks
/*************************************************************************/
void PackagedFile::FillDataBlocks(char buffer[RAWDATA_LENGTH], int size, std::vector<Data_Block_t> &datablck) {
  datablck.clear();
  datablck.resize(size);
  for (int i=0;i<size;i++) {    
    datablck[i].timestamp = (buffer[i*10+0] << 20) | (buffer[i*10+1] << 12) |
      (buffer[i*10+2] << 4)  | (buffer[i*10+3] >> 4);
    datablck[i].overflowB0 = ((buffer[i*10+3] & 0x08) != 0);
    datablck[i].overflowA0 = ((buffer[i*10+3] & 0x04) != 0);
    datablck[i].overflowB1 = ((buffer[i*10+3] & 0x02) != 0);
    datablck[i].overflowA1 = ((buffer[i*10+3] & 0x01) != 0);
    datablck[i].sample[3] = (buffer[i*10+4] << 4) |(buffer[i*10+5] >> 4);
    datablck[i].sample[2] = ((buffer[i*10+5] & 0xf) << 8) |(buffer[i*10+6]);
    datablck[i].sample[1] = (buffer[i*10+7] << 4) |(buffer[i*10+8] >> 4);
    datablck[i].sample[0] = ((buffer[i*10+8] & 0xf) << 8) |(buffer[i*10+9]);
  }
}



#endif // __PACKAGED_FILE_CPP__

