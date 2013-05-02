#ifndef __FISSION_FOIL_C__
#define __FISSION_FOIL_C__
//--------------------------------------------------------------------------------
// Author : R. W. Pattie Jr.
// Date   : Started March,2013
//
// Purpose : Analyze the fission chamber TAC and TOF data from the WNR run.
//           This has been extended to include the analysis of other detectors
//           attached to the DAQ in parallel with the fission chamber.
//--------------------------------------------------------------------------------

// -- ROOT libraries
#include <TTree.h>
#include <TPad.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TFile.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TMath.h>
#include <TLegend.h>
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
#define NCALIB 9.91599e8

using namespace std;

//---------------------------------------------------------------------------------------
// Define a few global histograms 
//
TH1F *hTAC,*hPulse;
TH2F *hNvsT,*hGasTubeMaxvsLast;
TH1F *hMicroPulseSeg;
TH2F *hGasTubeADCvsTOF;
TH2F *hRiseTimeVsADC;
TH1F *hADC_Sig;
TH1F *hADC_Bck;
TH1F *hRawTime,*hCorTime;
TH1F *hFCTof;
TH1F *hTAC_Energy;
TH1F *hADC_Energy;
TH1F *hDTAC_Energy;
TH1F *hGasTube_Energy;
TH1F *hMicroPulse;
TH1F *hGasTubeTOF;
TH1F *hGasTubeADC;
TH1F *hGasTubeRise;
//--------------------------------------------------------------------------------------
enum ChnType {
  MPGATE = 0,
  GASTUBE= 1,
  FFTAC  = 7,
  FFPH   = 6
};
std::vector<Coin_t> CoinTimes (8); 
//---------------------------------------------------------------------------------------
void Analyze_TAC(Fadc_Event event);
void Analyze_ADC(Fadc_Event event);
void FillGasTube(Fadc_Event event,Int_t toff);
void ScaleVarBinHisto(TH1F *hin);
void SetROOTOpt();
void DefineHists();
Double_t Average_TAC_Shelf(Fadc_Event eve);
void Find_Micro_Pulses();
void Rebin_Timing();
void Get_WNR_TOF();
void DrawTOF(TCanvas *c1,Int_t nrun);
void GenerateEnergySpec(TH1F* hTof, TH1F *&hEne,const char *title, const char *name,Bool_t CALIB);
Double_t EfromT(Double_t t,Double_t Dis);
void CorrectFissionCrossSection(TH1F *hin,Bool_t CAILB);
void DrawEnergySpec(TCanvas *c,Int_t nrun);
void AddEvent(Coin_t &Co,Long64_t time);
Bool_t Check_Coincidence(Int_t n1,Double_t trgtime,TTree *t,Int_t nevent);
//---------------------------------------------------------------------------------------
Int_t muPulse[400];
Int_t lastPulse = 0;
Long64_t RunTime = 0;
Double_t n_micro_pulses = 0.;
Fadc_Event event;
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
   
   tr->SetBranchAddress("first_time",&event.first_time);
   tr->SetBranchAddress("packet_time_s",&event.packet_time_s);
   tr->SetBranchAddress("packet_time_us",&event.packet_time_us);
   tr->SetBranchAddress("packet_time_l",&event.packet_time_l);
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
   for(Int_t i = 0 ; i < 8 ; i++)WavePr->GetThreshold(atoi(argv[1]),i);
   
   // Loop over the events,
   for(Int_t i = 0 ; i < tr->GetEntries() ; i++){
      tr->GetEntry(i);
      Int_t pretr    = 0;
      Int_t risetime = 0;
      if(i == tr->GetEntries()-1)RunTime = event.packet_time_l;
      AddEvent(CoinTimes[event.channel],event.first_time);
      switch(event.channel){
	// Look at the Digitized TAC Pulses
	case MPGATE:
	    if(event.last < 40 ){
	      hMicroPulseSeg->Fill((event.first_time+pretr+risetime)*16e-3);
	    }else{
	      pretr = WavePr->CalculatePreTrigger(event,MPGATE);
	      risetime = WavePr->TimeToPeak(event,pretr);
	      hMicroPulse->Fill((event.first_time+pretr+risetime)*16e-3);
	      lastPulse = event.first_time+pretr+risetime;
	    }
	    break;
	case GASTUBE:
	    pretr = WavePr->CalculatePreTrigger(event,GASTUBE);
	    risetime = WavePr->TimeToPeak(event,pretr);
	    if(event.last > 100){
	      hGasTubeRise->Fill(risetime);
	      FillGasTube(event,pretr+risetime);
	      hGasTubeMaxvsLast->Fill((Int_t)event.last,event.max-event.ped);
	    }
	    break;
	case FFTAC:
	    
	    if(Check_Coincidence( FFPH , (Double_t)(event.first_time*16e-3), tr ,i)){
		  std::cout<< "measured coincidence with micropulse" << std::endl;
		  tr->GetEntry(i);
		  Analyze_TAC(event);
	    }
	    break;
        // Look at the Analog signal..
	case FFPH:
	    Analyze_ADC(event);
	    WavePr->IncrimentWaveCnt();
	    pretr    = WavePr->CalculatePreTrigger(event,FFPH);
	    risetime = WavePr->TimeToPeak(event,pretr);
	    hRiseTimeVsADC->Fill(risetime*16e-3,event.max-event.ped);
	    break;
	default:
	    break;
      }
   }
   //----------------------------------------------
   // Do some analysis
   n_micro_pulses = RunTime*(625./1.8)*43.;
   Find_Micro_Pulses();
   Rebin_Timing();
   Get_WNR_TOF();
   
   GenerateEnergySpec(hFCTof     ,hTAC_Energy,"hTAC_Energy","Energy spectrum;Energy [MeV]; n/(#muP sr MeV)",kTRUE);
   GenerateEnergySpec(hCorTime   ,hADC_Energy,"hADC_Energy","Energy spectrum;Energy [MeV]; Counts",kFALSE);
   GenerateEnergySpec(hTAC       ,hDTAC_Energy,"hDTAC_Energy","Energy spectrum;Energy [MeV]; Counts",kFALSE);
   GenerateEnergySpec(hGasTubeTOF,hGasTube_Energy,"hGasTube_Energy","Energy spectrum;Energy [MeV]; Counts",kFALSE);
   
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
   
   DrawEnergySpec(c1,atoi(argv[1]));
 
   gPad->SetLogx(0);
   gPad->SetLogy(0);
   hRiseTimeVsADC->Draw();
   c1->Print(Form("ff_risetime_vs_adc_%d.pdf",atoi(argv[1])));

   c1->Clear();
   
   hMicroPulse->Draw();
   hMicroPulse->GetXaxis()->SetRangeUser(0,650);
   hMicroPulseSeg->SetLineColor(2);
   hMicroPulseSeg->Draw("same");
   TLegend *lMicro = new TLegend(0.5,0.7,0.85,0.85);
   lMicro->AddEntry(hMicroPulse,"Fully Digitized Pulses","l");
   lMicro->AddEntry(hMicroPulseSeg,"Pulse Segments","l");
   lMicro->SetFillColor(0);
   lMicro->Draw();
   
   c1->Print(Form("micro_pulse_%d.pdf",atoi(argv[1])));
   c1->Clear();
   hGasTubeMaxvsLast->Draw("colz");
   c1->Print(Form("gastubelastvsmax_%d.pdf",atoi(argv[1])));
   
   c1->Clear();
   hGasTubeADCvsTOF->Draw("colz");
   c1->Print(Form("gastube_adc_vs_t_%d.pdf",atoi(argv[1])));
   c1->Clear();
   
   hGasTubeRise->Draw();
   c1->Print(Form("gastube_risetime_%d.pdf",atoi(argv[1])));
   
   std::cout << "Integral in Signal after cut : " << hPulse->Integral() << " \t" << hADC_Sig->Integral() << std::endl;
   std::cout << "Integral in TAC after cuts   : " << hTAC->Integral() << std::endl;
   std::cout << "Run time : " << RunTime << std::endl;
   std::cout << "Total MicroPulses : " << RunTime*(625./1.8)*40 << std::endl;
   std::cout << "Total Neutron Flux : " << hADC_Energy->Integral() << std::endl;
   
   return -1;
}

Bool_t Check_Coincidence(Int_t n1,Double_t trgtime,TTree *t,Int_t nevent)
{
  Double_t t1,t2;
  Double_t dt = 2.0;
  // if there aren't enough events don't look for coincidence.
  // Although we should look ahead ....
  if(nevent <2)return kFALSE;
  //-----------------------------------------------------------------------
  Int_t e1 = nevent; // Set the event counts to the current event.
  Int_t e2 = nevent;
  // loop back to find the closest preceding events in channel n1.
  do{
      e1--;
      t->GetEntry(e1);
      t1 = event.first_time*16e-3;
  }while(event.channel != n1 && e1 > 0);
  // loop forward to find the closest event in channel n1, after the current event.
  do{
     e2++;
     t->GetEntry(e2);
     t2 = event.first_time*16e-3;
  }while(event.channel != n1 && e2 < t->GetEntries());
  // Take the difference of the current trigger time to the 
  // time stamps of surrounding events.
  Double_t diff1 = TMath::Abs(trgtime-t1);
  Double_t diff2 = TMath::Abs(trgtime-t2);
  // if the times are correlated to below 0.5 us then 
  // we define this as a coincidence.  This should be replaced by an 
  // adjustable parameter.
  if(diff1 < dt || diff2 < dt){
    // load the closer event as the coincidence event
    if(diff1 < diff2){
      t->GetEntry(e1);
    }else{
      t->GetEntry(e2);
    }
    // once loaded cut on some criteria, this should be a switch statement
    // allowing the user to define some channel or detector relevant cuts.
    if(event.last > 400)
      return kTRUE;
    else 
      return kFALSE;
  } else {
    // if the timing difference is dt > 0.5 return false.
    return kFALSE;
  }
}
//-------------------------------------------------------------------------------------
void AddEvent(Coin_t &Co,Long64_t time)
{
  
  if((int)Co.times.size() < 100)
    Co.times.push_back((Double_t)(time*16e-3));
  else {
    for(Int_t i = 0 ; i < 99 ; i++)Co.times[i] = Co.times[i+1];
      Co.times[99] = (Double_t)(time*16e-3);
  }
}

void FillGasTube(Fadc_Event eve,Int_t toff)
{
 
  Double_t tof = 0;
  /*
  if( eve.first_time + toff - lastPulse > 115){
      lastPulse = eve.first_time+toff;
      tof = 51.0e-3;
  } else {
      tof = ((eve.first_time+toff) - lastPulse)*16e-3 + 43.3e-3;
  }*/
  
 
   if(eve.first_time < 39060){
     tof = ((eve.first_time-61)%112)*16e-3;
   } else {
     tof = (eve.first_time-39060)*16e-3;
   }
   
   hGasTubeTOF->Fill(tof);
   hGasTubeADCvsTOF->Fill(tof,eve.max-eve.ped);
}

void ScaleVarBinHisto(TH1F *hin)
{
    for(Int_t i = 0; i < hin->GetNbinsX();i++)
      hin->SetBinContent(i,hin->GetBinContent(i)/hin->GetBinWidth(i));
}

void DrawEnergySpec(TCanvas *c1,Int_t nrun)
{

   TGraph *gEner = new TGraph("wnr_neutron_flux.txt"," %*lg %*lg %*lg %lg %lg %*lg %*lg %*lg");
   Double_t x,y;
   for(Int_t i = 0 ; i < gEner->GetN() ; i ++){
     gEner->GetPoint(i,x,y);
     gEner->SetPoint(i,x,y/NCALIB);
   }
   
   gEner->SetLineColor(4);
   gEner->SetMarkerColor(4);
   gEner->SetMarkerStyle(20);

   hTAC_Energy->SetLineColor(4);
   hTAC_Energy->SetMarkerColor(4);
   hTAC_Energy->Draw("e1");
   hTAC_Energy->GetXaxis()->SetRangeUser(1,1000);
   gEner->Draw("p");
   hTAC_Energy->GetYaxis()->SetRangeUser(1e5,7e8);
   hDTAC_Energy->SetLineColor(1);
   hDTAC_Energy->SetMarkerStyle(24);
   hDTAC_Energy->SetMarkerColor(1); 
   hDTAC_Energy->Draw("same e1 x0");
   hADC_Energy->SetLineColor(2);
   hADC_Energy->SetMarkerColor(2);
   hADC_Energy->SetMarkerStyle(20);
   hADC_Energy->SetMarkerSize(0.8);
   
   hADC_Energy->Draw("same e1 x0");   
   
   //hGasTube_Energy->SetLineColor(3);
   //hGasTube_Energy->Draw("same");
   
   gPad->SetLogy();
   gPad->SetLogx();
   
   TLegend leg(0.5,0.6,0.86,0.86);
   leg.SetFillColor(0);
   leg.AddEntry(hTAC_Energy,"WNR TAC","lp");
   leg.AddEntry(hDTAC_Energy,"fADC TAC","lp");
   leg.AddEntry(hADC_Energy,"Analog fADC","lp");
   leg.Draw();
   
   c1->Print(Form("ff_Converted_Energy_Spectra_%d.pdf",nrun));
   c1->Clear();
  
}


//-------------------------------------------------------------------------------------------
void DrawTOF(TCanvas *c1,Int_t nrun)
{
   c1->cd();
   
   
   std::cout << "counts in fission foil : " << hTAC->Integral() / n_micro_pulses << std::endl;
   std::cout << "counts in Drift Tubes  : " << hGasTubeTOF->Integral() / n_micro_pulses << std::endl;
   
   Double_t scl1 = 1./(n_micro_pulses*hCorTime->GetBinWidth(1)); //0.4*hTAC->GetBinContent(hTAC->GetMaximumBin()) / hCorTime->GetBinContent(hCorTime->GetMaximumBin());
   Double_t scl4 = 1./(n_micro_pulses*hTAC->GetBinWidth(1));
   hTAC->Scale(scl4);
   Double_t scl2 = (hFCTof->GetBinWidth(1)/hFCTof->Integral())*(hTAC->Integral());//1.2*hTAC->GetBinContent(hTAC->GetMaximumBin()) / hFCTof->GetBinContent(hFCTof->GetMaximumBin());
   Double_t scl3 = 1./(n_micro_pulses*hGasTubeTOF->GetBinWidth(1));//0.4*hTAC->GetBinContent(hTAC->GetMaximumBin()) / hGasTubeTOF->GetBinContent(hGasTubeTOF->GetMaximumBin());
   
   hCorTime->Scale(scl1);
   hFCTof->Scale(scl2);
   hGasTubeTOF->Scale(scl3);
   
   hTAC->SetTitle("");
   hTAC->Draw("");
   hTAC->GetYaxis()->CenterTitle();
   
   std::cout << "hTAC bin width ratio   " << hTAC->GetBinWidth(1) << "\t" << hCorTime->GetBinWidth(1) << "\t" << hCorTime->GetBinWidth(1) / hTAC->GetBinWidth(1) << std::endl;
   std::cout << "hFCTof bin width ratio " << hTAC->GetBinWidth(1) << "\t" << hFCTof->GetBinWidth(1) << "\t" << hFCTof->GetBinWidth(1) / hTAC->GetBinWidth(1) << std::endl;

   hCorTime->SetLineColor(2);
   hCorTime->SetMarkerColor(2);
   hCorTime->SetMarkerStyle(20);
   hCorTime->Draw("same P");
   hFCTof->SetLineColor(4);
   hFCTof->Draw("same HIST");
   hGasTubeTOF->SetLineColor(3);
   hGasTubeTOF->Draw("same hist");
   hTAC->GetYaxis()->SetRangeUser(1e-5,10);
   gPad->SetLogy();
   TH1F *hTAC_Clone     = (TH1F*)hTAC->Clone("hTAC_Clone");
   TH1F *hFCTof_Clone   = (TH1F*)hFCTof->Clone("hFCTof_Clone");
   TH1F *hCorTime_Clone = (TH1F*)hCorTime->Clone("hCorTime_Clone");
   
   TPad *pad1 = new TPad("pad1","pad1",0.45,0.45,0.87,0.89);
   pad1->Draw();
   pad1->cd();
   
   hTAC_Clone->SetTitle("");
   hTAC_Clone->Draw();
   hCorTime_Clone->Draw("same p");
   hFCTof_Clone->Draw("same Hist");
   //hTAC_Clone->GetYaxis()->SetRangeUser(0.1,700);
   hTAC_Clone->GetXaxis()->SetRangeUser(0,0.1);
   pad1->SetLogy();
   c1->Print(Form("fission_foil_TAC_%d.pdf",nrun));
   c1->Clear();
   
}
//--------------------------------------------------------------------------------------------------------------------------------
void GenerateEnergySpec(TH1F *hTof,TH1F *&hEne,const char *title, const char *name,Bool_t CALIB)
{
  std::vector<Double_t> ebins;
  std::vector<Int_t> tbins;
  Int_t nbin = 0;
  
  for(Int_t i = hTof->GetNbinsX(); i > 0 ; i--){
    Double_t dT = hTof->GetBinWidth(i); // Get the delta T
    if(hTof->GetBinCenter(i) > 0 && !(TMath::IsNaN(dT)) && 
      !(TMath::IsNaN(EfromT(hTof->GetBinCenter(i) + dT,13.)))){
      ebins.push_back(EfromT(hTof->GetBinCenter(i) + dT,13.));
      tbins.push_back(i);
      nbin++;
      } else if(TMath::IsNaN(EfromT(hTof->GetBinCenter(i) + dT,13.))){
    }
  }
  
  hEne = new TH1F(title,name,(int)ebins.size()-1,&ebins[0]);
  
  for(Int_t i = 0 ; i < (int)tbins.size() ; i ++){
      Int_t n = tbins[i];
      Double_t cnts = hTof->GetBinContent(n)*hTof->GetBinWidth(n)/hEne->GetBinWidth(i);
      hEne->SetBinContent(i,cnts);
      hEne->SetBinError(i,hTof->GetBinError(n)*hTof->GetBinWidth(n)/hEne->GetBinWidth(i));
  }
  
  CorrectFissionCrossSection(hEne,CALIB);
};
//-------------------------------------------------------------------------------------------
void DefineHists()
{
   hTAC = new TH1F("hTAC","Fission Foil TAC; Time [#mus];Counts",120,0,2);
   hPulse = new TH1F("hPulse","First Timestamp from the Fission Foil Wave Form;Time;Counts",112,0,1.8);
   
   hRawTime = new TH1F("hRawTime","RawTimestamps",50000,0,50000);
   hNvsT = new TH2F("hNvsT","Sample size vs. Pulse Height;N_{sample};Pulse Height (max(adc) - ped)",100,0,1000,100,0,4000);
   hADC_Bck = new TH1F("hADC_Bck","Bckgrd ADC;ADC (max-ped); Counts",400,0,4000);
   hADC_Sig = new TH1F("hADC_Sig","Signal ADC;ADC (max-ped); Counts",400,0,4000);
   hRiseTimeVsADC = new TH2F("hRiseTimeVsADC","Rise Time vs. Peak Heigh;"
			     "Rise Time [ns];Peak Height",100,0,2,100,0,4000);
   hMicroPulse = new TH1F("hMicroPulse","#muPulse Structure; Time [#mus];Counts",1000,0,1000);
   hMicroPulseSeg = new TH1F("hMicroPulseSeg","#muPulse Seg Structure; Time [#mus];Counts",1000,0,1000);
   
   hGasTubeADC = new TH1F("hGasTubeADC","Gas Tube ADC;ADC;Counts",4000,0,4000);
   hGasTubeTOF = new TH1F("hGasTubeTOF","Gas Tube ADC;ADC;Counts",112,0,1.8);
   hGasTubeMaxvsLast = new TH2F("hGasTubeMaxvsLast","Gas Tube",100,0,1000,100,0,4000);
   hGasTubeADCvsTOF  = new TH2F("hGasTubeADCvsTOF","Gas tube ADC vs. TOF;ADC:TOF",100,0,2,100,0,4000);
   hGasTubeRise = new TH1F("hGasTubeRise","Gas Tube Rise Time",100,0,100);
   //hTAC_Energy = new TH1F("hTAC_Energy","Energy spectrum; Energy (MeV); Counts",1000,0,1000);
   //hADC_Energy = new TH1F("hADC_Energy","Energy spectrum; Energy (MeV); Counts",100,0,1000);
}
//------------------------------------------------------------------------------------------
void Analyze_TAC(Fadc_Event event)
{
    if(event.last > 40) hTAC->Fill((2060.-Average_TAC_Shelf(event))*0.00051 + 0.0523);
}
//==========================================================================================
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
//=====================================================================================
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
//  TH1::SetDefaultSumw2(kTRUE);
  gStyle->SetPadRightMargin(0.12);
  gStyle->SetPadBottomMargin(0.16);
}
//=======================================================================================
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
//=======================================================================================
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
}
//=====================================================================================
void Rebin_Timing()
{
  
  hCorTime = new TH1F("hCorTime","Corrected Timing;TOF [#mus];Counts",200,0,2.0);
  Int_t npulse = 0;
  for(Int_t i = 1 ; i < hRawTime->GetNbinsX() ; i++){
    if(hRawTime->GetBinContent(i) > 0){
      Double_t rawtime = hRawTime->GetBinCenter(i);
      if(rawtime > muPulse[npulse] && rawtime < muPulse[npulse+1]){
	Double_t cortime = (rawtime  -  muPulse[npulse] )*16e-3;
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
//=====================================================================================
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
//====================================================================================
Double_t EfromT(Double_t t,Double_t Dis)
{
    t *= 1e-6;
   
    Double_t mn = MASSN*1e6;
    Double_t a = TMath::Power(Dis/(TMath::C()*t),2);
    Double_t Er = (1./sqrt(1-a) - 1)*mn;
   
    return Er*1e-6;
}
//=====================================================================================
void CorrectFissionCrossSection(TH1F *hin,Bool_t CALIB)
{
  
  TGraph *gU235 = new TGraph("u235_totalfission.txt","%lg %lg %*lg");
  TGraph *gU238 = new TGraph("u238_totalfission.txt","%lg %lg %*lg");
  
  Double_t domega = 3.42565e-6; // based on email from nik fotiadis sr 
  Double_t eff    = 0.98;       // efficiency based on wenders paper 
  Double_t rho    = 0.2;        // found in a report about areal densities of U N/cm^2
  
  for(Int_t i = 1 ; i <= hin->GetNbinsX() ; i++){
      // assume the counts are coming from both the 238U and 235U foils, so average the cross-sections
      Double_t sigma = (gU238->Eval(hin->GetBinCenter(i)*1e6) + 
                        gU238->Eval(hin->GetBinCenter(i)*1e6))/2.;
      
      Double_t sclr = CALIB ? sigma*domega*eff*rho/*NCALIB*/ : sigma*domega*eff*rho*0.55;//*n_micro_pulses;
      
      if(sigma > 0 ){
	  hin->SetBinContent(i,hin->GetBinContent(i)/(sclr));
	  hin->SetBinError(i,hin->GetBinError(i)/(sclr));
      }
  }
  delete gU235;
  delete gU238;
}

#endif
