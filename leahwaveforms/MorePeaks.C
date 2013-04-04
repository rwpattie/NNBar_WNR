// File: MorePeaks.C
// Name: Leah Broussard
// Date: 2013/4/4
// Purpose: Fits waveforms to better extract energy and timing
//
// CloseRun:        Closes all input files for active run
// OpenAvg:         Opens the average pulse shape file and builds fit function
// OpenWave:        Opens waveform statistics file
// OpenPeak:        Opens first pass energy/timing extractor
// OpenPulser:      Opens file with pulser ID
// OpenFixPulser:   Opens file with fixed pulser timing
// MorePeaks:       Fit waveforms that need it
// WriteToFile:     Write TTree MoreTree to file
// datafunction:    Fit function for real events
// pulserfunction:  Fit function for pulser events
// TwoPeak:         Fit function for piled up event
// BadStart:        Fit function for leftover tail of previous event
//
// Revision History:
// 2013/4/4:  LJB         File created based on cleaned up Ne19 version




#include "../Raw/Raw.h"
#include "TPluginManager.h"
#include <TApplication.h>
#define PEAKRES 100
#define VALLEYEND 20

//Average pulse shape
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
TF1* twof;
TF1* badf;
//Pulser ID
TFile* PFile;
TTree* PTree;
struct p_t{
  double time;
  double pulser;
  int prev;
  int next;
  bool ispulser;
};
p_t pulsertree;
//Pulser ID with pileup fix
TFile* FFile;
TTree* FTree;
struct fix_t{
  double tprev;
  double tnext;
  double tprev2;
  double tnext2;
  double fixpulser;
};
fix_t fixtree;
//Waveform info
TFile* WFile;
TTree* WTree;
struct w_t{
  bool badstart[N_ADC];  
  bool badstop[N_ADC];
  bool startZS[N_ADC];
  bool stopZS[N_ADC];
  unsigned int smpbadstart[N_ADC];
  unsigned int smppulsestart[N_ADC];
  unsigned int smppulsestop[N_ADC];
  unsigned int smpbadstop[N_ADC];
  unsigned int length[N_ADC];
  unsigned int firstsmp[N_ADC];
  unsigned int lastsmp[N_ADC];
  unsigned int firstval[N_ADC];
  unsigned int lastval[N_ADC];
  float first0[N_ADC];
  float first1[N_ADC];
  float last0[N_ADC];
  float last1[N_ADC];
};
w_t wavetree;
//First pass peakfinder
TFile* SPFile;
TTree* SPTree;
struct pk_t{
  double peak;
  double peakloc;
  double Lwid;
  int valley;  //location of valley after peak
  bool simple;
};
struct pkch_t{
  pk_t ch[N_ADC];
};
pkch_t peaktree;
//Output peakfinder
struct mpk_t{
  double* peak;
  double* peakloc;
  double* Lwid;
  double* valley;
  double* valleyloc;
  double* chiperndf;
  int num;
};
struct mpkch_t{
  mpk_t ch[N_ADC];
};
mpkch_t* moretree;
struct tree_t{//branch each ch
  mpk_t ch[N_ADC];
};
tree_t writetree;
//Raw waveform data
unsigned int NumEvents;
Raw* raw;


void CloseRun();
bool OpenWave(int num);
bool OpenPeak(int num);
bool OpenPulser(int num);
bool OpenFixPulser(int num);
bool OpenAvg(int num);
void MorePeaks();
void WriteToFile();
double datafunction(double* x, double* par);
double pulserfunction(double* x, double* par);
double TwoPeak(double* x, double* par);
double BadStart(double* x, double* par);
Double_t Interpolate(TGraph* g, Double_t x);

/*************************************************************************/
//                            Main function
/*************************************************************************/
int main(int argc, char* argv[])
{
  TApplication theApp("App",0,0);
  if (argc!=3) {
    cout << "Need an argument"<<endl;
    return 0;
  }
  int run1 = atoi(argv[1]);
  int run2 = atoi(argv[2]);
  if ((run1<0)||(run1>99999))
    return 0;
  if ((run2<0)||(run2>99999))
    return 0;
  //-----Initialize output data structures
  moretree = 0;
  for (unsigned int ch=0;ch<N_ADC;ch++) {
    writetree.ch[ch].num = 0;
    writetree.ch[ch].peak = new double[20];
    writetree.ch[ch].peakloc = new double[20];
    writetree.ch[ch].Lwid = new double[20];
    writetree.ch[ch].valley = new double[20];
    writetree.ch[ch].valleyloc = new double[20];
    writetree.ch[ch].chiperndf = new double[20];
  }
  /*************************************************************************/
  //Open waveform and helpers, replace with version applicable to your files
  /*************************************************************************/
  raw = new Raw(run1, false);
  for (int run = run1; run <= run2; run++) {
    if (run != 977) {
      cout << "Run " << run << endl;
      CloseRun();
      raw->Open(run,true);
      if (raw->run.num != 0 && OpenAvg(run) && OpenWave(run) && OpenPeak(run) && OpenPulser(run) && OpenFixPulser(run)) {
	MorePeaks();
	WriteToFile();
      }
    }
  }
  //-----Cleanup
  cout << "Done" << endl;
  for (unsigned int ch=0;ch<N_ADC;ch++) {
    delete [] writetree.ch[ch].peak;
    delete [] writetree.ch[ch].peakloc;
    delete [] writetree.ch[ch].Lwid;
    delete [] writetree.ch[ch].valley;
    delete [] writetree.ch[ch].valleyloc;
    delete [] writetree.ch[ch].chiperndf;
  }
  //-----For graph plotting
  theApp.Run();
  return 0;
}

/*************************************************************************/
//                              CloseRun
/*************************************************************************/
void CloseRun() {
  if (AFile!=0) {
    if (AFile->IsOpen())
      AFile->Close();
    delete AFile;
  }
  AFile = 0;
  PTree = 0;
  if (PFile!=0) {
    if (PFile->IsOpen())
      PFile->Close();
    delete PFile;
  }
  PFile = 0;
  FTree = 0;
  if (FFile!=0) {
    if (FFile->IsOpen())
      FFile->Close();
    delete FFile;
  }
  FFile = 0;
  WTree = 0;
  if (WFile!=0) {
    if (WFile->IsOpen())
      WFile->Close();
    delete WFile;
  }
  WFile = 0;
  SPTree = 0;
  if (SPFile!=0) {
    if (SPFile->IsOpen())
      SPFile->Close();
    delete SPFile;
  }
  SPFile = 0;

}

/*************************************************************************/
//                              OpenAvg
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
    CloseRun();
    return false;
  }
  AFile = new TFile(name);
  if (!AFile->IsOpen()) {
    CloseRun();
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
			      250,    //peak location
			      ch);    //channel
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
    pulser[ch].f->FixParameter(3,ch);
    pulser[ch].f->SetParameters(0,      //background
				9000,   //amplitude
				250,    //peak location
				ch);    //channel   
  }
  twof = new TF1("twof",TwoPeak,0,20000,6);
  badf = new TF1("badf",BadStart,0,20000,6);
  delete fpol0;
  delete fpolexpo;
  delete fpol1;
  return true;
}

/*************************************************************************/
//                             OpenWave
/*************************************************************************/
bool OpenWave(int num) {
  TString filename = Form("Waveform%05d.root",num);
  TString name;
  if (num < 851)
    name = PATH1;
  else if (num < 1419)
    name = PATH2;
  else if (num < 2090)
    name = PATH3;
  else
    name = PATH4;
  name+= "Info/";
  name+= filename;
  if (gSystem->AccessPathName(name)) {
    cout << name << " not found " << endl;
    CloseRun();
    return false;
  }
  WFile = new TFile(name);
  if (!WFile->IsOpen()) {
    cout << name << " failed to open." << endl;
    CloseRun();
    return false;
  }
  WTree = (TTree*)gDirectory->Get("wave");
  if (WTree == 0) {
    cout << name << " tree not found." << endl;
    CloseRun();
    return false;
  }
  else {
    WTree->SetBranchStatus("*",1);
    WTree->SetBranchAddress("wave",&wavetree.badstart[0]);
  }
  return true;
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
  name+= "Info/";
  name+= filename;
  if (gSystem->AccessPathName(name)) {
    cout << name << " not found " << endl;
    CloseRun();
    return false;
  }
  PFile = new TFile(name);
  if (!PFile->IsOpen()) {
    cout << name << " failed to open." << endl;
    CloseRun();
    return false;
  }
  PTree = (TTree*)gDirectory->Get("pulser");
  if (PTree == 0) {
    cout << name << " tree not found." << endl;
    CloseRun();
    return false;
  }
  else {
    PTree->SetBranchStatus("*",1);
    PTree->SetBranchAddress("pulser",&pulsertree.time);
  }
  return true;
}

/*************************************************************************/
//                           OpenFixPulser
/*************************************************************************/
bool OpenFixPulser(int num) {
  TString filename = Form("FixPulser%05d.root",num);
  TString name;
  if (num < 851)
    name = PATH1;
  else if (num < 1419)
    name = PATH2;
  else if (num < 2090)
    name = PATH3;
  else
    name = PATH4;
  name+= "Info/";
  name+= filename;
  if (gSystem->AccessPathName(name)) {
    cout << name << " not found " << endl;
    CloseRun();
    return false;
  }
  FFile = new TFile(name);
  if (!FFile->IsOpen()) {
    cout << name << " failed to open." << endl;
    CloseRun();
    return false;
  }
  FTree = (TTree*)gDirectory->Get("pulser2");
  if (FTree == 0) {
    cout << name << " tree not found." << endl;
    CloseRun();
    return false;
  }
  else {
    FTree->SetBranchStatus("*",1);
    FTree->SetBranchAddress("pulser2",&fixtree.tprev);
  }
  return true;
}

/*************************************************************************/
//                              OpenPeak
/*************************************************************************/
bool OpenPeak(int num) {
  TString filename = Form("Peaks%05d.root",num);
  TString name;
  if (num < 851)
    name = PATH1;
  else if (num < 1419)
    name = PATH2;
  else if (num < 2090)
    name = PATH3;
  else
    name = PATH4;
  name+= "Info/";
  name+= filename;
  if (gSystem->AccessPathName(name)) {
    cout << name << " not found " << endl;
    CloseRun();
    return false;
  }
  SPFile = new TFile(name);
  if (!SPFile->IsOpen()) {
    cout << name << " failed to open." << endl;
    CloseRun();
    return false;
  }
  SPTree = (TTree*)gDirectory->Get("peak");
  if (SPTree == 0) {
    cout << name << " tree not found." << endl;
    CloseRun();
    return false;
  }
  else {
    SPTree->SetBranchStatus("*",1);
    for (int ch=0;ch<N_ADC;ch++) {
      TString branchname = "ch";
      branchname += ch;
      SPTree->SetBranchAddress(branchname,&peaktree.ch[ch].peak);
    }
    return true;
  }
}

/*************************************************************************/
//                             MorePeaks
/*************************************************************************/
void MorePeaks(){
  PFile->cd();
  NumEvents = PTree->GetEntries();
  //-----Initialize tree
  if (moretree != 0)
    delete [] moretree;
  moretree = new mpkch_t[NumEvents];
  //-----Loop through waveforms
  unsigned int ev = 0;
  do {
    PFile->cd();
    PTree->GetEntry(ev);
    FFile->cd();
    FTree->GetEntry(ev);
    WFile->cd();
    WTree->GetEntry(ev);
    SPFile->cd();
    SPTree->GetEntry(ev);
    raw->GetEvent(ev);
    double peak[20];
    double peakloc[20];
    double Lwid[20];
    double valley[20];
    double valleyloc[20];
    double chiperndf[20];
    double lastvalid = raw->event.fullwave_size;
    //-----Turn off DAQ if muon
    if (raw->event.overflow) {
      for (unsigned int ch=0;ch<N_ADC;ch++) {
	if (raw->HasCh(ch,raw->event.chmask) && 
	    raw->event.overflowstart[ch] != -1) {
	  if (lastvalid > raw->event.overflowstart[ch])
	    lastvalid = raw->event.overflowstart[ch];
	}
      }
    }
    //-----Initialize event
    for (unsigned int ch=0;ch<N_ADC;ch++) {
      moretree[ev].ch[ch].peak = 0;
      moretree[ev].ch[ch].peakloc = 0;
      moretree[ev].ch[ch].Lwid = 0;
      moretree[ev].ch[ch].valley = 0;
      moretree[ev].ch[ch].valleyloc = 0;
      moretree[ev].ch[ch].chiperndf = 0;
      moretree[ev].ch[ch].num = 0;
      for (int i=0;i<20;i++) {
	peak[i] = -1;
	peakloc[i] = -1;
	Lwid[i] = -1;
	valley[i] = -1;
	valleyloc[i] = -1;
	chiperndf[i] = -1;
      }
      //-----Handle non-simple waveform
      if (raw->HasCh(ch,raw->event.chmask) && 
	  (!peaktree.ch[ch].simple || wavetree.badstart[ch]
	   || TMath::Abs(peaktree.ch[ch].peakloc - raw->data[ch].peakloc) > 15
	   || ((int)wavetree.lastsmp[ch] - peaktree.ch[ch].valley > VALLEYEND  &&  peaktree.ch[ch].valley != -1))) {//check value
	int smp = wavetree.smppulsestart[ch];
	if (wavetree.badstart[ch])
	  smp = 0;
	if (wavetree.firstsmp[ch] > wavetree.smppulsestart[ch])
	  smp = wavetree.firstsmp[ch];
	int cnt = 0;
	if (lastvalid > 0) {
	  peak[cnt] = raw->event.fwave[ch].y[smp];
	  peakloc[cnt] = smp;
	  bool peakfound = true;
	  bool valleyfound = true;
	  double prevpeak = -1;
	  double prevpeakloc = -1;
	  int prevcnt = -1;
	  //-----Loop over possible peaks in waveform
	  while (peakfound && valleyfound) {
	    peakfound = false;
	    //-----Find next local maximum
	    do {
	      if (peak[cnt] <= raw->event.fwave[ch].y[smp]) {
		peak[cnt] = raw->event.fwave[ch].y[smp];
		peakloc[cnt] = smp;
		smp++;
	      }
	      else {
		peakfound = true;
		int c = 0;
		while (peakfound && c < PEAKRES && smp+c < (int)lastvalid && smp+c <= (int)wavetree.lastsmp[ch]) {
		  if (peak[cnt] <= raw->event.fwave[ch].y[smp+c]) {
		    peakfound = false;
		    smp = smp+c;
		    peak[cnt] = raw->event.fwave[ch].y[smp];
		    peakloc[cnt] = smp;
		    smp++;
		  }
		  else
		    c++;
		}
	      }
	    }while (!peakfound && smp <= (int)lastvalid && smp <= (int)wavetree.lastsmp[ch]);
	    //-----Terminate if muon event and no peaks found before muon
	    if (!peakfound && raw->event.overflow) {
	      peak[cnt] = MAXADC;
	      peakloc[cnt] = lastvalid;
	      peakfound = false;
	      valleyfound = false;
	      cnt++;
	    }
	    else {
	      valleyfound = false;
	      if (peakfound || raw->event.overflow) {
		Lwid[cnt] = -1;
		if (peakloc[cnt]-LWIDTH > 0)
		  Lwid[cnt] = raw->event.fwave[ch].y[(int)peakloc[cnt]-LWIDTH];
		if (Lwid[cnt] > peak[cnt])
		  Lwid[cnt] = -1;
		//-----Find next local minimum
		valleyloc[cnt] = peakloc[cnt];
		valley[cnt] = peak[cnt];
		smp = peakloc[cnt] + VALLEYEND;
		do {
		  if (valley[cnt] > raw->event.fwave[ch].y[smp]) {
		    valley[cnt] = raw->event.fwave[ch].y[smp];
		    valleyloc[cnt] = smp;
		    smp++;
		  }
		  else {
		    valleyfound = true;
		    int c = 0;
		    while (valleyfound && c < PEAKRES && smp+c < (int)lastvalid && smp+c <= (int)wavetree.lastsmp[ch]) {
		      if (valley[cnt] > raw->event.fwave[ch].y[smp+c]) {
			valleyfound = false;
			smp = smp+c;
			valley[cnt] = raw->event.fwave[ch].y[smp];
			valleyloc[cnt] = smp;
			smp++;
		      }
		      else
			c++;
		    }
		  }
		}while (!valleyfound && smp < (int)lastvalid && smp <= (int)wavetree.lastsmp[ch]);
		if ((int)wavetree.lastsmp[ch] - valleyloc[cnt] <= VALLEYEND)
		  valleyfound = false;
		if (!valleyfound) {
		  valley[cnt] = -1;
		  valleyloc[cnt] = -1;
		}
		else {
		  smp = valleyloc[cnt] + VALLEYEND;
		}
	      }// peakfound || overflow
	      peakfound = true;
	      //-----Perform fit
	      TF1* fitf = data[ch].f;
	      if (pulsertree.ispulser && TMath::Abs((fixtree.fixpulser - pulsertree.time)*1.e8 - peakloc[cnt]) < 100)
		fitf = pulser[ch].f;
	      if (prevpeak == -1) {
		//-----Handle if tail of pulse from another waveform in this waveform
		if (wavetree.badstart[ch]) {
		  fitf = badf;
		  double line0 = wavetree.firstval[ch] - 250;
		  double line1 = data[ch].fall1 * line0;
		  double thispeak = peak[cnt]-250-(line0 + line1*peakloc[cnt]);
		  fitf->SetParameters(250,thispeak,peakloc[cnt],ch,line0,line1);
		  fitf->FixParameter(3,ch);
		}
		//-----Handle normal event
		else {
		  fitf->SetParameters(250,peak[cnt]-250,peakloc[cnt],ch);
		  fitf->FixParameter(3,ch);
		}
	      }//end no previous peak
	      else {
		//-----Handle piled up event
		data[ch].f->SetParameters(250,prevpeak,prevpeakloc,ch);
		double thispeak = peak[cnt] - data[ch].f->Eval(peakloc[cnt]);
		if (thispeak < 0)
		  thispeak = 50;
		fitf = twof;
		fitf->SetParameters(250,prevpeak,prevpeakloc,thispeak,peakloc[cnt],ch);
		fitf->FixParameter(5,ch);
	      }
	      //-----Determine fit range
	      double start = peakloc[cnt]-100;
	      if (wavetree.badstart[ch] && prevpeak == -1) {
		start = 0;
	      }
	      if (prevpeak != -1) {
		start = (peakloc[prevcnt]*3 + valleyloc[prevcnt])/4.;
	      }
	      double stop = peakloc[cnt]+150;
	      if (valleyfound) { 
		if ((peakloc[cnt] + valleyloc[cnt])/2. < stop)
		  stop = (peakloc[cnt] + valleyloc[cnt])/2.;
	      }
	      fitf->SetRange(start,stop);
	      //-----Fit pulses and store results
	      raw->EventGraph[ch]->Fit(fitf,"WQN","",start,stop);
	      if (prevpeak == -1) {
		peakloc[cnt] = fitf->GetParameter(2);
		peak[cnt] = fitf->Eval(peakloc[cnt]);
		Lwid[cnt] = fitf->Eval(peakloc[cnt]-LWIDTH);
		if (wavetree.badstart[ch]) {
		  double line0 = fitf->GetParameter(4);
		  double line1 = fitf->GetParameter(5);
		  peak[cnt] = peak[cnt] - (line0 + line1*peakloc[cnt]);
		  Lwid[cnt] = Lwid[cnt] - (line0 + line1*(peakloc[cnt]-LWIDTH));
		}
	      }
	      else {
		double lastbkg = fitf->GetParameter(0);
		double lastamp = fitf->GetParameter(1);
		double lastmean = fitf->GetParameter(2);
		data[ch].f->SetParameters(lastbkg,lastamp,lastmean,ch);
		peakloc[cnt] = fitf->GetParameter(4);
		peak[cnt] = fitf->Eval(peakloc[cnt]) - data[ch].f->Eval(peakloc[cnt]) + lastbkg; 
		Lwid[cnt] = fitf->Eval(peakloc[cnt]-LWIDTH) - data[ch].f->Eval(peakloc[cnt]-LWIDTH) + lastbkg;
	      }
	      //-----Found wiggle, probably not a real pulse
	      if (peak[cnt] - fitf->GetParameter(0) > 50) {
		prevpeak = fitf->Eval(peakloc[cnt]) - fitf->GetParameter(0);
		prevpeakloc = peakloc[cnt];
		prevcnt = cnt;
	      }
	      chiperndf[cnt] = fitf->GetChisquare()/fitf->GetNDF();
	      //-----Reset if at end of waveform
	      if (peakloc[cnt] - (int)wavetree.lastsmp[ch] > VALLEYEND) {
		peakloc[cnt] = -1;
		peak[cnt] = -1;
		Lwid[cnt] = -1;
		valleyloc[cnt] = -1;
		valley[cnt] = -1;
		chiperndf[cnt] = -1;
		valleyfound = false;
		peakfound = false;
	      }
	      if (peakfound) {
		cnt++;
		if (cnt > 20) {
		  cout << "************** ERROR" << endl;
		  cout << "Too many peaks found" << endl;
		  return;
		}
	      }
	    }//end not terminated at overflow
	  }//end loop through pile-ups
	}//end lastvalid > 0
	//-----Store found pulses into tree
	if (cnt > 0) {
	  moretree[ev].ch[ch].num = cnt;
	  moretree[ev].ch[ch].peak = new double[cnt];
	  moretree[ev].ch[ch].peakloc = new double[cnt];
	  moretree[ev].ch[ch].Lwid = new double[cnt];
	  moretree[ev].ch[ch].valley = new double[cnt];
	  moretree[ev].ch[ch].valleyloc = new double[cnt];
	  moretree[ev].ch[ch].chiperndf = new double[cnt];
	  for (int c=0;c<cnt;c++) {
	    moretree[ev].ch[ch].peak[c] = peak[c];
	    moretree[ev].ch[ch].peakloc[c] = peakloc[c];
	    moretree[ev].ch[ch].Lwid[c] = Lwid[c];
	    moretree[ev].ch[ch].valley[c] = valley[c];
	    moretree[ev].ch[ch].valleyloc[c] = valleyloc[c];
	    moretree[ev].ch[ch].chiperndf[c] = chiperndf[c];
	  }
	}
      }//end treatable event
    }//end loop over ch
    ev++;
    printf("Working...  %d \% \r",(int)TMath::Nint(100*(double)ev/NumEvents));
    fflush(stdout);
  }while (ev < NumEvents);
}
 
/*************************************************************************/
//                            WriteToFile
/*************************************************************************/
void WriteToFile() {
  TString filename = Form("MorePeaks%05d.root",raw->run.num);
  TString name;
  if (raw->run.num < 851)
    name = PATH1;
  else if (raw->run.num < 1419)
    name = PATH2;
  else if (raw->run.num < 2090)
    name = PATH3;
  else
    name = PATH4;
  name+= "Info/";
  name+= filename;
  cout << "Writing to " << name << endl;
  TFile* SFile = new TFile(name,"RECREATE");
  TTree* STree = new TTree("more","MorePeaks");
  for (int ch=0;ch<N_ADC;ch++) {
    TString branchname = "num";
    branchname += ch;
    TString numname = "num";
    numname += ch;
    TString leafname = numname;
    leafname += "/I";
    STree->Branch(branchname,&writetree.ch[ch].num,leafname);
    branchname = "peak";
    branchname += ch;
    leafname = "peak["; leafname += numname; leafname += "]/D";
    STree->Branch(branchname,&writetree.ch[ch].peak[0],leafname);
    branchname = "peakloc";
    branchname += ch;
    leafname = "peakloc[";leafname += numname; leafname += "]/D";
    STree->Branch(branchname,&writetree.ch[ch].peakloc[0],leafname);
    branchname = "Lwid";
    branchname += ch;
    leafname = "Lwid["; leafname += numname; leafname += "]/D";
    STree->Branch(branchname,&writetree.ch[ch].Lwid[0],leafname);
    branchname = "valley";
    branchname += ch;
    leafname = "valley["; leafname += numname; leafname += "]/D";
    STree->Branch(branchname,&writetree.ch[ch].valley[0],leafname);
    branchname = "valleyloc";
    branchname += ch;
    leafname = "valleyloc["; leafname += numname; leafname += "]/D";
    STree->Branch(branchname,&writetree.ch[ch].valleyloc[0],leafname);
    branchname = "chiperndf";
    branchname += ch;
    leafname = "chiperndf["; leafname += numname; leafname += "]/D";
    STree->Branch(branchname,&writetree.ch[ch].chiperndf[0],leafname);
  }
  for (unsigned int ev=0;ev<NumEvents;ev++) {
    for (unsigned int ch=0;ch<N_ADC;ch++) {
      writetree.ch[ch].peak[0] = -1;
      writetree.ch[ch].peakloc[0] = -1;
      writetree.ch[ch].Lwid[0] = -1;
      writetree.ch[ch].valley[0] = -1;
      writetree.ch[ch].valleyloc[0] = -1;
      writetree.ch[ch].chiperndf[0] = -1;
      writetree.ch[ch].num = moretree[ev].ch[ch].num;
      for (int c=0;c<moretree[ev].ch[ch].num;c++) {
	writetree.ch[ch].peak[c] = moretree[ev].ch[ch].peak[c];
	writetree.ch[ch].peakloc[c] = moretree[ev].ch[ch].peakloc[c];
	writetree.ch[ch].Lwid[c] = moretree[ev].ch[ch].Lwid[c];
	writetree.ch[ch].valley[c] = moretree[ev].ch[ch].valley[c];
	writetree.ch[ch].valleyloc[c] = moretree[ev].ch[ch].valleyloc[c];
	writetree.ch[ch].chiperndf[c] = moretree[ev].ch[ch].chiperndf[c];
      }
      delete [] moretree[ev].ch[ch].peak;
      delete [] moretree[ev].ch[ch].peakloc;
      delete [] moretree[ev].ch[ch].Lwid;
      delete [] moretree[ev].ch[ch].valley;
      delete [] moretree[ev].ch[ch].valleyloc;
      delete [] moretree[ev].ch[ch].chiperndf;
    }
    STree->Fill();
  }

  SFile->Write();
  SFile->Close();
}

/*************************************************************************/
//                            datafunction
/*************************************************************************/
double datafunction(double* x, double* par) {
  double xval = *x;
  double bkg = par[0];
  double amp = par[1];
  if (amp < 0)
    return bkg;
  double mean = par[2];
  int xint = (int)TMath::Floor(xval);
  if (xint > 0 && xint < raw->event.fullwave_size)
    if (raw->event.waveform[(int)par[3]].y[xint] == 0 
	|| raw->event.waveform[(int)par[3]].y[xint] == MAXADC
	|| raw->event.waveform[(int)par[3]].y[xint+1] == 0 
	|| raw->event.waveform[(int)par[3]].y[xint+1] == MAXADC) {
      TF1::RejectPoint();
    }
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
//                           pulserfunction
/*************************************************************************/
double pulserfunction(double* x, double* par) {
  double xval = *x;
  double bkg = par[0];
  double amp = par[1];
  if (amp < 0)
    return bkg;
  double mean = par[2];
  int xint = (int)TMath::Floor(xval);
  if (raw->event.waveform[(int)par[3]].y[xint] == 0 
      || raw->event.waveform[(int)par[3]].y[xint] == MAXADC
      || raw->event.waveform[(int)par[3]].y[xint+1] == 0 
      || raw->event.waveform[(int)par[3]].y[xint+1] == MAXADC) {
    TF1::RejectPoint();
  }
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
//                              TwoPeak
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
//                              BadStart
/*************************************************************************/
double BadStart(double* x, double* par) {
  double xval = *x;
  double bkg1 = par[0];
  double amp1 = par[1];
  double mean1 = par[2];
  double ch = par[3];
  double line0 = par[4];
  double line1 = par[5];
  double pars1[] = {bkg1,amp1,mean1,ch};
  double p1 = datafunction(x,pars1);
  double badstart = line0 + line1*xval;
  if (badstart < 0)
    badstart = 0;
  return p1 + badstart;
}

/*************************************************************************/
//                             Interpolate
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
