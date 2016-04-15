#include <nan.h>

NAN_METHOD(doesItWork) { info.GetReturnValue().Set(Nan::New("It works!").ToLocalChecked()); }

NAN_MODULE_INIT(init) { NAN_EXPORT(target, doesItWork); }

NODE_MODULE(route_annotator, init)
