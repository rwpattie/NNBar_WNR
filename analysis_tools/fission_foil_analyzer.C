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
#include "fission_detector.h"

using namespace std;

//---------------------------------------------------------------------------------------
// Define a few global histograms 
//
TH1F *hMicroPulseSeg;
TH2F *hGasTubeADCvsTOF;
TH1F *hGasTube_Energy;
TH2F *hGasTubeMaxvsLast;
TH1F *hMicroPulse;
TH1F *hGasTubeTOF;
TH1F *hGasTubeADC;
TH1F *hGasTubeRise;
TH1F *hGasTubeMulti;
//--------------------------------------------------------------------------------------
enum ChnType {
  MPGATE = 0,
  GASTUBE= 1,
  FFTAC  = 7,
  FFPH   = 6
};
std::vector<Coin_t> CoinTimes (8); 
//---------------------------------------------------------------------------------------
void FillGasTube(Fadc_Event event,Int_t toff,Int_t nWght,Int_t *peak);
void ScaleVarBinHisto(TH1F *hin);
void SetROOTOpt();
void DrawCanvas(TCanvas *c,TH1F *h,Int_t run,char *title);
void DefineHists();
void DrawTOF(TCanvas *c1,Int_t nrun);
Double_t EfromT(Double_t t,Double_t Dis);
void DrawEnergySpec(TCanvas *c,Int_t nrun);
void AddEvent(Coin_t &Co,Long64_t time);
Bool_t Check_Coincidence(Int_t n1,Double_t trgtime,TTree *t,Int_t nevent);
//---------------------------------------------------------------------------------------
Int_t lastPulse = 0;
Long64_t RunTime = 0;
Double_t n_micro_pulses = 0.;
Fadc_Event event;
//=======================================================================================
int main(int argc,char *argv[])
{
   // parse input string
   if(argc < 2){
      cout << "Use the following input format :  fission_foil_analyzer RUN# " << endl;
      return 2;
   }
   // Setup some simple ROOT plotting options.
   SetROOTOpt();
   // Open the root file... 
   TFile *f = new TFile(Form("%s/run00%dNA.root",getenv("WNR_OUTPUT_DATA"),
			     atoi(argv[1])),"READ");
   if(!(f->IsOpen())){
     cout << "file failed to open " << endl;
     return 5;
   }
   // Get the tree and set the branch address.
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
   // Define some global histograms.
   DefineHists();
   
   WaveProcessor *WavePr = new WaveProcessor();
   FissionChamber *FcChm = new FissionChamber(FFTAC,FFPH);
   for(Int_t i = 0 ; i < 8 ; i++)
     WavePr->GetThreshold(atoi(argv[1]),i);
   
   // Loop over the events,
   for(Int_t i = 0 ; i < tr->GetEntries() ; i++){
      tr->GetEntry(i);
      Int_t pretr    = 0;
      Int_t risetime = 0;
      if(i == tr->GetEntries()-1)RunTime = event.packet_time_l;
      AddEvent(CoinTimes[event.channel],event.first_time);
      if( event.channel == MPGATE ){
	// Look at the Digitized TAC Pulse
	    if( event.last < 40 ){
	      hMicroPulseSeg->Fill((event.first_time+pretr+risetime)*16e-3);
	    }else{
	      pretr = WavePr->CalculatePreTrigger(event,MPGATE);
	      risetime = WavePr->TimeToPeak(event,pretr);
	      hMicroPulse->Fill((event.first_time+pretr+risetime)*16e-3);
	      lastPulse = event.first_time+pretr+risetime;
	    }
      } else if( event.channel == GASTUBE ){
	    pretr = WavePr->CalculatePreTrigger(event,GASTUBE);
	    risetime = WavePr->TimeToPeak(event,pretr);
	    if( event.last > 100 ){
	      hGasTubeRise->Fill(risetime);
	      Int_t peaks[20];
	      Int_t nWght = WavePr->EventWght(event,peaks);
	      FillGasTube(event,pretr+risetime,nWght,peaks);
	      hGasTubeMaxvsLast->Fill((Int_t)event.last,event.max-event.ped);
	    }
      } else if( event.channel == FFTAC ){
	    //if(Check_Coincidence( FFPH , (Double_t)(event.first_time*16e-3), tr ,i)){
		//  std::cout<< "measured coincidence with micropulse" << std::endl;
		 // tr->GetEntry(i);
		  FcChm->Analyze_TAC(event);
	    //}
      } else if( event.channel == FFPH ){  
        // Look at the Analog signal..
	    FcChm->Analyze_ADC(event);
	    WavePr->IncrimentWaveCnt();
	    pretr    = WavePr->CalculatePreTrigger(event,FFPH);
	    risetime = WavePr->TimeToPeak(event,pretr);
	    FcChm->hRiseTimeVsADC->Fill(risetime*16e-3,event.max-event.ped);
      }
   }
   //----------------------------------------------
   // Do some analysis
   n_micro_pulses = RunTime*(625./1.8)*43.;
   FcChm->AnalyzeHistograms();

   
   TCanvas *c1 = new TCanvas("c1","c1");
//    Draw the time of flight spectra.
//    DrawTOF(c1,atoi(argv[1]));
//   
//    hPulse->Draw();
//    c1->Print(Form("fission_foil_first_time_%d.pdf",atoi(argv[1])));
//    c1->Clear();
//    gPad->SetLogy(0);
//    hNvsT->Draw("colz");
//    c1->Print(Form("ff_ADC_vs_Last_%d.pdf",atoi(argv[1])));
//    c1->Clear();
//    hADC_Bck->SetLineColor(2);
//    hADC_Bck->Draw();
//    hADC_Sig->SetLineColor(4);
//    hADC_Sig->Draw("same");
//    c1->Print(Form("ff_ADC_sig_bck_%d.pdf",atoi(argv[1])));
//    c1->Clear();
//    
//    DrawEnergySpec(c1,atoi(argv[1]));
//  
//    gPad->SetLogx(0);
//    gPad->SetLogy(0);
//    hRiseTimeVsADC->Draw();
//    c1->Print(Form("ff_risetime_vs_adc_%d.pdf",atoi(argv[1])));
// 
//    c1->Clear();
//    
//    hMicroPulse->Draw();
//    hMicroPulse->GetXaxis()->SetRangeUser(0,650);
//    hMicroPulseSeg->SetLineColor(2);
//    hMicroPulseSeg->Draw("same");
//    TLegend *lMicro = new TLegend(0.5,0.7,0.85,0.85);
//    lMicro->AddEntry(hMicroPulse,"Fully Digitized Pulses","l");
//    lMicro->AddEntry(hMicroPulseSeg,"Pulse Segments","l");
//    lMicro->SetFillColor(0);
//    lMicro->Draw();
//    
//    c1->Print(Form("micro_pulse_%d.pdf",atoi(argv[1])));
   c1->Clear();
   hGasTubeMaxvsLast->Draw("colz");
   c1->Print(Form("gastubelastvsmax_%d.pdf",atoi(argv[1])));
   
   c1->Clear();
   hGasTubeADCvsTOF->Draw("colz");
   c1->Print(Form("gastube_adc_vs_t_%d.pdf",atoi(argv[1])));
   c1->Clear();   
   hGasTubeRise->Draw();
   c1->Print(Form("gastube_risetime_%d.pdf",atoi(argv[1])));
   
   DrawCanvas(c1,hGasTubeMulti,atoi(argv[1]),"gastube_multi");
   DrawCanvas(c1,hGasTubeADC,atoi(argv[1]),"gastube_adc");
   
   std::cout << "Integral in Signal after cut : " << FcChm->hPulse->Integral() << " \t" << FcChm->hADC_Fission->Integral() << std::endl;
   std::cout << "Integral in TAC after cuts   : " << FcChm->hTAC->Integral() << std::endl;
   std::cout << "Run time : " << RunTime << std::endl;
   std::cout << "Total MicroPulses : " << RunTime*(625./1.8)*40 << std::endl;
   std::cout << "Total Neutron Flux : " << FcChm->hADC_Energy->Integral() << std::endl;
   
   return -1;
}

void DrawCanvas(TCanvas *c,TH1F *h,Int_t run,char *title)
{
   c->Clear();
   h->Draw();
   c->Print(Form("%s_%d.pdf",title,run)); 
}

Bool_t Check_Coincidence(Int_t n1,Double_t trgtime,TTree *t,Int_t nevent)
{
  Double_t t1,t2;
  Double_t dt = 2.0;
  // if there aren't enough events don't look for coincidence.
  // Although we should look ahead ....
  if( nevent < 2 )return kFALSE;
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

void FillGasTube(Fadc_Event eve,Int_t toff,Int_t nWght,Int_t *peak)
{
 
  Double_t tof = 0;
 
   if(eve.first_time < 39060){
     tof = ((eve.first_time-61)%112)*16e-3;
   } else {
     tof = (eve.first_time-39060)*16e-3;
   }
   
   hGasTubeTOF->Fill(tof,nWght);
   hGasTubeADCvsTOF->Fill(tof,eve.max-eve.ped);
   for(Int_t i = 0 ; i < nWght ; i++){
      hGasTubeADC->Fill(peak[i]);
   }
   hGasTubeMulti->Fill(nWght);
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

//    hTAC_Energy->SetLineColor(4);
//    hTAC_Energy->SetMarkerColor(4);
//    hTAC_Energy->Draw("e1");
//    hTAC_Energy->GetXaxis()->SetRangeUser(1,1000);
//    gEner->Draw("p");
//    hTAC_Energy->GetYaxis()->SetRangeUser(1e5,7e8);
//    hDTAC_Energy->SetLineColor(1);
//    hDTAC_Energy->SetMarkerStyle(24);
//    hDTAC_Energy->SetMarkerColor(1); 
//    hDTAC_Energy->Draw("same e1 x0");
//    hADC_Energy->SetLineColor(2);
//    hADC_Energy->SetMarkerColor(2);
//    hADC_Energy->SetMarkerStyle(20);
//    hADC_Energy->SetMarkerSize(0.8);
//    
//    hADC_Energy->Draw("same e1 x0");   
//    
//    //hGasTube_Energy->SetLineColor(3);
//    //hGasTube_Energy->Draw("same");
//    
//    gPad->SetLogy();
//    gPad->SetLogx();
//    
//    TLegend leg(0.5,0.6,0.86,0.86);
//    leg.SetFillColor(0);
//    leg.AddEntry(hTAC_Energy,"WNR TAC","lp");
//    leg.AddEntry(hDTAC_Energy,"fADC TAC","lp");
//    leg.AddEntry(hADC_Energy,"Analog fADC","lp");
//    leg.Draw();
   
   c1->Print(Form("ff_Converted_Energy_Spectra_%d.pdf",nrun));
   c1->Clear();
  
}


//-------------------------------------------------------------------------------------------
void DrawTOF(TCanvas *c1,Int_t nrun)
{
   c1->cd();
   
  /* 
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
   pad1->SetLogy();*/
   c1->Print(Form("fission_foil_TAC_%d.pdf",nrun));
   c1->Clear();
   
}
//-------------------------------------------------------------------------------------------
void DefineHists()
{

   hMicroPulse = new TH1F("hMicroPulse","#muPulse Structure; Time [#mus];Counts",1000,0,1000);
   hMicroPulseSeg = new TH1F("hMicroPulseSeg","#muPulse Seg Structure; Time [#mus];Counts",1000,0,1000);
   
   hGasTubeADC = new TH1F("hGasTubeADC","Gas Tube ADC;ADC;Counts",4000,0,4000);
   hGasTubeTOF = new TH1F("hGasTubeTOF","Gas Tube ADC;ADC;Counts",112,0,1.8);
   hGasTubeMaxvsLast = new TH2F("hGasTubeMaxvsLast","Gas Tube",100,0,1000,100,0,4000);
   hGasTubeADCvsTOF  = new TH2F("hGasTubeADCvsTOF","Gas tube ADC vs. TOF;ADC:TOF",100,0,2,100,0,4000);
   hGasTubeRise = new TH1F("hGasTubeRise","Gas Tube Rise Time",100,0,100);
   
   hGasTubeMulti = new TH1F("hGasTubeMulti","Gas Tube Multiplicity; N; Counts",20,0,20);
   
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

Double_t EfromT(Double_t t,Double_t Dis)
{
    t *= 1e-6;
   
    Double_t mn = MASSN*1e6;
    Double_t a = TMath::Power(Dis/(TMath::C()*t),2);
    Double_t Er = (1./sqrt(1-a) - 1)*mn;
   
    return Er*1e-6;
}
//=====================================================================================

#endif
