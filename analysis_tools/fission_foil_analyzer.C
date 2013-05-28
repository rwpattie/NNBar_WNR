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
#include "gastube_detector.h"

using namespace std;

//---------------------------------------------------------------------------------------
// Define a few global histograms 
//
TH1F *hMicroPulseSeg;
TH1F *hMicroPulse;
TH1F *hGasTubeEff;
TH1F *hGasTubeEffSp;

//--------------------------------------------------------------------------------------
enum ChnType {
  MPGATE   = 0,
  SPECTRA  = 1,
  GASTUBE  = 2,
  GASTUBE3 = 3,
  GASTUBE4 = 4,
  GASTUBE5 = 5,
  PMT      = 6,
  FFTAC    = 7,
  FFPH     = 8
};
std::vector<Coin_t> CoinTimes (8); 
//---------------------------------------------------------------------------------------
void FillGasTube(Fadc_Event event,Int_t toff,Int_t nWght,Int_t *peak);
void ScaleVarBinHisto(TH1F *hin);
void SetROOTOpt();
void DrawCanvas(TCanvas *c,TH1F *h,Int_t run,char *title,Bool_t DRAWLOG=kFALSE,
                     Bool_t DRAWXLOG=kFALSE,Bool_t CUST=kFALSE);
void DrawCanvasMulti(TCanvas *c,std::vector<TH1F*> h,Int_t run,char *title,Bool_t DRAWLOG=kFALSE,Bool_t DRAWXLOG=kFALSE,Bool_t CUST=kFALSE);
void DefineHists();
void DrawTOF(TCanvas *c1,Int_t nrun);
Double_t EfromT(Double_t t,Double_t Dis);
void DrawEnergySpec(TCanvas *c,Int_t nrun);
void AddEvent(Coin_t &Co,Long64_t time);
Double_t TOF(Long64_t ts);
Bool_t Check_Coincidence(Int_t n1,Double_t trgtime,TTree *t,Int_t nevent);
int percentDone(int per,int curline,int totline);
void GenerateEnergySpec(TH1F *hTof,TH1F *&hEne,const char *title, 
					const char *name,Bool_t CALIB);
void CreateEffic(TH1F *hN,TH1F *hD,TH1F *&hOut,const char *title,const char *name);
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
   
   Int_t nnrun = atoi(argv[1]);
   
   // Setup some simple ROOT plotting options.
   SetROOTOpt();
   // Open the root file... 
   TFile *f = new TFile(Form("%s/run00%dNA.root",getenv("WNR_OUTPUT_DATA"),
			     nnrun),"READ");
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
   GasTube *GsSp         = new GasTube(1,1);
   GasTube *GsTb         = new GasTube(2,5);
   
   FcChm->GenerateDeadTimeHisto(tr);
   GsTb->GenerateDeadTimeHisto(tr);
   GsSp->GenerateDeadTimeHisto(tr);
   
   for(Int_t i = 0 ; i < 8 ; i++)WavePr->GetThreshold(nnrun,i);
 
   percentDone(-1,0,tr->GetEntries());
   Int_t nextp = 0;
   
   // Loop over the events,
   for(Int_t i = 0 ; i < tr->GetEntries() ; i++){
      tr->GetEntry(i);
      Int_t perc = (Int_t)(((Double_t)i/(Double_t)tr->GetEntries())*100.);
      if(perc == nextp) nextp= percentDone(perc,i,tr->GetEntries());
      Int_t pretr    = 0;
      Int_t risetime = 0;
      if(i == tr->GetEntries()-1)RunTime = event.packet_time_l;
      AddEvent(CoinTimes[event.channel],event.first_time);
      if( event.channel == MPGATE ){
	// Look at the Digitized TAC Pulse
	    if( event.last < 40 ){
	      hMicroPulseSeg->Fill((event.first_time+pretr+risetime)*DTIME);
	    }else{
	      pretr = WavePr->CalculatePreTrigger(event,MPGATE);
	      risetime = WavePr->TimeToPeak(event,pretr);
	      hMicroPulse->Fill((event.first_time+pretr+risetime)*DTIME);
	      lastPulse = event.first_time+pretr+risetime;
	    }
      } else if( event.channel == GASTUBE3 ){
	    pretr    = WavePr->CalculatePreTrigger(event,event.channel);
	    risetime = WavePr->TimeToPeak(event,pretr);
	    if( event.last > 100){
	      Int_t peaks[20];
	      Int_t nWght = WavePr->EventWght(event,peaks);
	      Bool_t COIN = Check_Coincidence(6,(Double_t)(event.first_time*DTIME), tr ,i);
	      tr->GetEntry(i);
              GsTb->FillGasTube(event,pretr+risetime,nWght,peaks,risetime,COIN);
	    }
      } else if(event.channel == SPECTRA ){
	    pretr    = WavePr->CalculatePreTrigger(event,event.channel);
	    risetime = WavePr->TimeToPeak(event,pretr);
	    if( event.last > 100){
	      Int_t peaks[20];
	      Int_t nWght = WavePr->EventWght(event,peaks);
	      Bool_t COIN = Check_Coincidence(6,(Double_t)(event.first_time*DTIME), tr ,i);
	      tr->GetEntry(i);
              GsSp->FillGasTube(event,pretr+risetime,nWght,peaks,risetime,COIN);
	    }
      } else if( event.channel == FFTAC ){
	    //if(Check_Coincidence( FFPH , (Double_t)(event.first_time*DTIME), tr ,i)){
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
	    FcChm->hRiseTimeVsADC->Fill(risetime*DTIME,event.max-event.ped);
      }
   }
   //----------------------------------------------
   // Do some analysis
   n_micro_pulses = RunTime*(625./1.8)*43.;
   FcChm->AnalyzeHistograms();
   FcChm->Diagnostic();
   
   TCanvas *c1 = new TCanvas("c1","c1");
//    Draw the time of flight spectra.
//    DrawTOF(c1,nnrun);
//   
//    hPulse->Draw();
//    c1->Print(Form("fission_foil_first_time_%d.pdf",nnrun));
//    c1->Clear();
//    gPad->SetLogy(0);
//    hNvsT->Draw("colz");
//    c1->Print(Form("ff_ADC_vs_Last_%d.pdf",nnrun));
//    c1->Clear();
//    hADC_Bck->SetLineColor(2);
//    hADC_Bck->Draw();
//    hADC_Sig->SetLineColor(4);
//    hADC_Sig->Draw("same");
//    c1->Print(Form("ff_ADC_sig_bck_%d.pdf",nnrun));
//    c1->Clear();
//    
//    DrawEnergySpec(c1,nnrun);
//  
//    gPad->SetLogx(0);
//    gPad->SetLogy(0);
//    hRiseTimeVsADC->Draw();
//    c1->Print(Form("ff_risetime_vs_adc_%d.pdf",nnrun));
// 
    c1->Clear();
//    
   hMicroPulse->Draw();
   hMicroPulse->GetXaxis()->SetRangeUser(0,650);
   hMicroPulseSeg->SetLineColor(2);
   hMicroPulseSeg->Draw("same");
   TLegend *lMicro = new TLegend(0.5,0.7,0.85,0.85);
   lMicro->AddEntry(hMicroPulse,"Fully Digitized Pulses","l");
   lMicro->AddEntry(hMicroPulseSeg,"Pulse Segments","l");
   lMicro->SetFillColor(0);
   lMicro->Draw();
   c1->Print(Form("micro_pulse_%d.pdf",nnrun));
   
   c1->Clear();
   GsTb->hGasTubeMaxvsLast->Draw("colz");
   c1->Print(Form("gastubelastvsmax_%d.pdf",nnrun));
   
   c1->Clear();
   GsTb->hGasTubeADCvsTOF->Draw("colz");
   c1->Print(Form("gastube_adc_vs_t_%d.pdf",nnrun));
   c1->Clear();   
   
   DrawCanvas(c1,FcChm->hTAC,nnrun,"fchamber_cortime",kTRUE);
   DrawCanvas(c1,GsTb->hGasTubeRise,nnrun,"gastube_risetime");
   DrawCanvas(c1,GsTb->hGasTubeMulti,nnrun,"gastube_multi");
   DrawCanvas(c1,GsTb->hGasTubeADC,nnrun,"gastube_adc",kTRUE);
   DrawCanvas(c1,GsTb->hGasTubeTOF,nnrun,"gastube_TOF",kTRUE);
   DrawCanvas(c1,GsTb->hGasTube_PlasticTOF,nnrun,"gastube_plast_TOF",kTRUE);
   DrawCanvas(c1,GsTb->hGasTubeNoCoinTOF,nnrun,"gastube_noplast_TOF",kTRUE);
   DrawCanvas(c1,GsTb->hFullTime,nnrun,"gastube_deadtime_cor",kTRUE);
   
   std::vector<TH1F*> hists;
   hists.push_back(FcChm->hTAC);
   hists.push_back(GsTb->hGasTubeTOF);
   hists.push_back(GsTb->hGasTubeTOFSc);
   DrawCanvasMulti(c1,hists,nnrun,"gastube_TOFs",kTRUE);
   hists.clear();
   
   GsTb->GenerateEnergySpec(GsTb->hGasTubeTOFSc,GsTb->hGasTube_Energy,"hGasTube_Energy","Energy Spectrum",kFALSE);
   GsSp->GenerateEnergySpec(GsSp->hGasTubeTOFSc,GsSp->hGasTube_Energy,"hGasTube_Energy_Sp","Energy Spectrum",kFALSE);

   hists.push_back(GsTb->hGasTube_Energy);
   hists.push_back(GsSp->hGasTube_Energy);
   hists.push_back(FcChm->hTAC_RAW_Energy);
   hists.push_back(FcChm->hTAC_Energy);
   DrawCanvasMulti(c1,hists,nnrun,"gastube_energy",kTRUE,kTRUE);
   hists.clear();
   
  
   CreateEffic(GsTb->hGasTube_Energy,FcChm->hTAC_Energy,hGasTubeEff,"hGasTubeEff","Ar/Ethane Efficiency");
   CreateEffic(GsSp->hGasTube_Energy,FcChm->hTAC_Energy,hGasTubeEffSp,"hGasTubeEffSp","Spectra Gas Efficiency");
   
   hists.push_back(hGasTubeEff);
   hists.push_back(hGasTubeEffSp);
   
   DrawCanvasMulti(c1,hists,nnrun,"gastube_eff",kTRUE,kTRUE,kTRUE);
   
   std::cout << "Integral in Signal after cut : " << FcChm->hPulse->Integral() << " \t" << FcChm->hADC_Fission->Integral() << std::endl;
   std::cout << "Integral in TAC after cuts   : " << FcChm->hTAC->Integral() << std::endl;
   std::cout << "Run time : " << RunTime << std::endl;
   std::cout << "Total MicroPulses : " << RunTime*(625./1.8)*40 << std::endl;
   std::cout << "Total Neutron Flux : " << FcChm->hDTAC_Energy->Integral() << std::endl;
   std::cout << "Finished with the Analysis ************************************ " << std::endl;
   return -1;
}

void DrawCanvas(TCanvas *c,TH1F *h,Int_t run,char *title,Bool_t DRAWLOG,Bool_t DRAWXLOG,Bool_t CUST)
{
   if(h->Integral() == 0)return;
   c->Clear();
   
   if(DRAWLOG)
     gPad->SetLogy();
   else
     gPad->SetLogy(0);
   
   if(DRAWXLOG)
     gPad->SetLogx();
   else
     gPad->SetLogx(0);
   if(CUST)h->GetYaxis()->SetRangeUser(1e-6,1e-3); 
   h->Draw();
   c->Print(Form("%s_%d.pdf",title,run)); 
   c->Print(Form("%s_%d.gif",title,run)); 
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
      t1 = event.first_time*DTIME;
  }while(event.channel != n1 && e1 > 0);
  // loop forward to find the closest event in channel n1, after the current event.
  do{
     e2++;
     t->GetEntry(e2);
     t2 = event.first_time*DTIME;
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
    if(event.last > 20){
     // hGasTube_PlasticTOF->Fill(TOF(event.first_time));
      return kTRUE;
    }else 
      return kFALSE;
    } else {
    // if the timing difference is dt > 0.5 return false.
    return kFALSE;
  }
}
//-------------------------------------------------------------------------------------
Double_t TOF(Long64_t ts)
{
  
  Double_t tof = ((Double_t)((ts-63)%112))*16.e-3;
  
  return tof;
}
//-------------------------------------------------------------------------------------
void AddEvent(Coin_t &Co,Long64_t time)
{
  
  if((int)Co.times.size() < 100)
    Co.times.push_back((Double_t)(time*DTIME));
  else {
    for(Int_t i = 0 ; i < 99 ; i++)Co.times[i] = Co.times[i+1];
      Co.times[99] = (Double_t)(time*DTIME);
  }
}

void FillGasTube(Fadc_Event eve,Int_t toff,Int_t nWght,Int_t *peak)
{
/* 
  Double_t tof = 0;//TOF(eve.first_time);
 
   if(eve.first_time < 39060){
     tof = ((eve.first_time-63)%112)*DTIME;//+ toff*DTIME;
   } else {
     tof = (eve.first_time-39060)*DTIME;// + toff*DTIME;
   }
//    
   hGasTubeTOF->Fill(tof);
   hGasTubeADCvsTOF->Fill(tof,eve.max-eve.ped);
   //for(Int_t i = 0 ; i < nWght ; i++){
      hGasTubeADC->Fill(peak[0]);
   //}
   hGasTubeMulti->Fill(nWght);*/
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
   
  // c1->Print(Form("ff_Converted_Energy_Spectra_%d.pdf",nrun));
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
   //c1->Print(Form("fission_foil_TAC_%d.pdf",nrun));
   c1->Clear();
   
}
//-------------------------------------------------------------------------------------------
void DefineHists()
{

   hMicroPulse = new TH1F("hMicroPulse","#muPulse Structure; Time [#mus];Counts",1000,0,1000);
   hMicroPulseSeg = new TH1F("hMicroPulseSeg","#muPulse Seg Structure; Time [#mus];Counts",1000,0,1000);
  
}

//=====================================================================================

void DrawCanvasMulti(TCanvas *c,std::vector<TH1F*> h,Int_t run,char *title,Bool_t DRAWLOG,Bool_t DRAWXLOG,Bool_t CUST)
{
  
  c->Clear();
   if(DRAWLOG)
     gPad->SetLogy(1);
   else 
     gPad->SetLogy(0);
   
   if(DRAWXLOG)
     gPad->SetLogx(1);
   else
     gPad->SetLogx(0);
   
   h[0]->Draw();
   
   Double_t max = h[0]->GetBinContent(h[0]->GetMaximumBin());
   
   if((int)h.size() > 1){
     for(Int_t i = 1 ; i < (int)h.size() ; i++){
       if(h[i]->GetBinContent(h[i]->GetMaximumBin()) > max)
	 max = h[i]->GetBinContent(h[i]->GetMaximumBin());
       h[i]->SetLineColor(i+1);
       h[i]->SetMarkerColor(i+1);
       h[i]->Draw("same");
     }
   }
   
   if(CUST){
    h[0]->GetYaxis()->SetRangeUser(1e-6,1.e-2);
    for(Int_t i = 0 ; i < (int)h.size() ; i++)h[i]->SetMarkerStyle(20);
   }else
    h[0]->GetYaxis()->SetRangeUser(1,1.1*max);
   
   TLegend *l = new TLegend(0.6,0.2,0.85,0.45);
   for(Int_t i = 0 ; i < (int)h.size() ; i++){
      l->AddEntry(h[i],h[i]->GetTitle(),"l");
   }
  // l->Draw();
  c->Print(Form("%s_%d.pdf",title,run)); 
  c->Print(Form("%s_%d.gif",title,run)); 
  delete l;
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

void GenerateEnergySpec(TH1F *hTof,TH1F *&hEne,const char *title, 
					const char *name,Bool_t CALIB)
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
  
//  CorrectFissionCrossSection(hEne,CALIB);
};

//--------------------------------------------------------------------------------------------------
int percentDone(int per,int curline,int totline)
{
  if((per % 1 == 0 && per >0)||(per==-1)){
    cout << "\r [";
    if(per == -1)per = 0;
    int nstars = (int)((float)per/1.);
    for(int j = 0 ; j <= nstars ; j++)cout << "*";
    for(int j = nstars+1; j < 100 ; j++)cout << "-";
    cout <<"] \t" << 100 * (double)curline/(double)totline << " \% of " << curline << " / " << totline ;
  }
  fflush(stdout);
  per++;
  return per;
}
//-------------------------------------------------------------------------------------------------


void CreateEffic(TH1F *hN,TH1F *hD,TH1F *&hOut,const char *title, 
					const char *name)
{
  
  hOut = (TH1F*)hN->Clone(title);
  hOut->SetTitle(Form("%s ; Energy (MeV) ; Efficiency",name));
  hOut->Clear();
  //for(Int_t i = 0 ; i <= hOut->GetNbinsX() ; i++)hOut->SetBinContent(i,0);
  
  for(Int_t i = 1 ; i <= hN->GetNbinsX() ; i++){
     Double_t fbin = hN->GetBinCenter(i);
     Int_t    ibin = hD->FindFixBin(fbin);
     Double_t eff  = ( hD->GetBinContent(ibin) > 0 ) ? (hN->GetBinContent(i) / hD->GetBinContent(ibin)) : 1.0;
     cout << "bin " << i << " energy " << hOut->GetBinCenter(i) << "\t" << eff << "\t" << ibin << "\t" << fbin <<"\t" << hD->GetBinContent(ibin) << "\t" << hD->GetBinCenter(ibin) << "\t" << hN->GetBinContent(i) << endl;
     if(eff < 1){
	hOut->SetBinContent(i,eff);
        hOut->SetBinError(i, eff*sqrt( 1./hN->GetBinContent(i) + 1./hD->GetBinContent(ibin)));
     }else{ 
       hOut->SetBinContent(i,1e-9);
     }
  }
  
}

#endif
