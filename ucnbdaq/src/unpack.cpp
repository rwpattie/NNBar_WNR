// File: unpack.cpp
// Name: Leah Broussard
// Date: 2013/4/4
// Purpose: Converts run%05.fat files into root trees containing waveforms
//
// Revision History:
// 2013/4/4:  LJB  file created

#ifndef __UNPACK_CPP__
#define __UNPACK_CPP__

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "UCNBConfig.hh"
#include "PackagedFile.hh"


/*************************************************************************/
//                            Main Function
/*************************************************************************/
int main (int argc, char *argv[]) {  
  fprintf(stdout,"Welcome to Unpack version %d.%d\n",
	  UCNB_VERSION_MAJOR,
	  UCNB_VERSION_MINOR);
  if (argc != 2) {
    printf("Usage: %s filenum \n",argv[0]);
    return 1;
  }
  int filenum = atoi(argv[1]);

  //-----Open file
  PackagedFile InputFile;
  //InputFile.Open("/home/leah/Work/UCNB/DAQ/Files/run00730.fat");
  //InputFile.Open("run00730.fat");
  InputFile.Open(filenum);
  std::cout << "Is it open? " << InputFile.IsOpen() << std::endl;
  return 0; 
}


#endif // __UNPACK_CPP__
