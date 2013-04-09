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
int Check_Serial(int& serialcounter, int& packetserial);

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
  int packetcounter = 0;
  int lostpacketcounter = 0;
  do {
    //-----Read header + data
    if (!InputFile.ReadEvent(header,datablck)) {
      if (InputFile.IsFatEOF())
	continue;
      else
	cout << "Read Event failed!" << endl;
    }
    //-----Ignore duplicate packets
    int check = Check_Serial(serialcounter, header.packet_serial);
    if (check == -1) continue;
    else lostpacketcounter += check;
    //-----Unpack packet
    //InputFile.PrintHeader(header);
    //cout << "Have " << datablck.size() << " data blocks" << endl;
    WaveList.UnpackWaves(header,datablck);
    packetcounter++;
  } while (!InputFile.IsFatEOF());
  cout << "Read " << packetcounter << " packets, lost " << lostpacketcounter << endl;
}

/*************************************************************************/
//                             Check_Serial 
/*************************************************************************/
int Check_Serial(int& serialcounter, int& packetserial) {
  int lostpackets = 0;
  if (serialcounter == -1) 
    serialcounter = packetserial - 1;
  if (serialcounter > 65000 && packetserial < 1000)
    serialcounter -= PACKET_SERIAL_MAX;
  if (serialcounter + 1 < packetserial)
    lostpackets = packetserial - (serialcounter + 1);
  else if (serialcounter >= packetserial) {
    return -1;
  }
  serialcounter = packetserial;
  return lostpackets;
}


#endif // __UNPACK_CPP__
