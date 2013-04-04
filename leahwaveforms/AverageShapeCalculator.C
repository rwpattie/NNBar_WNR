// File: AverageShapeCalculator.C
// Name: Leah Broussard
// Date: 2013/4/4
// Purpose: Builds an average pulse shape
//
// OpenPulser:   Opens Pulser%05d.root file, contains pulser ID
// CalcShape:    Calculate info about each waveform
// CalcWave:     Calculate average shape
// FitPeak:      Find maximum of waveform
// FitLoc:       Find value of waveform at position
//
// Revision History:
// 2013/4/4:  LJB         File created based on cleaned up Ne19 version




#include "../Raw/Raw.h"
#include "TGraphErrors.h"
#include "TF1.h"

#define FITPOINTS 40    //number of points to fit
#define FITPOLY 5       //order+1 of fit polynomial
#define LWIDD 50        //left-width of event pulse
#define LWIDP 50        //left-width of pulser pulse
#define WRITE 1         //write to file?
#define NAMP 1          //number of energy ranges

#define WaveLength 1000
#define NBINS WaveLength

//Input files
Raw* r;
TFile* PFile;
TTree* PTree;
struct pulser_t{
  double time;            // time of event at smp 0
  double pulser;          // actual pulser peak time
  int prev;               // previous pulser event
  int next;               // next pulser event
  bool ispulser;          // is this a pulser
};
pulser_t pulser;

//Test pulse shape before including in average
struct pulse_t {
  double peak;
  double peakloc;
  double lwid;
  double lw;
  double llw;
  double rw;
  double rrw;
};
struct pch_t {
  pulse_t ch[N_ADC];
};
pch_t* pev;
struct width_t{
  TH1D* lw;
  TH1D* llw;
  TH1D* rw;
  TH1D* rrw;
};
width_t Pwid[N_ADC];
width_t Dwid[N_ADC];

//Output average pulse shapes
struct shape_t {
  double x[NBINS];
  double y[NBINS];
  int cnt[NBINS];
};
shape_t pshape[N_ADC];          //Pulser events
struct de_t{
  shape_t dshape[N_ADC];        //Real events for each amplitude range
};
de_t damp[NAMP];

TGraph* gPeak;                  // Peak finder graph
TF1*    fPeak;                  // Peak finder fit function


bool OpenPulser(int num);
void CalcShape(Raw* dataset);
bool FitPeak(Raw* dataset, int ch, double& peak, double& peakloc);
double FitLoc(Raw* dataset, int ch, double loc);
void CalcWave(Raw* dataset);


/*************************************************************************/
//                            Main Function
/*************************************************************************/
int main(int argc, char* argv[])
{
  if (argc!=2) {
    cout << "Usage: ./average.o Run"<<endl;
    return 0;
  }
  int run = atoi(argv[1]);
  if ((run<0)||(run>99999))
    return 0;

  //-----Initialize data structures
  gPeak = 0;
  fPeak = 0;
  for (int num=0;num<NBINS;num++) {
    for (int ch=0;ch<N_ADC;ch++) {
      pshape[ch].x[num] = num;
      pshape[ch].y[num] = 0;
      pshape[ch].cnt[num] = 0;
      for (int a=0;a<NAMP;a++) {
	damp[a].dshape[ch].x[num] = num;
	damp[a].dshape[ch].y[num] = 0;
	damp[a].dshape[ch].cnt[num] = 0;
      }
    }
  }
  pev = 0;
  //-----Initialize shape testers
  for (int ch=0;ch<N_ADC;ch++) {
    Pwid[ch].lw =0;
    Pwid[ch].llw =0;
    Pwid[ch].rw =0;
    Pwid[ch].rrw =0;
    Dwid[ch].lw =0;
    Dwid[ch].llw =0;
    Dwid[ch].rw =0;
    Dwid[ch].rrw =0;
  }
  /*************************************************************************/
  //Open waveform and helpers, replace with version applicable to your files
  /*************************************************************************/
  r = new Raw(run, false);
  bool POK = OpenPulser(run);
  if (r->run.num != 0 && POK) {
    CalcShape(r);
    r->Open(run,false); //hack: reset to beginning
    CalcWave(r);
  }
  return 0;
}

/*************************************************************************/
//                            OpenPulser
/*************************************************************************/
bool OpenPulser(int num) {
  TString filename = Form("Pulser%05d.root",num);
  TString name;
  if (num < 851)
    name = PATH1;
  else if (num < 1419)
    name = PATH2;
  else if (num < 2090)
    name = PATH3;
  else
    name = PATH4;
  name+= "Friends/";
  name+= filename;
  if (gSystem->AccessPathName(name)) {
    cout << name << " not found " << endl;
    return false;
  }
  PFile = new TFile(name);
  if (!PFile->IsOpen()) {
    return false;
  }
  PTree = (TTree*)gDirectory->Get("pulser");
  if (PTree == 0)
    return false;
  PTree->SetBranchStatus("*",1);
  PTree->SetBranchAddress("pulser",&pulser.time);
  return true;
}

/*************************************************************************/
//                            CalcShape
/*************************************************************************/
void CalcShape(Raw* dataset)
{
  //-----Create shape histograms
  for (int ch=0;ch<N_ADC;ch++) {
    TString name = "plw";
    name += ch;
    Pwid[ch].lw = new TH1D(name,name,200,0,200);
    name = "pllw";
    name += ch;
    Pwid[ch].llw = new TH1D(name,name,200,0,200);
    name = "prw";
    name += ch;
    Pwid[ch].rw = new TH1D(name,name,200,0,200);
    name = "prrw";
    name += ch;
    Pwid[ch].rrw = new TH1D(name,name,200,0,200);
  }
  for (int ch=0;ch<N_ADC;ch++) {
    TString name = "dlw";
    name += ch;
    Dwid[ch].lw = new TH1D(name,name,200,0,200);
    name = "dllw";
    name += ch;
    Dwid[ch].llw = new TH1D(name,name,200,0,200);
    name = "drw";
    name += ch;
    Dwid[ch].rw = new TH1D(name,name,200,0,200);
    name = "drrw";
    name += ch;
    Dwid[ch].rrw = new TH1D(name,name,200,0,200);
  }
  int NumEvents = dataset->NumEvents;
  if (pev != 0)
    delete pev;
  pev = new pch_t[NumEvents];

  //-----Loop through waveforms
  cout << "Processing " << NumEvents << " events" << endl;  
  int ev = 0;
  do {
    PFile->cd();
    PTree->GetEntry(ev);
    gROOT->cd();
    if (ev > 0)
      dataset->GetNextEvent();
    for (int ch=0;ch<N_ADC;ch++) {
      pev[ev].ch[ch].peak = -1;
      pev[ev].ch[ch].peakloc = -1;
      pev[ev].ch[ch].lwid = -1;
      if (!dataset->event.overflow && ((dataset->event.trigger[0].chmask>>ch)&0x1) 
	  && dataset->data[ch].peakloc > 100 && dataset->data[ch].peakloc < 300
	  && dataset->event.fullwave_size < WaveLength*0.6  ) {
	//-----Find maximum point
	double peakloc = -1, peak = -1, Lwidthval = -1;
	FitPeak(dataset,ch,peak,peakloc);
	pev[ev].ch[ch].peak = peak;
	pev[ev].ch[ch].peakloc = peakloc;
	//-----Find Left Width amplitude (alternative to baseline)
	double LWID;
	if (pulser.ispulser) {
	  LWID = LWIDP;
	}
	else
	  LWID = LWIDD;
	Lwidthval = FitLoc(dataset,ch,peakloc-LWID);
	pev[ev].ch[ch].lwid = Lwidthval;
	if (peak - Lwidthval > 300) {
	  //-----Store info about pulse shape
	  double llw = -1, lw = -1, rw = -1, rrw = -1;
	  if (peak != -1 && Lwidthval != -1) {
	    double scale = 1./(peak - Lwidthval);
	    double LV1 = 0.;
	    double LV2 = -1.5;
	    int smp = dataset->data[ch].peakloc;
	    bool ZS = (dataset->event.trigger[(dataset->GetTriggerofSmp(smp))].buffer[ch].len == 0);
	    while ((((dataset->event.waveform[ch].y[smp]-Lwidthval)*scale > LV1 && !ZS)
		    || ZS) && smp >= 0) {
	      smp--;
	      if (smp >= 0)
		ZS = (dataset->event.trigger[(dataset->GetTriggerofSmp(smp))].buffer[ch].len == 0);
	    }
	    if (smp >= 0 && !ZS && (dataset->event.waveform[ch].y[smp]-Lwidthval)*scale <= LV1)
	      lw = dataset->data[ch].peakloc - smp;
	    while ((((dataset->event.waveform[ch].y[smp]-Lwidthval)*scale > LV2 && !ZS)
		    || ZS) && smp >= 0) {
	      smp--;
	      if (smp >= 0)
		ZS = (dataset->event.trigger[(dataset->GetTriggerofSmp(smp))].buffer[ch].len == 0);
	    }
	    if (smp >= 0 && !ZS && (dataset->event.waveform[ch].y[smp]-Lwidthval)*scale <= LV2)
	      llw = dataset->data[ch].peakloc - smp;
	    smp = dataset->data[ch].peakloc;
	    ZS = (dataset->event.trigger[(dataset->GetTriggerofSmp(smp))].buffer[ch].len == 0);
	    while ((((dataset->event.waveform[ch].y[smp]-Lwidthval)*scale > LV1 && !ZS)
		    || ZS) && smp < dataset->event.fullwave_size) {
	      smp++;
	      if (smp < dataset->event.fullwave_size)
		ZS = (dataset->event.trigger[(dataset->GetTriggerofSmp(smp))].buffer[ch].len == 0);
	    }
	    if (smp < dataset->event.fullwave_size && !ZS && (dataset->event.waveform[ch].y[smp]-Lwidthval)*scale <= LV1)
	      rw = smp - dataset->data[ch].peakloc;
	    while ((((dataset->event.waveform[ch].y[smp]-Lwidthval)*scale > LV2 && !ZS)
		    || ZS) && smp < dataset->event.fullwave_size) {
	      smp++;
	      if (smp < dataset->event.fullwave_size)
		ZS = (dataset->event.trigger[(dataset->GetTriggerofSmp(smp))].buffer[ch].len == 0);
	    }
	    if (smp < dataset->event.fullwave_size && !ZS && (dataset->event.waveform[ch].y[smp]-Lwidthval)*scale <= LV2)
	      rrw = smp - dataset->data[ch].peakloc;
	  }
	  pev[ev].ch[ch].lw = lw;
	  pev[ev].ch[ch].llw = llw;
	  pev[ev].ch[ch].rw = rw;
	  pev[ev].ch[ch].rrw = rrw;
	  //-----Fill shape hists
	  if (pulser.ispulser) {
	    if (lw != -1)
	      Pwid[ch].lw->Fill(lw);
	    if (llw != -1)
	      Pwid[ch].llw->Fill(llw);
	    if (rw != -1)
	      Pwid[ch].rw->Fill(rw);
	    if (rrw != -1)
	      Pwid[ch].rrw->Fill(rrw);
	  }
	  else {
	    if (lw != -1)
	      Dwid[ch].lw->Fill(lw);
	    if (llw != -1)
	      Dwid[ch].llw->Fill(llw);
	    if (rw != -1)
	      Dwid[ch].rw->Fill(rw);
	    if (rrw != -1)
	      Dwid[ch].rrw->Fill(rrw);
	  }
	  gROOT->cd();
	} // peak - lwid > 300
      }//end if good pulse
    }//end loop through ch
    printf("Working........................  %d/%d     (%d \%) \r",ev,NumEvents,(int)TMath::Nint(100*(double)ev/NumEvents));
    fflush(stdout);
    ev++; 
  }while(ev < NumEvents);
}

/*************************************************************************/
//                            CalcWave
/*************************************************************************/
void CalcWave(Raw* dataset) {
  /*************************************************************************/
  //Open output file, replace with desired file/structure
  /*************************************************************************/
  TFile* dFile = 0;
  TString filename = Form("Avg%05d.root",r->run.num);
  TString name;
  if (r->run.num < 851)
    name = PATH1;
  else if (r->run.num < 1419)
    name = PATH2;
  else if (r->run.num < 2090)
    name = PATH3;
  else
    name = PATH4;
  name+= "Averages/";
  name+= filename;
  dFile = new TFile(name,"RECREATE");
  dFile->cd();

  //-----Loop through waveforms
  int NumEvents = dataset->NumEvents;
  cout << "Processing " << NumEvents << " events" << endl;  
  int ev = 0;
  do {
    PFile->cd();
    PTree->GetEntry(ev);
    gROOT->cd();
    if (ev > 0)
      dataset->GetNextEvent();
    for (int ch=0;ch<N_ADC;ch++) {
      double peak = pev[ev].ch[ch].peak;
      double peakloc = pev[ev].ch[ch].peakloc;
      double Lwidthval = pev[ev].ch[ch].lwid;
      double lw = pev[ev].ch[ch].lw;
      double llw = pev[ev].ch[ch].llw;
      double rw = pev[ev].ch[ch].rw;
      double rrw = pev[ev].ch[ch].rrw;
      double LW, LLW, RW, RRW;
      double LWs, LLWs, RWs, RRWs;
      dFile->cd();
      if (pulser.ispulser) {
	LW = Pwid[ch].lw->GetMean();
	LWs = Pwid[ch].lw->GetRMS();
	LLW = Pwid[ch].llw->GetMean();
	LLWs = Pwid[ch].llw->GetRMS();
	RW = Pwid[ch].rw->GetMean();
	RWs = Pwid[ch].rw->GetRMS();
	RRW = Pwid[ch].rrw->GetMean();
	RRWs = Pwid[ch].rrw->GetRMS();	
      }
      else {
	LW = Dwid[ch].lw->GetMean();
	LWs = Dwid[ch].lw->GetRMS();
	LLW = Dwid[ch].llw->GetMean();
	LLWs = Dwid[ch].llw->GetRMS();
	RW = Dwid[ch].rw->GetMean();
	RWs = Dwid[ch].rw->GetRMS();
	RRW = Dwid[ch].rrw->GetMean();
	RRWs = Dwid[ch].rrw->GetRMS();	
      } 
      //-----Reject pileup
      bool pileup = false;
      if (lw < LW - 5*LWs || lw > LW + 5*LWs ||
	  llw < LLW - 5*LLWs || llw > LLW + 5*LLWs ||
	  rw < RW - 5*RWs || rw > RW + 5*RWs ||
	  rrw < RRW - 5*RRWs || rrw > RRW + 5*RRWs)
	pileup = true;
      if (peak != -1 && Lwidthval != -1 && !pileup && (peak - Lwidthval > 300)) {
	double scale = 1.0/(peak - Lwidthval);
	for (int smp=0;smp<dataset->event.fullwave_size;smp++) {
	  int bin = (int)TMath::Nint(((double)smp - peakloc));
	  double val = ((double)dataset->event.waveform[ch].y[smp]-Lwidthval)*scale;
	  if (dataset->event.trigger[(dataset->GetTriggerofSmp(smp))].buffer[ch].len > 0
	      && bin+NBINS/2 >= 0 && bin+NBINS/2 < NBINS) {
	    if (WRITE)
	      dFile->cd();
	    if (pulser.ispulser) {
	      pshape[ch].y[bin+NBINS/2] += val;
	      pshape[ch].cnt[bin+NBINS/2]++;
	    }
	    else {
	      int a = (int)TMath::Floor(peak/1000.);
	      if (NAMP == 1)
		a = 0;
	      if (a >= 0 && a < NAMP) {
		damp[a].dshape[ch].y[bin+NBINS/2] += val;
		damp[a].dshape[ch].cnt[bin+NBINS/2]++;
	      }
	    }
	  }
	}//end loop through smp
      }//end if peak/Lwidthval ok
    }//end loop through ch   
    printf("Working........................  %d/%d     (%d \%) \r",ev,NumEvents,(int)TMath::Nint(100*(double)ev/NumEvents));
    fflush(stdout);
    ev++; 
  }while(ev < NumEvents);
  
  //-----Calculate Average
  for (int ch=0;ch<N_ADC;ch++)
    for (int num=0;num<NBINS;num++) {
      if (pshape[ch].cnt[num] > 0)
	pshape[ch].y[num] = pshape[ch].y[num]/pshape[ch].cnt[num];
      for (int a=0;a<NAMP;a++) {
	if (damp[a].dshape[ch].cnt[num] > 0)
	  damp[a].dshape[ch].y[num] = damp[a].dshape[ch].y[num]/damp[a].dshape[ch].cnt[num];
      }
    }
  //-----Write to file
  if (WRITE) {
    dFile->cd();
    TGraph* gAv;
    for (int a=0;a<NAMP;a++) {
      for (int ch=0;ch<N_ADC;ch++) {
	gAv = new TGraph(NBINS,damp[a].dshape[ch].x,damp[a].dshape[ch].y);
	TString name = "data";
	name += ch;
	if (NAMP > 1) {
	  name += "amp";
	  name += a;
	}
	gAv->SetName(name);
	gAv->SetMarkerStyle(1);
	gAv->Write();
      }
    }
    for (int ch=0;ch<N_ADC;ch++) {
      gAv = new TGraph(NBINS,pshape[ch].x,pshape[ch].y);
      TString name = "pulser";
      name += ch;
      gAv->SetName(name);
      gAv->SetMarkerStyle(1);
      gAv->Write();
    }
    for (int ch=0;ch<N_ADC;ch++) {
      Dwid[ch].lw->Write();
      Dwid[ch].llw->Write();
      Dwid[ch].rw->Write();
      Dwid[ch].rrw->Write();
    }
    for (int ch=0;ch<N_ADC;ch++) {
      Pwid[ch].lw->Write();
      Pwid[ch].llw->Write();
      Pwid[ch].rw->Write();
      Pwid[ch].rrw->Write();
    }
    dFile->Write();
    dFile->Close();
  }    
}

/*************************************************************************/
//                            FitPeak
/*************************************************************************/
bool FitPeak(Raw* dataset, int ch, double& peak, double& peakloc){
  double x[FITPOINTS], y[FITPOINTS], a[FITPOLY];
  int start = dataset->data[ch].peakloc - FITPOINTS/2;
  int stop = start + FITPOINTS;
  if (start > 0 && stop < dataset->event.fullwave_size) {
    bool ZS = false;
    for (int i=0;i<FITPOINTS;i++) {
      int bin = start + i;
      x[i] = bin;
      y[i] = dataset->event.waveform[ch].y[bin];
      if (dataset->event.waveform[ch].y[i] == MAXADC || 
	  dataset->event.trigger[(dataset->GetTriggerofSmp(i))].buffer[ch].len == 0) {
	y[i] = -1;
	ZS = true;
      }
    }
    if (!ZS) {
      if (gPeak !=0)
	delete gPeak;
      gPeak = new TGraph(FITPOINTS,x,y);
      gPeak->SetName("gPeak");
      gPeak->SetMarkerStyle(20);
      gPeak->LeastSquareFit(FITPOLY,a);
      if (fPeak !=0)
	delete fPeak;
      fPeak = new TF1("fPeak","pol4",start,start+FITPOINTS);
      fPeak->SetParameters(a);
      peakloc = fPeak->GetMaximumX(start,start+FITPOINTS);
      peak = fPeak->Eval(peakloc);
    }
  }
}
 
/*************************************************************************/
//                            FitLoc
/*************************************************************************/
double FitLoc(Raw* dataset, int ch, double loc){
  double x[FITPOINTS], y[FITPOINTS], a[FITPOLY];
  int start = (int)TMath::Nint(loc) - FITPOINTS/2;
  int stop = start + FITPOINTS;
  if (start > 0 && stop < dataset->event.fullwave_size) {
    bool ZS = false;
    for (int i=0;i<FITPOINTS;i++) {
      int bin = start + i;
      x[i] = bin;
      y[i] = dataset->event.waveform[ch].y[bin];
      if (dataset->event.waveform[ch].y[i] == MAXADC || 
	  dataset->event.trigger[(dataset->GetTriggerofSmp(i))].buffer[ch].len == 0) {
	y[i] = -1;
	ZS = true;
      }
    }
    if (!ZS) {
      if (gPeak !=0)
	delete gPeak;
      gPeak = new TGraph(FITPOINTS,x,y);
      gPeak->SetName("gPeak");
      gPeak->SetMarkerStyle(20);
      gPeak->LeastSquareFit(FITPOLY,a);
      if (fPeak !=0)
	delete fPeak;
      fPeak = new TF1("fPeak","pol4",start,start+FITPOINTS);
      fPeak->SetParameters(a);
      return fPeak->Eval(loc);
    }
  }
  return -1;
}



