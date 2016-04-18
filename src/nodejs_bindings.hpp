#pragma once

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

    /* Member function for Javascript object: [nodeId, nodeId, ..] -> [wayId, wayId, ..] */
    static NAN_METHOD(annotateRouteFromNodeIds);

    /* Member function for Javascript object: [[lon, lat], [lon, lat]] -> [wayId, wayId, ..] */
    static NAN_METHOD(annotateRouteFromLonLats);

    /* Member function for Javascript object: wayId -> [[key, value], [key, value]] */
    static NAN_METHOD(getAllTagsForWayId);

    /* Thread-safe singleton constructor */
    static Nan::Persistent<v8::Function> &constructor();

    /* Wrapping Annotator */
    explicit Annotator(Database database_) : database{std::move(database_)}, annotator{database} {}

    Database database;
    RouteAnnotator annotator;
};

NODE_MODULE(route_annotator, Annotator::Init)
