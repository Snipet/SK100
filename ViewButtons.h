#pragma once

#include "IControl.h"

using namespace iplug;
using namespace igraphics;

class ViewButtonsControl : public IControl {
public:
  ViewButtonsControl(const IRECT& b, IBitmap bm, int param) : IControl(b, param) {
    bitmap = bm;
  }

  void Draw(IGraphics& g) override {
    int frame = (GetValue())*3+2;
    g.DrawBitmap(bitmap, mRECT, frame);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override {
    if (x < mRECT.W() * 0.3333 + mRECT.L) {
      SetValue(0.f);
    }
    else if (x > mRECT.W() * 0.6666 + mRECT.L) {
      SetValue(1.f);
    }
    else {
      SetValue(0.5);
    }
    SetDirty(true);
  }

private:
  IBitmap bitmap;
};