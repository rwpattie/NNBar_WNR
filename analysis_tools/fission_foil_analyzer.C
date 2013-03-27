#ifndef __FISSION_FOIL_C__
#define __FISSION_FOIL_C__

#include <TTree.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TFile.h>

#include "typesdef.h"

int main(int argc,char *argv[])
{
   if(argc < 2){
      cout << "Use the following input format :  fission_foil_analyzer RUN# " << endl;
   }
   TFile *f = new TFile(Form("%s/run00%dNA.root",getenv("WNR_OUTPUT_DATA"),atoi(argv[1])),"READ");
   TTree *tr = (TTree*)f->Get("t");
   Fadc_Event event;
   tr->SetBranchAddress("first_time",&event.first_time);
   tr->SetBranchAddress("board",&event.board);
   tr->SetBranchAddress("last",&event.last);
   tr->SetBranchAddress("adc",event.adc);
   tr->SetBranchAddress("max",&event.max);
   tr->SetBranchAddress("min",&event.min);
   tr->SetBranchAddress("zero",&event.zero);
   tr->SetBranchAddress("ped",&event.ped);
   tr->SetBranchAddress("channel",&channel);
   
   TH1F *hTime = new TH1F("hTime","hTime",10000,0,1000);
 
  return -1;
}

#endif
