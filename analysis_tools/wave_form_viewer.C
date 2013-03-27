#include <TTree.h>
#include <TFile.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TROOT.h>
#include <TObject.h>
#include <TApplication.h>

#include "stdlib.h"
#include "stdio.h"
#include <iostream>

struct fadc_e {
  ULong64_t first_time;
  Int_t board;
  UShort_t last;
  UShort_t adc[5000];
  UShort_t max;
  UShort_t min;
  UShort_t zero;
  UShort_t ped;
  UShort_t channel;
};

int main(int argc,char *argv[])
{
  //  using namespace std;
  using std::cout;
  using std::endl;
  using std::cin;

  TApplication theApp("theApp",0,0);
  if(argc < 2 ){
    cout << "Use the following format : wave_form_viewer run#" << endl;
    return -1;
  }
  int nrun = atoi(argv[1]);
  TFile *file = new TFile(Form("/media/Data3/WNR_DATA/WNR/WNR-MIDAS_RUN/analyzed_runs/run00%dNA.root",nrun),"READ");
  TTree *tr   = (TTree*)file->Get("t");
   
  std::vector<Double_t> time,fadc;
  fadc_e event;
  tr->SetBranchAddress("fadc_event",&event.first_time);
  // TGraph gr;
  Int_t nevents = (Int_t)tr->GetEntries();
  TCanvas *c1 = new TCanvas("c1","c1");
  c1->cd();
  cout << "Total Events " << nevents << endl;
  Int_t nentry = 1;
  Int_t nn=0;
  
  Int_t chan = 0;
  cout << "Enter Channel Number or -1 for all channels : " << endl;
  cin >> chan;

  do{

    c1->Clear();
    fadc.clear();
    time.clear();
    
    tr->GetEntry(nentry);
    cout << "Looping through waveform" << endl;
    //if(event.channel == 0){
      if((int)event.channel == chan || chan == -1){
      for(Int_t i = 0 ; i < 5000 ; i++){
	if(i < event.last ){
	  Double_t adcvalue = (event.adc[i] < 4000) ? event.adc[i] : 0;
	  fadc.push_back(adcvalue);
	  time.push_back((Double_t)(event.first_time*16.0e-9 + (Double_t)i*4.0e-9));
	  cout  << "time stamp " << i << "\t" << event.first_time*16.0e-9 + i*4e-9 << "\t" << event.adc[i] << "\t" << event.last << endl;
	} 
      }
      
    //}
    TGraph gr((Int_t)time.size() -1 , &time[0] , &fadc[0]);
    gr.SetTitle(Form("Run : %d Waveform Traces for Board:Channel %d:%d",nrun,event.board,event.channel));
   // gr.GetXaxis().SetTitle("Time [ns]");
   // gr.GetYaxis().SetTitle("ADC Channel");
    gr.SetMarkerColor(2);
    gr.SetLineColor(2);
    gr.Draw("apl");
    c1->Update();
    gPad->Update();
    nn=0;
    cout << "Enter event number less than " << nevents << endl;
    cout << "-1 to end or 0 to select next event" << endl;
    cin >> nn;
    if(nn>0)
      nentry = nn;
    else if(nn == 0)
      nentry++;
    } else 
      nentry++;
  }while(nn!=-1 && nentry < nevents);
  
  TH1F *hAveWave[24];
  
  
  
  file->Close();

  theApp.Run();
  return -1;
}
