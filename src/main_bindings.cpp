#include "nodejs_bindings.hpp"
#include "lookup_bindings.hpp"

NAN_MODULE_INIT(Init) {
  Annotator::Init(target);
  Lookup::Init(target);
}

NODE_MODULE(route_annotator, Init)
