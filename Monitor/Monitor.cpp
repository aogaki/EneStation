// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include <algorithm>

#include <TSystem.h>
#include <TStyle.h>
#include <TBufferJSON.h>
#include <TCanvas.h>

#include "Monitor.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::INPORT_ERROR;
using DAQMW::FatalType::HEADER_DATA_MISMATCH;
using DAQMW::FatalType::FOOTER_DATA_MISMATCH;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char* monitor_spec[] =
  {
    "implementation_id", "Monitor",
    "type_name",         "Monitor",
    "description",       "Monitor component",
    "version",           "1.0",
    "vendor",            "Kazuo Nakayoshi, KEK",
    "category",          "example",
    "activity_type",     "DataFlowComponent",
    "max_instance",      "1",
    "language",          "C++",
    "lang_type",         "compile",
    ""
  };

// This factor is for fitting
constexpr auto kBGRange = 2.5;
constexpr auto kFitRange = 5.;
Double_t FitFnc(Double_t *pos, Double_t *par)
{  // This should be class not function.
  const auto x = pos[0];
  const auto mean = par[1];
  const auto sigma = par[2];

  const auto limitHigh = mean + kBGRange * sigma;
  const auto limitLow = mean - kBGRange * sigma;

  auto val = par[0] * TMath::Gaus(x, mean, sigma);

  auto backGround = 0.;
  if (x < limitLow)
    backGround = par[3] + par[4] * x;
  else if (x > limitHigh)
    backGround = par[5] + par[6] * x;
  else {
    auto xInc = limitHigh - limitLow;
    auto yInc = (par[5] + par[6] * limitHigh) - (par[3] + par[4] * limitLow);
    auto slope = yInc / xInc;

    backGround = (par[3] + par[4] * limitLow) + slope * (x - limitLow);
  }

  if (backGround < 0.) backGround = 0.;
  val += backGround;

  return val;
}

Monitor::Monitor(RTC::Manager* manager)
  : DAQMW::DaqComponentBase(manager),
  m_InPort("monitor_in",   m_in_data),
  m_in_status(BUF_SUCCESS),

  m_debug(false),

  fPool(mongocxx::uri("mongodb://daq:nim2camac@172.18.4.56/GBS"))
{
  // Registration: InPort/OutPort/Service

  // Set InPort buffers
  registerInPort ("monitor_in",  m_InPort);
   
  init_command_port();
  init_state_table();
  set_comp_name("MONITOR");

  gStyle->SetOptStat(1111);
  gStyle->SetOptFit(1111);
  fServ.reset(new THttpServer("http:8080?monitoring=5000;rw;noglobal"));
  //fServ.reset(new THttpServer());

  fP0 = 0;
  fP1 = 1;

  fCounter = 0;
  fLastEventCount = 0;
  fUploadInterval = 60; // in second
  fLastUpload = time(nullptr);

  fPeakPosition = 0.;
  fTargetPosition = 0.;

  fFitFnc.reset(new TF1("FitFnc", FitFnc, 0, 100, 7));

  fSpectrum.reset(new TSpectrum(knPeaks * 2));
  fPeakThreshold = 0.2;
  
  fGausResult.reset(new TF1("GausResult", "gaus"));
  fGausResult->SetLineColor(kMagenta);
  fGausResult->SetLineWidth(4);

  fBGResult.reset(new TPolyLine());
  fBGResult->SetLineColor(kGreen);
  fBGResult->SetLineWidth(4);
  fBGResult->SetFillStyle(0);
}

Monitor::~Monitor()
{
}

RTC::ReturnCode_t Monitor::onInitialize()
{
  if (m_debug) {
    std::cerr << "Monitor::onInitialize()" << std::endl;
  }
   
  return RTC::RTC_OK;
}

RTC::ReturnCode_t Monitor::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int Monitor::daq_dummy()
{
  gSystem->ProcessEvents();
  return 0;
}

int Monitor::daq_configure()
{
  std::cerr << "*** Monitor::configure" << std::endl;

  ::NVList* paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  if(fP1 == 0) fP1 = 1;
  auto binW = fP1;
  auto xlow = binW / 2.;
  auto nbinsx = 30. / binW; // Till 30 MeV
  auto xhigh = xlow + binW * nbinsx;
  fHist.reset(new TH1D("hist", "Energy distribution", nbinsx, xlow, xhigh));
  fHist->SetXTitle("[MeV]");

  fHistRaw.reset(new TH1D("raw", "Energy distribution (ADC)", 33000, 0.5, 33000.5));
   
  fServ->Register("/", fHist.get());
  fServ->Register("/", fHistRaw.get());

  gStyle->SetOptStat(1111);
  gStyle->SetOptFit(1111);

  // Using name of histogram "hist", NOT the variable "fHist"
  fServ->RegisterCommand("/Reset","/hist/->Reset()", "button;rootsys/icons/ed_delete.png");
  fServ->RegisterCommand("/ResetRaw","/raw/->Reset()", "button;rootsys/icons/ed_delete.png");

  return 0;
}

int Monitor::parse_params(::NVList* list)
{

  std::cerr << "param list length:" << (*list).length() << std::endl;

  int len = (*list).length();
  for (int i = 0; i < len; i+=2) {
    std::string sname  = (std::string)(*list)[i].value;
    std::string svalue = (std::string)(*list)[i+1].value;

    std::cerr << "sname: " << sname << "  ";
    std::cerr << "value: " << svalue << std::endl;

    if(sname == "p0") fP0 = std::stod(svalue);
    else if(sname == "p1") fP1 = std::stod(svalue);
    else if(sname == "UploadInterval") fUploadInterval = std::stoi(svalue);
    else if(sname == "PeakThreshold") fPeakThreshold = std::stod(svalue);
  }
   
  return 0;
}

int Monitor::daq_unconfigure()
{
  std::cerr << "*** Monitor::unconfigure" << std::endl;
   
  return 0;
}

int Monitor::daq_start()
{
  std::cerr << "*** Monitor::start" << std::endl;
  m_in_status  = BUF_SUCCESS;
  return 0;
}

int Monitor::daq_stop()
{
  std::cerr << "*** Monitor::stop" << std::endl;
  reset_InPort();

  return 0;
}

int Monitor::daq_pause()
{
  std::cerr << "*** Monitor::pause" << std::endl;

  return 0;
}

int Monitor::daq_resume()
{
  std::cerr << "*** Monitor::resume" << std::endl;

  return 0;
}

int Monitor::reset_InPort()
{
  int ret = true;
  while(ret == true) {
    ret = m_InPort.read();
  }

  return 0;
}

unsigned int Monitor::read_InPort()
{
  /////////////// read data from InPort Buffer ///////////////
  unsigned int recv_byte_size = 0;
  bool ret = m_InPort.read();

  //////////////////// check read status /////////////////////
  if (ret == false) { // false: TIMEOUT or FATAL
    m_in_status = check_inPort_status(m_InPort);
    if (m_in_status == BUF_TIMEOUT) { // Buffer empty.
      if (check_trans_lock()) {     // Check if stop command has come.
	set_trans_unlock();       // Transit to CONFIGURE state.
      }
    }
    else if (m_in_status == BUF_FATAL) { // Fatal error
      fatal_error_report(INPORT_ERROR);
    }
  }
  else {
    recv_byte_size = m_in_data.data.length();
  }

  if (m_debug) {
    std::cerr << "m_in_data.data.length():" << recv_byte_size
	      << std::endl;
  }

  return recv_byte_size;
}

int Monitor::daq_run()
{
  if (m_debug) {
    std::cerr << "*** Monitor::run" << std::endl;
  }

  unsigned int recv_byte_size = read_InPort();
  if (recv_byte_size == 0) { // Timeout
    return 0;
  }

  check_header_footer(m_in_data, recv_byte_size); // check header and footer
  unsigned int event_byte_size = get_event_size(recv_byte_size);

  /////////////  Write component main logic here. /////////////
  // online_analyze();
  /////////////////////////////////////////////////////////////

  FillHist(event_byte_size);

  constexpr long updateInterval = 10;
  if((fCounter++ % updateInterval) == 0){
    FindPeaks();
    FitHist();

    auto now = time(nullptr);
    if(fUploadInterval < now - fLastUpload) {
      PlotAndUpload();
      UploadFlux();
      fLastUpload = now;
    }

  } 

  gSystem->ProcessEvents();

  // inc_sequence_num();                       // increase sequence num.
  inc_total_data_size(event_byte_size);     // increase total data byte size

  return 0;
}

void Monitor::FillHist(int size)
{
  constexpr auto sizeCh = sizeof(PHAData::ChNumber);
  constexpr auto sizeTS = sizeof(PHAData::TimeStamp);
  constexpr auto sizeEne = sizeof(PHAData::Energy);

  constexpr unsigned int headerSize = 8;

  PHAData data;
  
  for(unsigned int i = headerSize; i < size + headerSize;) {
    // The order of data should be the same as Reader
    memcpy(&data.ChNumber, &m_in_data.data[i], sizeCh);
    i += sizeCh;

    memcpy(&data.TimeStamp, &m_in_data.data[i], sizeTS);
    i += sizeTS;

    memcpy(&data.Energy, &m_in_data.data[i], sizeEne);
    i += sizeEne;

    if(data.ChNumber == 0) {
      fHistRaw->Fill(data.Energy);
      // Reject the overflow events
      if(data.Energy < (1 << 15)) fHist->Fill(data.Energy * fP1 + fP0);
    }
  }
}

void Monitor::FindPeaks()
{
  /*
    fHistBG.reset(fSpectrum->Background((fHist.get())));
    fHistPeaks.reset((TH1D *)fHist->Clone("peaks"));
    for (auto iBin = 1; iBin <= fHist->GetNbinsX(); iBin++) {
    auto binContent = fHistPeaks->GetBinContent(iBin) - fHistBG->GetBinContent(iBin);
    if (binContent < 0) binContent = 0;
    fHistPeaks->SetBinContent(iBin, binContent);
    }
  */
  fSpectrum->Search(fHist.get(), 5, "", fPeakThreshold);
  auto peakPos = fSpectrum->GetPositionX();
  std::sort(peakPos, peakPos + fSpectrum->GetNPeaks(),
	    [](const Double_t &left, const Double_t &right){return left > right;});
  fPeakPosition = peakPos[0];  // Most right peak
   
  //for (auto iPeak = 0; iPeak < knPeaks; iPeak++) {
  //std::cout << peakPos[iPeak] << std::endl;
  //}
}

void Monitor::FitHist()
{
  auto mean = fPeakPosition;
  auto sigma = 0.01;
  auto height = fHist->GetBinContent(fHist->FindBin(mean));

  auto gausFit = new TF1("GausFit", "gaus");
   
  gausFit->SetParameter(0, height);
  gausFit->SetParameter(1, mean);
  gausFit->SetParameter(2, sigma);
  gausFit->SetRange(mean - sigma, mean + sigma);
  gausFit->SetParLimits(1, 0.9 * mean, 1.1 * mean);
  gausFit->SetParLimits(2, 0., 10.);
  fHist->Fit(gausFit, "RQN");

  mean = gausFit->GetParameter(1);
  sigma = gausFit->GetParameter(2);
  gausFit->SetRange(mean - sigma, mean + sigma);
  fHist->Fit(gausFit, "RQN");
    
  mean = gausFit->GetParameter(1);
  sigma = gausFit->GetParameter(2);
  fFitFnc->SetRange(mean - kFitRange * sigma, mean + kFitRange * sigma);
  fFitFnc->SetParameters(height, mean, sigma, 0, 0, 0, 0);
  fHist->Fit(fFitFnc.get(), "RQ");
   
  mean = fFitFnc->GetParameter(1);
  sigma = fFitFnc->GetParameter(2);
  fFitFnc->SetRange(mean - kFitRange * sigma, mean + kFitRange * sigma);
  // fFitFnc->SetParameters(height, mean, sigma, 0, 0, 0, 0);
  fHist->Fit(fFitFnc.get(), "RQ");
   
  //std::cout << mean <<"\t"<< sigma << std::endl;

  delete gausFit;
}
/*
  void Monitor::PlotResult()
  {
  fCanvas->cd();

  // Reset to the initial state
  //fHist->SetAxisRange(-1111, -1111);
  //fFitFnc->SetParameters(par);
  }
*/
void Monitor::PlotAndUpload()
{
  // PlotResult();
  auto canvas = new TCanvas("canvas", "Fit result");

  auto par = fFitFnc->GetParameters();
  auto height = par[0];
  auto mean = par[1];
  auto sigma = par[2];
  fHist->SetAxisRange(mean - 10 * sigma, mean + 10 * sigma);
  fHist->Draw();

  // fGausResult->Clear();
  fGausResult->SetParameters(height, mean, sigma);
  fGausResult->SetRange(mean - 10 * sigma, mean + 10 * sigma);
  fGausResult->Draw("SAME");

  //fBGResult->Clear();
  fFitFnc->SetParameter(0, 0);
  auto x = mean - kFitRange * sigma;
  auto y = fFitFnc->Eval(x);
  auto point = 0;
  fBGResult->SetPoint(point++, x, y);
   
  x = mean - kBGRange * sigma;
  y = fFitFnc->Eval(x);
  fBGResult->SetPoint(point++, x, y);
   
  x = mean + kBGRange * sigma;
  y = fFitFnc->Eval(x);
  fBGResult->SetPoint(point++, x, y);
   
  x = mean + kFitRange * sigma;
  y = fFitFnc->Eval(x);
  fBGResult->SetPoint(point++, x, y);

  fBGResult->Draw("SAME");

  canvas->Modified();
  canvas->Update();

  // Upload result
  auto conn = fPool.acquire();
  auto collection = (*conn)["GBS"]["Energy"];

  auto result = TBufferJSON::ToJSON(canvas);
  // MongoDB driver do not allow to use $pair as key
  result.ReplaceAll("$pair", "aogaki_pair");

  auto FWHM = sigma * 2 * sqrt(2 * log(2));
  FWHM *= 1000; // MeV -> keV
   
  bsoncxx::builder::stream::document buf{};
  buf << "mean" << Form("%2.4f", mean)
      << "fwhm" << Form("%4.2f", FWHM)
      << "time" << std::to_string(time(0))
      << "fit" << result.Data();
  collection.insert_one(buf.view());
  buf.clear();

  canvas->Print("/DAQ/tmp.pdf", "pdf");

  fHist->SetAxisRange(-1111, -1111);
   
  delete canvas;
}

void Monitor::UploadFlux()
{
  constexpr auto countRange = 1; // +- countRange * sigma = count area
  auto mean = fFitFnc->GetParameter(1);
  auto sigma = fFitFnc->GetParameter(2);
  auto startBin = fHist->FindBin(mean - countRange * sigma);
  auto stopBin = fHist->FindBin(mean + countRange * sigma);
  if(startBin > stopBin) std::swap(startBin, stopBin);

  auto sum = 0;
  for(auto iBin = startBin; iBin <= stopBin; iBin++){
    sum += fHist->GetBinContent(iBin);
  }

  if(sum < fLastEventCount) { // After reset or something 
    fLastEventCount = sum; // Do nothing
  } else {
    auto flux = sum - fLastEventCount;
    fLastEventCount = sum;

    auto rate = double(sum) / double(fUploadInterval);

    auto conn = fPool.acquire();
    auto collection = (*conn)["GBS"]["Flux"];
    bsoncxx::builder::stream::document buf{};
    buf << "count" << std::to_string(flux) << "hz" << std::to_string(rate)
	<< "time" << std::to_string(time(nullptr));  
    collection.insert_one(buf.view());
    buf.clear();
  }

}

extern "C"
{
  void MonitorInit(RTC::Manager* manager)
  {
    RTC::Properties profile(monitor_spec);
    manager->registerFactory(profile,
			     RTC::Create<Monitor>,
			     RTC::Delete<Monitor>);
  }
};
