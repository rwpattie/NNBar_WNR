#ifndef __FISSION_FOIL_C__
#define __FISSION_FOIL_C__
// -- ROOT libraries
#include <TTree.h>
#include <TPad.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TFile.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TMath.h>
// -- c libraries
#include <stdio.h>     
#include <stdlib.h> 
#include <iostream>

// -- Analyzer includes
#include "typesdef.h"
#include "waveprocessing.h"

#define NSAMPLECUT 400
#define FITHI 650
#define FITLOW 350
#define MASSN 939.565378

using namespace std;

//---------------------------------------------------------------------------------------
// Define a few global histograms 
//
TH1F *hTAC,*hPulse;
TH2F *hNvsT;
TH1F *hADC_Sig;
TH1F *hADC_Bck;
TH1F *hRawTime,*hCorTime;
TH1F *hFCTof;
TH1F *hTAC_Energy;
TH1F *hADC_Energy;
TH1F *hDTAC_Energy;

enum ChnType {
  FFTAC  = 7,
  FFPH   = 6,
  MPGATE = 0
};
//---------------------------------------------------------------------------------------
void Analyze_TAC(Fadc_Event event);
void Analyze_ADC(Fadc_Event event);
void SetROOTOpt();
void DefineHists();
Double_t Average_TAC_Shelf(Fadc_Event eve);
void Find_Micro_Pulses();
void Rebin_Timing();
void Get_WNR_TOF();
void DrawTOF(TCanvas *c1,Int_t nrun);
void GenerateEnergySpec(TH1F* hTof, TH1F *&hEne,const char *title, const char *name);
Double_t EfromT(Double_t t,Double_t Dis);
//---------------------------------------------------------------------------------------
Int_t muPulse[400];

//=======================================================================================
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
   //-----------------------------------------------
   DefineHists();
   
   WaveProcessor *WavePr = new WaveProcessor();
   WavePr->GetThreshold(atoi(argv[1]),6);
   // Loop over the events,
   for(Int_t i = 0 ; i < tr->GetEntries() ; i++){
      tr->GetEntry(i);
      switch(event.channel){
	// Look at the Digitized TAC Pulses
	case 7:
	    Analyze_TAC(event);
	    break;
        // Look at the Analog signal..
	case 6:
	    Analyze_ADC(event);
	    WavePr->IncrimentWaveCnt();
	    WavePr->CalculatePreTrigger(event);
	    break;
	default:
	    break;
      }
   }
   //----------------------------------------------
   // Do some analysis
   Find_Micro_Pulses();
   Rebin_Timing();
   Get_WNR_TOF();
   GenerateEnergySpec(hFCTof  ,hTAC_Energy,"hTAC_Energy","Energy spectrum;Energy [MeV]; Counts");
   GenerateEnergySpec(hCorTime,hADC_Energy,"hADC_Energy","Energy spectrum;Energy [MeV]; Counts");
   GenerateEnergySpec(hTAC    ,hDTAC_Energy,"hDTAC_Energy","Energy spectrum;Energy [MeV]; Counts");
   
   TCanvas *c1 = new TCanvas("c1","c1");
   // Draw the time of flight spectra.
   DrawTOF(c1,atoi(argv[1]));
  
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
   c1->Clear();
   hTAC_Energy->Draw();
   hTAC_Energy->GetXaxis()->SetRangeUser(.1,1000);
   hTAC_Energy->GetYaxis()->SetRangeUser(0.1,40);
   
   hDTAC_Energy->Scale(17);
   hDTAC_Energy->SetLineColor(4);
   hDTAC_Energy->Draw("same e1");
   hADC_Energy->SetLineColor(2);
   hADC_Energy->SetMarkerColor(2);
   hADC_Energy->SetMarkerStyle(20);
   hADC_Energy->Scale(2);
   hADC_Energy->Draw("same e1");
   hTAC_Energy->Draw("same");
   gPad->SetLogy();
   gPad->SetLogx();
   c1->Print(Form("ff_Converted_Energy_Spectra_%d.pdf",atoi(argv[1])));
   
   std::cout << "Integral in Signal after cut : " << hPulse->Integral() << " \t" << hADC_Sig->Integral() << std::endl;
   std::cout << "Integral in TAC after cuts   : " << hTAC->Integral() << std::endl;
   
   return -1;
}
//--------------------------------------------------------------------------------------------------------------------------------
void DrawTOF(TCanvas *c1,Int_t nrun)
{
   c1->cd();
   hTAC->SetTitle("");
   hTAC->Draw("");
   hTAC->GetYaxis()->CenterTitle();
  
   Double_t scl1 = 0.4*hTAC->GetBinContent(hTAC->GetMaximumBin()) / hCorTime->GetBinContent(hCorTime->GetMaximumBin());
   Double_t scl2 = 1.4*hTAC->GetBinContent(hTAC->GetMaximumBin()) / hFCTof->GetBinContent(hFCTof->GetMaximumBin());
   hCorTime->Scale(scl1);
   hFCTof->Scale(scl2);
   
   //std::cout << "hTAC bin width ratio   " << hTAC->GetBinWidth(1) << "\t" << hCorTime->GetBinWidth(1) << "\t" << hCorTime->GetBinWidth(1) / hTAC->GetBinWidth(1) << std::endl;
   //std::cout << "hFCTof bin width ratio " << hTAC->GetBinWidth(1) << "\t" << hFCTof->GetBinWidth(1) << "\t" << hFCTof->GetBinWidth(1) / hTAC->GetBinWidth(1) << std::endl;

   hCorTime->SetLineColor(2);
   hCorTime->Draw("same e1");
   hFCTof->SetLineColor(4);
   hFCTof->Draw("same HIST");
   gPad->SetLogy();
   TH1F *hTAC_Clone     = (TH1F*)hTAC->Clone("hTAC_Clone");
   TH1F *hFCTof_Clone   = (TH1F*)hFCTof->Clone("hFCTof_Clone");
   TH1F *hCorTime_Clone = (TH1F*)hCorTime->Clone("hCorTime_Clone");
   TPad *pad1 = new TPad("pad1","pad1",0.45,0.45,0.87,0.89);
   pad1->Draw();
   pad1->cd();
   
   hTAC_Clone->SetTitle("");
   hTAC_Clone->Draw();
   hCorTime_Clone->Draw("same e1");
   hFCTof_Clone->Draw("same Hist");
   hTAC_Clone->GetYaxis()->SetRangeUser(0.1,700);
   hTAC_Clone->GetXaxis()->SetRangeUser(0,0.1);
   pad1->SetLogy();
   hTAC->Rebin(4); 
   hTAC->Scale(0.25);
   c1->Print(Form("fission_foil_TAC_%d.pdf",nrun));
   c1->Clear();
}
//--------------------------------------------------------------------------------------------------------------------------------
void GenerateEnergySpec(TH1F *hTof,TH1F *&hEne,const char *title, const char *name)
{
  std::vector<Double_t> ebins;
  std::vector<Int_t> tbins;
  Int_t nbin = 0;
  
  for(Int_t i = hTof->GetNbinsX(); i > 0 ; i--){
    Double_t dT = hTof->GetBinWidth(i); // Get the delta T
    if(hTof->GetBinCenter(i) > 0 && !(TMath::IsNaN(dT)) && !(TMath::IsNaN(EfromT(hTof->GetBinCenter(i) + dT,13.)))){
      ebins.push_back(EfromT(hTof->GetBinCenter(i) + dT,13.));
      tbins.push_back(i);
    // Double_t EE =  EfromT(hTof->GetBinCenter(i),13.);
   //   std::cout << i << "  ebins["<<nbin<<"] = " << ebins[nbin] << " dT " << dT << " \t" << hTof->GetBinCenter(i) << endl;
      nbin++;
    } else if(TMath::IsNaN(EfromT(hTof->GetBinCenter(i) + dT,13.))){
//	cout << "For unphysical energy there are " << hTof->GetBinContent(i) << " counts in the timing distribution at " << hTof->GetBinCenter(i) << "." << endl;
    }
  }
  
  hEne = new TH1F(title,name,(int)ebins.size()-1,&ebins[0]);
  
  for(Int_t i = 0 ; i < (int)tbins.size() ; i ++){
      Int_t n = tbins[i];
      Double_t cnts = hTof->GetBinContent(n)*hTof->GetBinWidth(n)/hEne->GetBinWidth(i);
      hEne->SetBinContent(i,cnts);
  }
};
//---------------------------------------------------------------------------------------------------------------------------------
void DefineHists()
{
   hTAC = new TH1F("hTAC","Fission Foil TAC; Time [#mus];Counts",1200,0,2);
   hPulse = new TH1F("hPulse","First Timestamp from the Fission Foil Wave Form;Time;Counts",112,0,1.8);
   hRawTime = new TH1F("hRawTime","RawTimestamps",50000,0,50000);
   hNvsT = new TH2F("hNvsT","Sample size vs. Pulse Height;N_{sample};Pulse Height (max(adc) - ped)",100,0,1000,100,0,4000);
   hADC_Bck = new TH1F("hADC_Bck","Bckgrd ADC;ADC (max-ped); Counts",400,0,4000);
   hADC_Sig = new TH1F("hADC_Sig","Signal ADC;ADC (max-ped); Counts",400,0,4000);
   //hTAC_Energy = new TH1F("hTAC_Energy","Energy spectrum; Energy (MeV); Counts",1000,0,1000);
   //hADC_Energy = new TH1F("hADC_Energy","Energy spectrum; Energy (MeV); Counts",100,0,1000);
}
//-----------------------------------------------------------------------------------------------------------------------
void Analyze_TAC(Fadc_Event event)
{
    if(event.last > 40) hTAC->Fill((2060.-Average_TAC_Shelf(event))*0.00051 + 0.0473);
}

void Analyze_ADC(Fadc_Event event)
{
  Double_t cyclet = 112.;
  Double_t to     = 96.;
  Int_t icycle = (Int_t)(((Double_t)event.first_time - to) / cyclet);
  hNvsT->Fill((Float_t)event.last,(Float_t)(event.max-event.ped));
  
  if(event.last < NSAMPLECUT){
    hADC_Bck->Fill((Float_t)(event.max-event.ped));
  } else {
    hADC_Sig->Fill((Float_t)(event.max-event.ped));
    hPulse->Fill((Float_t)((Float_t)event.first_time - to - (icycle*cyclet))*16e-3);
    hRawTime->Fill(event.first_time);
  }
}

void SetROOTOpt()
{
  gROOT->SetStyle("Plain");
  gStyle->SetPalette(1);
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

Double_t Average_TAC_Shelf(Fadc_Event eve)
{
  Int_t nbins = FITHI - FITLOW;
  Double_t aver = 0;
  Double_t ped  = 0;
  for(Int_t i = FITLOW ; i < FITHI ; i++)aver+=(Double_t)eve.adc[i];
  for(Int_t i = 0 ; i < 100 ;i++)ped += (Double_t)eve.adc[i]; 
  aver = aver / (Double_t)nbins;
  ped  = ped / 100.;
  return aver - ped ; 
}

void Find_Micro_Pulses()
{
  Int_t npulse = 0;

  for(Int_t i = 1 ; i < hRawTime->GetNbinsX() ; i++){
    if(hRawTime->GetBinContent(i) > 0 ){
      if(npulse == 0){
	muPulse[npulse] = hRawTime->GetBinCenter(i);
	npulse++;
      }else if(hRawTime->GetBinCenter(i) - 90 > muPulse[npulse-1]){
	muPulse[npulse] = hRawTime->GetBinCenter(i);
	npulse++;
      }
    }
  }
  //  for(int i = 0 ; i < npulse ; i++)
  //  std::cout << "Pulse i : " << i << " at  t = " << muPulse[i]*16e-3 << std::endl;
};

void Rebin_Timing()
{
  hCorTime = new TH1F("hCorTime","Corrected Timing;TOF [#mus];Counts",200,0,2.0);
  Int_t npulse = 0;
  for(Int_t i = 1 ; i < hRawTime->GetNbinsX() ; i++){
    if(hRawTime->GetBinContent(i) > 0){
      Double_t rawtime = hRawTime->GetBinCenter(i);
      if(rawtime > muPulse[npulse] && rawtime < muPulse[npulse+1]){
	Double_t cortime = (rawtime - muPulse[npulse])*16e-3;
	for(Int_t j = 0 ; j < hRawTime->GetBinContent(i);j++)
	  hCorTime->Fill(cortime + 0.0433, 1.);
      } else if(rawtime > muPulse[npulse] && rawtime > muPulse[npulse+1]){
	npulse++;
	Double_t cortime = (rawtime - muPulse[npulse])*16e-3;
	for(Int_t j = 0 ; j < hRawTime->GetBinContent(i);j++)
	  hCorTime->Fill(cortime + 0.0433, 1.);
      }
    }
  }
}

void Get_WNR_TOF()
{
    TFile *fWNR = new TFile("FCtof.root","READ");
    TH1F *TOF = (TH1F*)fWNR->Get("TOF");
    hFCTof = new TH1F("hFCTof","WNR TAC;Time [#mus];Counts"
                      ,TOF->GetNbinsX(),TOF->GetXaxis()->GetXmin()/1e3
                      ,TOF->GetXaxis()->GetXmax()/1e3);
    
    for(Int_t i = 0 ; i < TOF->GetNbinsX() ; i++){
      if(TOF->GetBinContent(i) > 0){
	  for(Int_t j = 0 ; j < TOF->GetBinContent(i) ; j++){
	      hFCTof->Fill((TOF->GetBinCenter(i)+43.3)/1e3,1.);
	  }
      }
    }
}

Double_t EfromT(Double_t t,Double_t Dis)
{
    t *= 1e-6;
    Double_t mn = MASSN*1e6;
    Double_t a = TMath::Power(Dis/(TMath::C()*t),2);
    Double_t Er = (1./sqrt(1-a) - 1)*mn;
    //cout << Er << "\t" << a << endl;
    return Er*1e-6;
}


#endif
