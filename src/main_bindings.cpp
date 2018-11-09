#include "nodejs_bindings.hpp"
#include "segment_bindings.hpp"
#include "way_bindings.hpp"

NAN_MODULE_INIT(Init)
{
    Annotator::Init(target);
    SegmentSpeedLookup::Init(target);
    WaySpeedLookup::Init(target);
}

NODE_MODULE(route_annotator, Init)
