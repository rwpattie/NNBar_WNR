#ifndef __WAVE_FORM_MATH__
#define __WAVE_FORM_MATH__

#include "wave_form_math.h"


Int_t Find_Peaks(std::vector<Double_t> val,std::vector<Double_t>der,
		 std::vector<Double_t>& pkv,std::vector<Int_t>& pkt,
		 Double_t thres)
{
  Bool_t bthrsh = kTRUE;
  Double_t max = 0;
  Int_t pktime = 0;
  for(UInt_t i = 0 ; i < val.size() ; i++){
    if(val[i] > thres && bthrsh){
      bthrsh = kFALSE;
      max = val[i];
    } else if( val[i] > thres && !bthrsh){
      if(val[i] > max) {
	max =  val[i]; 
	pktime = (int)i;
      }
    } else if( val[i] < thres && !bthrsh){
      bthrsh = kTRUE;
      pkv.push_back(val[pktime]);
      pkt.push_back(pktime);
    }
  }
  std::cout<< " I found " << (Int_t)pkv.size() << std::endl;
  return (Int_t)pkv.size();
}
 
void SmoothArray(std::vector<Double_t> t1,std::vector<Double_t>f1,
		 std::vector<Double_t>& t2,std::vector<Double_t>& f2,
		 Int_t smwidth)
{
  Int_t offset = smwidth / 2;
  for(Int_t i = offset ; i < (int)f1.size() - offset ; i++){
      t2.push_back(t1[i]);
      Double_t fsum = 0;
      for(int j = 0 ; j < smwidth ; j++){
	fsum += f1[i+j-offset];
      }
      f2.push_back(fsum/(double)smwidth);
  }
  
}

Double_t DerivArray(std::vector<Double_t> val,std::vector<Double_t>& der)
{
    //very simple 2 point derivative. probably very wrong but a first step....
    Double_t dt = 1.;
    Int_t trgp = 0;
    Int_t maxp = 0;
    for(Int_t i = 1 ; i < (int)val.size() ; i++){
      der.push_back((val[i+1] - val[i-1])/(2.*dt));
      if(der[i-1] > 5 && trgp == 0 )trgp = i;
      if(trgp > 0  && der[i-1] < 0 && maxp == 0)maxp = i;
    }
    Double_t t0 = SolveLine((double)(trgp-2),der[trgp-2],(double)(trgp+2),der[trgp+2],5.);
    Double_t t1 = SolveLine((double)(maxp-2),der[maxp-2],(double)(maxp+2),der[maxp+2],0.);
    return (double)(t1 - t0)*4.;
}

Double_t SolveLine(Double_t x1,Double_t y1,Double_t x2,Double_t y2,Double_t yp)
{
    Double_t m = (y2-y1)/(x2-x1);
    //y = mx + b ... (y-b)/m = x ...  m = (yp - y1)/(xp - x1) ...  xp = x1 + (yp-y1)/m
    Double_t xp = x1 + (yp-y1)/m;
    return xp;
}

Double_t FindTriggerTime(std::vector<Double_t> val,std::vector<Double_t> time,Double_t thrs)
{
    Int_t ntrg=0;
    while(val[ntrg] < thrs && ntrg < (int)val.size())
      ntrg++;
    
    if(ntrg == (int)val.size()){
      thrs*=0.9;
      ntrg=0;
      while(val[ntrg] < thrs && ntrg < (int)val.size())
      ntrg++;
    }
    
    Double_t tTrig = SolveLine(time[ntrg-2],val[ntrg-2],time[ntrg+2],val[ntrg+2],thrs);
    return tTrig;
}

Double_t FindWidth(std::vector<Double_t> val,std::vector<Double_t> time,Double_t thrs)
{
    Int_t t0 = 0;  // first trigger crossing
    Int_t t1 = 0;  // second trigger crossing
    
    for(UInt_t i = 0 ; i < val.size() ; i++){
	if(val[i] > thrs && t0 == 0) t0 = i;
	if(val[i] < thrs && t0 > 0 && t1 == 0) t1 = i;
    }
    if(t0 == 0){
        thrs*=0.9;
	for(UInt_t i = 0 ; i < val.size() ; i++){
	  if(val[i] > thrs && t0 == 0)t0 = i;
	  if(val[i] < thrs && t0 > 0 && t1 == 0) t1 = i;
	}
    }
     
    Double_t tt0 = SolveLine(time[t0-2],val[t0-2],time[t0+2],val[t0+2],thrs);
    Double_t tt1 = SolveLine(time[t1-2],val[t1-2],time[t1+2],val[t1+2],thrs);

    return (tt1-tt0);
}

#endif