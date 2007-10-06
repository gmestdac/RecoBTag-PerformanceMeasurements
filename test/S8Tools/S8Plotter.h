#ifndef S8Plotter_h
#define S8Plotter_h
/** \class S8Plotter
 *  
 * Analyze ROOT files produced by analyzer and create plots
 *
 * \author Francisco Yumiceva, Fermilab (yumiceva@fnal.gov)
 *
 * \version $Id: S8Plotter.h,v 1.3 2007/05/09 03:34:07 yumiceva Exp $
 *
 */

#include "TROOT.h"
#include "TChain.h"
#include "TFile.h"
#include "TString.h"
#include "TH1.h"
#include "TH2.h"
#include "TCanvas.h"

#include <stdlib.h>
#include <iostream>
#include <string>
#include <map>

#include "RecoBTag/PerformanceMeasurements/interface/BTagEvent.h"
#include "RecoBTag/PerformanceMeasurements/interface/BTagLeptonEvent.h"


class S8Plotter {

  public:
	
	S8Plotter(TString filename);
	virtual ~S8Plotter();

	virtual Int_t  GetEntry(Long64_t entry);
	virtual void     Init();
	virtual void     Loop();
	virtual Long64_t LoadTree(Long64_t entry);
	virtual void     Show(Long64_t entry = -1);
	virtual Int_t    Cut(Long64_t entry);
	virtual Bool_t   Notify();
	//virtual std::string itoa(int i);
	//virtual std::string ftoa(float i);
	void Add(TString filename);
	void Verbose(bool option=true) { fVerbose = option; };
	void Book();
	void SampleName(TString sample) {
		fsamplename = sample;
	};
	void Print(std::string extension="png",std::string tag="") {
		if ( tag != "" ) tag = "_"+tag;
		
		for(std::map<std::string,TCanvas*>::const_iterator icv=cv_map.begin(); icv!=cv_map.end(); ++icv){

			std::string tmpname = icv->first;
			TCanvas *acv = icv->second;
			acv->Print(TString(tmpname+tag+"."+extension));
		}		
	};
	
	void SaveToFile(TString filename="S8_plots.root") {

		//if (!foutfile) {
			
		foutfile = new TFile(filename,"RECREATE");
		for(std::map<std::string,TH1* >::const_iterator ih=h1.begin(); ih!=h1.end(); ++ih){
			TH1 *htemp = ih->second;
			htemp->Write();
		}
		for(std::map<std::string,TH2* >::const_iterator ih=h2.begin(); ih!=h2.end(); ++ih){
			TH2 *htemp = ih->second;
			htemp->Write();
		}
		
		foutfile->Write();
		foutfile->Close();
	};

  private:
	
	BTagEvent *fS8evt;
	
	TChain            *fChain;
	Int_t             fCurrent;
	TFile            *ffile;
	TFile            *foutfile;
	TString           fsamplename;
	bool              fVerbose;

	std::map<std::string, TCanvas*> cv_map;
	std::map<std::string, TH1*> h1;
	std::map<std::string, TH2*> h2;
	
	std::vector < std::string > quark_label;
	std::map<std::string, int>  quark_color;
	std::map<std::string, std::string > cut_label;
	
};

#endif

#ifdef S8Plotter_cxx
S8Plotter::S8Plotter(TString filename)
{
	//ffile = (TFile*)gROOT->GetListOfFiles()->FindObject(filename);
	//ffile = new TFile(filename);
	//TTree *tree = (TTree*)gDirectory->Get("summary");

	fVerbose = false;
	
	fChain = new TChain("summary");
	fChain->Add(filename);
	
	fS8evt = new BTagEvent();
	//fBTagSummary[1] = new BTagSummary();
	//fBTagSummary[2] = new BTagSummary();

		
	//cout << " file loaded and objects created" << endl;
	Init();
	
}
void S8Plotter::Add(TString filename)
{
	fChain->Add(filename);
}

S8Plotter::~S8Plotter()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

std::string itoa(int i) {
  char temp[20];
  sprintf(temp,"%d",i);
  return((std::string)temp);
}

std::string ftoa(float i, const char *format="0") {
  char temp[20];
  if ( format == "0" ) sprintf(temp,"%.2f",i);
  else sprintf(temp,format,i);
  
  return((std::string)temp);
}

std::string GetStringFlavour(int i) {
	if ( i == 5 ) { return "_b"; }
	else if ( i == 4 ) { return "_c"; }
	else if ( i>0 && i<=3 ) { return "_udsg"; }
	else if ( i == 21 ) { return "_udsg"; }
	else if ( i == 0 ) { return "ambiguous";}
	else { 
		//cout << " not defined flavour " << i << endl;
		return "";
	}
}

void NormalizeHistograms(std::vector< TH1* > hist) {

	for ( std::vector< TH1* >::size_type ihist = 0; ihist != hist.size() ; ++ihist ) {
		hist[ihist]->Scale(1./hist[ihist]->Integral());
	}
	
}

Int_t S8Plotter::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}

Long64_t S8Plotter::LoadTree(Long64_t entry)
{
// Set the environment to read one entry
   if (!fChain) return -5;
   Long64_t centry = fChain->LoadTree(entry);
   if (centry < 0) return centry;
   if (fChain->IsA() != TChain::Class()) return centry;
   TChain *chain = (TChain*)fChain;
   if (chain->GetTreeNumber() != fCurrent) {
      fCurrent = chain->GetTreeNumber();
      Notify();
   }
   return centry;
}

void S8Plotter::Init()
{

	//if (!tree) return;

	//fChain = tree;
	fCurrent = -1;
	
	fChain->SetBranchAddress("s8.",&fS8evt);
	//fChain->SetBranchAddress("summaryKVF.",&fBTagSummary[1]);
	//fChain->SetBranchAddress("summaryTKF.",&fBTagSummary[2]);

	//cout << "Init done" << endl;
}

Bool_t S8Plotter::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normaly not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void S8Plotter::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t S8Plotter::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef S8Plotter_cxx