// File: WaveformAnalyzer.cpp
// Name: Leah Broussard
// Date: 2013/4/11
// Purpose: Analyzes waveforms for energy and timing
//
// Revision History:
// 2013/4/11: LJB  Shaping waveforms with poly4 fit

#ifndef WAVEFORM_ANALYZER_CPP__
#define WAVEFORM_ANALYZER_CPP__

#include "WaveformAnalyzer.hh"

/*************************************************************************/
//                            Constructor
/*************************************************************************/
WaveformAnalyzer::WaveformAnalyzer() {
  g = 0;
  f = 0;
  h = 0;
  dodraw = true;
}

/*************************************************************************/
//                             Destructor
/*************************************************************************/
WaveformAnalyzer::~WaveformAnalyzer() {
  if (g!=0) delete g;
  if (h!=0) delete h;
  if (f!=0) delete f;
}

/*************************************************************************/
//                              GetEntry    
/*************************************************************************/
void WaveformAnalyzer::GetEntry(int ev) {
  if (!IsOpen())
    return;
  SAFile.RootTree->GetEntry(ev);
}

/*************************************************************************/
//                                Plot
/*************************************************************************/
void WaveformAnalyzer::Plot(int ev) {
  if (!IsOpen())
    return;
  GetEntry(ev);
  Plot();
}

void WaveformAnalyzer::Plot() {
  double x[5000], y[5000];
  for (int i=0;i<SAFile.fadc_event.last;i++) {
    x[i] = i;
    y[i] = SAFile.fadc_event.adc[i];
  }
  if (g!=0)
    delete g;
  g = new TGraph(SAFile.fadc_event.last,x,y);
  g->SetMarkerStyle(20);
  g->GetXaxis()->SetLimits(-0.5,SAFile.fadc_event.last-0.5);
  g->GetYaxis()->SetRangeUser(2500,4000);
  g->Draw("AP");
}

/*************************************************************************/
//                             FindMinLoc
/*************************************************************************/
void WaveformAnalyzer::FindMinLoc(int& min, int& mini) {
  if (!IsOpen())
    return;
  min = SAFile.fadc_event.adc[0];
  mini = 0;
  for (int i=0;i<SAFile.fadc_event.last;i++) {
    if (min > SAFile.fadc_event.adc[i]) {
      min = SAFile.fadc_event.adc[i];
      mini = i;
    }
  }
}

/*************************************************************************/
//                             LocalFitMin
/*************************************************************************/
void WaveformAnalyzer::LocalFit(int start, int window) {
  double* x = new double[window];
  double* y = new double[window];
  double a[FITPOLY];
  if (start < 0)
    start = 0;
  for (int i=0;i<window;i++) {
    x[i] = start + i;
    y[i] = SAFile.fadc_event.adc[start + i];
  }
  if (g!=0)
    delete g;
  g = new TGraph(window,x,y);
  // todo: account for packet loss gaps
  g->LeastSquareFit(FITPOLY,a);
  if (f!=0)
    delete f;
  f = new TF1("fPoint","pol4",start,start+window);
  f->SetLineColor(kBlue);
  f->SetParameters(a);
  if (dodraw) {
    Plot();
    fPoint->Draw("same");
  }
  delete [] x;
  delete [] y;
}

void WaveformAnalyzer::LocalFitMin(int window) {
  int min, mini;
  FindMinLoc(min,mini);
  if (min == 0)
    f->SetParameters(0,0,0,0);
  else {
    int start = mini - window/2;
    LocalFit(start,window);
  }
}

void WaveformAnalyzer::LocalFitZero(int window) {
  LocalFit(0,window);
}

double WaveformAnalyzer::FindAmplitudeFast() {
  dodraw = false;
  LocalFitMin(200);
  double peak = f->GetMinimum();
  if (peak == 0)
    return 0;
  LocalFitZero(200);
  double baseline = f->Eval(0);
  dodraw = true;
  return baseline - peak;
}

void WaveformAnalyzer::BuildSpectrumFast(int ch) {
  int nentries = SAFile.RootTree->GetEntries();
  if (h!=0)
    delete h;
  h = new TH1D("h","h",1000,0,2000);
  for (int i=0;i<nentries;i++) {
    printf("Working....%d/%d  (%d \%)\r",i,nentries,100*i/nentries);
    fflush(stdout);
    GetEntry(i);
    double amp = 0;
    if (SAFile.fadc_event.last > 4)
      amp = FindAmplitudeFast();
    h->Fill(amp);
  }
  h->Draw();
}


#endif // WAVEFORM_ANALYZER_CPP__
