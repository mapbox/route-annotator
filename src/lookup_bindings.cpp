#include "lookup_bindings.hpp"

NAN_MODULE_INIT(Lookup::Init)
{
    const auto whoami = Nan::New("Lookup").ToLocalChecked();

    auto fnTp = Nan::New<v8::FunctionTemplate>(New);
    fnTp->SetClassName(whoami);
    fnTp->InstanceTemplate()->SetInternalFieldCount(1);

    SetPrototypeMethod(fnTp, "loadCSV", loadCSV);
    SetPrototypeMethod(fnTp, "GetAnnotations", GetAnnotations);

    const auto fn = Nan::GetFunction(fnTp).ToLocalChecked();
    constructor().Reset(fn);

    Nan::Set(target, whoami, fn);
}

NAN_METHOD(Lookup::New)
{
    if (info.Length() != 0)
        return Nan::ThrowTypeError("No types expected");

    if (info.IsConstructCall())
    {
        auto *const self = new Lookup;
        self->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
    }
    else
    {
        return Nan::ThrowTypeError(
            "Cannot call constructor as function, you need to use 'new' keyword");
    }

    // Handle `new T()` as well as `T()`
    if (!info.IsConstructCall())
    {
        auto init = Nan::New(constructor());
        info.GetReturnValue().Set(init->NewInstance());
        return;
    }
}

NAN_METHOD(Lookup::loadCSV)
{
    // In case we already loaded a dataset, this function will transactionally swap in a new one
    auto *const self = Nan::ObjectWrap::Unwrap<Lookup>(info.Holder());

    if (info.Length() != 2 || !info[0]->IsString() || !info[1]->IsFunction())
        return Nan::ThrowTypeError("String and callback expected");

    const Nan::Utf8String utf8String(info[0]);

    if (!(*utf8String))
        return Nan::ThrowError("Unable to convert to Utf8String");

    std::string path{*utf8String, *utf8String + utf8String.length()};

    struct CSVLoader final : Nan::AsyncWorker
    {
        explicit CSVLoader(Lookup &self_, Nan::Callback *callback, std::string path_)
            : Nan::AsyncWorker(callback), self{self_}, path{std::move(path_)}
        {
        }

        void Execute() override
        {
            try
            {
                auto tmpmap = std::make_unique<Hashmap>(path);

                swap(self.datamap, tmpmap);
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

        Lookup &self;
        std::string path;
    };

    auto *callback = new Nan::Callback{info[1].As<v8::Function>()};
    Nan::AsyncQueueWorker(new CSVLoader{*self, callback, std::move(path)});
}

NAN_METHOD(Lookup::GetAnnotations)
{
    auto *const self = Nan::ObjectWrap::Unwrap<Lookup>(info.Holder());

    if (info.Length() != 2 || !info[0]->IsArray())
        return Nan::ThrowTypeError("Two arguments expected: nodeIds (Array), Callback");

    auto callback = info[1].As<v8::Function>();
    const auto jsNodeIds = info[0].As<v8::Array>();
    // Guard against empty or one nodeId for which no wayId can be assigned
    if (jsNodeIds->Length() < 2)
        return Nan::ThrowTypeError(
            "GetAnnotations expects 'nodeIds' (Array(Number)) of at least length 2");

    std::vector<external_nodeid_t> nodes_to_query(jsNodeIds->Length());

    for (uint32_t i = 0; i < jsNodeIds->Length(); ++i)
    {
        v8::Local<v8::Value> jsNodeId = jsNodeIds->Get(i);
        if (!jsNodeId->IsNumber())
            return Nan::ThrowTypeError("NodeIds must be an array of numbers");
        auto signedNodeId = Nan::To<int64_t>(jsNodeId).FromJust();
        if (signedNodeId < 0)
            return Nan::ThrowTypeError(
                "GetAnnotations expects 'nodeId' within (Array(Number))to be non-negative");

        external_nodeid_t nodeId = static_cast<external_nodeid_t>(signedNodeId);
        nodes_to_query[i] = nodeId;
    }

    struct Worker final : Nan::AsyncWorker
    {
        using Base = Nan::AsyncWorker;

        Worker(Lookup &self_,
               Nan::Callback *callback,
               std::vector<external_nodeid_t> nodeIds)
            : Base(callback), self{self_}, nodeIds(std::move(nodeIds))
        {
        }
        void Execute() override { result_annotations = self.datamap->getValues(nodeIds); }
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

        Lookup &self;
        std::vector<external_nodeid_t> nodeIds;
        std::vector<congestion_speed_t> result_annotations;
    };

    Nan::AsyncQueueWorker(new Worker{*self, new Nan::Callback{callback}, std::move(nodes_to_query)});
}

Nan::Persistent<v8::Function> &Lookup::constructor()
{
    static Nan::Persistent<v8::Function> init;
    return init;
}
