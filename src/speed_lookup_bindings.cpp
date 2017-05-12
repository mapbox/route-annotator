#include "speed_lookup_bindings.hpp"
#include <vector>

NAN_MODULE_INIT(SpeedLookup::Init)
{
    const auto whoami = Nan::New("SpeedLookup").ToLocalChecked();

    auto fnTp = Nan::New<v8::FunctionTemplate>(New);
    fnTp->SetClassName(whoami);
    fnTp->InstanceTemplate()->SetInternalFieldCount(1);

    SetPrototypeMethod(fnTp, "loadCSV", loadCSV);
    SetPrototypeMethod(fnTp, "getRouteSpeeds", getRouteSpeeds);

    const auto fn = Nan::GetFunction(fnTp).ToLocalChecked();
    constructor().Reset(fn);

    Nan::Set(target, whoami, fn);
}

NAN_METHOD(SpeedLookup::New)
{
    if (info.Length() != 0)
        return Nan::ThrowTypeError("No types expected");

    if (info.IsConstructCall())
    {
        auto *const self = new SpeedLookup;
        self->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
    }
    else
    {
        return Nan::ThrowTypeError(
            "Cannot call constructor as function, you need to use 'new' keyword");
    }
}

/**
 * Loads a csv file asynchronously
 * @function loadCSV
 * @param {string} path the path to the CSV file to load
 * @param {function} callback function to call when the file is done loading
 */
NAN_METHOD(SpeedLookup::loadCSV)
{
    // In case we already loaded a dataset, this function will transactionally swap in a new one
    if (info.Length() != 2 || (!info[0]->IsString() && !info[0]->IsArray()) ||
        !info[1]->IsFunction())
        return Nan::ThrowTypeError("String (or array of strings) and callback expected");

    std::vector<std::string> paths;

    if (info[0]->IsString())
    {
        const Nan::Utf8String utf8String(info[0]);

        if (!(*utf8String))
            return Nan::ThrowError("Unable to convert to Utf8String");

        paths.push_back({*utf8String, *utf8String + utf8String.length()});
    }
    else if (info[0]->IsArray())
    {
        auto arr = v8::Local<v8::Array>::Cast(info[0]);
        if (arr->Length() < 1)
        {
            return Nan::ThrowTypeError("Array must contain at least one filename");
        }
        for (std::uint32_t idx = 0; idx < arr->Length(); ++idx)
        {
            const Nan::Utf8String utf8String(arr->Get(idx));

            if (!(*utf8String))
                return Nan::ThrowError("Unable to convert to Utf8String");

            paths.push_back({*utf8String, *utf8String + utf8String.length()});
        }
    }
    else
    {
        return Nan::ThrowTypeError("First parameter should be a string or an array");
    }

    struct CSVLoader final : Nan::AsyncWorker
    {
        explicit CSVLoader(v8::Local<v8::Object> self_,
                           Nan::Callback *callback,
                           std::vector<std::string> paths_)
            : Nan::AsyncWorker(callback), paths{std::move(paths_)}
        {
            SaveToPersistent("self", self_);
        }

        void Execute() override
        {
            try
            {
                map = std::make_shared<SegmentSpeedMap>();
                for (const auto &path : paths)
                {
                    map->loadCSV(path);
                }
            }
            catch (const std::exception &e)
            {
                return SetErrorMessage(e.what());
            }
        }

        void HandleOKCallback() override
        {
            Nan::HandleScope scope;
            auto self = GetFromPersistent("self")->ToObject();
            auto *unwrapped = Nan::ObjectWrap::Unwrap<SpeedLookup>(self);
            swap(unwrapped->datamap, map);
            const constexpr auto argc = 1u;
            v8::Local<v8::Value> argv[argc] = {Nan::Null()};
            callback->Call(argc, argv);
        }

        std::vector<std::string> paths;
        std::shared_ptr<SegmentSpeedMap> map;
    };

    auto *callback = new Nan::Callback{info[1].As<v8::Function>()};
    Nan::AsyncQueueWorker(new CSVLoader{info.Holder(), callback, std::move(paths)});
}

/**
 * Fetches the values in the hashtable for pairs of nodes
 * @function getRouteSpeeds
 * @param {array} nodes an array of node IDs to look up in pairs
 * @param {function} callback receives the results of the search as an array
 * @example
 *   container.getRouteSpeeds([23,43,12],(err,result) => {
 *      console.log(result);
 *   });
 */
NAN_METHOD(SpeedLookup::getRouteSpeeds)
{
    auto *const self = Nan::ObjectWrap::Unwrap<SpeedLookup>(info.Holder());

    if (info.Length() != 2 || !info[0]->IsArray())
        return Nan::ThrowTypeError("Two arguments expected: nodeIds (Array), Callback");

    auto callback = info[1].As<v8::Function>();
    const auto jsNodeIds = info[0].As<v8::Array>();
    // Guard against empty or one nodeId for which no wayId can be assigned
    if (jsNodeIds->Length() < 2)
        return Nan::ThrowTypeError(
            "getRouteSpeeds expects 'nodeIds' (Array(Number)) of at least length 2");

    std::vector<external_nodeid_t> nodes_to_query(jsNodeIds->Length());

    for (uint32_t i = 0; i < jsNodeIds->Length(); ++i)
    {
        v8::Local<v8::Value> jsNodeId = jsNodeIds->Get(i);
        if (!jsNodeId->IsNumber())
            return Nan::ThrowTypeError("NodeIds must be an array of numbers");
        auto signedNodeId = Nan::To<int64_t>(jsNodeId).FromJust();
        if (signedNodeId < 0)
            return Nan::ThrowTypeError(
                "getRouteSpeeds expects 'nodeId' within (Array(Number))to be non-negative");

        external_nodeid_t nodeId = static_cast<external_nodeid_t>(signedNodeId);
        nodes_to_query[i] = nodeId;
    }

    struct Worker final : Nan::AsyncWorker
    {
        using Base = Nan::AsyncWorker;

        Worker(std::shared_ptr<SegmentSpeedMap> datamap_,
               Nan::Callback *callback,
               std::vector<external_nodeid_t> nodeIds)
            : Base(callback), datamap{datamap_}, nodeIds(std::move(nodeIds))
        {
        }
        void Execute() override { result_annotations = datamap->getValues(nodeIds); }
        void HandleOKCallback() override
        {
            Nan::HandleScope scope;

            auto jsAnnotations = Nan::New<v8::Array>(result_annotations.size());

            for (std::size_t i = 0; i < result_annotations.size(); ++i)
            {
                auto jsAnnotation = Nan::New<v8::Number>(result_annotations[i]);
                (void)Nan::Set(jsAnnotations, i, jsAnnotation);
            }

            const auto argc = 2u;
            v8::Local<v8::Value> argv[argc] = {Nan::Null(), jsAnnotations};

            callback->Call(argc, argv);
        }

        std::shared_ptr<SegmentSpeedMap> datamap;
        std::vector<external_nodeid_t> nodeIds;
        std::vector<segment_speed_t> result_annotations;
    };

    Nan::AsyncQueueWorker(
        new Worker{self->datamap, new Nan::Callback{callback}, std::move(nodes_to_query)});
}

Nan::Persistent<v8::Function> &SpeedLookup::constructor()
{
    static Nan::Persistent<v8::Function> init;
    return init;
}
