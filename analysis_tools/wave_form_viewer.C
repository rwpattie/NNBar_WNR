#include <TTree.h>
#include <TFile.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TROOT.h>
#include <TObject.h>
#include <TApplication.h>
#include <TSQLServer.h>
#include <TLine.h>
#include <TSQLRow.h>
#include <TSQLResult.h>

#include "stdlib.h"
#include "stdio.h"
#include <iostream>

struct fadc_e {
  ULong64_t first_time;
  ULong64_t global_time;
  Int_t     board;
  UShort_t  last;
  UShort_t  adc[5000];
  UShort_t  max;
  UShort_t  min;
  UShort_t  zero;
  UShort_t  ped;
  UShort_t  channel;
};

using namespace std;

Double_t GetThreshold(Int_t nrun, Int_t nchn,Int_t nthrsh)
{
  
  TSQLResult *res;
  TSQLRow    *row;
  Double_t thresh=0;
  
  if(nchn < 0 || nchn > 7){
      cout << "Channel " << nchn << " is not valid !!!! " << endl;
      return -1;
  }
  
  TSQLServer *sql = TSQLServer::Connect("mysql://localhost/wnr_run_info",
					getenv("UCNADBUSER"),getenv("UCNADBPASS"));
  
  char query[500];
  sprintf(query,"select upper_threshold,lower_threshold from channel_info where run_number = %d and channel = %d",
	  nrun,nchn);
  
  res = (TSQLResult*)sql->Query(query);
  if(res->GetRowCount() != 0){
      while((row = (TSQLRow*)res->Next())){
	  thresh = (Double_t)atof(row->GetField(nthrsh));
      }
  }
  
  return thresh; 
}

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
  TFile *file = new TFile(Form("%s/run00%dNA.root",getenv("WNR_OUTPUT_DATA"),nrun),"READ");
  TTree *tr   = (TTree*)file->Get("t");
   
  std::vector<Double_t> time,fadc;
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
  
  Int_t nevents = (Int_t)tr->GetEntries();
  TCanvas *c1 = new TCanvas("c1","c1");
  c1->cd();
  cout << "Total Events " << nevents << endl;
  Int_t nentry = 1;
  Int_t nn=0;
  
  Int_t chan = 0;
  cout << "Enter Channel Number or -1 for all channels : " << endl;
  cin >> chan;
  
  TLine *thr = new TLine(0,1,0,1);
  TLine *lthr = new TLine(0,1,0,1);
  thr->SetLineColor(1);
  thr->SetLineStyle(2);
  lthr->SetLineColor(1);
  lthr->SetLineStyle(2);
  
  std::vector<Double_t> uthreshold,lthreshold;
  for(Int_t i = 0 ; i < 8 ; i++)uthreshold.push_back(GetThreshold(nrun,i,0));
  for(Int_t i = 0 ; i < 8 ; i++)lthreshold.push_back(GetThreshold(nrun,i,1));
  
  if(chan >-1 && chan < 8){
    thr->SetY1(uthreshold[chan]);
    thr->SetY2(uthreshold[chan]);
    lthr->SetY1(lthreshold[chan]);
    lthr->SetY2(lthreshold[chan]);
  }
  
  do{

    c1->Clear();
    fadc.clear();
    time.clear();
    
    tr->GetEntry(nentry);
    cout << "Looping through waveform" << endl;
   
    if(((int)event.channel == chan || chan == -1) && event.last > 10){
      for(Int_t i = 0 ; i < event.last ; i++){
	  Double_t adcvalue = (event.adc[i] < 4096) ? event.adc[i] : 4096;
	  fadc.push_back(adcvalue);
	  time.push_back((Double_t)(event.first_time*16.0e-9 + (Double_t)i*4.0e-9));
	  cout  << "time stamp " << i << "\t" << event.first_time*16.0e-9 + i*4e-9 << "\t" << event.adc[i] << "\t" << event.last << endl;
    }
    
    thr->SetX1(time[0]);
    thr->SetX2(time[(Int_t)time.size()-1]);
    lthr->SetX1(time[0]);
    lthr->SetX2(time[(Int_t)time.size()-1]);
    if(chan<0){
      thr->SetY1(uthreshold[chan]);
      thr->SetY2(uthreshold[chan]);
      lthr->SetY1(lthreshold[chan]);
      lthr->SetY2(lthreshold[chan]);
    }
      
    TGraph gr((Int_t)time.size() -1 , &time[0] , &fadc[0]);
    gr.SetTitle(Form("Run : %d Waveform Traces for Board:Channel %d:%d",nrun,event.board,event.channel));
    gr.SetMarkerColor(2);
    gr.SetLineColor(2);
    gr.Draw("apl");
    thr->Draw("same");
    lthr->Draw("same");
    c1->Update();
    gPad->Update();
    nn=0;
    cout << "Enter event number less than " << nevents << endl;
    cout << "-1 to end or 0 to select next event" << endl;
    cin >> nn;
    if(nn > 0)
      nentry = nn;
    else if(nn == 0)
      nentry++;
    else if(nn == -3)
      c1->Print(Form("waveform_%d_event_%d.pdf",nrun,nentry));
    }else 
      nentry++;
  }while(nn!=-1 && nentry < nevents);
  
  file->Close();

  theApp.Run();
  return -1;
}
