#pragma once

#include <memory>
#include <utility>

#include <nan.h>

#include "annotator.hpp"
#include "database.hpp"

class Annotator final : public Nan::ObjectWrap
{
  public:
    /* Set up wrapper Javascript object */
    static NAN_MODULE_INIT(Init);

  private:
    /* Constructor call for Javascript object, as in "new Annotator(..)" or "Annotator(..)" */
    static NAN_METHOD(New);

    /* Member function for Javascript object to parse and load the OSM extract */
    static NAN_METHOD(loadOSMExtract);

    /* Member function for Javascript object: [nodeId, nodeId, ..] -> [wayId, wayId, ..] */
    static NAN_METHOD(annotateRouteFromNodeIds);

    /* Member function for Javascript object: [[lon, lat], [lon, lat]] -> [wayId, wayId, ..] */
    static NAN_METHOD(annotateRouteFromLonLats);

    /* Member function for Javascript object: wayId -> [[key, value], [key, value]] */
    static NAN_METHOD(getAllTagsForWayId);

    /* Thread-safe singleton constructor */
    static Nan::Persistent<v8::Function> &constructor();

    /* Wrapping Annotator; both database and annotator do not provide default ctor: wrap in ptr */
    bool createRTree;
    std::unique_ptr<Database> database;
    std::unique_ptr<RouteAnnotator> annotator;
};
