// File: unpack.cpp
// Name: Leah Broussard
// Date: 2013/4/4
// Purpose: Unpacks packets in run%05.fat files into waveforms
//
// ReadPackets: Converts packets into waveforms
//
// Revision History:
// 2013/4/4:  LJB  file created
// 2013/4/8:  LJB  baby steps toward reading waveforms

#ifndef UNPACK_CPP__
#define UNPACK_CPP__

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "UCNBConfig.hh"
#include "PackagedFile.hh"
#include "Waveform.hh"

void ReadPackets(PackagedFile& InputFile, Waveform& WaveList);
bool Check_Serial(int& serialcounter, int& packetserial);

/*************************************************************************/
//                            Main Function
/*************************************************************************/
int main (int argc, char *argv[]) {  
  using std::cout;
  using std::endl;
  
  cout << "Welcome to Unpack version " << UCNB_VERSION_MAJOR << ".";
  cout << UCNB_VERSION_MINOR << endl;
  if (argc != 2) {
    cout << "Usage: " << argv[0] << " filenum" << endl;
    cout << "Test Robby's code using " << argv[0] << " 420 " << endl;
    cout << "Check our code using " << argv[0] << " 730+ " << endl;
    return 1;
  }
  int filenum = atoi(argv[1]);

  //-----Open file
  PackagedFile InputFile(filenum);
  if (!InputFile.IsFatOpen()) {
    cout << "File Not Open!" << endl;
    return 0;
  }
  Waveform WaveList;
  //-----Read packets
  ReadPackets(InputFile, WaveList);


  return 0; 
}


/*************************************************************************/
//                             ReadPackets  
/*************************************************************************/
void ReadPackets(PackagedFile& InputFile, Waveform& WaveList) {
  output_header header;
  std::vector<Data_Block_t> datablck;

  int serialcounter = -1;
  for (int i=0;i<2;i++) {
    //-----Read header + data
    if (!InputFile.ReadEvent(header,datablck)) {
      cout << "Read Event failed!" << endl;
      return;
    }
    //-----Ignore duplicate packets
    if (!Check_Serial(serialcounter, header.packet_serial))
      continue;
    //-----Unpack packet
    InputFile.PrintHeader(header);
    cout << "Have " << datablck.size() << " data blocks" << endl;
    WaveList.UnpackWaves(header,datablck);
  }
}

/*************************************************************************/
//                             Check_Serial 
/*************************************************************************/
bool Check_Serial(int& serialcounter, int& packetserial) {
  if (serialcounter == -1) 
    serialcounter = packetserial - 1;
  if (serialcounter > 65000 && packetserial < 1000)
    serialcounter -= PACKET_SERIAL_MAX;
  if (serialcounter + 1 < packetserial)
    cout << "Lost packets detected" << endl;
  if (serialcounter > packetserial) {
    cout << "Duplicate packet detected" << endl;
    return false;
  }
  serialcounter = packetserial;
  return true;
}


#endif // __UNPACK_CPP__
