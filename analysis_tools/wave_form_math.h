#ifndef __WAVE_FORM_MATH_HH__
#define __WAVE_FORM_MATH_HH__

#include "stdlib.h"
#include <TROOT.h>
#include "stdio.h"
#include <iostream>

void SmoothArray(std::vector<Double_t> t1,std::vector<Double_t>f1,
		 std::vector<Double_t>& t2,std::vector<Double_t>& f2,
		 Int_t smwidth);
Double_t DerivArray(std::vector<Double_t> val,std::vector<Double_t>& der);
Double_t SolveLine(Double_t x1,Double_t y1,Double_t x2,Double_t y2,Double_t yp);
Double_t FindTriggerTime(std::vector<Double_t> val,std::vector<Double_t> time,
			 Double_t thrs);
Double_t FindWidth(std::vector<Double_t> val,std::vector<Double_t> time,
		   Double_t thrs);
Int_t Find_Peaks(std::vector<Double_t> val,std::vector<Double_t>der,
		 std::vector<Double_t>& pkv,std::vector<Int_t>& pkt,Double_t thresh);

#endif