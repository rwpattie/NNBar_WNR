#ifndef __FISSION_FOIL_C__
#define __FISSION_FOIL_C__
// -- ROOT libraries
#include <TTree.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TFile.h>
// -- c libraries
#include <stdio.h>     
#include <stdlib.h> 
#include <iostream>

#include "typesdef.h"

using namespace std;

enum ChnType {
  FFTAC = 0,
  FFPH  = 1,
  MPGATE = 7
};

int main(int argc,char *argv[])
{
   if(argc < 2){
      cout << "Use the following input format :  fission_foil_analyzer RUN# " << endl;
      return 2;
   }
   
   TFile *f = new TFile(Form("%s/run00%dNA.root",getenv("WNR_OUTPUT_DATA"),
			     atoi(argv[1])),"READ");
   if(!(f->IsOpen())){
     cout << "file failed to open " << endl;
     return 5;
   }
   
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
   tr->SetBranchAddress("channel",&event.channel);
   
   TH1F *hTAC = new TH1F("hTAC","Fission Foil TAC;ADC[chl];Counts",4096,0,4096);
   TH1F *hPulse = new TH1F("hPulse","First Timestamp from the Fission Foil Wave Form;Time;Counts",1500,0,1500);
   
   for(Int_t i = 0 ; i < tr->GetEntries() ; i++){
      tr->GetEntry(i);
      if(event.channel == FFTAC)hTAC->Fill((Float_t)(event.max-event.ped));
      if(event.channel == FFPH)hPulse->Fill((Float_t)(event.first_time)*16.e-3);
   }
   TCanvas *c1 = new TCanvas("c1","c1");
   c1->Divide(2,1);
   c1->cd(1);
   hTAC->Draw();
   c1->cd(2);
   hPulse->Draw();
   c1->Print("fission_foil.pdf");
   return -1;
}

#endif
