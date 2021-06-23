#pragma once

#include "IControl.h"
#include "SK100Proc.h"

using namespace iplug;
using namespace igraphics;

class TestViewer : public IControl {
public:
  TestViewer(const IRECT& b) : IControl(b) {
    comp.init(44100);
  }

  void Draw(IGraphics& g) override {
    comp.setThreshold(DBToAmp(GetDelegate()->GetParam(kComp2Threshold)->Value()));
    comp.setRatio(GetDelegate()->GetParam(kComp1Ratio)->Value());
    comp.MakeGainBuffer();
    g.FillRect(IColor(255, 30, 30, 30), mRECT);
    g.DrawData(IColor(255, 255, 255, 255), mRECT, comp.getBuffer(), comp.getLength(), nullptr, 0, 5);
    SetDirty(false);
  }

private:
  Comp2Processor comp;
};