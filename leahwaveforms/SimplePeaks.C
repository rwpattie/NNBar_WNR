// File: SimplePeaks.C
// Name: Leah Broussard
// Date: 2013/4/5
// Purpose: Extracts energy and timing of simple waveforms
//
// CloseRun:        Closes all input files for active run
// OpenWave:        Opens waveform statistics file
// SimplePeaks:     Finds energy/timing of each simple waveform
// WriteToFile:     Write TTree MoreTree to file
//
// Revision History:
// 2013/4/5:  LJB         File created based on cleaned up Ne19 version


#include "../Raw/Raw.h"
#define PEAKRES 100

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
//Output struct
struct pk_t{
  double peak;
  double peakloc;
  double Lwid;
  int valley;
  bool simple;
};
struct pkch_t{
  pk_t ch[N_ADC];
};
pkch_t* peaktree;
struct tree_t{
  pk_t ch[N_ADC];
};
tree_t writetree;
//Raw waveform data
unsigned int NumEvents;
Raw* raw;

bool OpenWave(int num);
void SimplePeaks();
void WriteToFile();

/*************************************************************************/
//                            Main function
/*************************************************************************/
int main(int argc, char* argv[])
{
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
  peaktree = 0;
  /*************************************************************************/
  //Open waveform and helpers, replace with version applicable to your files
  /*************************************************************************/
  raw = new Raw(run1, false);
  for (int run = run1; run <= run2; run++) {
    if (run != 977) {
      cout << "Run " << run << endl;
      raw->Open(run,true);
      if (raw->run.num != 0 && OpenWave(run)) {
	SimplePeaks();
	WriteToFile();
      }
    }
  }
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
  name+= "Friends/";
  name+= filename;
  if (gSystem->AccessPathName(name)) {
    cout << name << " not found " << endl;
    return false;
  }
  WFile = new TFile(name);
  if (!WFile->IsOpen()) {
    cout << name << " failed to open." << endl;
    return false;
  }
  WTree = (TTree*)gDirectory->Get("wave");
  if (WTree == 0) {
    cout << name << " tree not found." << endl;
    return false;
  }
  else {
    WTree->SetBranchStatus("*",1);
    WTree->SetBranchAddress("wave",&wavetree.badstart[0]);
  }
  return true;
}

/*************************************************************************/
//                            SimplePeaks
/*************************************************************************/
void SimplePeaks(){
  WFile->cd();
  NumEvents = WTree->GetEntries();
  //-----Initialize tree
  if (peaktree != 0)
    delete [] peaktree;
  peaktree = new pkch_t[NumEvents];
  //-----Loop through waveforms
  unsigned int ev = 0;
  do {
    WFile->cd();
    WTree->GetEntry(ev);
    raw->GetEvent(ev);
    for (unsigned int ch=0;ch<N_ADC;ch++) {
      //-----Zero channel
      peaktree[ev].ch[ch].peak = -1;
      peaktree[ev].ch[ch].peakloc = -1;
      peaktree[ev].ch[ch].Lwid = -1;
      peaktree[ev].ch[ch].valley = -1;
      peaktree[ev].ch[ch].simple = true;
      if (raw->HasCh(ch,raw->event.chmask)) {
	//-----Don't handle muons or zero suppressed gaps
	peaktree[ev].ch[ch].simple = false;
	bool HandleThisEvent = true;
	if (raw->event.overflow) {
	  HandleThisEvent = false;
	}
	int tr1 = raw->GetTriggerofSmp(wavetree.firstsmp[ch]);
	int tr2 = raw->GetTriggerofSmp(wavetree.lastsmp[ch]);
	for (int tr = tr1;tr<=tr2;tr++)
	  if (raw->IsTrChZS(tr,ch)) {
	    HandleThisEvent = false;
	  }
	//-----Handle simple waveform
	if (HandleThisEvent){
	  peaktree[ev].ch[ch].simple = true;
	  //-----Find next local maximum
	  int smp = wavetree.smppulsestart[ch];
	  if (wavetree.smppulsestart[ch] < wavetree.firstsmp[ch])
	    smp = wavetree.firstsmp[ch];
	  peaktree[ev].ch[ch].peak = raw->event.waveform[ch].y[smp];
	  peaktree[ev].ch[ch].peakloc = smp;
	  bool peakfound = false;
	  do {
	    if (peaktree[ev].ch[ch].peak <= raw->event.waveform[ch].y[smp]) {
	      peaktree[ev].ch[ch].peak = raw->event.waveform[ch].y[smp];
	      peaktree[ev].ch[ch].peakloc = smp;
	      smp++;
	    }
	    else {
	      peakfound = true;
	      int c = 0;
	      while (peakfound && c < PEAKRES) {
		if (peaktree[ev].ch[ch].peak <= raw->event.waveform[ch].y[smp+c]) {
		  peakfound = false;
		  smp = smp+c;
		  peaktree[ev].ch[ch].peak = raw->event.waveform[ch].y[smp];
		  peaktree[ev].ch[ch].peakloc = smp;		  
		  smp++;
		}
		else
		  c++;
	      }
	    }
	  }while (!peakfound && smp < (int)wavetree.lastsmp[ch]);
	  if (!peakfound) {
	    peaktree[ev].ch[ch].peak = -1;
	    peaktree[ev].ch[ch].peakloc = -1;
	    peaktree[ev].ch[ch].simple = false;
	  }
	  else {
	    //-----Local fit to find peak with better resolution
	    int start = (int)peaktree[ev].ch[ch].peakloc - FITPOINTS/2;
	    if (start < (int)wavetree.firstsmp[ch]) {
	      peaktree[ev].ch[ch].peak = -1;
	      peaktree[ev].ch[ch].peakloc = -1;
	      peaktree[ev].ch[ch].simple = false;
	    }
	    else {
	      double x[FITPOINTS], y[FITPOINTS], a[FITPOLY];
	      for (int i=0;i<FITPOINTS;i++) {
		x[i] = start + i;
		y[i] = raw->event.waveform[ch].y[start + i];
	      }
	      TGraph* gPoint = new TGraph(FITPOINTS,x,y);
	      gPoint->LeastSquareFit(FITPOLY,a);
	      TF1* fPoint = new TF1("fPoint","pol4",start,start+FITPOINTS);
	      fPoint->SetParameters(a);
	      peaktree[ev].ch[ch].peakloc = fPoint->GetMaximumX(start,start+FITPOINTS);
	      peaktree[ev].ch[ch].peak = fPoint->Eval(peaktree[ev].ch[ch].peakloc);
	      delete gPoint;
	      delete fPoint;
	    }
	    //-----Local fit to rise time region
	    start = (int)peaktree[ev].ch[ch].peakloc - LWIDTH - FITPOINTS/2;
	    if (start < (int)wavetree.firstsmp[ch]) {
	      peaktree[ev].ch[ch].Lwid = -1;
	      peaktree[ev].ch[ch].simple = false;
	    }
	    else {
	      double x[FITPOINTS], y[FITPOINTS], a[FITPOLY];
	      for (int i=0;i<FITPOINTS;i++) {
		x[i] = start + i;
		y[i] = raw->event.waveform[ch].y[start + i];
		TGraph* gPoint = new TGraph(FITPOINTS,x,y);
		gPoint->LeastSquareFit(FITPOLY,a);
		TF1* fPoint = new TF1("fPoint","pol4",start,start+FITPOINTS);
		fPoint->SetParameters(a);
		peaktree[ev].ch[ch].Lwid = fPoint->Eval(peaktree[ev].ch[ch].peakloc - LWIDTH);
		delete gPoint;
		delete fPoint;
	      }
	    }
	    //-----Find next local minimum
	    peaktree[ev].ch[ch].valley = smp;
	    peakfound = false;
	    do {
	      if (raw->event.waveform[ch].y[peaktree[ev].ch[ch].valley]
		  > raw->event.waveform[ch].y[smp]) {
		peaktree[ev].ch[ch].valley = smp;
		smp++;
	      }
	      else {
		peakfound = true;
		int c = 0;
		while (peakfound && c < PEAKRES) {
		  if (raw->event.waveform[ch].y[peaktree[ev].ch[ch].valley]
		      > raw->event.waveform[ch].y[smp+c]) {
		    peakfound = false;
		    smp = smp+c;
		    peaktree[ev].ch[ch].valley = smp;		  
		    smp++;
		  }
		  else
		    c++;
		}
	      }
	    }while (!peakfound && smp < (int)wavetree.lastsmp[ch]);
	  }
	}
      }
    }
    ev++;
    printf("Working...  %d \% \r",(int)TMath::Nint(100*(double)ev/NumEvents));
    fflush(stdout);
  }while (ev < NumEvents);
}

/*************************************************************************/
//                            WriteToFile
/*************************************************************************/
void WriteToFile() {
  TString filename = Form("Peaks%05d.root",raw->run.num);
  TString name;
  if (raw->run.num < 851)
    name = PATH1;
  else if (raw->run.num < 1419)
    name = PATH2;
  else if (raw->run.num < 2090)
    name = PATH3;
  else
    name = PATH4;
  name+= "Friends/";
  name+= filename;
  cout << "Writing to " << name << endl;
  TFile* SFile = new TFile(name,"RECREATE");
  TTree* STree = new TTree("peak","Waveform");
  for (int ch=0;ch<N_ADC;ch++) {
    TString branchname = "ch";
    branchname += ch;
    TString leafname = "peak/D:peakloc:Lwid:valley/I:simple/O";
    STree->Branch(branchname,&writetree.ch[ch].peak,leafname);
    }
  for (unsigned int ev=0;ev<NumEvents;ev++) {
    for (unsigned int ch=0;ch<N_ADC;ch++) {
      writetree.ch[ch].peak = peaktree[ev].ch[ch].peak;
      writetree.ch[ch].peakloc = peaktree[ev].ch[ch].peakloc;
      writetree.ch[ch].Lwid = peaktree[ev].ch[ch].Lwid;
      writetree.ch[ch].valley = peaktree[ev].ch[ch].valley;
      writetree.ch[ch].simple = peaktree[ev].ch[ch].simple;
    }
    STree->Fill();
  }
  SFile->Write();
  SFile->Close();
}
