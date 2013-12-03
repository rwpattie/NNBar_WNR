#ifndef __WAVE_FORM_VIEWER__
#define __WAVE_FORM_VIEWER__

/* -------------------------------------------------------------------
 *  Author : R.W.Pattie Jr.
 *  Date Created : 2-10-2013
 *  Purpose : This programs is a designed to display the waveforms from
 *            the muCap fADC cards used in the WNR detector tests.  In
 *            addition to simple visual inspection many of the modules
 *            used in further analysis are tested here, so that their
 *            application and results can be examined on an event-by-
 *            event basis.
 * -------------------------------------------------------------------- */

#include <TTree.h>
#include <TFile.h>
#include <TAxis.h>
#include <TF1.h>
#include <TLatex.h>
#include <TLegend.h>
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
#include "typesdef.h"
#include "stdio.h"
#include "wave_form_math.h"
#include <iostream>

using namespace std;

// void SmoothArray(std::vector<Double_t> t1,std::vector<Double_t>f1,
// 		 std::vector<Double_t>& t2,std::vector<Double_t>& f2,
// 		 Int_t smwidth);
// Double_t DerivArray(std::vector<Double_t> val,std::vector<Double_t>& der);
// Double_t SolveLine(Double_t x1,Double_t y1,Double_t x2,Double_t y2,Double_t yp);
// Double_t FindTriggerTime(std::vector<Double_t> val,std::vector<Double_t> time,
// 			 Double_t thrs);
// Double_t FindWidth(std::vector<Double_t> val,std::vector<Double_t> time,
// 		   Double_t thrs);
Double_t GetThreshold(Int_t nrun, Int_t nchn,Int_t nthrsh,Int_t nboard);
//--------------------------------------------------------------------------------------
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
  TFile *file = new TFile(Form("%s/run%05dNA.root",getenv("WNR_OUTPUT_DATA"),nrun),"READ");
  TTree *tr   = (TTree*)file->Get("t");
   
  std::vector<Double_t> time,fadc,smfadc,smtime,smfadc5,smtime5,derv5;
  std::vector<Double_t> peak_v,ptimes;
  std::vector<Int_t> peak_t;
  
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
  
  TF1 *fAve = new TF1("fAve","[0]",0,10);
  TF1 *fSlp = new TF1("fSlp","pol4",0,0.1);
  fAve->SetLineColor(4);
  Int_t nevents = (Int_t)tr->GetEntries();
  TCanvas *c1 = new TCanvas("c1","c1");
  cout << "Total Events " << nevents << endl;
  Int_t nentry = 1;
  Int_t nn=0;
  
  Int_t chan = 0;
  Int_t nbrd = -1;
  cout << "Enter Channel Number or -1 for all channels : " << endl;
  cin >> chan;
  if(chan > -1){
      cout << "Enter board number : " << endl;
      cin >> nbrd;
  }
  
  
  TLine *thr = new TLine(0,1,0,1);
  TLine *lthr = new TLine(0,1,0,1);
  thr->SetLineColor(1);
  thr->SetLineStyle(2);
  lthr->SetLineColor(1);
  lthr->SetLineStyle(2);
  
  std::vector<Double_t> uthreshold,lthreshold;
  for(Int_t j = 1 ; j < 4 ; j++){
    for(Int_t i = 0 ; i < 8 ; i++)uthreshold.push_back(GetThreshold(nrun,i,0,j));
    for(Int_t i = 0 ; i < 8 ; i++)lthreshold.push_back(GetThreshold(nrun,i,1,j));
  }
  
  if(chan >-1 && chan < 8){
    thr->SetY1(uthreshold[chan+nbrd*8]);
    thr->SetY2(uthreshold[chan+nbrd*8]);
    lthr->SetY1(lthreshold[chan+nbrd*8]);
    lthr->SetY2(lthreshold[chan+nbrd*8]);
  }
  
  do{

    c1->Clear();
    c1->Divide(1,2);
    fadc.clear();
    smfadc.clear();
    peak_t.clear();
    peak_v.clear();
    ptimes.clear();
    smtime.clear();
    time.clear();
    smfadc5.clear();
    smtime5.clear();
    derv5.clear();
    
    tr->GetEntry(nentry);
    cout << "Looping through waveform" << endl;
   
    if(((int)event.channel == chan || chan == -1) && 
      ((int)event.board == nbrd || nbrd == -1) &&
      event.last > 10){
      for(Int_t i = 0 ; i < event.last ; i++){
	  Double_t adcvalue = (event.adc[i] < 4096) ? event.adc[i] : 4096;
	  fadc.push_back(adcvalue);
	  time.push_back((Double_t)(event.first_time*16.0e-9 + 
				   (Double_t)i*4.0e-9));
    }
    Double_t curthresh  = uthreshold[(int)event.channel+((int)event.board-1)*8];
    Double_t curlthresh = lthreshold[(int)event.channel+((int)event.board-1)*8];
    cout << curthresh << "\t" << event.board <<  endl;
    thr->SetX1(time[0]);
    thr->SetX2(time[(Int_t)time.size()-1]);
    lthr->SetX1(time[0]);
    lthr->SetX2(time[(Int_t)time.size()-1]);
    if(chan<0){
      thr->SetY1(curthresh);
      thr->SetY2(curthresh);
      lthr->SetY1(curlthresh);
      lthr->SetY2(curlthresh);
    }
    // use moving average to smooth out the wave forms
    SmoothArray(time,fadc,smtime,smfadc,3);  
    SmoothArray(time,fadc,smtime5,smfadc5,5);  
    Double_t rise_time = DerivArray(smfadc5,derv5);
    Double_t trigger_time = FindTriggerTime(smfadc5,smtime5,curthresh);
    Double_t width = FindWidth(smfadc5,smtime5,curthresh);
    Int_t npeaks = Find_Peaks(smfadc5,derv5,peak_v,peak_t,curthresh);
    
    for(Int_t i = 0 ; i < (int)peak_t.size() ; i++)
      ptimes.push_back(smtime5[peak_t[i]]);
    
    
    TGraph gr((Int_t)time.size() -1, &time[0] , &fadc[0]);
    TGraph gsm((Int_t)smtime.size()  -1, &smtime[0] , &smfadc[0]);
    TGraph gsm5((Int_t)smtime5.size() -1 , &smtime5[0] , &smfadc5[0]);
    TGraph gder((Int_t)derv5.size() -1 , &smtime5[1] , &derv5[0]);
    TGraph gPk((Int_t)peak_v.size(),&ptimes[0],&peak_v[0]);
    gPk.SetMarkerColor(2);
    gPk.SetMarkerStyle(23);
    
    gr.SetTitle(Form("Run : %d Waveform Traces for Board:Channel %d:%d",
		     nrun,event.board,event.channel));
    gr.GetXaxis()->SetTitle("Time [#mu s]");
    gr.GetYaxis()->SetTitle("Amplitude [ADC units]");
    gr.SetMarkerColor(2);
    gr.SetLineColor(2);
    gsm.SetLineColor(4);
    gsm.SetMarkerColor(4);
    gsm5.SetLineColor(1);
    gsm5.SetMarkerColor(1);
    c1->cd(1);
    gr.Draw("apl");
    gr.SetName("gr");
    gr.Fit(fAve,"RMEQ","",time[1],time[20]);
  //  gr.Fit(fSlp,"RME","same",time[30],time[50]);
    gsm.Draw("pl");
    gsm.SetName("gsm");
    gsm5.Draw("pl");
    gsm.SetName("gsm5");
    thr->Draw("same");
    lthr->Draw("same");
    gPk.Draw("p");
    TLegend leg(0.6,0.55,0.85,0.85);
    leg.SetFillColor(0);
    leg.SetLineColor(0);
    leg.AddEntry("gr","Raw Trace","l");
    leg.AddEntry("gsm","3 sample Ave","l");
    leg.AddEntry("gsm5","5 sample Ave","l");
    leg.AddEntry("gr",Form("Rise Time : %5.2f ns",rise_time),"");
    leg.AddEntry("gr",Form("Trigger Time : %5.2f #mu s",trigger_time*1e6),"");
    leg.AddEntry("gr",Form("Pulse Width : %5.2f #mu s",width*1e6),"");
    leg.Draw();
    c1->cd(2);
    gder.SetMarkerStyle(7);
    gder.Draw("apl");
    gder.SetTitle("Waveform Derivative : 3 -sample averaging");
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
//----------------------------------------------------------------------------------------------------
// void SmoothArray(std::vector<Double_t> t1,std::vector<Double_t>f1,
// 		 std::vector<Double_t>& t2,std::vector<Double_t>& f2,
// 		 Int_t smwidth)
// {
//   Int_t offset = smwidth / 2;
//   for(Int_t i = offset ; i < (int)f1.size() - offset ; i++){
//       t2.push_back(t1[i]);
//       Double_t fsum = 0;
//       for(int j = 0 ; j < smwidth ; j++){
// 	fsum += f1[i+j-offset];
//       }
//       f2.push_back(fsum/(double)smwidth);
//   }
//   
// }
// 
// Double_t DerivArray(std::vector<Double_t> val,std::vector<Double_t>& der)
// {
//     //very simple 2 point derivative. probably very wrong but a first step....
//     Double_t dt = 1.;
//     Int_t trgp = 0;
//     Int_t maxp = 0;
//     for(Int_t i = 1 ; i < (int)val.size() ; i++){
//       der.push_back((val[i+1] - val[i-1])/(2.*dt));
//       if(der[i-1] > 5 && trgp == 0 )trgp = i;
//       if(trgp > 0  && der[i-1] < 0 && maxp == 0)maxp = i;
//     }
//     Double_t t0 = SolveLine((double)(trgp-2),der[trgp-2],(double)(trgp+2),der[trgp+2],5.);
//     Double_t t1 = SolveLine((double)(maxp-2),der[maxp-2],(double)(maxp+2),der[maxp+2],0.);
//     return (double)(t1 - t0)*4.0;
// }
// 
// Double_t SolveLine(Double_t x1,Double_t y1,Double_t x2,Double_t y2,Double_t yp)
// {
//     Double_t m = (y2-y1)/(x2-x1);
//     //y = mx + b ... (y-b)/m = x ...  m = (yp - y1)/(xp - x1) ...  xp = x1 + (yp-y1)/m
//     Double_t xp = x1 + (yp-y1)/m;
//     return xp;
// }
// 
// Double_t FindTriggerTime(std::vector<Double_t> val,std::vector<Double_t> time,Double_t thrs)
// {
//     Int_t ntrg=0;
//     while(val[ntrg] < thrs && ntrg < (int)val.size())
//       ntrg++;
//     
//     if(ntrg == (int)val.size()){
//       thrs*=0.9;
//       ntrg=0;
//       while(val[ntrg] < thrs && ntrg < (int)val.size())
//       ntrg++;
//     }
//     
//     Double_t tTrig = SolveLine(time[ntrg-2],val[ntrg-2],time[ntrg+2],val[ntrg+2],thrs);
//     return tTrig;
// }
// 
// Double_t FindWidth(std::vector<Double_t> val,std::vector<Double_t> time,Double_t thrs)
// {
//     Int_t t0 = 0;  // first trigger crossing
//     Int_t t1 = 0;  // second trigger crossing
//     
//     for(UInt_t i = 0 ; i < val.size() ; i++){
// 	if(val[i] > thrs && t0 == 0) t0 = i;
// 	if(val[i] < thrs && t0 > 0 && t1 == 0) t1 = i;
//     }
//     if(t0 == 0){
//         thrs*=0.9;
// 	for(UInt_t i = 0 ; i < val.size() ; i++){
// 	  if(val[i] > thrs && t0 == 0)t0 = i;
// 	  if(val[i] < thrs && t0 > 0 && t1 == 0) t1 = i;
// 	}
//     }
//      
//     Double_t tt0 = SolveLine(time[t0-2],val[t0-2],time[t0+2],val[t0+2],thrs);
//     Double_t tt1 = SolveLine(time[t1-2],val[t1-2],time[t1+2],val[t1+2],thrs);
// 
//     return (tt1-tt0);
// }
//-----------------------------------------------------------------------------------------------
Double_t GetThreshold(Int_t nrun, Int_t nchn,Int_t nthrsh,Int_t nboard)
{
  
  TSQLResult *res;
  TSQLRow    *row;
  Double_t thresh=0;
  
  if(nchn < 0 || nchn > 7){
      cout << "Channel " << nchn << " is not valid !!!! " << endl;
      return -1;
  }
  
  TSQLServer *sql = TSQLServer::Connect("mysql://localhost/wnr_run_info",
					getenv("UCNADBUSER"),
					getenv("UCNADBPASS"));
  
  char query[500];
  sprintf(query,"select upper_threshold,lower_threshold from channel_info where"
                " run_number = %d and channel = %d and board =%d",
	  nrun,nchn,nboard);
  
  res = (TSQLResult*)sql->Query(query);
  
  if(res->GetRowCount() != 0){
      while((row = (TSQLRow*)res->Next())){
	  thresh = (Double_t)atof(row->GetField(nthrsh));
      }
  }
  
  return thresh; 
}

#endif