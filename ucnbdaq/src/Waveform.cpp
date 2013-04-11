// File: Waveforms.cpp
// Name: Leah Broussard
// Date: 2013/4/8
// Purpose: Unpacks and holds list of waveforms in file
//
// Revision History:
// 2013/4/8:  LJB  simple data unpacking
// 2013/4/10: LJB  packet_header_ptr is lost on vector resize; use index
//                 instead; fixed waveform vector push; added linking
//                 linking routine and Get functions
// 2013/4/11: LJB  links waveforms with small time gaps

#ifndef WAVEFORM_CPP__
#define WAVEFORM_CPP__

#include "Waveform.hh"

/*************************************************************************/
//                            Constructor
/*************************************************************************/
Waveform::Waveform() {
  linked = false;
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
    waveform_t currentwave;
    currentwave.packet_header_idx = packet_header_list.size() - 1;
    currentwave.linkprev = 0;
    currentwave.linknext = 0;
    currentwave.timestamp = datablck[smp].timestamp;
    int tcnt = 0;
    bool newwaveform = false;
    do {
      for (int i=0;i<4;i++)
	currentwave.data.push_back(datablck[smp].sample[i]);
      if (++smp<datablck.size())
	if (currentwave.timestamp + tcnt + 1 != datablck[smp].timestamp)
	  newwaveform = true;
      tcnt++;
    }while(smp<datablck.size() && !newwaveform);
    waveform_list.push_back(currentwave);
  }
}

/*************************************************************************/
//                             LinkWaves
/*************************************************************************/
void Waveform::LinkWaves() {
  int lastindex[NUMCH];
  int timestamp[NUMCH];
  for (int c=0;c<NUMCH;c++) {
    timestamp[c] = -1;
  }
  for (int i=0;i<waveform_list.size();i++) {
    int idx = waveform_list[i].packet_header_idx;
    int ch = packet_header_list[idx].fadc_number;
    if (timestamp[ch] == -1) {
      timestamp[ch] = waveform_list[i].timestamp;
      lastindex[ch] = i;
    }
    else {
      int diff = (waveform_list[i].timestamp > timestamp[ch]) ?
	waveform_list[i].timestamp - timestamp[ch] : 
	timestamp[ch] + cycleoffset - waveform_list[i].timestamp;
      if (diff*4 - waveform_list[lastindex[ch]].data.size() < 1000) { 
	waveform_list[i].linkprev = &waveform_list[lastindex[ch]];
	waveform_list[lastindex[ch]].linknext = &waveform_list[i];
      }
      lastindex[ch] = i;
      timestamp[ch] = waveform_list[i].timestamp;      
    }
  }
  linked = true;
}

/*************************************************************************/
//                             GetWaveform
/*************************************************************************/
bool Waveform::GetWaveform(int i, std::vector<Int_t>& dest){
  if (!linked)
    LinkWaves();
  if (waveform_list[i].linkprev != 0)
    return false;
  dest = waveform_list[i].data;
  struct waveform_t* waveptr = &waveform_list[i];
  while (waveptr->linknext != 0) {
    Long64_t timestamp = waveptr->timestamp;
    int size = waveptr->data.size();
    waveptr = waveptr->linknext;
    int diff = (waveptr->timestamp > timestamp) ?
      waveptr->timestamp - timestamp : 
      timestamp + cycleoffset - waveptr->timestamp;
    int numzeros = diff*4 - size;
    if (numzeros > 0) {
      dest.insert(dest.end(),numzeros,0);
    }
    dest.insert(dest.end(),waveptr->data.begin(),waveptr->data.end());
  }
  return true;
}

/*************************************************************************/
//                              GetHeader
/*************************************************************************/
void Waveform::GetHeader(int i, output_header& header) {
  int idx = waveform_list[i].packet_header_idx;
  header.board_number = packet_header_list[idx].board_number;
  header.packet_serial = packet_header_list[idx].packet_serial;
  header.fadc_number = packet_header_list[idx].fadc_number;
  header.data_size = packet_header_list[idx].data_size;
  header.tv_usec = packet_header_list[idx].tv_usec;
  header.tv_sec = packet_header_list[idx].tv_sec;
  header.admin_message = packet_header_list[idx].admin_message;
  header.buffer_number = packet_header_list[idx].buffer_number;
}

#endif // WAVEFORM_CPP__

