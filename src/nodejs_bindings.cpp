#include <cstdint>

#include <stdexcept>
#include <string>
#include <utility>

#include "extractor.hpp"
#include "types.hpp"

#include "nodejs_bindings.hpp"

NAN_MODULE_INIT(Annotator::Init)
{
    const auto whoami = Nan::New("Annotator").ToLocalChecked();

    auto fnTp = Nan::New<v8::FunctionTemplate>(New);
    fnTp->SetClassName(whoami);
    fnTp->InstanceTemplate()->SetInternalFieldCount(1);

    SetPrototypeMethod(fnTp, "annotateRouteFromNodeIds", annotateRouteFromNodeIds);

    const auto fn = Nan::GetFunction(fnTp).ToLocalChecked();

    constructor().Reset(fn);

    Nan::Set(target, whoami, fn);
}

NAN_METHOD(Annotator::New)
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

            auto *const self = new Annotator(std::move(database));

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

NAN_METHOD(Annotator::annotateRouteFromNodeIds)
{
    auto *const self = Nan::ObjectWrap::Unwrap<Annotator>(info.Holder());

    // TODO(daniel-j-h): what about TypedArrays? IsArray() not superset of IsTypedArray()
    // https://v8docs.nodesource.com/node-4.2/dc/d0a/classv8_1_1_value.html
    if (info.Length() != 1 || !info[0]->IsArray())
        return Nan::ThrowTypeError("Array of node ids expected");

    const auto jsNodeIds = info[0].As<v8::Array>();

    std::vector<external_nodeid_t> externalIds(jsNodeIds->Length());

    for (std::size_t i{0}; i < jsNodeIds->Length(); ++i)
    {
        const auto nodeId = Nan::Get(jsNodeIds, i).ToLocalChecked();

        if (!nodeId->IsNumber())
            return Nan::ThrowTypeError("Array of number type expected");

        externalIds[i] = Nan::To<std::int64_t>(nodeId).FromJust();
        // TODO(daniel-j-h): Why is there no conversion to uint64_t?
        // https://github.com/nodejs/nan/blob/master/doc/converters.md#nanto
        // externalIds[i] = Nan::To<external_nodeid_t>(nodeId).FromJust();
    }

    // Note: memory for externalIds could be reclaimed after the translation to internalIds
    const auto internalIds = self->annotator.external_to_internal(externalIds);
    const auto wayIds = self->annotator.annotateRoute(internalIds);

    const auto annotated = Nan::New<v8::Array>(wayIds.size());

    for (std::size_t i{0}; i < wayIds.size(); ++i)
    {
        const auto wayId = wayIds[i];

        if (wayId == INVALID_WAYID)
            (void)Nan::Set(annotated, i, Nan::Null());
        else
            (void)Nan::Set(annotated, i, Nan::New<v8::Number>(wayIds[i]));
    }

    info.GetReturnValue().Set(annotated);
}

Nan::Persistent<v8::Function> &Annotator::constructor()
{
    static Nan::Persistent<v8::Function> init;
    return init;
}
