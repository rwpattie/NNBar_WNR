// File: PackagedFile.cpp
// Name: Leah Broussard
// Date: 2013/4/4
// Purpose: Handles output files created by MIDAS containing Fred Digitizer
//          data.
//
// Revision History:
// 2013/4/4:  LJB  can open .fat files in various ways
// 2013/4/5:  LJB  added open constructors, extended to ts.txt and .mid files,
//                 added data read routines
// 2013/4/8:  LJB  fixed data read routine, added print header and filestream 
//                 checker
// 2013/4/9:  LJB  data read now does an endian swap, to anticipate future
//                 compatibility changes

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
  if (ext==FAT || ext==MID)
    fFileList[ext].fFileStream.open(filename.c_str(),std::ios::binary);
  else if (ext==TS)
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
  //hardcode file name until I get a handle on our conventions
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
//                            CheckStream
/*************************************************************************/
bool PackagedFile::CheckStream(std::ifstream& stream) {
  if (!stream.good()) {
    if (stream.eof()) {      
      return false;
    }
    if (stream.fail()) {
      cout << "I/O logical error" << endl;
      return false;
    }
    if (stream.bad()) {
      cout << "Read error" << endl;
      return false;
    }
  }
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
    cout << "Board number does not match record." << endl;
    //return false;   do we need this? LJB
  }
  if (!ReadData(header.data_size,datablck))
    return false;
  return true;
}

/*************************************************************************/
//                             ReadHeader
/*************************************************************************/
bool PackagedFile::ReadHeader(output_header& header) {
  if (!IsFatOpen()) {
    cout << "Error: file not open" << endl;
    return false;
  }
  if (!CheckStream(fFileList[FAT].fFileStream))
      return false;
  //LJB This may eventually depend on .fat version number
  fFileList[FAT].fFileStream.read(reinterpret_cast<char *>(&header.board_number),sizeof(Int_t));
  fFileList[FAT].fFileStream.read(reinterpret_cast<char *>(&header.packet_serial),sizeof(Int_t));
  fFileList[FAT].fFileStream.read(reinterpret_cast<char *>(&header.fadc_number),sizeof(Int_t));
  fFileList[FAT].fFileStream.read(reinterpret_cast<char *>(&header.data_size),sizeof(Int_t));
  fFileList[FAT].fFileStream.read(reinterpret_cast<char *>(&header.tv_usec),sizeof(Int_t));
  fFileList[FAT].fFileStream.read(reinterpret_cast<char *>(&header.tv_sec),sizeof(Int_t));
  fFileList[FAT].fFileStream.read(reinterpret_cast<char *>(&header.admin_message),sizeof(Int_t));
  fFileList[FAT].fFileStream.read(reinterpret_cast<char *>(&header.buffer_number),sizeof(Int_t));

  if (!CheckStream(fFileList[FAT].fFileStream))
      return false;
  return true;
}

/*************************************************************************/
//                              ReadData
/*************************************************************************/
bool PackagedFile::ReadData(Int_t datasize, std::vector<Data_Block_t> &datablck) {
  if (datasize <= 0) {
    cout << "Error: Data size 0" << endl;
    return false;
  }
  if (datasize%10 != 0) {
    cout << "Error: Bad data size" << endl;
    return false;
  }
  int size = datasize/10;
  datablck.clear();
  datablck.resize(size);

  for (int i=0;i<size;i++) {
    UInt_t buffer; //needs unsigned cast
    fFileList[FAT].fFileStream.read(reinterpret_cast<char *>(&buffer),4);
    SwapEndian(buffer,4);
    datablck[i].timestamp = (buffer >> 4);
    datablck[i].overflowB0 = (buffer & 0x08);
    datablck[i].overflowA0 = (buffer & 0x04);
    datablck[i].overflowB1 = (buffer & 0x02);
    datablck[i].overflowA1 = (buffer & 0x01);
    fFileList[FAT].fFileStream.read(reinterpret_cast<char *>(&buffer),3);
    SwapEndian(buffer,3);
    datablck[i].sample[3] = ((buffer & 0xFFF000)>>12);
    datablck[i].sample[2] =  (buffer & 0x000FFF);
    fFileList[FAT].fFileStream.read(reinterpret_cast<char *>(&buffer),3);
    SwapEndian(buffer,3);
    datablck[i].sample[1] = ((buffer & 0xFFF000)>>12);
    datablck[i].sample[0] =  (buffer & 0x000FFF);
    if (!CheckStream(fFileList[FAT].fFileStream))
      return false;
  }
  return true;
}

/*************************************************************************/
//                            FillDataBlocks
/*************************************************************************/
void PackagedFile::FillDataBlocks(UChar_t buffer[RAWDATA_LENGTH], int size, std::vector<Data_Block_t> &datablck) {
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

/*************************************************************************/
//                            PrintHeader
/*************************************************************************/
void PackagedFile::PrintHeader(output_header header){
  cout << "This packet's header:" << endl;
  cout << "** Board number:   " << header.board_number << endl;
  cout << "** Packet Serial:  " << header.packet_serial << endl;
  cout << "** FADC number:    " << header.fadc_number << endl;
  cout << "** Data size:      " << header.data_size << endl;
  cout << "** TV usec:        " << header.tv_usec << endl;
  cout << "** TV sec:         " << header.tv_sec << endl;
  cout << "** Admin. message: " << header.admin_message << endl;
  cout << "** Buffer number:  " << header.buffer_number << endl;
}

/*************************************************************************/
//                             SwapEndian
/*************************************************************************/
void PackagedFile::SwapEndian(UInt_t& swapme, int bytes) {
  if (bytes <= 1)
    return;
  UInt_t val = 0;
  for (int i=0;i<bytes;i++) {
    val |= ((swapme >> 8*i) & 0xFF) << (8*(bytes-1)-8*i);
  }
  swapme = val;
}

#endif // __PACKAGED_FILE_CPP__

