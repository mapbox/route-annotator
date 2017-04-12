#include <cstdint>

#include <memory>
#include <stdexcept>
#include <string>

#include "extractor.hpp"
#include "types.hpp"

#include "nodejs_bindings.hpp"

#include <boost/numeric/conversion/cast.hpp>

NAN_MODULE_INIT(Annotator::Init)
{
    const auto whoami = Nan::New("Annotator").ToLocalChecked();

    auto fnTp = Nan::New<v8::FunctionTemplate>(New);
    fnTp->SetClassName(whoami);
    fnTp->InstanceTemplate()->SetInternalFieldCount(1);

    SetPrototypeMethod(fnTp, "loadOSMExtract", loadOSMExtract);
    SetPrototypeMethod(fnTp, "annotateRouteFromNodeIds", annotateRouteFromNodeIds);
    SetPrototypeMethod(fnTp, "annotateRouteFromLonLats", annotateRouteFromLonLats);
    SetPrototypeMethod(fnTp, "getAllTagsForWayId", getAllTagsForWayId);

    const auto fn = Nan::GetFunction(fnTp).ToLocalChecked();

    constructor().Reset(fn);

    Nan::Set(target, whoami, fn);
}

NAN_METHOD(Annotator::New)
{
    if (info.Length() != 0)
        return Nan::ThrowTypeError("No types expected");

    if (info.IsConstructCall())
    {
        auto *const self = new Annotator;
        self->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
    }
    else
    {
        return Nan::ThrowTypeError(
            "Cannot call constructor as function, you need to use 'new' keyword");
    }
}

NAN_METHOD(Annotator::loadOSMExtract)
{
    // In case we already loaded a dataset, this function will transactionally swap in a new one
    auto *const self = Nan::ObjectWrap::Unwrap<Annotator>(info.Holder());

    if (info.Length() != 2 || !info[0]->IsString() || !info[1]->IsFunction())
        return Nan::ThrowTypeError("String and callback expected");

    const Nan::Utf8String utf8String(info[0]);

    if (!(*utf8String))
        return Nan::ThrowError("Unable to convert to Utf8String");

    std::string path{*utf8String, *utf8String + utf8String.length()};

    struct OSMLoader final : Nan::AsyncWorker
    {
        explicit OSMLoader(Annotator &self_, Nan::Callback *callback, std::string path_)
            : Nan::AsyncWorker(callback), self{self_}, path{std::move(path_)}
        {
        }

        void Execute() override
        {
            try
            {
                // Note: provide strong exception safety guarantee (rollback)
                auto database = std::make_unique<Database>();
                Extractor extractor{path, *database};
                auto annotator = std::make_unique<RouteAnnotator>(*database);

                // Transactionally swap (noexcept)
                swap(self.database, database);
                swap(self.annotator, annotator);
            }
            catch (const std::exception &e)
            {
                return SetErrorMessage(e.what());
            }
        }

        void HandleOKCallback() override
        {
            Nan::HandleScope scope;
            const constexpr auto argc = 1u;
            v8::Local<v8::Value> argv[argc] = {Nan::Null()};
            callback->Call(argc, argv);
        }

        Annotator &self;
        std::string path;
    };

    auto *callback = new Nan::Callback{info[1].As<v8::Function>()};
    Nan::AsyncQueueWorker(new OSMLoader{*self, callback, std::move(path)});
}

NAN_METHOD(Annotator::annotateRouteFromNodeIds)
{
    auto *const self = Nan::ObjectWrap::Unwrap<Annotator>(info.Holder());

    if (!self->database || !self->annotator)
        return Nan::ThrowError("No OSM data loaded");

    if (info.Length() != 2 || !info[0]->IsArray() || !info[1]->IsFunction())
        return Nan::ThrowTypeError("Array of node ids and callback expected");

    const auto jsNodeIds = info[0].As<v8::Array>();

    // Guard against empty or one nodeId for which no wayId can be assigned
    if (jsNodeIds->Length() < 2)
        return info.GetReturnValue().Set(Nan::New<v8::Array>());

    std::vector<external_nodeid_t> externalIds(jsNodeIds->Length());

    for (std::size_t i{0}; i < jsNodeIds->Length(); ++i)
    {
        const auto nodeIdValue = Nan::Get(jsNodeIds, i).ToLocalChecked();

        if (!nodeIdValue->IsNumber())
            return Nan::ThrowTypeError("Array of number type expected");

        // Javascript has no UInt64 type, we have to go through floating point types.
        // Only safe until Number.MAX_SAFE_INTEGER, which is 2^53-1, guard with checked cast.
        const auto nodeIdDouble = Nan::To<double>(nodeIdValue).FromJust();

        try
        {
            const auto nodeId = boost::numeric_cast<external_nodeid_t>(nodeIdDouble);
            externalIds[i] = nodeId;
        }
        catch (const boost::numeric::bad_numeric_cast &e)
        {
            return Nan::ThrowError(e.what());
        };
    }

    struct WayIdsFromNodeIdsLoader final : Nan::AsyncWorker
    {
        explicit WayIdsFromNodeIdsLoader(Annotator &self_,
                                         Nan::Callback *callback,
                                         std::vector<external_nodeid_t> externalIds_)
            : Nan::AsyncWorker(callback), self{self_}, externalIds{std::move(externalIds_)}
        {
        }

        void Execute() override
        {
            const auto internalIds = self.annotator->external_to_internal(externalIds);
            wayIds = self.annotator->annotateRoute(internalIds);
        }

        void HandleOKCallback() override
        {
            Nan::HandleScope scope;

            auto annotated = Nan::New<v8::Array>(wayIds.size());

            for (std::size_t i{0}; i < wayIds.size(); ++i)
            {
                const auto wayId = wayIds[i];

                if (wayId == INVALID_WAYID)
                    (void)Nan::Set(annotated, i, Nan::Null());
                else
                    (void)Nan::Set(annotated, i, Nan::New<v8::Number>(wayIds[i]));
            }

            const constexpr auto argc = 2u;
            v8::Local<v8::Value> argv[argc] = {Nan::Null(), annotated};

            callback->Call(argc, argv);
        }

        Annotator &self;
        std::vector<external_nodeid_t> externalIds;
        annotated_route_t wayIds;
    };

    auto *callback = new Nan::Callback{info[1].As<v8::Function>()};
    Nan::AsyncQueueWorker(new WayIdsFromNodeIdsLoader{*self, callback, std::move(externalIds)});
}

NAN_METHOD(Annotator::annotateRouteFromLonLats)
{
    auto *const self = Nan::ObjectWrap::Unwrap<Annotator>(info.Holder());

    if (!self->database || !self->annotator)
        return Nan::ThrowError("No OSM data loaded");

    if (info.Length() != 2 || !info[0]->IsArray() || !info[1]->IsFunction())
        return Nan::ThrowTypeError("Array of [lon, lat] arrays and callback expected");

    auto jsLonLats = info[0].As<v8::Array>();

    // Guard against empty or one coordinate for which no wayId can be assigned
    if (jsLonLats->Length() < 2)
        return info.GetReturnValue().Set(Nan::New<v8::Array>());

    std::vector<point_t> coordinates(jsLonLats->Length());

    for (std::size_t i{0}; i < jsLonLats->Length(); ++i)
    {
        auto lonLatValue = Nan::Get(jsLonLats, i).ToLocalChecked();

        if (!lonLatValue->IsArray())
            return Nan::ThrowTypeError("Array of [lon, lat] expected");

        auto lonLatArray = lonLatValue.As<v8::Array>();

        if (lonLatArray->Length() != 2)
            return Nan::ThrowTypeError("Array of [lon, lat] expected");

        const auto lonValue = Nan::Get(lonLatArray, 0).ToLocalChecked();
        const auto latValue = Nan::Get(lonLatArray, 1).ToLocalChecked();

        if (!lonValue->IsNumber() || !latValue->IsNumber())
            return Nan::ThrowTypeError("Array of two numbers [lon, lat] expected");

        const auto lon = Nan::To<double>(lonValue).FromJust();
        const auto lat = Nan::To<double>(latValue).FromJust();

        coordinates[i] = {lon, lat};
    }

    struct WayIdsFromLonLatsLoader final : Nan::AsyncWorker
    {
        explicit WayIdsFromLonLatsLoader(Annotator &self_,
                                         Nan::Callback *callback,
                                         std::vector<point_t> coordinates_)
            : Nan::AsyncWorker(callback), self{self_}, coordinates{std::move(coordinates_)}
        {
        }

        void Execute() override
        {
            const auto internalIds = self.annotator->coordinates_to_internal(coordinates);
            wayIds = self.annotator->annotateRoute(internalIds);
        }

        void HandleOKCallback() override
        {
            Nan::HandleScope scope;

            auto annotated = Nan::New<v8::Array>(wayIds.size());

            for (std::size_t i{0}; i < wayIds.size(); ++i)
            {
                const auto wayId = wayIds[i];

                if (wayId == INVALID_WAYID)
                    (void)Nan::Set(annotated, i, Nan::Null());
                else
                    (void)Nan::Set(annotated, i, Nan::New<v8::Number>(wayIds[i]));
            }

            const constexpr auto argc = 2u;
            v8::Local<v8::Value> argv[argc] = {Nan::Null(), annotated};

            callback->Call(argc, argv);
        }

        Annotator &self;
        std::vector<point_t> coordinates;
        annotated_route_t wayIds;
    };

    auto *callback = new Nan::Callback{info[1].As<v8::Function>()};
    Nan::AsyncQueueWorker(new WayIdsFromLonLatsLoader{*self, callback, std::move(coordinates)});
}

NAN_METHOD(Annotator::getAllTagsForWayId)
{
    auto *const self = Nan::ObjectWrap::Unwrap<Annotator>(info.Holder());

    if (!self->database || !self->annotator)
        return Nan::ThrowError("No OSM data loaded");

    if (info.Length() != 2 || !info[0]->IsNumber() || !info[1]->IsFunction())
        return Nan::ThrowTypeError("Array of [lon, lat] arrays and callback expected");

    const auto wayId = Nan::To<wayid_t>(info[0]).FromJust();

    struct TagsForWayIdLoader final : Nan::AsyncWorker
    {
        explicit TagsForWayIdLoader(Annotator &self_, Nan::Callback *callback, wayid_t wayId_)
            : Nan::AsyncWorker(callback), self{self_}, wayId{std::move(wayId_)}
        {
        }

        void Execute() override { range = self.annotator->get_tag_range(wayId); }

        void HandleOKCallback() override
        {
            Nan::HandleScope scope;

            auto tags = Nan::New<v8::Object>();

            for (auto i = range.first; i <= range.second; ++i)
            {
                const auto key = self.annotator->get_tag_key(i);
                const auto value = self.annotator->get_tag_value(i);

                tags->Set(Nan::New(std::cref(key)).ToLocalChecked(),
                          Nan::New(std::cref(value)).ToLocalChecked());
            }

            const constexpr auto argc = 2u;
            v8::Local<v8::Value> argv[argc] = {Nan::Null(), tags};

            callback->Call(argc, argv);
        }

        Annotator &self;
        wayid_t wayId;
        tagrange_t range;
    };

    auto *callback = new Nan::Callback{info[1].As<v8::Function>()};
    Nan::AsyncQueueWorker(new TagsForWayIdLoader{*self, callback, std::move(wayId)});
}

Nan::Persistent<v8::Function> &Annotator::constructor()
{
    static Nan::Persistent<v8::Function> init;
    return init;
}
