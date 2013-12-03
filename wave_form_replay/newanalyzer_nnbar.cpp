#ifndef __NEWANALYZER_CPP__
#define __NEWANALYZER_CPP__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <deque>
#include <algorithm>
#include <cassert>
#include <iostream>

#include "TFile.h"
#include "TString.h"
#include "TCanvas.h"
#include "TTree.h"
#include "TSQLResult.h"
#include "TSQLRow.h"
#include "TSQLServer.h"

#define RAWDATA_LENGTH 2048

// Define Data Structures-----------------------------------------------------------------

struct Fadc_Event {
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

struct output_header{
  Int_t       board_number;
  Int_t       packet_serial;
  Int_t       fadc_number;
  Int_t       data_size;
  Int_t       tv_usec;
  Int_t       tv_sec;
  Int_t       admin_message;
  Int_t       buffer_number;
};

struct Data_Block_t {
     Long64_t timestamp;
     Bool_t overflowB0;  
     Bool_t overflowA0;  
     Bool_t overflowB1;  
     Bool_t overflowA1;   
     Int_t sample[4];     
};

struct fadc_channel_data_t {
    UShort_t data[5000];
    UShort_t mn;
    UShort_t mx;
    UShort_t lastindex;
    Long64_t ft;
    Long64_t glt;
    Int_t    time_s;
    Int_t    time_us;
    Int_t    time_local;
    Int_t    curr;
    UInt_t   cycle;
};

struct fadc_board_t {
    fadc_channel_data_t channel_data[8];
};

// ---------------------------------------------------------------------------------------
// Define Function Prototypes
Double_t GetThreshold(Int_t nrun, Int_t nchn,Int_t nthrsh,Int_t nboard);
void process_file(char *flnm,TString dir_save);
void Check_Serial(Int_t *lastser,output_header o,Int_t bn);
void SmoothArray(std::vector<Double_t> t1,std::vector<Double_t>f1,
		 std::vector<Double_t>& t2,std::vector<Double_t>& f2,
		 Int_t smwidth);
Double_t DerivArray(std::vector<Double_t> val,std::vector<Double_t>& der);
Double_t SolveLine(Double_t x1,Double_t y1,Double_t x2,Double_t y2,Double_t yp);
Double_t FindTriggerTime(std::vector<Double_t> val,std::vector<Double_t> time,Double_t thrs);
Double_t FindWidth(std::vector<Double_t> val,std::vector<Double_t> time,Double_t thrs);

UInt_t GetMPCycle(Int_t ts,Int_t tus);
Data_Block_t Fill_Data_Blck(UChar_t raw[RAWDATA_LENGTH],Int_t i);
UShort_t Get_Zero(UShort_t *data,Int_t index);
void Set_Sample_Data(fadc_board_t *fadc,output_header o,Data_Block_t blck,UShort_t bn,Bool_t FIRST,Bool_t INCRCYCLE,Long64_t nfrm);
void initialize_fadc_data(fadc_board_t fadc,Int_t nboards,Bool_t FIRST,Int_t nch);
void process_file(char *flnm,TString dir_save);
void FillTree(fadc_board_t *fadc,Fadc_Event &fadc_event,TTree *t,Int_t bn,Int_t nchnl,Int_t nbd,Double_t thrs);
void SetBranches(TTree *tree,Fadc_Event fadc_event);
//-------------------------------------------------------------------------------------------
// Define a set of global variables 
ULong64_t cycleoffset = pow(2,28);  // Represents the maximum time measured by the 28-bit timestamp
UShort_t boardnum[3]={1,2,3};      // active board numbers as set by the hardware switch...
Int_t initial_time = 0;
//---------------------------------------------------------------------------------------
int main (int argc, char *argv[]) {
	
	if (argc < 2) {
		printf("Usage: newanalyzer <raw data filename> \n");
		return 1;
	}
		
	TString dir_save(getenv("WNR_OUTPUT_DATA"));
	TFile *out_file = new TFile(dir_save + "/" + TString(argv[1]).ReplaceAll(".fat","")+"NA.root", "RECREATE");
	std::cout << out_file->GetName() << std::endl;
	
	process_file(argv[1],dir_save);
	
	out_file->Write();
	out_file->Close();
	
	return 0;
}
//-------------------------------------------------------------------------------------------
Double_t GetThreshold(Int_t nrun, Int_t nchn,Int_t nthrsh,Int_t nboard)
{
  
  TSQLResult *res;
  TSQLRow    *row;
  Double_t thresh=0;
  
  if(nchn < 0 || nchn > 7){
      std::cout << "Channel " << nchn << " is not valid !!!! " << std::endl;
      return -1;
  }
  
  TSQLServer *sql = TSQLServer::Connect("mysql://localhost/wnr_run_info",
					getenv("UCNADBUSER"),getenv("UCNADBPASS"));
  
  char query[500];
  sprintf(query,"select upper_threshold,lower_threshold from channel_info"
		" where run_number = %d and channel = %d and board = %d",
		nrun,nchn,nboard);
  
  res = (TSQLResult*)sql->Query(query);
  if(res->GetRowCount() != 0){
      while((row = (TSQLRow*)res->Next())){
	  thresh = (Double_t)atof(row->GetField(nthrsh));
      }
  }
  
  return thresh; 
}
//--------------------------------------------------------------------------------------
void Check_Serial(Int_t *lastser,output_header o,Int_t bn)
{
      if(lastser[bn]== -1){
	lastser[bn] = o.packet_serial;
      } else if(lastser[bn] == o.packet_serial + 65535){
	printf("Looping serials on board #%d.\n", boardnum[bn]);
	lastser[bn] = o.packet_serial;
      } else if(lastser[bn] != o.packet_serial - 1) {
	//printf("Missing packets between %d and %d.\n",lastser[bn],o.packet_serial);
	lastser[bn] = o.packet_serial;
      } else{
	lastser[bn] = o.packet_serial;
      }
};

void FillTree(fadc_board_t *fadc,Fadc_Event &fadc_event,TTree *t,Int_t bn,Int_t nchnl,Int_t nbd,Double_t thres)
{
	  Int_t index = fadc[bn].channel_data[nchnl].lastindex;
	  fadc_event.zero = Get_Zero(fadc[bn].channel_data[nchnl].data,index);
	  std::vector<Double_t> time,fadcv,smtime5,smfadc5,derv5;
	  
	  for (Int_t k = 0; k < index; k++){
	      fadc_event.adc[k] = fadc[bn].channel_data[nchnl].data[k];
	      time.push_back((double)fadc[bn].channel_data[nchnl].ft*16e-9 + (double)k*4e-9);
	      fadcv.push_back(fadc[bn].channel_data[nchnl].data[k]);
	      //if(nchnl ==7) std::cout << k << "\t" << fadc_event.adc[k] <<  std::endl;
	  }
	  // run a 5 sample moving averager over the waveform
	  if(index > 10){
	    SmoothArray(time,fadcv,smtime5,smfadc5,5);  
	    fadc_event.riseTime = DerivArray(smfadc5,derv5);
	    fadc_event.trigger  = FindTriggerTime(smfadc5,smtime5,thres);
	    fadc_event.width    = FindWidth(smfadc5,smtime5,thres);
	  } else {
	    fadc_event.riseTime = 0.;
	    fadc_event.trigger = 0.;
	    fadc_event.width = 0.;
	    
	  }
	  
	  fadc_event.first_time     = fadc[bn].channel_data[nchnl].ft;
	  fadc_event.board          = nbd;
	  fadc_event.max            = fadc[bn].channel_data[nchnl].mx;
	  fadc_event.min            = fadc[bn].channel_data[nchnl].mn;
	  fadc_event.channel        = (UShort_t)nchnl;
	  fadc_event.last           = index;
	  fadc_event.cycle          = fadc[bn].channel_data[nchnl].cycle;
	  fadc_event.global_time    = fadc[bn].channel_data[nchnl].glt;
	  fadc_event.packet_time_s  = fadc[bn].channel_data[nchnl].time_s;
	  fadc_event.packet_time_us = fadc[bn].channel_data[nchnl].time_us;
	  fadc_event.packet_time_l  = fadc[bn].channel_data[nchnl].time_local;
	  
	  if (index > 0 ){
	    UShort_t mean = 0;
	    if(index > 15){
	      for(int i = 0 ; i < 15 ; i++)
		mean += fadc[bn].channel_data[nchnl].data[i];
	      fadc_event.ped = mean/15.;
	    } else 
	       fadc_event.ped = 0;
	    t->Fill();
	  }
};
//-----------------------------------------------------------------------------------------
Data_Block_t Fill_Data_Blck(UChar_t raw[RAWDATA_LENGTH],Int_t i)
{
      //--------------------------------------------------------------------------
      // -- Set the bit masks and shifts to evaulate a block of 
      // -- raw data.
      //  bits
      //  78-52       timestamp
      //  51-48       overflow
      //  47-36       sample[3]
      //  35-24       sample[2]
      //  23-12       sample[1]
      //  11-0        sample[0]
      
      Data_Block_t Current_Block;
      
      Current_Block.timestamp =  (raw[i*10+0] << 20) | (raw[i*10+1] << 12) |
			         (raw[i*10+2] << 4)  | (raw[i*10+3] >> 4);
      
      Current_Block.overflowB0  = ((raw[i*10+3] & 0x08) != 0);
      Current_Block.overflowA0  = ((raw[i*10+3] & 0x04) != 0);
      Current_Block.overflowB1  = ((raw[i*10+3] & 0x02) != 0);
      Current_Block.overflowA1  = ((raw[i*10+3] & 0x01) != 0);
      
      Current_Block.sample[3]    = (raw[i*10+4] << 4) |(raw[i*10+5] >> 4);
      Current_Block.sample[2]    = ((raw[i*10+5] & 0xf) << 8) |(raw[i*10+6]);
      Current_Block.sample[1]    = (raw[i*10+7] << 4) |(raw[i*10+8] >> 4);
      Current_Block.sample[0]    = ((raw[i*10+8] & 0xf) << 8) |(raw[i*10+9]);
      
      return Current_Block;
}
//-----------------------------------------------------------------------------------------
UShort_t Get_Zero(UShort_t *data,Int_t index)
{
    Int_t sum = 0;
    for (Int_t i = 0; i < 6; i++){
       sum += data[i * 4] + data[(i * 4) + 1] + data[(i * 4) + 2] + data[(i * 4) + 3];
    }
    if(index == 0 )return 1;
    return sum / index;
}
//================================================================================================================
void Set_Sample_Data(fadc_board_t *fadc,output_header o,Data_Block_t blck,UShort_t bn,Bool_t FIRST,Bool_t INCRCYCLE,
		     Long64_t nfrm)
{
      // Get the last index and channel number and save them to local variables
      Int_t index  = fadc[bn].channel_data[o.fadc_number].lastindex;
      Int_t nchnl  = o.fadc_number;
      Int_t ncycle = fadc[bn].channel_data[nchnl].cycle;
      if(initial_time == 0) initial_time = o.tv_sec;
      //---------------------------------------------------------------------------------
      for(Int_t ii = 0 ; ii < 4 ; ii++){
	//loop over the 4 samples in the packet setting the fadc.channel_data.data[nsample] variable
	fadc[bn].channel_data[nchnl].data[index + ii] = blck.sample[ii];
	// check if the current sample is greater than the previously recorded mx 
	// or less than the last mn
	if (blck.sample[ii] > fadc[bn].channel_data[nchnl].mx)fadc[bn].channel_data[nchnl].mx = blck.sample[ii];
	if (blck.sample[ii] < fadc[bn].channel_data[nchnl].mn)fadc[bn].channel_data[nchnl].mn = blck.sample[ii];
      }
      // Set the current timestamp
      fadc[bn].channel_data[nchnl].curr = blck.timestamp;
      // Set the last index
      fadc[bn].channel_data[nchnl].lastindex += 4;
      // if the first packet set the first time variable
      if(FIRST){
	  fadc[bn].channel_data[nchnl].ft         = blck.timestamp; 
	  fadc[bn].channel_data[nchnl].glt        = blck.timestamp + (cycleoffset * ncycle);
	  fadc[bn].channel_data[nchnl].time_s     = o.tv_sec;
	  fadc[bn].channel_data[nchnl].time_us    = o.tv_usec;
	  fadc[bn].channel_data[nchnl].time_local = o.tv_sec - initial_time;
      }
      // if the clock has looped incriment the cycle counter.
      if(INCRCYCLE || blck.timestamp == 0){
	fadc[bn].channel_data[nchnl].cycle++;
	if(fadc[bn].channel_data[nchnl].cycle < GetMPCycle(fadc[bn].channel_data[nchnl].time_local,o.tv_usec)){
		fadc[bn].channel_data[nchnl].cycle = GetMPCycle(fadc[bn].channel_data[nchnl].time_local,o.tv_usec);
	    }
      }
}
//------------------------------------------------------------------------------------------------------
void SetBranches(TTree *tree,Fadc_Event fadc_event)
{
  
}
//----------------------------------------------------------------------------------------------------
void initialize_fadc_data(fadc_board_t *fadc,Int_t nboards,Bool_t FIRST,Int_t nch)
{
    for(Int_t i = 0 ; i < nboards ; i++ ){
      if(FIRST){
	for(Int_t j = 0 ; j < 8 ; j++ ){
	    if(FIRST){
	      // if this is the first packet read from the file reset the current timestamp and cycle counters
	      // to initial values of -21 and 0.
	      fadc[i].channel_data[j].curr    = -21; 
	      fadc[i].channel_data[j].cycle   = 0;
	    }
	    fadc[i].channel_data[j].mx        = 0; // Set the max value to 0 for future comparison
	    fadc[i].channel_data[j].mn        = 4096; // Set the min value to 4096 for future comparison
	    fadc[i].channel_data[j].lastindex = 0; // Reset the index of the last element.
	}
      } else {
	    fadc[i].channel_data[nch].mx        = 0; // Set the max value to 0 for future comparison
	    fadc[i].channel_data[nch].mn        = 4096; // Set the min value to 4096 for future comparison
	    fadc[i].channel_data[nch].lastindex = 0; // Reset the index of the last element.
      }
    }
}
//-----------------------------------------------------------------------------
void process_file(char *flnm,TString dir_save) 
{
  //-----------------------------------------------------------------------------------------
  using namespace std;
  // Struct to hold tree data
  Fadc_Event fadc_event;
  TString dir_input(getenv("WNR_RAW_DATA")); 
  FILE *inf = fopen(dir_input + "/" + flnm, "rb");
  FILE *tsrec = fopen(dir_save + "/" + TString(flnm).ReplaceAll(".fat","") + "ts.txt","w");
  
  char runnumber[5] = {flnm[3],flnm[4],flnm[5],flnm[6],flnm[7]};
  Int_t nrun = atoi(runnumber);
  std::cout << "Run number is " << nrun << std::endl;
  if(!tsrec){
    printf("Can't open timestamps file.");
    exit(1);
  }
  
  if(!inf){
    fprintf(stderr, "Unable to open %s: \n", flnm);
    exit(1);
  }
  // structure with the header data.
  output_header o;
  // An array to hold the board data..
  fadc_board_t fadc[3];
  // set the fadc channel variable to their initial values
  initialize_fadc_data(fadc,3,true,-1); 
  // Create a character array to hold the raw data from the packet.
  UChar_t raw[RAWDATA_LENGTH];
  
  Int_t lastser[3] = {-1,-1,-1};
  // Get the thresholds....
  std::vector<Int_t> threshold;
  for(Int_t j = 1 ; j < 4 ; j++)
    for(Int_t i = 0 ; i < 8 ; i++)
	threshold.push_back((Int_t)GetThreshold(nrun,i,0,j));  
  Int_t sample = 8;
  //---------------------------------------------------------------------------------------
  // create an output tree..............
  TTree *t = new TTree("t","t");
  //SetBranches(t,fadc_event);
  t->Branch("first_time",&fadc_event.first_time,"first_time/l");
  t->Branch("global_time",&fadc_event.global_time,"global_time/l");
  t->Branch("packet_time_s",&fadc_event.packet_time_s,"packet_time_s/I");
  t->Branch("packet_time_us",&fadc_event.packet_time_us,"packet_time_us/I");
  t->Branch("packet_time_l",&fadc_event.packet_time_l,"packet_time_l/I");
  t->Branch("board",&fadc_event.board,"board/I");
  t->Branch("last",&fadc_event.last,"last/s");
  t->Branch("cycle",&fadc_event.cycle,"cycle/i");
  t->Branch("adc",fadc_event.adc,"adc[last]/s");
  t->Branch("max",&fadc_event.max,"max/s");
  t->Branch("min",&fadc_event.min,"min/s");
  t->Branch("zero",&fadc_event.zero,"zero/s");
  t->Branch("ped",&fadc_event.ped,"ped/s");
  t->Branch("channel",&fadc_event.channel,"channel/s");
  t->Branch("trigger",&fadc_event.trigger,"trigger/D");
  t->Branch("risetime",&fadc_event.riseTime,"risetime/D");
  t->Branch("width",&fadc_event.width,"width/D");
  //--------------------------------------------------------------------------------------
  // loop through the raw data file
  Long64_t nframe = 0;
  Int_t nevent = 0;
  while(!feof(inf)) {
    // read the header of the current event.
    fread(&o, sizeof(o), 1, inf);
//     printf("Here's a bunch of shit:\n");
//     printf("board_number = %d, packet serial = %d\n",o.board_number,o.packet_serial);
//     printf("fadc number = %d, data size = %d\n",o.fadc_number,o.data_size);
    //if(o.data_size <= 0) continue;
    Bool_t bnfound = false; // logical variable to check if a board is found.
    UShort_t bn    = 0;
    // locate boards by comparing the list of board id numbers to what is reported in the 
    // header.  This exhanges the board-id with an index.  Since a data block may have
    // multiple data streams in is possible that there could be events from different
    // boards in the current packet.
    for(Short_t i=0 ; i < 3 ; i++){
      if(o.board_number == boardnum[i]){
	bn      = i;
	bnfound = true;
      } 
    }
    //-------------------------------------------------------------------------------------
    // So this set if/else statements seems to be looking for breaks in the packet stream
    //
    Check_Serial(lastser,o,bn);
    
    //-------------------------------------------------------------------------
    if(o.data_size <= 0) continue; 
    // Read data from the .fat file
    fread(raw, o.data_size, 1, inf);
    
    if(o.data_size%10 != 0) {
      printf("Data size is not correct!\n");
      continue;
    }
    if(bnfound==false){
      printf("Board number does not match record.\n");
      continue;
    }
    Int_t num_words = o.data_size/10;
    //if(o.fadc_number == 7){
    for (Int_t j = 0; j < num_words; j++) {
      // ..........................................................................................
      // if the current block of data input the blck struct
      Data_Block_t blck = Fill_Data_Blck(raw,j);
      // parse the fadc data.......................................................................
      if (fadc[bn].channel_data[o.fadc_number].curr == -21) {
      // Looks for first packet in the file.......
	Set_Sample_Data(fadc,o,blck,bn,true,false,nframe);
      } else if ( fadc[bn].channel_data[o.fadc_number].curr == blck.timestamp - 1 ) {
	// I guess this is just some intermediate packet...
	// the logical statement looks like if curr is 1 less than the packet timestamp then 
	// this is just a sequential packet.
	Set_Sample_Data(fadc,o,blck,bn,false,false,nframe);
      } else if ( fadc[bn].channel_data[o.fadc_number].curr == blck.timestamp + cycleoffset - 1) {
	// output warning message
	fprintf(tsrec,"Old Timestamp = %u, New Timestamp = %lli, cycle = %u, Channel = %d\n",
		fadc[bn].channel_data[o.fadc_number].curr,blck.timestamp,
		fadc[bn].channel_data[o.fadc_number].cycle-1,o.fadc_number);
	Set_Sample_Data(fadc,o,blck,bn,false,true,nframe);
      } else if(blck.timestamp == 0){
		Set_Sample_Data(fadc,o,blck,bn,false,true,nframe);
      }else { 
	 // Fill tree data struct..................................................................
	
	  FillTree(fadc,fadc_event,t,bn,o.fadc_number,o.board_number,
		   (double)threshold[o.fadc_number+bn*8]);
	  nevent++;
	  // increase the cycle number if the current timestamp exceeds the block timestamp
	  Int_t nchnl = o.fadc_number;
	  if (fadc[bn].channel_data[nchnl].curr > blck.timestamp) {
	    fadc[bn].channel_data[nchnl].cycle++;
	    fprintf(tsrec,"Old Timestamp = %u, New Timestamp = %lli, cycle = %u, Channel = %d\n",fadc[bn].channel_data[nchnl].curr,
		    blck.timestamp,fadc[bn].channel_data[nchnl].cycle-1,nchnl);
	    if(fadc[bn].channel_data[nchnl].cycle < GetMPCycle(fadc[bn].channel_data[nchnl].time_local,o.tv_usec)){
		fadc[bn].channel_data[nchnl].cycle = GetMPCycle(fadc[bn].channel_data[nchnl].time_local,o.tv_usec);
	    }
	  }
	  initialize_fadc_data(fadc,3,false,o.fadc_number);
	  nframe = 0;
	  Set_Sample_Data(fadc,o,blck,bn,true,false,nframe);
	}
    //}
     }
  }
  fclose(inf);
}

UInt_t GetMPCycle(Int_t ts,Int_t tus)
{
 
  Double_t tt = (double)ts + (double)tus*1e-6;
  UInt_t   mp = (UInt_t)(tt/0.025);
  if(mp > 7e6) mp = 23;
  return mp-23;
}


void SmoothArray(std::vector<Double_t> t1,std::vector<Double_t>f1,
		 std::vector<Double_t>& t2,std::vector<Double_t>& f2,
		 Int_t smwidth)
{
  Int_t offset = smwidth / 2;
  for(Int_t i = offset ; i < (int)f1.size() - offset ; i++){
      t2.push_back(t1[i]);
      Double_t fsum = 0;
      for(int j = 0 ; j < smwidth ; j++){
	fsum += f1[i+j-offset];
      }
      f2.push_back(fsum/(double)smwidth);
  }
  
}

Double_t DerivArray(std::vector<Double_t> val,std::vector<Double_t>& der)
{
    //very simple 2 point derivative. probably very wrong but a first step....
    Double_t dt = 1.;
    Int_t trgp = 0;
    Int_t maxp = 0;
    
    for(Int_t i = 1 ; i < (int)val.size()-1 ; i++){
      der.push_back((val[i+1] - val[i-1])/(2.*dt));
      if(der[i-1] > 5 && trgp == 0 )trgp = i;
      if(trgp > 0  && der[i-1] < 0 && maxp == 0)maxp = i;
    }
    Double_t t0 = SolveLine((double)(trgp-2),der[trgp-2],(double)(trgp+2),der[trgp+2],5.);
    Double_t t1 = SolveLine((double)(maxp-2),der[maxp-2],(double)(maxp+2),der[maxp+2],0.);
    return (double)(t1 - t0)*4.0;
}

Double_t SolveLine(Double_t x1,Double_t y1,Double_t x2,Double_t y2,Double_t yp)
{
    Double_t m = (y2-y1)/(x2-x1);
    //y = mx + b ... (y-b)/m = x ...  m = (yp - y1)/(xp - x1) ...  xp = x1 + (yp-y1)/m
    Double_t xp = x1 + (yp-y1)/m;
    return xp;
}

Double_t FindTriggerTime(std::vector<Double_t> val,std::vector<Double_t> time,Double_t thrs)
{
    Int_t ntrg=0;
    while(val[ntrg] < thrs && ntrg < (int)val.size())
      ntrg++;
    
    if(ntrg == (int)val.size()){
      thrs*=0.9;
      ntrg=0;
      while(val[ntrg] < thrs && ntrg < (int)val.size())
      ntrg++;
    }
    
    Double_t tTrig = SolveLine(time[ntrg-2],val[ntrg-2],time[ntrg+2],val[ntrg+2],thrs);
    return tTrig;
}

Double_t FindWidth(std::vector<Double_t> val,std::vector<Double_t> time,Double_t thrs)
{
    Int_t t0 = 0;  // first trigger crossing
    Int_t t1 = 0;  // second trigger crossing
    
    for(UInt_t i = 0 ; i < val.size() ; i++){
	if(val[i] > thrs && t0 == 0)t0 = i;
	if(val[i] < thrs && t0 > 0 && t1 == 0) t1 = i;
    }
    if(t0 == 0){
        thrs*=0.9;
	for(UInt_t i = 0 ; i < val.size() ; i++){
	  if(val[i] > thrs && t0 == 0)t0 = i;
	  if(val[i] < thrs && t0 > 0 && t1 == 0) t1 = i;
	}
    }
     
    
    Double_t tt0 = SolveLine(time[t0-2],val[t0-2],time[t0+2],val[t0+2],thrs);
    Double_t tt1 = SolveLine(time[t1-2],val[t1-2],time[t1+2],val[t1+2],thrs);
    return (tt1-tt0);
}

  
#endif