#include <Arduino.h>
#include <rgb_pwm.hpp>
#include <gfx_cpp14.hpp>
using namespace arduino;
using namespace gfx;
rgb_pwm<rgb_pwm_group<pwm_traits<32,0>,pwm_traits<25,1>,pwm_traits<26,2>>> p;
uint8_t r = 0;
int rd = 1;
uint8_t g = 127;
int gd = 1;
uint8_t b = 255;
int bd = -1;

void setup() {
  
  Serial.begin(115200);
}

void loop() {
  decltype(p)::pixel_type px;
  px.channel<0>(r);
  if(rd==1) {
    if(r==255) {
      rd = -1;
    }
  } else {
    if(r==0) {
      rd = 1;
    }
  }
  r+=rd;

  px.channel<1>(g);
  if(gd==1) {
    if(g==255) {
      gd = -1;
    }
  } else {
    if(g==0) {
      gd = 1;
    }
  }
  g+=gd;

  px.channel<2>(b);
  if(bd==1) {
    if(b==255) {
      bd = -1;
    }
  } else {
    if(b==0) {
      bd = 1;
    }
  }
  b+=bd;
  p.point({0,0},px);
  delay(10);

}