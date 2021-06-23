#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "SK100Proc.h"
#include "ISender.h"

const int kNumPresets = 1;

enum EParams
{
  kComp1InGain = 0,
  kComp1OutGain,
  kComp1Threshold,
  kComp1Ratio,
  kComp1Attack,
  kComp1Release,
  kComp2InGain,
  kComp2OutGain,
  kComp2Threshold,
  kComp2Ratio,
  kComp2Attack,
  kComp2Release,
  kVUView,
  kNumParams
};

enum EControls {
  kVUMeter = 0
};

using namespace iplug;
using namespace igraphics;

class SK100 final : public Plugin
{
public:
  SK100(const InstanceInfo& info);
  void OnParamChange(int idx) override;

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnIdle() override;
#endif

private:
  Processor* processor;
  ISender<1> mSender;
};
