#include <v8.h>
#include <nan.h>
#include <iostream>

using namespace v8;
using namespace node;

#define JS_STR(...) Nan::New<v8::String>(__VA_ARGS__).ToLocalChecked()
#define JS_INT(val) Nan::New<v8::Integer>(val)
#define JS_NUM(val) Nan::New<v8::Number>(val)
#define JS_FLOAT(val) Nan::New<v8::Number>(val)
#define JS_BOOL(val) Nan::New<v8::Boolean>(val)

namespace vmone {

class VmOne : public ObjectWrap {
  public:
    static Handle<Object> Initialize();
  protected:
    static NAN_METHOD(New);
    static NAN_METHOD(Run);

    VmOne(Local<Object> globalInit);
    ~VmOne();

    Nan::Persistent<Context> context;
    Nan::Persistent<Object> global;
};

Handle<Object> VmOne::Initialize() {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("VmOne"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto, "run", Run);

  Local<Function> ctorFn = ctor->GetFunction();

  return scope.Escape(ctorFn);
}

NAN_METHOD(VmOne::New) {
  if (info[0]->IsObject()) {
    Local<Object> global = Local<Object>::Cast(info[0]);

    Local<Object> vmOneObj = Local<Object>::Cast(info.This());

    VmOne *vmOne = new VmOne(global);
    vmOne->Wrap(vmOneObj);

    info.GetReturnValue().Set(vmOneObj);
  } else {
    Nan::ThrowError("invalid arguments");
  }
}

NAN_METHOD(VmOne::Run) {
  if (info[0]->IsString()) {
    std::cout << "script 1" << "\n";

    Local<String> src = Local<String>::Cast(info[0]);
    Local<String> resourceName = info[1]->IsString() ?
        Local<String>::Cast(info[1])
      :
        JS_STR("script");
    Local<Integer> lineOffset = info[2]->IsNumber() ?
        Local<Number>::Cast(info[2])->ToInteger()
      :
        Integer::New(Isolate::GetCurrent(), 0);
    Local<Integer> colOffset = info[3]->IsNumber() ?
        Local<Number>::Cast(info[3])->ToInteger()
      :
        Integer::New(Isolate::GetCurrent(), 0);

    VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
    Local<Context> contextLocal = Nan::New(vmOne->context);

    std::cout << "script 2" << "\n";

    ScriptOrigin scriptOrigin(
      resourceName,
      lineOffset,
      colOffset
    );

    std::cout << "script 3" << "\n";

    {
      Nan::TryCatch tryCatch;
      MaybeLocal<Script> scriptMaybe = Script::Compile(contextLocal, src, &scriptOrigin);

      if (!tryCatch.HasCaught()) {
        Local<Script> script = scriptMaybe.ToLocalChecked();
        Local<Value> result = script->Run();

        if (!tryCatch.HasCaught()) {
          info.GetReturnValue().Set(result);
        } else {
          tryCatch.ReThrow();
        }
      } else {
        tryCatch.ReThrow();
      }
    }
  } else {
    Nan::ThrowError("invalid arguments");
  }
}

VmOne::VmOne(Local<Object> globalInit) {
  Local<Context> localContext = Context::New(Isolate::GetCurrent());
  Local<Object> contextGlobal = localContext->Global();

  Local<Array> propertyNames = globalInit->GetOwnPropertyNames(localContext).ToLocalChecked();
  size_t numPropertyNames = propertyNames->Length();
  for (size_t i = 0; i < numPropertyNames; i++) {
    Local<String> key = propertyNames->Get(i)->ToString();
    Local<Value> value = globalInit->Get(localContext, key).ToLocalChecked();
    contextGlobal->Set(localContext, key, value);
  }

  localContext->AllowCodeGenerationFromStrings(true);

  context.Reset(localContext);
}
VmOne::~VmOne() {}

void Init(Handle<Object> exports) {
  exports->Set(JS_STR("VmOne"), VmOne::Initialize());
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)

}
