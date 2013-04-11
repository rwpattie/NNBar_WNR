// File: FADCEvent.hh
// Name: Leah Broussard
// Date: 2013/4/11
// Purpose: Holds an FADC event
//
// Revision History:
// 2013/4/11:  LJB  file created; increased adc array size

 
#ifndef FADC_EVENT_HH__
#define FADC_EVENT_HH__


class FADCEvent
{

  public:
  FADCEvent() {}
  ~FADCEvent() {}
  ULong64_t first_time;  
  ULong64_t global_time;
  Int_t     packet_time_s;
  Int_t     packet_time_us;
  Int_t     packet_time_l;
  Int_t     board;
  Int_t     last;    //TTree requires Int_t for array index
  UShort_t  adc[10000];
  UShort_t  max;
  UShort_t  min;
  UShort_t  zero;
  UShort_t  ped;
  UShort_t  channel;
};

#endif // FADC_EVENT_HH__
