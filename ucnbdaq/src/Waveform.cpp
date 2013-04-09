// File: Waveforms.cpp
// Name: Leah Broussard
// Date: 2013/4/8
// Purpose: Unpacks and holds list of waveforms in file
//
// Revision History:
// 2013/4/8:  LJB  simple data unpacking

#ifndef WAVEFORM_CPP__
#define WAVEFORM_CPP__

#include "Waveform.hh"

/*************************************************************************/
//                            Constructor
/*************************************************************************/
Waveform::Waveform() {
  
}

/*************************************************************************/
//                              Destructor
/*************************************************************************/
Waveform::~Waveform() {
}

/*************************************************************************/
//                             UnpackWaves
/*************************************************************************/
void Waveform::UnpackWaves(output_header header, std::vector<Data_Block_t> &datablck) {
  packet_header_list.push_back(header);
  int smp = 0;
  while(smp<datablck.size()) {
    cout << "Start waveform" << endl;
    waveform_t currentwave;
    currentwave.packet_header_ptr = &packet_header_list.back();
    currentwave.linkprev = 0;
    currentwave.linknext = 0;
    currentwave.timestamp = datablck[smp].timestamp;
    int tcnt = 0;
    bool newwaveform = false;
    do {
      cout << "Timestamp is now " << datablck[smp].timestamp << endl;
      for (int i=0;i<4;i++)
	currentwave.data.push_back(datablck[smp].sample[i]);
      if (++smp<datablck.size())
	if (currentwave.timestamp + tcnt + 1 != datablck[smp].timestamp)
	  newwaveform = true;
      tcnt++;
    }while(smp<datablck.size() && !newwaveform);
  }
}



#endif // WAVEFORM_CPP__

