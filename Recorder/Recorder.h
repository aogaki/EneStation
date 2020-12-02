// -*- C++ -*-
/*!
 * @file 
 * @brief
 * @date
 * @author
 *
 */

#ifndef RECORDER_H
#define RECORDER_H

#include <memory>

#include <TTree.h>

#include "DaqComponentBase.h"

#include "../../TDigiTES/include/TPSDData.hpp"

using namespace RTC;

class Recorder
  : public DAQMW::DaqComponentBase
{
public:
  Recorder(RTC::Manager* manager);
  ~Recorder();

  // The initialize action (on CREATED->ALIVE transition)
  // former rtc_init_entry()
  virtual RTC::ReturnCode_t onInitialize();

  // The execution action that is invoked periodically
  // former rtc_active_do()
  virtual RTC::ReturnCode_t onExecute(RTC::UniqueId ec_id);

private:
  TimedOctetSeq          m_in_data;
  InPort<TimedOctetSeq>  m_InPort;

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
  int reset_InPort();

  unsigned int read_InPort();
  //int online_analyze();

  BufferStatus m_in_status;
  bool m_debug;

  // For ROOT file recording
  void ResetTree();
  int FillData(unsigned int dataSize);
  void WriteData();

  unsigned long fLastSave;
  unsigned long fSaveInterval;
  unsigned int fSubRunNumber;
  
  std::unique_ptr<TTree> fTree;
  unsigned char fCh; // the data type should be same as Recorder
  uint64_t fTimeStamp;
  uint16_t fEnergy;
};


extern "C"
{
  void RecorderInit(RTC::Manager* manager);
};

#endif // RECORDER_H
