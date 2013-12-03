#include <TCanvas.h>
#include <TH1D.h>
#include <TF1.h>
#include <TTree.h>
#include <TFile.h>
#include <TMath.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

struct fadc_e {
  ULong64_t first_time;
  ULong64_t global_time;
  Int_t     packet_time_s;
  Int_t     packet_time_us;
  Int_t     packet_time_l;
  Int_t     board;
  UInt_t    cycle;
  Double_t  trigger;
  Double_t  riseTime;
  Double_t  width;
  UShort_t  last;
  UShort_t  adc[5000];
  UShort_t  max;
  UShort_t  min;
  UShort_t  zero;
  UShort_t  ped;
  UShort_t  channel;
};

#define MASSN 939.565378 //neutron mass in MeV
#define dt 1.78882
#define SIZECUT 100

Double_t EfromT(Double_t t,Double_t Dis);
void DrawHistos(TH1D **h,TCanvas *c,Int_t n,Bool_t logx,Bool_t logy,TH1D **ht);
void GenerateEnergySpec(TH1D *hTof,TH1D *&hEne,const char *title,
					const char *name,Bool_t CALIB,Bool_t Raw);
void FitBackgrounds(TH1D **h);

void doit(Int_t nrun,Int_t nchn){


  TFile *file = new TFile(Form("%s/run%05dNA.root",
                          getenv("WNR_OUTPUT_DATA"),nrun),"READ");
  TTree *tr   = (TTree*)file->Get("t");

  fadc_e event;
  tr->SetBranchAddress("first_time",&event.first_time);
  tr->SetBranchAddress("board",&event.board);
  tr->SetBranchAddress("last",&event.last);
  tr->SetBranchAddress("adc",event.adc);
  tr->SetBranchAddress("max",&event.max);
  tr->SetBranchAddress("min",&event.min);
  tr->SetBranchAddress("zero",&event.zero);
  tr->SetBranchAddress("ped",&event.ped);
  tr->SetBranchAddress("channel",&event.channel);
  tr->SetBranchAddress("trigger",&event.trigger);
  tr->SetBranchAddress("cycle",&event.cycle);
  tr->SetBranchAddress("packet_time_l",&event.packet_time_l);

  TH1D *hTof[8];
  TH1D *hBck[8];
  for(Int_t i = 0 ; i < 8 ; i++){
    hTof[i] = new TH1D(Form("hTof[%d]",i),
		       Form("Time of flight Chn %d;TOF [#mus];Counts",i),2000,0,2.0);
    hBck[i] = new TH1D(Form("hBck[%d]",i),
		       Form("Background rates after macro pulse;TOF [ms];Counts",i),6000,0,7.0);
  }
  Int_t micro = 0;
  // 
  // a set of timing offsets that for the gastubes are assumed to be due to 
  // differences in cable length
  Double_t offset[8] = {0.935,0.935,0.945,0.965,0.935,0.9245,0.944,0.945};
  Double_t nmacro = 0;
  Double_t total_time = 0;
  
  for(Int_t i = 0 ; i < tr->GetEntries(); i++){
	tr->GetEntry(i);
	Int_t ch = event.channel;
	if(event.cycle > nmacro) nmacro = (Double_t) event.cycle;
	if(event.packet_time_l > total_time) total_time = (Double_t)event.packet_time_l;
        if(event.trigger*1e6 < 625 && event.last > SIZECUT){
          Int_t cyc = (int)((event.trigger*1e6-offset[ch])/dt);
          Double_t tof = event.trigger*1e6 - offset[ch] - (double)cyc*dt;
          if(tof>0)hTof[ch]->Fill(tof);
        }  
          if(event.last > SIZECUT){
          Double_t tof = event.trigger*1e3 - offset[ch]*1e-3; 
          if(tof>0)hBck[ch]->Fill(tof);
	}
  }
  
  cout << "Total time : " << total_time << " s " << total_time / 3600. << " hrs " << endl;
  cout << "Calculated cycles : " << total_time*40. << endl;
  cout << "Number of macro pulses : " << nmacro << endl;
  cout << "          micro pulses : " << nmacro*350. << endl;
  
  TCanvas *c1 = new TCanvas("c1","c1",800,700);
  c1->Divide(2,2);
  
  FitBackgrounds(hTof);
  DrawHistos(hTof,c1,1,kFALSE,kFALSE,hTof);
  
  TH1D *hEnergy[8];
  for(Int_t i = 0 ; i < 8 ; i++)
    GenerateEnergySpec(hTof[i],hEnergy[i],
		       Form("Energy Spectrum %d",i),
		       Form("hEnergy[%d]; Energy [MeV];Counts/MeV",i),kFALSE,kFALSE);
 
 DrawHistos(hEnergy,c1,2,kTRUE,kTRUE,hTof); 
 DrawHistos(hBck,c1,3,kFALSE,kFALSE,hTof);

 c1->Print(Form("rough_analysis_run_%d_norm.pdf",nrun));
}


void FitBackgrounds(TH1D **h)
{
    TF1 *fpre = new TF1("fpre","[0]",0,0.08);
    TF1 *fpost = new TF1("fpost","[0]",1.1,1.74);
    fpre->SetParameter(0,10);
    fpost->SetParameter(0,10);
    
    std::vector<Double_t> post,pre,poste,pree;
    
    for(Int_t i = 0 ; i < 8 ; i++){
	h[i]->Fit(fpre,"QMERN0");
	h[i]->Fit(fpost,"QMERN0");
	post.push_back(fpost->GetParameter(0));
	poste.push_back(fpost->GetParError(0));
	pre.push_back(fpre->GetParameter(0));
	pree.push_back(fpre->GetParError(0));
	std::cout << "Pre counts per bin "  << pre[i] << " +/- " << pree[i] << std::endl;
	std::cout << "Post counts per bin " << post[i] << " +/- " << poste[i] << std::endl;
    }
    for(Int_t i = 0 ; i < 8 ; i++){
      for(Int_t j = 0 ; j < h[i]->GetNbinsX() ; j++){
	h[i]->SetBinContent(j,h[i]->GetBinContent(j) - post[i]);
      }
    }
}


void DrawHistos(TH1D **h,TCanvas *c,Int_t n,Bool_t logx,Bool_t logy,TH1D **ht)
{
  
  c->cd(n);
  for(Int_t i = 0 ; i < 8 ; i++){
      if(i==0)
	h[i]->Draw();
      else{
	if(ht[i]->Integral() > 1e4){
	  h[i]->SetLineColor(i+1);
	  h[i]->SetMarkerColor(i+1);
	  h[i]->Draw("same");
	}
      }
  }
  if(logy)gPad->SetLogy();
  if(logx)gPad->SetLogx();
}

void GenerateEnergySpec(TH1D *hTof,TH1D *&hEne,const char *title,
					const char *name,Bool_t CALIB,Bool_t Raw)
{
  std::vector<Double_t> ebins;
  std::vector<Int_t> tbins;
  Int_t nbin = 0;
  
  for(Int_t i = hTof->GetNbinsX(); i > 0 ; i--){
    Double_t dT = hTof->GetBinWidth(i); // Get the delta T
    if(hTof->GetBinCenter(i) < 1.8 && hTof->GetBinCenter(i) > 0.087
      && !(TMath::IsNaN(dT)) && !(TMath::IsNaN(EfromT(hTof->GetBinCenter(i) + dT,23.)))){
      ebins.push_back(EfromT(hTof->GetBinCenter(i) + dT,23.));
      tbins.push_back(i);
      nbin++;
      } else if(TMath::IsNaN(EfromT(hTof->GetBinCenter(i) + dT,23.))){
    }
  }
  
  hEne = new TH1D(title,name,(int)ebins.size()-1,&ebins[0]);
  
  for(Int_t i = 0 ; i < (int)tbins.size() ; i ++){
      Int_t n = tbins[i];
      Double_t cnts = hTof->GetBinContent(n)*hTof->GetBinWidth(n)/hEne->GetBinWidth(i);
      hEne->SetBinContent(i,cnts);
      hEne->SetBinError(i,hTof->GetBinError(n)*hTof->GetBinWidth(n)/hEne->GetBinWidth(i));
  }
  
//  CorrectFissionCrossSection(hEne,CALIB);
};

Double_t EfromT(Double_t t,Double_t Dis)
{
    t *= 1e-6;
   
    Double_t mn = MASSN*1e6;
    Double_t a = TMath::Power(Dis/(TMath::C()*t),2);
    Double_t Er = (1./sqrt(1-a) - 1)*mn;
   
    return Er*1e-6;
}
