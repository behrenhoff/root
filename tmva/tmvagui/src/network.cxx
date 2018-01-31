#include "TMVA/network.h"
#include <iostream>

#include "TArrow.h"
#include "TEllipse.h"
#include "TPaveLabel.h"
#include "TCanvas.h"
#include "TH2F.h"
#include "TFile.h"
#include "TString.h"
#include "TDirectory.h"
#include "TKey.h"
#include "TText.h"

using std::cout;
using std::endl;


// this macro prints out a neural network generated by MethodMLP graphically
// @author: Matt Jachowski, jachowski@stanford.edu

TFile* Network_GFile = 0;

static Int_t c_DarkBackground = TColor::GetColor( "#6e7a85" );


Bool_t MovieMode = kFALSE;

void TMVA::draw_network(TString dataset, TFile* f, TDirectory* d, const TString& hName, 
                        Bool_t movieMode , const TString& epoch  )
{
   Bool_t __PRINT_LOGO__ = kTRUE;
   Network_GFile = f;

   MovieMode = movieMode;
   if (MovieMode) c_DarkBackground = TColor::GetColor( "#707F7F" );

   // create canvas
   TStyle* TMVAStyle = gROOT->GetStyle("TMVA"); // the TMVA style
   Int_t canvasColor = TMVAStyle->GetCanvasColor(); // backup
   TMVAStyle->SetCanvasColor( c_DarkBackground );

   Int_t titleFillColor = TMVAStyle->GetTitleFillColor();
   Int_t titleTextColor = TMVAStyle->GetTitleTextColor();
   Int_t borderSize     = TMVAStyle->GetTitleBorderSize();

   TMVAStyle->SetTitleFillColor( c_DarkBackground );
   TMVAStyle->SetTitleTextColor( TColor::GetColor( "#FFFFFF" ) );
   TMVAStyle->SetTitleBorderSize( 0 );
   
   static Int_t icanvas = -1;
   Int_t ixc = 100 + (icanvas)*40;
   Int_t iyc =   0 + (icanvas+1)*20;   
   if (MovieMode) ixc = iyc = 0;
   TString canvasnumber =  Form( "c%i", icanvas );
   TString canvastitle = Form("Neural Network Layout for: %s", d->GetName());
   TCanvas* c = new TCanvas( canvasnumber, canvastitle, 
                             ixc, 0 + (icanvas+1)*20, 1000, 650  );
   icanvas++;
   TIter next = d->GetListOfKeys();
   TKey *key( 0 );
   Int_t numHists = 0;

   // loop over all histograms with hName in name again
   next.Reset();
   Double_t maxWeight = 0;
   // find max weight
   while ((key = (TKey*)next())) {

      TClass *cl = gROOT->GetClass(key->GetClassName());
      if (!cl->InheritsFrom("TH2F")) 
         {
            continue;          
         }else
         {
            std::cout<<key->GetClassName()<<"----"<<cl->InheritsFrom("TH2F")<<"----"<<hName<<std::endl;          
         }
  
      TH2F* h = (TH2F*)key->ReadObj();    
      if (!h) {
         cout << "Big troubles in \"draw_network\" (1)" << endl;
         exit(1);
      }
      std::cout<<h->GetName()<<"----"<<hName<<std::endl;
      if (TString(h->GetName()).Contains( hName )){
         numHists++;
 
         Int_t n1 = h->GetNbinsX();
         Int_t n2 = h->GetNbinsY();
         for (Int_t i = 0; i < n1; i++) {
            for (Int_t j = 0; j < n2; j++) {
               Double_t weight = TMath::Abs(h->GetBinContent(i+1, j+1));
               if (maxWeight < weight) maxWeight = weight;
            }
         }
      }
   }
   if (numHists == 0) {
      cout << "Error: could not find histograms" << endl;
      //exit(1);
   }

   // draw network
   next.Reset();
   //cout << "check4a" << endl;

   Int_t count = 0;
   while ((key = (TKey*)next())) {
      //cout << "check4b" << endl;

      TClass *cl = gROOT->GetClass(key->GetClassName());
      if (!cl->InheritsFrom("TH2F")) continue;    
      //cout << "check4c" << endl;

      TH2F* h = (TH2F*)key->ReadObj();    
      //cout << (h->GetName()) << endl;
      if (!h) {
         cout << "Big troubles in \"draw_network\" (2)" << endl;
         exit(1);
      }
      //cout << (h->GetName()) << endl;
      if (TString(h->GetName()).Contains( hName )) {
         //cout << (h->GetName()) << endl;
         draw_layer(dataset,c, h, count++, numHists+1, maxWeight);
      }
      //cout << "check4d" << endl;

   }
   draw_layer_labels(numHists+1);

   // add epoch
   if (MovieMode) {
      TText* t = new TText();            
      t->SetTextSize( 0.04 );
      t->SetTextColor( 0 );
      t->SetTextAlign( 31 );      
      t->DrawTextNDC( 1 - c->GetRightMargin(), 1 - c->GetTopMargin() - 0.033, 
                      Form( "Epoch: %s", epoch.Data() ) );
   }

   // ============================================================
   if (__PRINT_LOGO__) TMVAGlob::plot_logo();
   // ============================================================  

   c->Update();
   if (MovieMode) {
      // save to file
      TString dirname  = "movieplots";
      TString foutname = dirname + "/" + hName;
      foutname.Resize( foutname.Length()-5 );
      foutname.ReplaceAll("epochmonitoring___","");
      foutname += ".gif";
      
      cout << "storing file: " << foutname << endl;
      c->Print(foutname);     
      c->Clear();
      delete c;
   }
   else {
      TString fname = dataset+"/plots/network";
      TMVAGlob::imgconv( c, fname );
   }

   // reset global style changes so that it does not affect other plots
   TMVAStyle->SetCanvasColor    ( canvasColor );
   TMVAStyle->SetTitleFillColor ( titleFillColor );
   TMVAStyle->SetTitleTextColor ( titleTextColor );
   TMVAStyle->SetTitleBorderSize( borderSize );

}

void TMVA::draw_layer_labels(Int_t nLayers)
{
   const Double_t LABEL_HEIGHT = 0.032;
   const Double_t LABEL_WIDTH  = 0.20;
   Double_t effWidth = 0.8*(1.0-LABEL_WIDTH)/nLayers;
   Double_t height = 0.8*LABEL_HEIGHT;
   Double_t margY = LABEL_HEIGHT - height;

   for (Int_t i = 0; i < nLayers; i++) {
      TString label = Form("Layer %i", i);
      if (i == nLayers-1) label = "Output layer";
      Double_t cx = i*(1.0-LABEL_WIDTH)/nLayers+1.0/(2.0*nLayers)+LABEL_WIDTH;
      Double_t x1 = cx-0.8*effWidth/2.0;
      Double_t x2 = cx+0.8*effWidth/2.0;
      Double_t y1 = margY;
      Double_t y2 = margY + height;

      TPaveLabel *p = new TPaveLabel(x1, y1, x2, y2, label+"", "br");
      p->SetFillColor(gStyle->GetTitleFillColor());
      p->SetTextColor(gStyle->GetTitleTextColor());
      p->SetFillStyle(1001);
      p->SetBorderSize( 0 );
      p->Draw();
   }
}

void TMVA::draw_input_labels(TString dataset,Int_t nInputs, Double_t* cy, 
                             Double_t rad, Double_t layerWidth)
{
   const Double_t LABEL_HEIGHT = 0.04;
   const Double_t LABEL_WIDTH  = 0.20;
   Double_t width = LABEL_WIDTH + (layerWidth-4*rad);
   Double_t margX = 0.01;
   Double_t effHeight = 0.8*LABEL_HEIGHT;

   TString *varNames = get_var_names(dataset,nInputs);
   if (varNames == 0) exit(1);

   TString input;

   for (Int_t i = 0; i < nInputs; i++) {
      if (i != nInputs-1) input = varNames[i];
      else                input = "Bias node";
      Double_t x = margX + width;
      Double_t y = cy[i] - effHeight;

      TText* t = new TText();
      t->SetTextColor(gStyle->GetTitleTextColor());
      t->SetTextAlign(31);
      t->SetTextSize(LABEL_HEIGHT);
      if (i == nInputs-1) t->SetTextColor( TColor::GetColor( "#AFDCEC" ) );
      t->DrawText( x, y+0.018, input + " :");
   }

   delete[] varNames;
}

TString* TMVA::get_var_names(TString dataset, Int_t nVars )
{
   const TString directories[6] = { "InputVariables_NoTransform",
                                    "InputVariables_DecorrTransform",
                                    "InputVariables_PCATransform",
                                    "InputVariables_Id",
                                    "InputVariables_Norm",
                                    "InputVariables_Deco"};
   
   TDirectory* dir = 0;
   for (const auto & directorie : directories) {
      dir = (TDirectory*)Network_GFile->GetDirectory(dataset.Data())->Get( directorie );
      if (dir != 0) break;
   }
   if (dir==0) {
      cout << "*** Big troubles in macro \"network.cxx\": could not find directory for input variables, "
           << "and hence could not determine variable names --> abort" << endl;
      return 0;
   }
   dir->cd();

   TString* vars = new TString[nVars];
   Int_t ivar = 0;

   // loop over all objects in directory
   TIter next(dir->GetListOfKeys());
   TKey* key = 0;
   while ((key = (TKey*)next())) {
      if (key->GetCycle() != 1) continue;

      if (!TString(key->GetName()).Contains("__S") &&
          !TString(key->GetName()).Contains("__r") &&
          !TString(key->GetName()).Contains("Regression"))
         continue;
      if (TString(key->GetName()).Contains("target"))
         continue;

      // make sure, that we only look at histograms
      TClass *cl = gROOT->GetClass(key->GetClassName());
      if (!cl->InheritsFrom("TH1")) continue;
      TH1 *sig = (TH1*)key->ReadObj();
      TString hname = sig->GetTitle();
      
      vars[ivar] = hname; ivar++;

      if (ivar > nVars-1) break;
   }      
   
   if (ivar != nVars-1) { // bias layer and targets are also in nVars counts
      cout << "*** Troubles in \"network.cxx\": did not reproduce correct number of "
           << "input variables: " << ivar << " != " << nVars << endl;
   }

   return vars;
}

void TMVA::draw_activation(TCanvas* c, Double_t cx, Double_t cy, 
                           Double_t radx, Double_t rady, Int_t whichActivation)
{
   TImage *activation = NULL;

   switch (whichActivation) {
   case 0:
      activation = TMVA::TMVAGlob::findImage("sigmoid-small.png");
      break;
   case 1:
      activation = TMVA::TMVAGlob::findImage("line-small.png");
      break;
   default:
      cout << "Activation index " << whichActivation << " is not known." << endl;
      cout << "You messed up or you need to modify network.cxx to introduce a new "
           << "activation function (and image) corresponding to this index" << endl;
   }

   if (activation == NULL) {
      cout << "Could not create an image... exit" << endl;
      return;
   }
  
   activation->SetConstRatio(kFALSE);

   radx *= 0.7;
   rady *= 0.7;
   TString name = Form("activation%f%f", cx, cy);
   TPad* p = new TPad(name+"", name+"", cx-radx, cy-rady, cx+radx, cy+rady);

   p->Draw();
   p->cd();

   activation->Draw();
   c->cd();
}

void TMVA::draw_layer(TString dataset,TCanvas* c, TH2F* h, Int_t iHist, 
                      Int_t nLayers, Double_t maxWeight)
{
   const Double_t MAX_NEURONS_NICE = 12;
   const Double_t LABEL_HEIGHT = 0.03;
   const Double_t LABEL_WIDTH  = 0.20;
   Double_t ratio = ((Double_t)(c->GetWindowHeight())) / c->GetWindowWidth();
   Double_t rad, cx1, *cy1, cx2, *cy2;

   // this is the smallest radius that will still display the activation images
   rad = 0.04*650/c->GetWindowHeight();

   Int_t nNeurons1 = h->GetNbinsX();
   cx1 = iHist*(1.0-LABEL_WIDTH)/nLayers + 1.0/(2.0*nLayers) + LABEL_WIDTH;
   cy1 = new Double_t[nNeurons1];

   Int_t nNeurons2 = h->GetNbinsY();
   cx2 = (iHist+1)*(1.0-LABEL_WIDTH)/nLayers + 1.0/(2.0*nLayers) + LABEL_WIDTH;
   cy2 = new Double_t[nNeurons2];

   Double_t effRad1 = rad;
   if (nNeurons1 > MAX_NEURONS_NICE)
      effRad1 = 0.8*(1.0-LABEL_HEIGHT)/(2.0*nNeurons1);

   for (Int_t i = 0; i < nNeurons1; i++) {
      cy1[nNeurons1-i-1] = i*(1.0-LABEL_HEIGHT)/nNeurons1 + 1.0/(2.0*nNeurons1) + LABEL_HEIGHT;

      if (iHist == 0) {

         TEllipse *ellipse = new TEllipse( cx1, cy1[nNeurons1-i-1], 
                                           effRad1*ratio, effRad1, 0, 360, 0 );
         ellipse->SetFillColor(TColor::GetColor( "#fffffd" ));
         ellipse->SetFillStyle(1001);
         ellipse->Draw();

         if (i == 0) ellipse->SetLineColor(9);

         if (nNeurons1 > MAX_NEURONS_NICE) continue;

         Int_t whichActivation = 0;
         if (iHist==0 || iHist==nLayers-1 || i==0) whichActivation = 1;
         draw_activation(c, cx1, cy1[nNeurons1-i-1], 
                         rad*ratio, rad, whichActivation);
      }
   }

   if (iHist == 0) draw_input_labels(dataset,nNeurons1, cy1, rad, (1.0-LABEL_WIDTH)/nLayers);

   Double_t effRad2 = rad;
   if (nNeurons2 > MAX_NEURONS_NICE)
      effRad2 = 0.8*(1.0-LABEL_HEIGHT)/(2.0*nNeurons2);

   for (Int_t i = 0; i < nNeurons2; i++) {
      cy2[nNeurons2-i-1] = i*(1.0-LABEL_HEIGHT)/nNeurons2 + 1.0/(2.0*nNeurons2) + LABEL_HEIGHT;

      TEllipse *ellipse = 
         new TEllipse(cx2, cy2[nNeurons2-i-1], effRad2*ratio, effRad2, 0, 360, 0);
      ellipse->SetFillColor(TColor::GetColor( "#fffffd" ));
      ellipse->SetFillStyle(1001);
      ellipse->Draw();

      if (i == 0 && nNeurons2 > 1) ellipse->SetLineColor(9);

      if (nNeurons2 > MAX_NEURONS_NICE) continue;

      Int_t whichActivation = 0;
      if (iHist+1==0 || iHist+1==nLayers-1 || i==0) whichActivation = 1;
      draw_activation(c, cx2, cy2[nNeurons2-i-1], rad*ratio, rad, whichActivation);
   }

   for (Int_t i = 0; i < nNeurons1; i++) {
      for (Int_t j = 0; j < nNeurons2; j++) {
         draw_synapse(cx1, cy1[i], cx2, cy2[j], effRad1*ratio, effRad2*ratio,
                      h->GetBinContent(i+1, j+1)/maxWeight);
      }
   }

   delete [] cy1;
   delete [] cy2;
}

void TMVA::draw_synapse(Double_t cx1, Double_t cy1, Double_t cx2, Double_t cy2,
                        Double_t  rad1, Double_t rad2, Double_t weightNormed)
{
   const Double_t TIP_SIZE   = 0.01;
   const Double_t MAX_WEIGHT = 8;
   const Double_t MAX_COLOR  = 100;  // red
   const Double_t MIN_COLOR  = 60;   // blue

   if (weightNormed == 0) return;

   //   gStyle->SetPalette(100, NULL);

   TArrow *arrow = new TArrow(cx1+rad1, cy1, cx2-rad2, cy2, TIP_SIZE, ">");
   arrow->SetFillColor(1);
   arrow->SetFillStyle(1001);
   arrow->SetLineWidth((Int_t)(TMath::Abs(weightNormed)*MAX_WEIGHT+0.5));
   arrow->SetLineColor((Int_t)((weightNormed+1.0)/2.0*(MAX_COLOR-MIN_COLOR)+MIN_COLOR+0.5));
   arrow->Draw();
}

// input: - Input file (result from TMVA),
//        - use of TMVA plotting TStyle
void TMVA::network(TString dataset, TString fin , Bool_t useTMVAStyle )
{
   // set style and remove existing canvas'
   TMVAGlob::Initialize( useTMVAStyle );

   // checks if file with name "fin" is already open, and if not opens one
   TFile* file = TMVAGlob::OpenFile( fin );  
   TIter next(file->GetDirectory(dataset.Data())->GetListOfKeys());
   TKey *key(0);
   while( (key = (TKey*)next()) ) {      
      if (!TString(key->GetName()).BeginsWith("Method_MLP")) continue;
      if( ! gROOT->GetClass(key->GetClassName())->InheritsFrom("TDirectory") ) continue;

      cout << "--- Found directory: " << ((TDirectory*)key->ReadObj())->GetName() << endl;

      TDirectory* mDir = (TDirectory*)key->ReadObj();

      TIter keyIt(mDir->GetListOfKeys());
      TKey *titkey;
      while((titkey = (TKey*)keyIt())) {
         if( ! gROOT->GetClass(titkey->GetClassName())->InheritsFrom("TDirectory") ) continue;

         TDirectory* dir = (TDirectory *)titkey->ReadObj();
         dir->cd();  
         TList titles;
         UInt_t ni = TMVAGlob::GetListOfTitles( dir, titles );
         if (ni==0) {
            cout << "No titles found for Method_MLP" << endl;
            return;
         }
         draw_network(dataset, file, dir );
      }
   }

   return;
}

