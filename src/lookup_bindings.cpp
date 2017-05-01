#include "lookup_bindings.hpp"


Lookup::Lookup(Hashmap annotations_) :
annotations(std::make_shared<Hashmap>(std::move(annotations_))) {}

NAN_MODULE_INIT(Lookup::Init) {
  const auto whoami = Nan::New("Lookup").ToLocalChecked();

  auto fnTp = Nan::New<v8::FunctionTemplate>(New);
  fnTp->SetClassName(whoami);
  fnTp->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(fnTp, "GetAnnotations", GetAnnotations);

  const auto fn = Nan::GetFunction(fnTp).ToLocalChecked();
  constructor().Reset(fn);

  Nan::Set(target, whoami, fn);
}

NAN_METHOD(Lookup::New) {
  // Handle `new T()` as well as `T()`
  if (!info.IsConstructCall()) {
    auto init = Nan::New(constructor());
    info.GetReturnValue().Set(init->NewInstance());
    return;
  }

  if (info.Length() != 1 || !info[0]->IsString())
    return Nan::ThrowTypeError("Single object argument expected: Data filename string");

  const Nan::Utf8String utf8String(info[0]);

  if (!(*utf8String))
      return Nan::ThrowError("Unable to convert to filename (Utf8String)");

  std::string filename{*utf8String, *utf8String + utf8String.length()};

  Hashmap annotations(filename);

  auto* self = new Lookup(std::move(annotations));

  self->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Lookup::GetAnnotations) {
  auto* const self = Nan::ObjectWrap::Unwrap<Lookup>(info.Holder());
  (void) self;

  if (info.Length() != 2 || !info[0]->IsArray())
    return Nan::ThrowTypeError("Two arguments expected: nodeIds (Array), Callback");

  auto callback = info[1].As<v8::Function>();
  const auto jsNodeIds = info[0].As<v8::Array>();
  // Guard against empty or one nodeId for which no wayId can be assigned
  if (jsNodeIds->Length() < 2)
      return Nan::ThrowTypeError("GetAnnotations expects 'nodeIds' (Array(Number)) of at least length 2");

    std::vector<external_nodeid_t> resulting_nodeIds(jsNodeIds->Length());

  for (uint32_t i = 0; i < jsNodeIds->Length(); ++i)
  {
    v8::Local<v8::Value> jsNodeId = jsNodeIds->Get(i);
    if (!jsNodeId->IsNumber())
      return Nan::ThrowTypeError("NodeIds must be an array of numbers");
    auto signedNodeId = Nan::To<int64_t>(jsNodeId).FromJust();
    if(signedNodeId < 0)
      return Nan::ThrowTypeError("GetAnnotations expects 'nodeId' within (Array(Number))to be non-negative");
    // auto nodeId = Nan::To<external_nodeid_t>(jsNodeId).FromJust();
    external_nodeid_t nodeId = static_cast<external_nodeid_t>(signedNodeId);
    resulting_nodeIds[i] = nodeId;
  }

  struct Worker final : Nan::AsyncWorker {
    using Base = Nan::AsyncWorker;

    Worker(std::shared_ptr<Hashmap> annotations, Nan::Callback* callback, std::vector<external_nodeid_t> nodeIds)
        : Base(callback), annotations{std::move(annotations)}, nodeIds(std::move(nodeIds)) {}
    void Execute() override {
      result_annotations = annotations->getValues(nodeIds);
    }
    void HandleOKCallback() override {
      Nan::HandleScope scope;

      auto jsAnnotations = Nan::New<v8::Array>(result_annotations.size());

      for (std::size_t i = 0; i < result_annotations.size(); ++i) {
        auto jsAnnotation = Nan::New<v8::Number>(result_annotations[i]);
        (void)Nan::Set(jsAnnotations, i, jsAnnotation);
      }

      const auto argc = 2u;
      v8::Local<v8::Value> argv[argc] = {Nan::Null(), jsAnnotations};

      callback->Call(argc, argv);
    }
    std::shared_ptr<Hashmap> annotations; // inc ref count to keep alive for async cb
    std::vector<external_nodeid_t> nodeIds;
    std::vector<congestion_speed_t> result_annotations;
  };

  auto* worker = new Worker{self->annotations,
                            new Nan::Callback{callback},
                            resulting_nodeIds};

  Nan::AsyncQueueWorker(worker);
}

Nan::Persistent<v8::Function>& Lookup::constructor() {
  static Nan::Persistent<v8::Function> init;
  return init;
}
