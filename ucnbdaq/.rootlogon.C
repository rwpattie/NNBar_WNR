{
  gROOT->ProcessLine(".include inc/ ");
  gROOT->ProcessLine(".L src/FredDigitizer.cpp");
  gROOT->ProcessLine(".L src/SimpleAnalysisFile.cpp");
  gROOT->ProcessLine(".L src/WaveformAnalyzer.cpp");
}
