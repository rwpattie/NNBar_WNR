#ifndef __TYPES_H__
#define __TYPES_H__

struct Fadc_Event {
  ULong64_t first_time;
  Int_t board;
  UShort_t last;
  UShort_t adc[5000];
  UShort_t max;
  UShort_t min;
  UShort_t zero;
  UShort_t ped;
  UShort_t channel;
};

#endif