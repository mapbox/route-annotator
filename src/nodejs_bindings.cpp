#include <cstdint>

#include <stdexcept>
#include <string>
#include <utility>

#include <nan.h>

#include "annotator.hpp"
#include "database.hpp"
#include "extractor.hpp"
#include "types.hpp"

class Annotator final : public Nan::ObjectWrap
{
  public:
    /* Set up wrapper Javascript object */
    static NAN_MODULE_INIT(Init)
    {
        const auto whoami = Nan::New("Annotator").ToLocalChecked();

        auto fnTp = Nan::New<v8::FunctionTemplate>(New);
        fnTp->SetClassName(whoami);
        fnTp->InstanceTemplate()->SetInternalFieldCount(1);

        SetPrototypeMethod(fnTp, "annotateRouteFromNodeIds", annotateRouteFromNodeIds);

        auto fn = Nan::GetFunction(fnTp).ToLocalChecked();

        constructor().Reset(fn);

        Nan::Set(target, whoami, fn);
    }

  private:
    /* Constructor call for Javascript object, as in "new Annotator(..)" or "Annotator(..)" */
    static NAN_METHOD(New)
    {
        if (info.Length() != 1 || !info[0]->IsString())
            return Nan::ThrowTypeError("String to extract exptected");

        if (info.IsConstructCall())
        {
            const Nan::Utf8String utf8String(info[0]);

            if (!(*utf8String))
                return Nan::ThrowError("Unable to convert to Utf8String");

            const std::string extract{*utf8String, *utf8String + utf8String.length()};

            try
            {
                Database database;
                Extractor extractor{extract, database};

                auto *self = new Annotator(std::move(database));

                self->Wrap(info.This());
            }
            catch (const std::exception &e)
            {
                return Nan::ThrowError(e.what());
            }

            info.GetReturnValue().Set(info.This());
        }
        else
        {
            const constexpr auto argc = 1u;

            v8::Local<v8::Value> argv[argc] = {
                info[0],
            };

            auto init = Nan::New(constructor());

            info.GetReturnValue().Set(init->NewInstance(argc, argv));
        }
    }

    /* Member function for Javascript object: [nodeId, nodeId] -> [dummy, dummy] */
    static NAN_METHOD(annotateRouteFromNodeIds)
    {
        // TODO(daniel-j-h): what about TypedArrays? IsArray() not superset of IsTypedArray()
        // https://v8docs.nodesource.com/node-4.2/dc/d0a/classv8_1_1_value.html
        if (info.Length() != 1 || !info[0]->IsArray())
            return Nan::ThrowTypeError("Array of node ids expected");

        auto *self = Nan::ObjectWrap::Unwrap<Annotator>(info.Holder());

        const auto nodeIds = info[0].As<v8::Array>();

        for (std::uint32_t i{0}; i < nodeIds->Length(); ++i)
        {
            auto item = Nan::Get(nodeIds, i).ToLocalChecked();

            if (!item->IsNumber())
                return Nan::ThrowTypeError("Array of number type expected");
        }

        const constexpr auto length = 1u;
        auto annotated = Nan::New<v8::Array>(length);
        (void)Nan::Set(annotated, 0, Nan::New<v8::Number>(10));
        (void)Nan::Set(annotated, 1, Nan::New<v8::Number>(20));

        info.GetReturnValue().Set(annotated);
    }

    /* Thread-safe singleton constructor */
    static inline Nan::Persistent<v8::Function> &constructor()
    {
        static Nan::Persistent<v8::Function> init;
        return init;
    }

    /* Wrapping Annotator */
    explicit Annotator(Database database_) : database{std::move(database_)}, annotator{database} {}

    Database database;
    RouteAnnotator annotator;
};

NODE_MODULE(route_annotator, Annotator::Init)
