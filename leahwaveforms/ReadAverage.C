// File: ReadAverage.C
// Name: Leah Broussard
// Date: 2013/4/4
// Purpose: Example script which turns average pulse shape into a fit function
//
// OpenAvg:         Opens the average pulse shape file and build fit function
// datafunction:    Fit function for real events
// pulserfunction:  Fit function for pulser events
// TwoPeak:         Example pileup function
// Interpolate:     Linear interpolation to fill gaps between samples
//
// Revision History:
// 2013/4/4:  LJB         File created based on cleaned up Ne19 version




#include "../Raw/Raw.h"
#include "TGraphErrors.h"
#include "TF1.h"

//Least Squares Fit parameters
#define FITPOINTS 40    //number of points to fit
#define FITPOLY 5       //order+1 of fit polynomial
#define LWIDD 50        //left-width of pulse
#define LWIDP 50        //left-width of pulse
#define WRITE 1         //write to file?
#define DEBUG 0         //print statements
#define NAMP 2          //energy ranges

#define WaveLength 1000
#define NBINS WaveLength

TFile* AFile;
struct g_t{
  TGraph* graph;
  double base;
  double riseexpo0;
  double riseexpo1;
  double fall0;
  double fall1;
  TF1* f;
};
g_t data[N_ADC];
g_t pulser[N_ADC];

bool OpenAvg(int num);
double datafunction(double* x, double* par);
double pulserfunction(double* x, double* par);
Double_t Interpolate(TGraph* g, Double_t x);


/*************************************************************************/
//                               OpenAvg
/*************************************************************************/
bool OpenAvg(int num) {
  TString filename = Form("Avg%05d.root",num);
  TString name;
  if (num < 851)
    name = PATH1;
  else if (num < 1419)
    name = PATH2;
  else if (num < 2090)
    name = PATH3;
  else
    name = PATH4;
  name+= "Averages/";
  name+= filename;
  if (gSystem->AccessPathName(name)) {
    cout << name << " not found " << endl;
    return false;
  }
  AFile = new TFile(name);
  if (!AFile->IsOpen()) {
    return false;
  }
  TF1* fpol0 = new TF1("fpol0","pol0",0,1000);
  TF1* fpolexpo = new TF1("fpolexpo","pol0(0) + expo(1)",0,1000);
  TF1* fpol1 = new TF1("fpol1","pol1",0,1000);
  /*************************************************************************/
  //Estimate waveform behavior out-of-bounds, change to fit your pulse shape
  //Only need to interpolate if you have full waveforms
  /*************************************************************************/
  for (int ch=0;ch<N_ADC;ch++) {
    TString name = "data";
    name += ch;
    name += "amp1";
    data[ch].graph = (TGraph*)gDirectory->Get(name);
    fpol0->SetParameter(0,0);
    data[ch].graph->Fit(fpol0,"QN","",350,370);
    fpolexpo->SetParameters(fpol0->GetParameter(0),-30,0.05);
    data[ch].graph->Fit(fpolexpo,"QN","",350,400);
    data[ch].base = fpolexpo->GetParameter(0);
    data[ch].riseexpo0 = fpolexpo->GetParameter(1);
    data[ch].riseexpo1 = fpolexpo->GetParameter(2);
    fpol1->SetParameters(0,1);
    data[ch].graph->Fit(fpol1,"QN","",760,800);
    data[ch].fall0 = fpol1->GetParameter(0);
    data[ch].fall1 = fpol1->GetParameter(1);
    name = "fd";
    name += ch;
    data[ch].f = new TF1(name,datafunction,0,20000,4);
    data[ch].f->FixParameter(3,ch);
    data[ch].f->SetParameters(0,      //background
			      1000,   //amplitude
			      250);   //peak location
    name = "pulser";
    name += ch;
    pulser[ch].graph = (TGraph*)gDirectory->Get(name);
    fpol0->SetParameter(0,0);
    pulser[ch].graph->Fit(fpol0,"QN","",320,340);
    fpolexpo->SetParameters(fpol0->GetParameter(0),-45,0.1);
    pulser[ch].graph->Fit(fpolexpo,"QN","",320,380);
    pulser[ch].base = fpolexpo->GetParameter(0);
    pulser[ch].riseexpo0 = fpolexpo->GetParameter(1);
    pulser[ch].riseexpo1 = fpolexpo->GetParameter(2);
    fpol1->SetParameters(0,1);
    pulser[ch].graph->Fit(fpol1,"QN","",760,800);
    pulser[ch].fall0 = fpol1->GetParameter(0);
    pulser[ch].fall1 = fpol1->GetParameter(1);
    name = "fp";
    name += ch;
    pulser[ch].f = new TF1(name,pulserfunction,0,20000,4);
    pulser[ch].f->FixParameter(3,ch);   //channel
    pulser[ch].f->SetParameters(0,      //background
				9000,   //amplitude
				250);   //peak location
  }
  delete fpol0;
  delete fpolexpo;
  delete fpol1;
  return true;
}

/*************************************************************************/
//                             datafunction
/*************************************************************************/
double datafunction(double* x, double* par) {
  double xval = *x;
  double bkg = par[0];
  double amp = par[1];
  double mean = par[2];
  xval = xval + 500 - mean;
  int ch = (int)TMath::Nint(par[3]);
  double retval = 0;
  if (xval < 350)
    retval = data[ch].base + TMath::Exp(data[ch].riseexpo0 + data[ch].riseexpo1*xval);
  if (xval >= 350 && xval <= 800)
    retval = Interpolate(data[ch].graph,xval);
  if (xval > 800) {
    retval = data[ch].fall0 + data[ch].fall1*xval;
    if (retval < data[ch].base) {
      retval = data[ch].base;
    }
  }
  retval = (retval - data[ch].base)/(1. - data[ch].base); //normalize
  retval = bkg + amp*retval;  //scale
  return retval;
};

/*************************************************************************/
//                            pulserfunction
/*************************************************************************/
double pulserfunction(double* x, double* par) {
  double xval = *x;
  double bkg = par[0];
  double amp = par[1];
  double mean = par[2];
  xval = xval + 500 - mean;
  int ch = (int)TMath::Nint(par[3]);
  double retval = 0;
  if (xval < 350)
    retval = pulser[ch].base + TMath::Exp(pulser[ch].riseexpo0 + pulser[ch].riseexpo1*xval);
  if (xval >= 350 && xval <= 800)
    retval = Interpolate(pulser[ch].graph,xval);
  if (xval > 800) {
    retval = pulser[ch].fall0 + pulser[ch].fall1*xval;
    if (retval < pulser[ch].base) {
      retval = pulser[ch].base;
    }
  }
  retval = (retval - pulser[ch].base)/(1. - pulser[ch].base); //normalize
  retval = bkg + amp*retval;  //scale
  return retval;
};

/*************************************************************************/
//                               TwoPeak
/*************************************************************************/
double TwoPeak(double* x, double* par) {
  double bkg1 = par[0];
  double amp1 = par[1];
  double mean1 = par[2];
  double bkg2 = 0;
  double amp2 = par[3];
  double mean2 = par[4];
  double ch = par[5];
  double pars1[] = {bkg1,amp1,mean1,ch};
  double pars2[] = {bkg2,amp2,mean2,ch};
  double p1 = datafunction(x,pars1);
  double p2 = datafunction(x,pars2);
  return p1 + p2;
}


/*************************************************************************/
//                               Interpolate
/*************************************************************************/
Double_t Interpolate(TGraph* g, Double_t x) {
  int i0 = (int)TMath::Floor(x);
  int i1 = (int)TMath::Ceil(x);
  if (i0 == i1)
    i1 = i0 + 1;
  double x0 = 0, x1 = 0, y0 = 0, y1 = 0;
  g->GetPoint(i0,x0,y0);
  g->GetPoint(i1,x1,y1);
  return y0 + (x-x0)*((y1-y0)/(x1-x0));
}

