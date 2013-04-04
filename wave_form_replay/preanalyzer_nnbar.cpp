#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <deque>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <list>

#include "TFile.h"
#include "TString.h"
#include "TCanvas.h"
#include "TTree.h"

#define RAWDATA_LENGTH 2048

// Define Data Structures

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

// Define Function Prototypes

// Define Global Variables

// MAIN

int main (int argc, char *argv[]){
  
  if (argc < 2) {
    printf("Usage: newanalyzer <raw data filename> \n");
    return 1;
  }

  TString dir("/home/daqfadc/MIDAS_DAQ/test/online/data");
  //TString dir("/host/UCNBETAS/daq/data_nnbar");
  FILE *inf=fopen(dir+"/"+argv[1],"rb");
  FILE *of=fopen(dir+"/"+TString(argv[1]).ReplaceAll(".fat","")+"pa.fat","wb");

  if(!inf){
    fprintf(stderr,"Unable to open %s.\n",argv[1]);
    exit(1);
  }

  if(!of){
    fprintf(stderr,"Unable to open output file.\n");
    exit(1);
  }
  
  printf("Opened the file %s.\n",argv[1]);

  std::vector<int> serials;
  std::vector<int> time_sec;
  std::vector<int> unique_time_sec;
  std::vector<int> dupe_indices;
  output_header o;
  bool found_dupe=false;

  UChar_t raw[RAWDATA_LENGTH];

  while(!feof(inf)){
    fread(&o,sizeof(o),1,inf);
    serials.push_back(o.packet_serial);
    time_sec.push_back(o.tv_sec);
    if(o.data_size<=0)continue;
    fread(raw,o.data_size,1,inf);
  }

  for(Int_t aa=0;aa<=65535;aa++){
    unique_time_sec.clear();
    for(Int_t ab=0;ab<serials.size();ab++){
      if(serials[ab]!=aa)continue;
      if(unique_time_sec.size()==0){
	unique_time_sec.push_back(time_sec[ab]);
      }
      else{
	found_dupe=false;
	for(Int_t ac=0;ac<unique_time_sec.size();ac++){
	  if(abs(time_sec[ab]-unique_time_sec[ac])<=2&!found_dupe){
	    dupe_indices.push_back(ab);
	    found_dupe=true;
	  }
	}
	if(!found_dupe){
	  unique_time_sec.push_back(time_sec[ab]);
	}
      }
    }
  printf("Checked Serial %d.\r",aa);
  fflush(stdout);
  }
  printf("\n");

  printf("Sorting vector now.\n");
  sort(dupe_indices.begin(),dupe_indices.end());
  printf("Finished sorting.\n");\
  printf("Found %u dupes.  Removing...\n",dupe_indices.size());
  fseek(inf,0,SEEK_SET);

  Int_t ae=0;
  for(Int_t ad=0;ad<serials.size();ad++){
    fread(&o,sizeof(o),1,inf);
    fread(raw,o.data_size,1,inf);
    if(ae<dupe_indices.size()){
      if(dupe_indices[ae]==ad){
	ae++;
	continue;
      }
    }
    fwrite(&o,sizeof(o),1,of);
    fwrite(raw,o.data_size,1,of);
  }    

  fclose(inf);
  fclose(of);
  return 0;
}
