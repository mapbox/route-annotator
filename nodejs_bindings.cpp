#include <cstdint>

#include <chrono>
#include <string>
#include <thread>
#include <utility>

#include <nan.h>

class Annotator final : public Nan::ObjectWrap
{
  public:
    static NAN_MODULE_INIT(Init)
    {
        const auto whoami = Nan::New("Annotator").ToLocalChecked();

        auto fnTp = Nan::New<v8::FunctionTemplate>(New);
        fnTp->SetClassName(whoami);
        fnTp->InstanceTemplate()->SetInternalFieldCount(1);

        SetPrototypeMethod(fnTp, "getValue", GetValue);
        SetPrototypeMethod(fnTp, "getValueAsync", GetValueAsync);

        auto fn = Nan::GetFunction(fnTp).ToLocalChecked();

        constructor().Reset(fn);

        Nan::Set(target, whoami, fn);
    }

  private:
    static NAN_METHOD(New)
    {
        if (info.Length() != 1 || !info[0]->IsUint32())
            return Nan::ThrowError("First argument has to be an UInt");

        const auto id = Nan::To<std::uint32_t>(info[0]).FromJust();

        if (info.IsConstructCall())
        {
            auto *self = new Annotator(id);
            self->Wrap(info.This());

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

    static NAN_METHOD(GetValue)
    {
        if (info.Length() != 0)
            return Nan::ThrowError("No argument expected");

        auto *self = Nan::ObjectWrap::Unwrap<Annotator>(info.Holder());
        info.GetReturnValue().Set(self->id);

        std::this_thread::sleep_for(std::chrono::seconds{5});
    }

    static NAN_METHOD(GetValueAsync)
    {
        if (info.Length() != 1 || !info[0]->IsFunction())
            return Nan::ThrowError("First argument has to be a callback");

        struct Worker final : Nan::AsyncWorker
        {
            explicit Worker(Nan::Callback *callback) : Nan::AsyncWorker(callback) {}

            void Execute() override
            {
                // return SetErrorMessage("Oops");
                std::this_thread::sleep_for(std::chrono::seconds{5});
                response = 0;
            };

            void HandleOKCallback() override
            {
                Nan::HandleScope scope;

                const constexpr auto argc = 2u;

                v8::Local<v8::Value> argv[argc] = {
                    Nan::Null(),                    // err
                    Nan::New<v8::Number>(response), // response
                };

                callback->Call(argc, argv);
            }

            int response = -1; // dummy
        };

        auto *callback = new Nan::Callback{info[0].As<v8::Function>()};
        Nan::AsyncQueueWorker(new Worker{callback});
    }

    static inline Nan::Persistent<v8::Function> &constructor()
    {
        static Nan::Persistent<v8::Function> init;
        return init;
    }

    /* Impl. */
    explicit Annotator(std::uint32_t id_) : id{id_} {}
    const std::uint32_t id;
};

NODE_MODULE(route_annotator, Annotator::Init)
