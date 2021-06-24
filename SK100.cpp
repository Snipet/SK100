#include "SK100.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "VUMeter.h"
#include "IBubbleControl.h"
#include "ViewButtons.h"
#include "TestViewer.h"

float ratios[7] = {
  1.3,
  1.6,
  2.f,
  2.5,
  3.f,
  4.f,
  5.f
};


SK100::SK100(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{

  GetParam(kComp1InGain)->InitDouble("Comp 1 Input", 0., -10., 10., 0.01, "db");
  GetParam(kComp1OutGain)->InitDouble("Comp 1 Output", 0., -10., 10., 0.01, "db");
  GetParam(kComp1Attack)->InitDouble("Comp 1 Attack", 20., 0., 200., 0.01, "ms", 0, "", IParam::ShapePowCurve(2.5));
  GetParam(kComp1Release)->InitDouble("Comp 1 Release", 20., 1., 200., 0.01, "ms", 0, "", IParam::ShapePowCurve(2.5));
  GetParam(kComp1Threshold)->InitDouble("Comp 1 Threshold", 0., -20, 0, 0.01, "db");
  GetParam(kComp1Ratio)->InitDouble("Comp 1 Ratio", 2., 1., 10., 0.1, "", 0, "", IParam::ShapePowCurve(2.3));

  GetParam(kComp2InGain)->InitDouble("Comp 2 Input", 0., -10., 10., 0.01, "db");
  GetParam(kComp2OutGain)->InitDouble("Comp 2 Output", 0., -10., 10., 0.01, "db");
  GetParam(kComp2Attack)->InitDouble("Comp 2 Attack", 20., 1., 200., 0.01, "ms", 0, "", IParam::ShapePowCurve(2.5));
  GetParam(kComp2Release)->InitDouble("Comp 2 Release", 100., 1., 2000., 0.01, "ms", 0, "", IParam::ShapePowCurve(2.5));
  GetParam(kComp2Threshold)->InitDouble("Comp 2 Threshold", 0., -20, 0, 0.01, "db");
  GetParam(kComp2Ratio)->InitEnum("Comp 2 Ratio", 0, { "1.3", "1.6", "2", "2.5", "3", "4", "5" });
  GetParam(kVUView)->InitEnum("VU Meter View", 1, {"Comp 1", "Both", "Comp 2"});

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {

    pGraphics->AttachBackground(BACKGROUND_FN);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->AttachBubbleControl(new IBubbleControl(IText(21.f, EAlign::Center, IColor(255, 245, 245, 245)), IColor(255, 20, 20, 20), IColor(255, 15, 15, 15)));
    pGraphics->EnableMultiTouch(true);

    IBitmap knob1 = pGraphics->LoadBitmap(KNOB1_FN, 45);
    IBitmap knob2 = pGraphics->LoadBitmap(KNOB2_FN, 45);
    IBitmap knob3 = pGraphics->LoadBitmap(KNOB3_FN, 45);
    IBitmap knob4 = pGraphics->LoadBitmap(KNOB4_FN, 45);
    IBitmap knob5 = pGraphics->LoadBitmap(KNOB5_FN, 45);
    IBitmap knob6 = pGraphics->LoadBitmap(KNOB6_FN, 45);
    IBitmap knob7 = pGraphics->LoadBitmap(KNOB7_FN, 45);
    IBitmap knob8 = pGraphics->LoadBitmap(KNOB8_FN, 45);
    IBitmap knob9 = pGraphics->LoadBitmap(KNOB9_FN, 45);
    IBitmap knob10 = pGraphics->LoadBitmap(KNOB10_FN, 45);
    IBitmap knob11 = pGraphics->LoadBitmap(KNOB11_FN, 45);
    IBitmap knob12 = pGraphics->LoadBitmap(KNOB12_FN, 45);
    IBitmap vumeter = pGraphics->LoadBitmap(VUMETER_FN, 45);
    IBitmap vuButtons = pGraphics->LoadBitmap(VIEWBUTTONS_FN, 4);

    pGraphics->AttachControl(new IBKnobControl(IRECT(88, 66, 88, 66), knob1, kComp1InGain))->SetActionFunction(ShowBubbleHorizontalActionFunc);
    pGraphics->AttachControl(new IBKnobControl(IRECT(208, 66, 208, 66), knob2, kComp1Threshold))->SetActionFunction(ShowBubbleHorizontalActionFunc);
    pGraphics->AttachControl(new IBKnobControl(IRECT(328, 66, 328, 66), knob3, kComp1Ratio))->SetActionFunction(ShowBubbleHorizontalActionFunc);

    pGraphics->AttachControl(new IBKnobControl(IRECT(88, 159, 88, 159), knob4, kComp1OutGain))->SetActionFunction(ShowBubbleHorizontalActionFunc);
    pGraphics->AttachControl(new IBKnobControl(IRECT(208, 159, 208, 159), knob5, kComp1Attack))->SetActionFunction(ShowBubbleHorizontalActionFunc);
    pGraphics->AttachControl(new IBKnobControl(IRECT(328, 159, 328, 159), knob6, kComp1Release))->SetActionFunction(ShowBubbleHorizontalActionFunc);

    pGraphics->AttachControl(new IBKnobControl(IRECT(673, 66, 673, 66), knob7, kComp2InGain))->SetActionFunction(ShowBubbleHorizontalActionFunc);
    pGraphics->AttachControl(new IBKnobControl(IRECT(791, 66, 791, 66), knob8, kComp2Threshold))->SetActionFunction(ShowBubbleHorizontalActionFunc);
    pGraphics->AttachControl(new IBKnobControl(IRECT(910, 66, 910, 66), knob9, kComp2Ratio))->SetActionFunction(ShowBubbleHorizontalActionFunc);

    pGraphics->AttachControl(new IBKnobControl(IRECT(673, 159, 673, 159), knob10, kComp2OutGain))->SetActionFunction(ShowBubbleHorizontalActionFunc);
    pGraphics->AttachControl(new IBKnobControl(IRECT(791, 159, 791, 159), knob11, kComp2Attack))->SetActionFunction(ShowBubbleHorizontalActionFunc);
    pGraphics->AttachControl(new IBKnobControl(IRECT(910, 159, 910, 159), knob12, kComp2Release))->SetActionFunction(ShowBubbleHorizontalActionFunc);

    pGraphics->AttachControl(new VUMeter(IRECT(377, 39, 621, 183), vumeter), kVUMeter);
    pGraphics->AttachControl(new ViewButtonsControl(IRECT(392, 191, 608, 215), vuButtons, kVUView));
    //pGraphics->AttachControl(new TestViewer(IRECT(0, 0, PLUG_HEIGHT, PLUG_HEIGHT)));
  };
#endif

  processor = new Processor(GetSampleRate());
}

#if IPLUG_DSP
void SK100::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  processor->ProcessBlock(inputs, outputs, nFrames);
  float difference = processor->getGainDifference();
  if (inputs[0][0] == 0) {
    difference = 0;
  }
  mSender.PushData({ kVUMeter, {difference} });
}
#endif

void SK100::OnIdle() {
  mSender.TransmitData(*this);
}

void SK100::OnParamChange(int idx) {
  auto value = GetParam(idx)->Value();
  switch (idx) {
  case kComp1InGain:
    processor->setInGain1(DBToAmp(value));
    break;

  case kComp1OutGain:
    processor->setOutGain1(DBToAmp(value));
    break;

  case kComp1Threshold:
    processor->setThreshold1(DBToAmp(value));
    break;

  case kComp1Release:
    processor->setRelease1(value * (GetSampleRate() / 1000));
    break;

  case kComp1Attack:
    processor->setAttack1(value * (GetSampleRate() / 1000));
    break;

  case kComp1Ratio:
    processor->setRatio1(value);
    break;

  case kComp2InGain:
    processor->setInGain2(DBToAmp(value));
    break;

  case kComp2OutGain:
    processor->setOutGain2(DBToAmp(value));
    break;

  case kComp2Threshold:
    processor->setThreshold2(DBToAmp(value));
    break;

  case kComp2Release:
    processor->setRelease2(value * (GetSampleRate() / 1000));
    break;

  case kComp2Attack:
    processor->setAttack2(value * (GetSampleRate() / 1000));
    break;

  case kComp2Ratio:
    processor->setRatio2(ratios[(int)value]);
    break;

  case kVUView:
    processor->setView(value);
  }
}
