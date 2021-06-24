#pragma once

#include "IControl.h"

using namespace iplug;
using namespace igraphics;

class VUFilter {
public:
  VUFilter() {
    buf0 = buf1 = 0;
  }

  float tick(float input) {
    float f = 0.5;
    float q = 0.2;
    float fb = q + q / (1 - f);
    buf0 = buf0 + f * (input - buf0 + fb * (buf0 - buf1));
    buf1 = buf1 + f * (buf0 - buf1);
    return buf1;
  }

private:
  float buf0, buf1;
};

class VUMeter : public IControl {
public:
  VUMeter(const IRECT& b, IBitmap bm) : IControl(b) {
    bitmap = bm;
    value = 0;
  }

  void Draw(IGraphics& g) override{
    float db = filter.tick(value);
    db = Clip(db, 0.f, 12.f);
    int n = ((db) / 12.f) * 45;
    //value = fmod(value + 0.07, PI * 2);
    //int n = ((sin(value) + 1) / 2) * 45;
    g.DrawBitmap(bitmap, mRECT, n);
    SetDirty(false);
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    {
      IByteStream stream(pData, dataSize);

      int pos = 0;
      ISenderData<1> d;
      pos = stream.Get(&d, pos);
      value = d.vals[0];
    }
  }

private:
  IBitmap bitmap;
  int n;
  float value;
  VUFilter filter;
};
