#pragma once

#include <nan.h>

#include "hashmap.hpp"


class Lookup : public Nan::ObjectWrap {
public:
  static NAN_MODULE_INIT(Init);

private:
  static NAN_METHOD(New);

  static NAN_METHOD(GetAnnotations);

  static Nan::Persistent<v8::Function>& constructor(); //CPP Land

  // Wrapped Object

  Lookup(Hashmap annotations);

  std::shared_ptr<const Hashmap> annotations; // if you want async call
};


