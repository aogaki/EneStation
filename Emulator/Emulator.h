// -*- C++ -*-
/*!
 * @file 
 * @brief
 * @date
 * @author
 *
 */

#ifndef EMULATOR_H
#define EMULATOR_H

#include <string>
#include <random>

#include "TH1.h"

#include "DaqComponentBase.h"
#include "../../TDigiTES/include/TPHAData.hpp"


using namespace RTC;

class Emulator
  : public DAQMW::DaqComponentBase
{
public:
  Emulator(RTC::Manager* manager);
  ~Emulator();

  // The initialize action (on CREATED->ALIVE transition)
  // former rtc_init_entry()
  virtual RTC::ReturnCode_t onInitialize();

  // The execution action that is invoked periodically
  // former rtc_active_do()
  virtual RTC::ReturnCode_t onExecute(RTC::UniqueId ec_id);

private:
  TimedOctetSeq          m_out_data;
  OutPort<TimedOctetSeq> m_OutPort;

private:
  int daq_dummy();
  int daq_configure();
  int daq_unconfigure();
  int daq_start();
  int daq_run();
  int daq_stop();
  int daq_pause();
  int daq_resume();

  int parse_params(::NVList* list);
  int read_data_from_detectors();
  int set_data(unsigned int data_byte_size);
  int write_OutPort();

  static constexpr int SEND_BUFFER_SIZE = 4096;
  unsigned char m_data[SEND_BUFFER_SIZE];
  unsigned int m_recv_byte_size;

  BufferStatus m_out_status;
  bool m_debug;

  // Data
  std::string fFileName;
  std::string fHistName;
  bool fIsADC;
  TH1D *fHist;

  std::mt19937 fGenerator;
  std::bernoulli_distribution fRandomBool;
  std::poisson_distribution<int> fRandomPoisson;
};


extern "C"
{
  void EmulatorInit(RTC::Manager* manager);
};

#endif // EMULATOR_H
