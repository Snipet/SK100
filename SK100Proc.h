#pragma once

#include "IControl.h"

using namespace iplug;

class SPeakFinder {
public:
  SPeakFinder() {
    moving = 0;
    release = 5000;
  }

  float tick(float input) {
    if (abs(input) > moving) {
      moving = abs(input);
    }
    else {
      moving = (moving * release) / (release + 1);
    }
    return moving;
  }

  inline void setRelease(float r) {
    release = r;
  }

  inline float get() {
    return moving;
  }

private:
  float moving;
  float release;
};


class SEnvelope {
public:
  SEnvelope() {
    point = 0;
    attack = 0;
    decay = 0;
    sustain = 0;
  }

  inline void setAttack(float a) {
    attack = a;
  }

  inline void setDecay(float d) {
    decay = d;
  }

  inline void setSustain(float s) {
    sustain = s;
  }

  void tickForward() {
    if (point < attack + decay) {
      point++;
    }
  }

  float get() {
    if (point < attack) {
      return point / attack;
    }
    else {
      return interpolate(1, sustain, (point - attack) / decay);
    }
  }

  void retrigger() {
    point = 0;
  }

private:
  float point;
  float attack;
  float decay;
  float sustain;

  float interpolate(float y1, float y2, float mu) {
    return y2 * mu + (1 - mu) * y1;
  }

};


class LowpassFilter {
public:
  LowpassFilter() {
    buf0 = buf1 = 0;
  }

  void setCutoff(float c) {
    cutoff = c;
  }

  float tick(float input) {
    float inv = 1 - cutoff;
    buf0 = input * cutoff + buf0 * inv;
    buf1 = buf0 * cutoff + buf1 * inv;
    return buf1;
  }

  float get() {
    return buf1;
  }

private:
  float buf0;
  float buf1;
  float cutoff;
};

class TransientDetector {
public:
  TransientDetector() {
    finder.setRelease(200);
    filter.setCutoff(0.2);
  }

  bool tick(float input) {
    float current = filter.tick(finder.tick(input)); //Peaking signal lowpassed to allow for attack
    if (abs(input) > current * 3) {
      return true;
    }
    else {
      return false;
    }
  }

private:
  SPeakFinder finder;
  LowpassFilter filter;
};

class FIRFilter {
public:
  FIRFilter() {
    length = 350;
    taps = new float[length];
    history = new float[length];
    last_index = 0;
    normalize = 1;
    createTaps();
    CreateNormalization();
    resetHistory();
  }

  void createTaps() {
    float inc = 4.f / (float)length;
    float x = 0;
    for (int i = 0; i < length; i++) {
      taps[i] = BlackmanWindow(x, 5);
      x += inc;
    }
  }

  float tick(float input) {
    history[last_index++] = input;
    if (last_index == length) {
      last_index = 0;
    }
    float acc = 0;
    int index = last_index, i;
    for (i = 0; i < length; ++i) {
      index = index != 0 ? index - 1 : length - 1;
      acc += history[index] * taps[i];
    };
    return acc;
  }

  float BlackmanWindow(float x, float n) {
    return 0.42 - 0.5 * (cos((2 * PI * x) / (n - 1))) + 0.08 * cos((4 * PI * x) / (n - 1));
  }

  void resetHistory() {
    for (int i = 0; i < length; i++) {
      history[i] = 0;
    }
  }

  float* getTaps() {
    return taps;
  }

  int getLength() {
    return length;
  }

  void CreateNormalization(int size = 200) {
    resetHistory();
    float x = 0;
    float inc = 1.f / (float)size;
    float value = 0;
    float max = 0;
    for (int i = 0; i < size; i++) {
      value = tick(x);
      if (value > max) {
        max = value;
      }
      x += inc;
    }
    normalize = 1.f / max;
  }

private:
  float* taps;
  float* history;
  int length;
  int last_index;
  float normalize;
};


class Comp1Processor {
public:
  Comp1Processor() {
    gain = 1;
    inCompression = 0;
    lastInCompression = 0;
    v = 0;
    releaseCounter = 0;
    attackCounter = 0;
  }

  void init(float fs) {
    sampleRate = fs;
    sustain = fs / 441;
    finder.setRelease(fs / 44);
  }

  inline void setInGain(float a) {
    inGain = a;
  }

  inline void setOutGain(float a) {
    outGain = a;
  }

  inline void setThreshold(float a) {
    threshold = a;
  }

  inline void setRatio(float a) {
    ratio = a;
  }

  inline void setAttack(float a) {
    attack = a;
  }

  inline void setRelease(float a) {
    release = a;
  }

  float tick(float input) {
    float m = finder.tick(input);
    lastInCompression = inCompression;
    if (finder.get() > threshold) {
      inCompression = sustain;
    }
    else {
      if (inCompression > 0) {
        inCompression--;
      }
    }

    if (inCompression == 0 && lastInCompression == 1) {
      //The release just kicked in
      releaseCounter = release * (1 - v);
    }
    if (inCompression != 0 && lastInCompression == 0) {
      //The compressor just kicked in
      attackCounter = attack * (1 - v);
    }
    float level = 0;
    if (m != 0) {
      level = getGain(m, threshold, 0.1, ratio) / m;
    }
    if (inCompression > 0) {
      if (attack != 0) {
        if (attackCounter < attack) {
          attackCounter++;
          v = attackCounter / attack;
        }
      }
      else {
        v = 1;
      }
      gain = interpolate(1, level, v, 0.9);
    }
    else {
      if (releaseCounter < release) {
        releaseCounter++;
        v = releaseCounter / release;
      }
      gain = interpolate(level, 1, v, 1);
    }

    return input * gain;

  }

  float getGain(float x, float t, float w, float r) {
    if (x - t < -(w / 2)) {
      return x;
    }
    else if (abs(x - t) < (w / 2)) {
      float a = (1 / r) - 1;
      float b = x - t + (w / 2);
      b *= b;
      return x + (a * b) / (2 * w);
    }
    else {
      return t + ((x - t) / r);
    }
  }


private:
  float sampleRate;
  float inGain;
  float outGain;
  float threshold;
  float ratio;
  float attack;
  float release;
  SPeakFinder finder;
  float gain;
  float inCompression;
  float lastInCompression;
  float v;
  float sustain;
  float attackCounter;
  float releaseCounter;
  float interpolate(float y1, float y2, float mu, float r) {
    return pow(y2 * mu + (1 - mu) * y1, r);
  }
};

class Comp2Processor {
public:
  Comp2Processor() {
    length = 2000;
    gainBuffer = new float[length];
    inCompression = 0;
    lastInCompression = 0;
    ResetGainBuffer();
    lowpass.setCutoff(0.1);
    attackCounter = 0;
    releaseCounter = 0;
    gain = 0;
    v = 0;
  }

  void init(float fs) {
    sampleRate = fs;
    finder.setRelease(fs / 44.f);
    sustain = fs / 30;
    MakeGainBuffer();
    needsUpdate = false;
    updateTickSize = fs / 20;
    updateTick = 0;
  }

  void MakeGainBufferTest() {
    //Create the buffer
    float x = 0;
    float max = 0;
    float inc = 1 / (float)length;
    for (int i = 0; i < length; i++) {
      gainBuffer[i] = filter.tick(x);
      if (gainBuffer[i] > max) {
        max = gainBuffer[i];
      }
      x += inc;
    }

    //Normalize
    float coeff = 1 / max;
    for (int i = 0; i < length; i++) {
      gainBuffer[i] *= coeff;
    }
  }

  //Creates the gainBuffer for the compressor from given parameter values
  void MakeGainBuffer() {
    ResetGainBuffer();
    float top = threshold;
    WriteLine(0, 0, top, top);
    float ratioX = Clip(top+0.18f + (ratio/sqrt(ratio))*0.08f, 0.f, 1.f);
    float reverseRatio = 1 / ratio;
    float ratioY = top + (ratioX - top) * reverseRatio;
    WriteLine(top, top, ratioX, ratioY);
    float distance = 1 - ratioX;
    WriteLine(ratioX, ratioY, 1.f, ratioY+distance*(1/(ratio*0.1+1)));
    SmoothGainBuffer();
  }

  //Creates a line in the gain buffer. All values are normalized 0 -> 1. R refers to the positive or negative curvature of the line
  void WriteLine(float x1, float y1, float x2, float y2, float r = 1) {
    int beginning = x1 * length;
    int end = x2 * length; //Assumes x2 > x1
    int size = end - beginning;
    for (int i = 0; i < size; i++) {
      float v = (float)i / (float)size;
      gainBuffer[beginning + i] = interpolate(y1, y2, v, r);
    }
  }


  //Uses a FIR filter to smooth out the rough edges
  void SmoothGainBuffer() {
    filter.resetHistory();
    float* dry = new float[length];
    float max = 0;
    float dryMax = 0;
    for (int i = 0; i < length; i++) {
      dry[i] = gainBuffer[i];
      if (dry[i] > dryMax) {
        dryMax = dry[i];
      }
      gainBuffer[i] = filter.tick(gainBuffer[i]);
      if (gainBuffer[i] > max) {
        max = gainBuffer[i];
      }
    }

    float coeff = dryMax / max;
    float v = 0;
    for (int i = 0; i < length; i++) {
      gainBuffer[i] *= coeff;
      gainBuffer[i] = interpolate(dry[i], gainBuffer[i], v, 1);
      if (v < 1) {
        v += 0.001;
      }
    }

  }

  void ResetGainBuffer() {
    for (int i = 0; i < length; i++) {
      gainBuffer[i] = 0;
    }
  }

  int getLength() {
    return length;
  }

  float* getBuffer() {
    return gainBuffer;
  }


  inline void setThreshold(float a) {
    threshold = a;
    needsUpdate = true;
  }

  inline void setRatio(float a) {
    ratio = a;
    needsUpdate = true;
  }

  inline void setAttack(float a) {
    attack = a;
  }

  inline void setRelease(float a) {
    release = a;
  }

  float tick(float input) {

    if (needsUpdate && updateTick == 0) {
      needsUpdate = false;
      updateTick = updateTickSize;
      MakeGainBuffer();
    }

    if (updateTick > 0) {
      updateTick--;
    }

    float process = tanh(input);
    float m = lowpass.tick(finder.tick(process));
    lastInCompression = inCompression;
    if (m > threshold) {
      inCompression = sustain;
    }
    else {
      if (inCompression > 0) {
        inCompression--;
      }
    }

    if (inCompression == 0 && lastInCompression == 1) {
      //The release just kicked in
      releaseCounter = release * (1 - v);
    }
    if (inCompression != 0 && lastInCompression == 0) {
      //The compressor just kicked in
      attackCounter = attack * (1 - v);
    }

    float level = 0;
    if (m != 0) {
      level = getGain(m) / m;
    }
    if (inCompression > 0) {
      if (attack != 0) {
        if (attackCounter < attack) {
          attackCounter++;
          v = attackCounter / attack;
        }
      }
      else {
        v = 1;
      }
      gain = interpolate(1, level, v, 0.9);
    }
    else {
      if (releaseCounter < release) {
        releaseCounter++;
        v = releaseCounter / release;
      }
      gain = interpolate(level, 1, v, 1);
    }
    return gain * process;
  }

  float getGain(float x) {
    float phase = x * (float)length;
    int bottom = floor(phase);
    int top = bottom + 1;
    return interpolate(gainBuffer[bottom], gainBuffer[top], phase - bottom, 1);
  }

private:
  float inCompression;
  float lastInCompression;
  float sampleRate;
  float threshold;
  float ratio;
  float attack;
  float release;
  float* gainBuffer;
  int length;
  int sustain;
  float v;
  bool needsUpdate;
  int updateTick;
  int updateTickSize;
  float attackCounter;
  float releaseCounter;
  float gain;
  FIRFilter filter;
  SPeakFinder finder;
  LowpassFilter lowpass;

  float interpolate(float y1, float y2, float mu, float r) {
    return pow(y2 * mu + (1 - mu) * y1, r);
  }
};


class MonoProcessor {
public:
  MonoProcessor() {
    comp1 = new Comp1Processor();
    comp2 = new Comp2Processor();
  }

  void init(float fs) {
    comp1->init(fs);
    comp2->init(fs);
  }

  float tick(float input) {
    float process = input;
    if (view != 2) {
      before.tick(process);
    }
    process = comp1->tick(process * inGain1);

    if (view == 2) {
      before.tick(process);
    }

    if (view == 0) {
      after.tick(process);
    }

    process *= outGain1;
    process = comp2->tick(process * inGain2);

    if (view != 0) {
      after.tick(process);
    }
    process *= outGain2;
    return process;
  }

  inline void setInGain1(float a) {
    comp1->setInGain(a);
    inGain1 = a;
  }

  inline void setOutGain1(float a) {
    comp1->setOutGain(a);
    outGain1 = a;
  }

  inline void setThreshold1(float a) {
    comp1->setThreshold(a);
  }

  inline void setRatio1(float a) {
    comp1->setRatio(a);
  }

  inline void setAttack1(float a) {
    comp1->setAttack(a);
  }

  inline void setRelease1(float a) {
    comp1->setRelease(a);
  }

  inline void setThreshold2(float a) {
    comp2->setThreshold(a);
  }

  inline void setRatio2(float a) {
    comp2->setRatio(a);
  }

  inline void setAttack2(float a) {
    comp2->setAttack(a);
  }

  inline void setRelease2(float a) {
    comp2->setRelease(a);
  }

  inline void setInGain2(float a) {
    inGain2 = a;
  }

  inline void setOutGain2(float a) {
    outGain2 = a;
  }

  float getGainDifference() {
    return AmpToDB(before.get()) - AmpToDB(after.get());
  }

  inline void setView(int a) {
    view = a;
  }

private:
  Comp1Processor* comp1;
  Comp2Processor* comp2;
  SPeakFinder before;
  SPeakFinder after;
  FIRFilter filter;
  float inGain1;
  float outGain1;
  float inGain2;
  float outGain2;
  int view;
};

class Processor {
public:
  Processor(float fs) {
    channels = 2;
    processors = new MonoProcessor[channels];
    for (int i = 0; i < channels; i++) {
      processors[i].init(fs);
    }
  }

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) {
    for (int c = 0; c < channels; c++) {
      for (int s = 0; s < nFrames; s++) {
        outputs[c][s] = processors[c].tick(inputs[c][s]);
      }
    }
  }

  inline void setInGain1(float a) {
    for (int i = 0; i < channels; i++) {
      processors[i].setInGain1(a);
    }
  }

  inline void setOutGain1(float a) {
    for (int i = 0; i < channels; i++) {
      processors[i].setOutGain1(a);
    }
  }

  inline void setThreshold1(float a) {
    for (int i = 0; i < channels; i++) {
      processors[i].setThreshold1(a);
    }
  }

  inline void setRatio1(float a) {
    for (int i = 0; i < channels; i++) {
      processors[i].setRatio1(a);
    }
  }

  inline void setAttack1(float a) {
    for (int i = 0; i < channels; i++) {
      processors[i].setAttack1(a);
    }
  }

  inline void setRelease1(float a) {
    for (int i = 0; i < channels; i++) {
      processors[i].setRelease1(a);
    }
  }

  inline void setThreshold2(float a) {
    for (int i = 0; i < channels; i++) {
      processors[i].setThreshold2(a);
    }
  }

  inline void setRatio2(float a) {
    for (int i = 0; i < channels; i++) {
      processors[i].setRatio2(a);
    }
  }

  inline void setAttack2(float a) {
    for (int i = 0; i < channels; i++) {
      processors[i].setAttack2(a);
    }
  }

  inline void setRelease2(float a) {
    for (int i = 0; i < channels; i++) {
      processors[i].setRelease2(a);
    }
  }

  inline void setView(int a) {
    for (int i = 0; i < channels; i++) {
      processors[i].setView(a);
    }
  }


  inline void setInGain2(float a) {
    for (int i = 0; i < channels; i++) {
      processors[i].setInGain2(a);
    }
  }

  inline void setOutGain2(float a) {
    for (int i = 0; i < channels; i++) {
      processors[i].setOutGain2(a);
    }
  }

  float getGainDifference() {
    float s = 0;
    for (int i = 0; i < channels; i++) {
      s += processors[i].getGainDifference();
    }
    return s / (float)channels;
  }

private:
  float sampleRate;
  int channels;
  MonoProcessor* processors;
};