#pragma once

#include <nan.h>

#include "hashmap.hpp"

class Lookup : public Nan::ObjectWrap
{
  public:
    static NAN_MODULE_INIT(Init);

  private:
    static NAN_METHOD(New);

    static NAN_METHOD(loadCSV);

    static NAN_METHOD(GetAnnotations);

    static Nan::Persistent<v8::Function> &constructor(); // CPP Land

    std::unique_ptr<Hashmap> datamap; // if you want async call
};
