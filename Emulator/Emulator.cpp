// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include <TFile.h>

#include "Emulator.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::OUTPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char* emulator_spec[] =
{
   "implementation_id", "Emulator",
   "type_name",         "Emulator",
   "description",       "Emulator component",
   "version",           "1.0",
   "vendor",            "Kazuo Nakayoshi, KEK",
   "category",          "example",
   "activity_type",     "DataFlowComponent",
   "max_instance",      "1",
   "language",          "C++",
   "lang_type",         "compile",
   ""
};

Emulator::Emulator(RTC::Manager* manager)
   : DAQMW::DaqComponentBase(manager),
   m_OutPort("emulator_out", m_out_data),
   m_recv_byte_size(0),
   m_out_status(BUF_SUCCESS),
   m_debug(false),
   fFileName(""),
   fHistName(""),
   fHist(nullptr)
{
   // Registration: InPort/OutPort/Service

   // Set OutPort buffers
   registerOutPort("emulator_out", m_OutPort);

   init_command_port();
   init_state_table();
   set_comp_name("EMULATOR");

   fGenerator = std::mt19937(time(nullptr));
   fRandomBool = std::bernoulli_distribution(0.1);
   fRandomPoisson = std::poisson_distribution<int>(3);
   
   fIsADC = true;
}

Emulator::~Emulator()
{
   delete fHist;
}

RTC::ReturnCode_t Emulator::onInitialize()
{
   if (m_debug) {
      std::cerr << "Emulator::onInitialize()" << std::endl;
   }

   return RTC::RTC_OK;
}

RTC::ReturnCode_t Emulator::onExecute(RTC::UniqueId ec_id)
{
   daq_do();

   return RTC::RTC_OK;
}

int Emulator::daq_dummy()
{
   return 0;
}

int Emulator::daq_configure()
{
   std::cerr << "*** Emulator::configure" << std::endl;

   ::NVList* paramList;
   paramList = m_daq_service0.getCompParams();
   parse_params(paramList);

   delete fHist;
   fHist = nullptr;
   
   auto file = new TFile(fFileName.c_str(), "READ");
   fHist = (TH1D*) file->Get(fHistName.c_str());
   fHist->SetDirectory(nullptr);
   file->Close();
   delete file;
   
   return 0;
}

int Emulator::parse_params(::NVList* list)
{
   std::cerr << "param list length:" << (*list).length() << std::endl;

   int len = (*list).length();
   for (int i = 0; i < len; i+=2) {
      std::string sname  = (std::string)(*list)[i].value;
      std::string svalue = (std::string)(*list)[i+1].value;

      std::cerr << "sname: " << sname << "  ";
      std::cerr << "value: " << svalue << std::endl;

      if(sname == "fileName") fFileName = svalue;
      else if(sname == "histName") fHistName = svalue;
      else if(sname == "sourceType"){
	if(svalue == "ADC") fIsADC = true;
	else fIsADC = false;
      }
   }

   return 0;
}

int Emulator::daq_unconfigure()
{
   std::cerr << "*** Emulator::unconfigure" << std::endl;

   delete fHist;
   fHist = nullptr;

   return 0;
}

int Emulator::daq_start()
{
   std::cerr << "*** Emulator::start" << std::endl;

   m_out_status = BUF_SUCCESS;

   return 0;
}

int Emulator::daq_stop()
{
   std::cerr << "*** Emulator::stop" << std::endl;

   return 0;
}

int Emulator::daq_pause()
{
   std::cerr << "*** Emulator::pause" << std::endl;

   return 0;
}

int Emulator::daq_resume()
{
   std::cerr << "*** Emulator::resume" << std::endl;

   return 0;
}

int Emulator::read_data_from_detectors()
{
  int received_data_size = 0;
  /// write your logic here
  auto data = PHAData();
  constexpr auto sizeCh = sizeof(PHAData::ChNumber);
  constexpr auto sizeTS = sizeof(PHAData::TimeStamp);
  constexpr auto sizeEne = sizeof(PHAData::Energy);
  data.ChNumber = 0;
  data.TimeStamp = 0;

  if(fRandomBool(fGenerator)) {
    const auto nData = fRandomPoisson(fGenerator);
     for(auto i = 0; i < nData; i++) {
        memcpy(&m_data[received_data_size], &(data.ChNumber), sizeCh);
        received_data_size += sizeCh;

        memcpy(&m_data[received_data_size], &(data.TimeStamp), sizeTS);
        received_data_size += sizeTS;

	if(fIsADC) data.Energy = fHist->GetRandom();
	else data.Energy = fHist->GetRandom() * 1000;
        memcpy(&m_data[received_data_size], &(data.Energy), sizeEne);
        received_data_size += sizeEne;
     }
  }
  
  return received_data_size;
}

int Emulator::set_data(unsigned int data_byte_size)
{
   unsigned char header[8];
   unsigned char footer[8];

   set_header(&header[0], data_byte_size);
   set_footer(&footer[0]);

   ///set OutPort buffer length
   m_out_data.data.length(data_byte_size + HEADER_BYTE_SIZE + FOOTER_BYTE_SIZE);
   memcpy(&(m_out_data.data[0]), &header[0], HEADER_BYTE_SIZE);
   memcpy(&(m_out_data.data[HEADER_BYTE_SIZE]), &m_data[0], data_byte_size);
   memcpy(&(m_out_data.data[HEADER_BYTE_SIZE + data_byte_size]), &footer[0],
          FOOTER_BYTE_SIZE);

   return 0;
}

int Emulator::write_OutPort()
{
   ////////////////// send data from OutPort  //////////////////
   bool ret = m_OutPort.write();

   //////////////////// check write status /////////////////////
   if (ret == false) {  // TIMEOUT or FATAL
      m_out_status  = check_outPort_status(m_OutPort);
      if (m_out_status == BUF_FATAL) {   // Fatal error
         fatal_error_report(OUTPORT_ERROR);
      }
      if (m_out_status == BUF_TIMEOUT) { // Timeout
         return -1;
      }
   }
   else {
      m_out_status = BUF_SUCCESS; // successfully done
   }

   return 0;
}

int Emulator::daq_run()
{
   if (m_debug) {
      std::cerr << "*** Emulator::run" << std::endl;
   }

   if (check_trans_lock()) {  // check if stop command has come
      set_trans_unlock();    // transit to CONFIGURED state
      return 0;
   }

   if (m_out_status == BUF_SUCCESS) {   // previous OutPort.write() successfully done
      m_recv_byte_size = read_data_from_detectors();
      if (m_recv_byte_size > 0) {
         set_data(m_recv_byte_size); // set data to OutPort Buffer
      } else {
         return 0;
      }
   }

   if (write_OutPort() < 0) {
      ;     // Timeout. do nothing.
   }
   else {    // OutPort write successfully done
     // inc_sequence_num();                     // increase sequence num.
      inc_total_data_size(m_recv_byte_size);  // increase total data byte size
   }

   return 0;
}

extern "C"
{
   void EmulatorInit(RTC::Manager* manager)
   {
      RTC::Properties profile(emulator_spec);
      manager->registerFactory(profile,
                               RTC::Create<Emulator>,
                               RTC::Delete<Emulator>);
   }
};
