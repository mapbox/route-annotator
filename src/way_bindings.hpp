#pragma once

#include <nan.h>

#include "way_speed_map.hpp"

class WaySpeedLookup : public Nan::ObjectWrap
{
  public:
    static NAN_MODULE_INIT(Init);

  private:
    static NAN_METHOD(New);

    static NAN_METHOD(loadCSV);

    static NAN_METHOD(getRouteSpeeds);

    static Nan::Persistent<v8::Function> &constructor(); // CPP Land

    std::shared_ptr<WaySpeedMap> datamap; // if you want async call
};
