#include "nodejs_bindings.hpp"
#include "speed_lookup_bindings.hpp"

NAN_MODULE_INIT(Init)
{
    Annotator::Init(target);
    SpeedLookup::Init(target);
}

NODE_MODULE(route_annotator, Init)
