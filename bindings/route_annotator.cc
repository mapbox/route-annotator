#include <cstdint>

#include <string>
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
        auto *self = Nan::ObjectWrap::Unwrap<Annotator>(info.Holder());
        info.GetReturnValue().Set(self->id);
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
