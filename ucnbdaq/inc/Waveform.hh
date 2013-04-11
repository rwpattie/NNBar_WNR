// File: Waveforms.hh
// Name: Leah Broussard
// Date: 2013/4/8
// Purpose: Unpacks and holds list of waveforms in file
//
// Revision History:
// 2013/4/8:  LJB  simple data unpacking
// 2013/4/10  LJB  vector ptr replaced with index, added Get/Link functions
 
#ifndef WAVEFORM_HH__
#define WAVEFORM_HH__

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>

#include "FredDigitizer.hh"

using std::cout;
using std::endl;
using std::vector;

class Waveform
{

private:

  bool linked;
  vector<output_header> packet_header_list;
  struct waveform_t{
    int packet_header_idx;
    struct waveform_t* linkprev;
    struct waveform_t* linknext;
    Long64_t timestamp;
    std::vector<Int_t> data;
  };
  std::vector<waveform_t> waveform_list;

public:

  Waveform();
  ~Waveform();
  void UnpackWaves(output_header header, std::vector<Data_Block_t> &datablck);
  void LinkWaves();
  bool GetWaveform(int i, std::vector<Int_t>& dest);
  void GetData(int i, std::vector<Int_t>& dest) {dest = waveform_list[i].data;}
  void GetHeader(int i, output_header& header);
  inline Long64_t GetTimestamp(int i) {return waveform_list[i].timestamp;}
  inline int GetSize() {return waveform_list.size();}
};

#endif // WAVEFORM_HH__
