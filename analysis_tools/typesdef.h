#ifndef __TYPES_H__
#define __TYPES_H__

struct Wave_Averages_t{
  Double_t RiseTime;
  Double_t Length;
  Double_t DeltaT;
  Double_t Width;
  Double_t Pedestal;
};

struct Fadc_Event {
  ULong64_t first_time;
  ULong64_t global_time;
  Int_t     packet_time_s;
  Int_t     packet_time_us;
  Int_t     packet_time_l;
  Int_t     board;
  UShort_t  last;
  UShort_t  adc[5000];
  UShort_t  max;
  UShort_t  min;
  UShort_t  zero;
  UShort_t  ped;
  UShort_t  channel;
};

struct Chan_t{
    Int_t uthresh; // upper channel threshold
    Int_t lthresh; // lower channel threshold
    Int_t pre;     // pretrigger samples
    Int_t post;   // posttrigger samples
};

struct Coin_t{
  std::vector<Double_t> times;
};

#endif