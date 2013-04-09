// File: Waveforms.hh
// Name: Leah Broussard
// Date: 2013/4/8
// Purpose: Unpacks and holds list of waveforms in file
//
// Revision History:
// 2013/4/8:  LJB  simple data unpacking
 
#ifndef WAVEFORM_HH__
#define WAVEFORM_HH__

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>

#include "FredDigitizer.hh"

using std::cout;
using std::endl;

class Waveform
{

private:

  std::vector<output_header> packet_header_list;
  struct waveform_t{
    output_header* packet_header_ptr;
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

};

#endif // WAVEFORM_HH__
