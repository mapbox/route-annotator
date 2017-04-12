#include "lookup_bindings.hpp"


Lookup::Lookup(Hashmap annotations_) :
annotations(std::make_shared<const Hashmap>(std::move(annotations_))) {}

NAN_MODULE_INIT(Lookup::Init) {
  const auto whoami = Nan::New("Lookup").ToLocalChecked();

  auto fnTp = Nan::New<v8::FunctionTemplate>(New);
  fnTp->SetClassName(whoami);
  fnTp->InstanceTemplate()->SetInternalFieldCount(1);

  // SetPrototypeMethod(fnTp, "GetAnnotations", GetAnnotations);

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

  if (info.Length() != 1 || !info[0]->IsObject())
    return Nan::ThrowTypeError("Single object argument expected: Data filename string");

  auto opts = info[0].As<v8::Object>();

  auto maybeFilename = Nan::Get(opts, Nan::New("filename").ToLocalChecked());

  auto filenameOk = !maybeFilename.IsEmpty() && maybeFilename.ToLocalChecked()->IsString();

  if (!filenameOk)
    return Nan::ThrowTypeError("Lookup expects 'filename' (String)");

  auto filename = *v8::String::Utf8Value(maybeFilename.ToLocalChecked());
  Hashmap annotations(filename);

  auto* self = new Lookup(std::move(annotations));

  self->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Lookup::GetAnnotations) {
  auto* const self = Nan::ObjectWrap::Unwrap<Lookup>(info.Holder());

  if (info.Length() != 1 || !info[0]->IsArray())
    return Nan::ThrowTypeError("One argument expected: nodeIds (Array)");

  auto opts = info[0].As<v8::Array>();

  auto maybeNodeIds = Nan::Get(opts, Nan::New("nodeIds").ToLocalChecked());

  auto nodeIdsOk = !maybeNodeIds.IsEmpty() && maybeNodeIds.ToLocalChecked()->IsArray() && maybeNodeIds->Length() > 1 && maybeNodeIds->Get(0)->IsNumber();

  if (!nodeIdsOk)
    return Nan::ThrowTypeError("GetAnnotations expects 'nodeIds' (Array(Number))");

  // // TODO: put in its own file
  // struct Worker final : Nan::AsyncWorker {
  //   using Base = Nan::AsyncWorker;

  //   Worker(std::shared_ptr<const CostMatrix> costs_, Nan::Callback* callback, int numNodes, int numVehicles, int vehicleDepot,
  //          const ort::RoutingModelParameters& modelParams_, const ort::RoutingSearchParameters& searchParams_)
  //       : Base(callback), costs{std::move(costs_)},
  //         model(numNodes, numVehicles, ort::RoutingModel::NodeIndex{vehicleDepot}, modelParams_), modelParams(modelParams_),
  //         searchParams(searchParams_), routes{} {}

  //   void Execute() override {
  //     struct CostMatrixAdapter {
  //       int64 operator()(ort::RoutingModel::NodeIndex from, ort::RoutingModel::NodeIndex to) const {
  //         return costMatrix(from.value(), to.value());
  //       }

  //       const CostMatrix& costMatrix;
  //     } const costAdapter{*costs};

  //     const auto costEvaluator = NewPermanentCallback(&costAdapter, &CostMatrixAdapter::operator());
  //     model.SetArcCostEvaluatorOfAllVehicles(costEvaluator);

  //     const auto* solution = model.SolveWithParameters(searchParams);

  //     if (!solution)
  //       SetErrorMessage("Unable to find a solution");

  //     const auto cost = solution->ObjectiveValue();
  //     (void)cost; // TODO: use

  //     model.AssignmentToRoutes(*solution, &routes);
  //   }

  //   void HandleOKCallback() override {
  //     Nan::HandleScope scope;

  //     auto jsRoutes = Nan::New<v8::Array>(routes.size());

  //     for (int i = 0; i < routes.size(); ++i) {
  //       const auto& route = routes[i];

  //       auto jsNodes = Nan::New<v8::Array>(route.size());

  //       for (int j = 0; j < route.size(); ++j)
  //         (void)Nan::Set(jsNodes, j, Nan::New<v8::Number>(route[j].value()));

  //       (void)Nan::Set(jsRoutes, i, jsNodes);
  //     }

  //     const auto argc = 2u;
  //     v8::Local<v8::Value> argv[argc] = {Nan::Null(), jsRoutes};

  //     callback->Call(argc, argv);
  //   }

  //   std::shared_ptr<const CostMatrix> costs; // inc ref count to keep alive for async cb

  //   ort::RoutingModel model;
  //   ort::RoutingModelParameters modelParams;
  //   ort::RoutingSearchParameters searchParams;

  //   std::vector<std::vector<ort::RoutingModel::NodeIndex>> routes;
  // };

  // TODO: overflow
  auto timeLimit = Nan::To<int>(maybeTimeLimit.ToLocalChecked()).FromJust();
  auto depotNode = Nan::To<int>(maybeDepotNode.ToLocalChecked()).FromJust();

  // See routing_parameters.proto and routing_enums.proto
  auto modelParams = ort::RoutingModel::DefaultModelParameters();
  auto searchParams = ort::RoutingModel::DefaultSearchParameters();

  // TODO: Make configurable from Js?
  auto firstSolutionStrategy = ort::FirstSolutionStrategy::AUTOMATIC;
  auto metaHeuristic = ort::LocalSearchMetaheuristic::AUTOMATIC;

  searchParams.set_first_solution_strategy(firstSolutionStrategy);
  searchParams.set_local_search_metaheuristic(metaHeuristic);
  searchParams.set_time_limit_ms(timeLimit);

  auto* worker = new Worker{self->costs,                 //
                            new Nan::Callback{callback}, //
                            numNodes,                    //
                            numVehicles,                 //
                            depotNode,                   //
                            modelParams,                 //
                            searchParams};               //

  Nan::AsyncQueueWorker(worker);
}

Nan::Persistent<v8::Function>& Lookup::constructor() {
  static Nan::Persistent<v8::Function> init;
  return init;
}
