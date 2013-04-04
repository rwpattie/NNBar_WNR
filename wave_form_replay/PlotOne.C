#define NCH 7
struct Fadc_Event {
  ULong64_t first_time;  
  ULong64_t global_time;
  Int_t     packet_time_s;
  Int_t     packet_time_us;
  Int_t     packet_time_l;
  Int_t     board;
  UShort_t  last;
  UShort_t  adc[5000];
  UShort_t  max;
  UShort_t  min;
  UShort_t  zero;
  UShort_t  ped;
  UShort_t  channel;
};

TDirectory* top = new TDirectory("top","top");
TFile* f;
TTree* t;
Fadc_Event fadc_event;
bool open = false;
int RunNum;

void OpenIt(Int_t runnum) {
  TString filename = Form("/home/daqfadc/MIDAS_DAQ/test/online/data/analyzed/run%05dNA.root",runnum);
f = new TFile(filename);
  t = (TTree*)gROOT->FindObject("t");
  //Handle either type of tree
  if (t->GetNbranches() == 1)
    t->SetBranchAddress("fadc_event",&fadc_event.first_time);
  else {
    t->SetBranchAddress("first_time",&fadc_event.first_time);
    t->SetBranchAddress("global_time",&fadc_event.global_time);
    t->SetBranchAddress("packet_time_s",&fadc_event.packet_time_s);
    t->SetBranchAddress("packet_time_us",&fadc_event.packet_time_us);
    t->SetBranchAddress("packet_time_l",&fadc_event.packet_time_l);
    t->SetBranchAddress("board",&fadc_event.board);
    t->SetBranchAddress("last",&fadc_event.last);
    t->SetBranchAddress("adc",fadc_event.adc);
    t->SetBranchAddress("max",&fadc_event.max);
    t->SetBranchAddress("min",&fadc_event.min);
    t->SetBranchAddress("zero",&fadc_event.zero);
    t->SetBranchAddress("ped",&fadc_event.ped);
    t->SetBranchAddress("channel",&fadc_event.channel);
  }
  open = true;
  RunNum = runnum;
}

void CloseIt() {
  if (f->IsOpen()) {
    f->Close();
    delete f;
    f = 0;
    t = 0;
    RunNum = -1;
    open = false;
  }
}

void Plot(int ev) {
  if (!open) {
    OpenIt(658);
  }
  t->GetEntry(ev);
  cout << "Plotting channel " << fadc_event.channel << endl;
  double x[5000], y[5000];
  for (int i=0;i<fadc_event.last;i++) {
    x[i] = i;
    y[i] = fadc_event.adc[i];
  }
  TGraph* g = new TGraph(fadc_event.last,x,y);
  g->SetMarkerStyle(20);
  g->Draw("AP");
}

void Overlay() {
  OpenIt(658);
  ScrewIt();
  OpenIt(660);
  ScrewIt();
  OpenIt(661);
  ScrewIt();
  TCanvas* c = new TCanvas();
  c->Divide(2,4);
  TString n1, n2, n3;
  top->cd();
  for (int i=0;i<NCH;i++) {
    c->cd(i+1);
    n1 = "hRun658ch";
    n1 += i;
    TH1D* h1 = (TH1D*)gROOT->FindObject(n1);
    n2 = "hRun660ch";
    n2 += i;
    TH1D* h2 = (TH1D*)gROOT->FindObject(n2);
    h2->SetLineColor(kGreen);
    h2->Draw("same");
    n3 = "hRun661ch";
    n3 += i;
    TH1D* h3 = (TH1D*)gROOT->FindObject(n3);
    h3->SetLineColor(kRed);
    h3->Draw("same");
  }
  
}

void ScrewIt() {
  TH1D* h[NCH];
  for (int i=0;i<NCH;i++) {
    top->cd();
    TString name = "hRun";
    name += RunNum;
    name += "ch";
    name += i;
    h[i] = (TH1D*)gROOT->FindObject(name);
    if (h[i] != 0)
      delete h[i];
    h[i] = new TH1D(name,name,2100,-100,2000);
  }
  f->cd();
  int num = t->GetEntries();
  //if (num > 50000)
  //num = 50000; // lazy
  int update = 0;
  for (int ev=0;ev<num;ev++) {
    if (ev > update) {
      printf("Run %d  Working....%d/%d  (%d \%)\r",RunNum,ev,num,100*ev/num);
      fflush(stdout);
      update += num/1000;
    }
    t->GetEntry(ev);
    if (fadc_event.last > 100) {
      double min = FindMinimum();
      top->cd();
      h[fadc_event.channel]->Fill(3600-min);
      f->cd();
    }
  }
  top->cd();
  TCanvas* c = new TCanvas();
  c->SetLogy();
  c->Divide(2,4);
  for (int i=0;i<NCH;i++) {
    c->GetPad(i)->SetLogy();
    c->cd(i+1);
    h[i]->Draw();
    /* Never mind this for now
    if (i==5) {
      TF1* fct = new TF1("f","gaus",900,1200);
      h[i]->Fit(fct,"Q","",900,1200);
      double mean = fct->GetParameter(1);
      h[i]->Fit(fct,"Q","",mean-35,mean+35);
      h[i]->GetXaxis()->SetRangeUser(mean-100,mean+100);
      cout << "Found mean at " << fct->GetParameter(1) << " +/- " << fct->GetParError(1) << endl;
      cout << "sigma is " << fct->GetParameter(2) << " +/- " << fct->GetParError(2) << endl;
    }
    */
  }
  printf("Run %d  Done!                                                             \n",RunNum);
  f->cd();
}

void MakeASpectrum(int ch) {
  TString name = "h";
  name += ch;
  TH1D* hist = (TH1D*)gROOT->FindObject(name);
  if (hist != 0)
    delete hist;
  hist = new TH1D(name,name,1000,0,3000);
  int num = t->GetEntries();
  for (int ev=0;ev<num;ev++) {
    t->GetEntry(ev);
    if (fadc_event.channel == ch && fadc_event.last > 200) {
      double min = FindMinimum();
      hist->Fill(3600-min);
    }
  }
  hist->Draw();
}

double FindMinimum() { //of whatever is stored
  double min = fadc_event.adc[0];
  for (int i=0;i<fadc_event.last;i++) {
    if (min > fadc_event.adc[i])
      min = fadc_event.adc[i];
  }
  return min;
}
