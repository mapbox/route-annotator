#pragma once

#include <nan.h>

#include "segment_speed_map.hpp"

class SegmentSpeedLookup : public Nan::ObjectWrap
{
  public:
    static NAN_MODULE_INIT(Init);

  private:
    static NAN_METHOD(New);

    static NAN_METHOD(loadCSV);

    static NAN_METHOD(getRouteSpeeds);

    static Nan::Persistent<v8::Function> &constructor(); // CPP Land

    std::shared_ptr<SegmentSpeedMap> datamap; // if you want async call
};
