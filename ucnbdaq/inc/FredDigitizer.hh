// File: FredDigitizer.hh
// Name: Leah Broussard
// Date: 2013/4/5
// Purpose: Global objects pertaining to the digitizers
//
// Revision History:
// 2013/4/5:  LJB  file created
// 2013/4/10: LJB  added NUMCH global

#ifndef FRED_DIGITIZER_HH__
#define FRED_DIGITIZER_HH__

#include <math.h>

#include "Rtypes.h"
#define RAWDATA_LENGTH 2048
#define PACKET_SERIAL_MAX 65535
#define NUMBOARDS 3
#define NUMCH 8

extern const ULong64_t cycleoffset;
extern const UShort_t boardnum[NUMBOARDS];

struct output_header{
  Int_t board_number;
  Int_t packet_serial;
  Int_t fadc_number;
  Int_t data_size;
  Int_t tv_usec;
  Int_t tv_sec;
  Int_t admin_message;
  Int_t buffer_number;
};

struct Data_Block_t {
  Long64_t timestamp;
  Bool_t overflowB0;  
  Bool_t overflowA0;  
  Bool_t overflowB1;  
  Bool_t overflowA1;   
  Int_t sample[4];     
};




#endif
