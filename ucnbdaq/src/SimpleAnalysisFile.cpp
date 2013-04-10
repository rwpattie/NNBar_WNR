// File: SimpleAnalysisFile.cpp
// Name: Leah Broussard
// Date: 2013/4/9
// Purpose: Handles the zeroth level analysis ROOT TTree
//
// Revision History:
// 2013/4/9:  LJB  skeleton class
// 2013/4/10: LJB  added open/create/fill/write functions and analysis 
//                 routines. open doesnt set branches yet. typing is 
//                 inconsistent in order to preserve original structures.

 
#ifndef SIMPLE_ANALYSIS_FILE_CPP__
#define SIMPLE_ANALYSIS_FILE_CPP__

#include "SimpleAnalysisFile.hh"

/*************************************************************************/
//                            Constructor
/*************************************************************************/
SimpleAnalysisFile::SimpleAnalysisFile() {
  RootFile = 0;
  RootTree = 0;
  createmode = false;
}

/*************************************************************************/
//                              Destructor
/*************************************************************************/
SimpleAnalysisFile::~SimpleAnalysisFile() {
  Close();
}

/*************************************************************************/
//                               Close   
/*************************************************************************/
void SimpleAnalysisFile::Close() {
  if (RootFile != 0) {
    if (RootFile->IsOpen())
      RootFile->Close();
    delete RootFile;
  }
  RootFile = 0;
  RootTree = 0;
  createmode = false;
}

/*************************************************************************/
//                                Open   
/*************************************************************************/
bool SimpleAnalysisFile::Open(std::string path, std::string name){
  Close();
  std::string filename = path;
  filename.append("/");
  filename.append(name);
  if (gSystem->AccessPathName(filename.c_str())) {
    return false;
  }
  RootFile = new TFile(filename.c_str());
  if (!RootFile->IsOpen()) {
    Close();
    return false;
  }
  createmode = false;
}

bool SimpleAnalysisFile::Open(std::string filename){
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

bool SimpleAnalysisFile::Open(int filenum){
  char tempstr[255];
  sprintf(tempstr,"run%05dNA-test.root",filenum);
  std::string filename = tempstr;
  return Open(filename);
}

/*************************************************************************/
//                               Create  
/*************************************************************************/
bool SimpleAnalysisFile::Create(std::string path, std::string name) {
  Close();
  std::string filename = path;
  if (gSystem->AccessPathName(filename.c_str())) {
    return false;
  }
  filename.append("/");
  filename.append(name);
  RootFile = new TFile(filename.c_str(),"RECREATE");
  if (!RootFile->IsOpen()) {
    cout << "Failed to create ROOT TFile" << endl;
    Close();
    return false;
  }
  RootTree = new TTree("t","t");

  last = RAWDATA_LENGTH;
  RootTree->Branch("first_time",&first_time,"first_time/l");
  RootTree->Branch("global_time",&global_time,"global_time/l");
  RootTree->Branch("packet_time_s",&packet_time_s,"packet_time_s/I");
  RootTree->Branch("packet_time_us",&packet_time_us,"packet_time_us/I");
  RootTree->Branch("packet_time_l",&packet_time_l,"packet_time_l/I");
  RootTree->Branch("board",&board,"board/I");
  RootTree->Branch("last",&last,"last/I");
  RootTree->Branch("adc",adc,"adc[last]/s");
  RootTree->Branch("max",&max,"max/s");
  RootTree->Branch("min",&min,"min/s");
  RootTree->Branch("zero",&zero,"zero/s");
  RootTree->Branch("ped",&ped,"ped/s");
  RootTree->Branch("channel",&channel,"channel/s");

  createmode = true;
  return true;
}

bool SimpleAnalysisFile::Create(std::string filename) {
  std::size_t pos = filename.rfind("/");
  if (pos!=std::string::npos) {
    std::string path = filename.substr(0,pos+1);
    std::string name = filename.substr(pos+1,filename.length());
    return Create(path, name);
  }
  else { //no path in filename
    //1. Use INPUT_PATH defined in UCNBConfig.hh
    //2. Use environmental variables (getenv("WNR_RAW_DATA"))
    //3. Use Files/ directory
    //4. Use local directory
    std::string name = filename; 
    const int ntrypath = 7;
    std::string trypath[] = {INPUT_PATH,getenv("WNR_RAW_DATA"),
			     "file","File","files","Files","."};
    int tp = 0;
    bool success = false;
    do {
      success = Create(trypath[tp],name);
    }while (++tp<ntrypath && !success);
    return success;
  }
}

bool SimpleAnalysisFile::Create(int filenum){
  char tempstr[255];
  sprintf(tempstr,"run%05dNA-test.root",filenum);
  std::string filename = tempstr;
  return Create(filename);
}

/*************************************************************************/
//                            AnalyzePackets
/*************************************************************************/
void SimpleAnalysisFile::AnalyzePackets(Waveform& WaveList) {
  std::vector<Int_t> data;
  output_header header;
  Long64_t timestamp;
  Long64_t current_timestamp[NUMCH];
  Int_t initial_time = -1;
  int ncycle[NUMCH]; // in case channel timestamps are out of sync
  for (int c=0;c<NUMCH;c++)
    ncycle[c] = 0;
  for (int i=0;i<WaveList.GetSize();i++) {
    //-----Grab waveform info
    WaveList.GetData(i,data);
    WaveList.GetHeader(i,header);
    timestamp = WaveList.GetTimestamp(i);
    //-----Fill header info
    board = header.board_number;
    channel = header.fadc_number;
    //-----Update time variables
    if (current_timestamp[channel] > timestamp)
      ncycle[channel]++;
    if (initial_time == -1)
      initial_time = header.tv_sec;
    first_time = timestamp;
    global_time = timestamp + (cycleoffset * ncycle[channel]);
    packet_time_s = header.tv_sec;
    packet_time_us = header.tv_usec;
    packet_time_l = header.tv_sec - initial_time;
    //-----Store waveform
    last = data.size();
    for (int j=0;j<last;j++)
      adc[j] = static_cast<UShort_t>(data[j]);
    //-----Calculate statistics
    max = CalcMax(data);
    min = CalcMin(data);
    zero = CalcZero(data);
    ped = CalcPed(data);
    FillTree();
  }
}

/*************************************************************************/
//                              CalcMax  
/*************************************************************************/
UShort_t SimpleAnalysisFile::CalcMax(std::vector<Int_t> data) {
  Int_t max = 0;
  for (int i=0;i<data.size();i++) {
    if (max < data[i])
      max = data[i];
  }
  return static_cast<UShort_t>(max);
}

/*************************************************************************/
//                              CalcMin  
/*************************************************************************/
UShort_t SimpleAnalysisFile::CalcMin(std::vector<Int_t> data) {
  Int_t min = pow(2,12)-1;
  for (int i=0;i<data.size();i++) {
    if (min > data[i])
      min = data[i];
  }
  return static_cast<UShort_t>(min);
}

/*************************************************************************/
//                              CalcPed
/*************************************************************************/
UShort_t SimpleAnalysisFile::CalcPed(std::vector<Int_t> data) {
  if (data.size() < 15)
    return 0;
  Int_t ped = 0;
  for (int i=0;i<data.size() && i<15;i++) {
    ped += data[i];
  }
  ped = ped/15.;
  return static_cast<UShort_t>(ped);
}

/*************************************************************************/
//                             CalcZero
/*************************************************************************/
UShort_t SimpleAnalysisFile::CalcZero(std::vector<Int_t> data) {
  if (data.size() == 0)
    return 1;
  Int_t zero = 0;
  for (int i=0;(i*4+3<data.size()) && i<6;i++) {
    zero += data[i*4] + data[i*4 + 1] + data[i*4 + 2] + data[i*4 + 3];
  }
  return static_cast<UShort_t>(zero/data.size());
}

/*************************************************************************/
//                              FillTree 
/*************************************************************************/
void SimpleAnalysisFile::FillTree(){
  if (!RootFile->IsOpen()) {
    std::cout << "File not open" << std::endl;
    return;
  }
  if (RootTree == 0) {
    std::cout << "Tree does not exist" << std::endl;
    return;    
  }
  if (!createmode) {
    std::cout << "Create new file first" << std::endl;
    return;    
  }
  RootFile->cd();
  RootTree->Fill();
}

/*************************************************************************/
//                                Write  
/*************************************************************************/
void SimpleAnalysisFile::Write(){
  if (!RootFile->IsOpen()) {
    std::cout << "File not open" << std::endl;
    return;
  }
  if (!createmode) {
    std::cout << "Create new file first" << std::endl;
    return;    
  }
  RootFile->cd();
  RootFile->Write();
  Close();
}

#endif // SIMPLE_ANALYSIS_FILE_CPP__

