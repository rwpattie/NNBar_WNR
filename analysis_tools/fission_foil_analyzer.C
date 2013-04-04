#ifndef __FISSION_FOIL_C__
#define __FISSION_FOIL_C__
// -- ROOT libraries
#include <TTree.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TFile.h>
#include <TROOT.h>
#include <TStyle.h>
// -- c libraries
#include <stdio.h>     
#include <stdlib.h> 
#include <iostream>

#include "typesdef.h"

#define NSAMPLECUT 400

using namespace std;

TH1F *hTAC,*hPulse;
TH2F *hNvsT;
TH1F *hADC_Sig;
TH1F *hADC_Bck;

enum ChnType {
  FFTAC  = 7,
  FFPH   = 6,
  MPGATE = 0
};

void Analyze_TAC(Fadc_Event event);
void Analyze_ADC(Fadc_Event event);
void SetROOTOpt();
void DefineHists();

int main(int argc,char *argv[])
{
   if(argc < 2){
      cout << "Use the following input format :  fission_foil_analyzer RUN# " << endl;
      return 2;
   }
   
   SetROOTOpt();
   
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
   
   DefineHists();
  
   for(Int_t i = 0 ; i < tr->GetEntries() ; i++){
      tr->GetEntry(i);
 
      switch(event.channel){
	case 7:
	    Analyze_TAC(event);
	    break;
	case 6:
	    Analyze_ADC(event);
	    break;
	default:
	    break;
      }
   }
   
   TCanvas *c1 = new TCanvas("c1","c1");
   hTAC->Draw();
   gPad->SetLogy();
   c1->Print(Form("fission_foil_TAC_%d.pdf",atoi(argv[1])));
   c1->Clear();
   hPulse->Draw();
   c1->Print(Form("fission_foil_first_time_%d.pdf",atoi(argv[1])));
   c1->Clear();
   gPad->SetLogy(0);
   hNvsT->Draw("colz");
   c1->Print(Form("ff_ADC_vs_Last_%d.pdf",atoi(argv[1])));
   c1->Clear();
   hADC_Bck->SetLineColor(2);
   hADC_Bck->Draw();
   hADC_Sig->SetLineColor(4);
   hADC_Sig->Draw("same");
   c1->Print(Form("ff_ADC_sig_bck_%d.pdf",atoi(argv[1])));
   
   return -1;
}

void DefineHists()
{
   hTAC = new TH1F("hTAC","Fission Foil TAC;ADC[chl];Counts",410,0,4100);
   hPulse = new TH1F("hPulse","First Timestamp from the Fission Foil Wave Form;Time;Counts",200,0,2);
   hNvsT = new TH2F("hNvsT","Sample size vs. Pulse Height;N_{sample};Pulse Height (max(adc) - ped)",100,0,1000,100,0,4000);
   hADC_Bck = new TH1F("hADC_Bck","Bckgrd ADC;ADC (max-ped); Counts",400,0,4000);
   hADC_Sig = new TH1F("hADC_Sig","Signal ADC;ADC (max-ped); Counts",400,0,4000);
   
}

void Analyze_TAC(Fadc_Event event)
{
    if(event.last > 40) hTAC->Fill((float)(event.max-event.ped));
}

void Analyze_ADC(Fadc_Event event)
{
    hPulse->Fill((Float_t)(event.first_time)*16e-3);
    hNvsT->Fill((Float_t)event.last,(Float_t)(event.max-event.ped));
    if(event.last < NSAMPLECUT){
      hADC_Bck->Fill((Float_t)(event.max-event.ped));
    } else {
      hADC_Sig->Fill((Float_t)(event.max-event.ped));
    }
}

void SetROOTOpt()
{
  gROOT->SetStyle("Plain");
  gStyle->SetPalette(1);
//  TH1::SetDefaultSumw2(kTRUE);
  gStyle->SetOptStat(kFALSE); 
  gStyle->SetPadLeftMargin(0.15);
  gROOT->ForceStyle();
  gStyle->SetTextSize(1.01);
  gStyle->SetLabelSize(0.055,"xy");
  gStyle->SetTitleSize(0.06,"xy");
  gStyle->SetTitleOffset(1.2,"x");
  gStyle->SetTitleOffset(1.2,"y");
  gStyle->SetPadTopMargin(0.1);
  gStyle->SetPadRightMargin(0.12);
  gStyle->SetPadBottomMargin(0.16);
}

#endif
