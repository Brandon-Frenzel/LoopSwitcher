/*
  Library for preset structure for relay-based true bypass programmable loop switch for guitar pedal effects.
  Created by Brandon A. Frenzel, April 3, 2019.
  //TODO: copyright information.
*/
#ifndef SwitcherPreset_h
#define SwitcherPreset_h

class Preset {
  private:
    bool loopA = false;
    bool loopB = false;
    bool loopC = false;
    bool loopD = false;

 public:
  Preset(bool a, bool b, bool c, bool d)
  {
    loopA = a;
    loopB = b;
    loopC = c;
    loopD = d;
  }

  setLoopBypass(int 
}

#endif
