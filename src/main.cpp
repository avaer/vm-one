#include <v8.h>
#include <uv.h>
#include <nan.h>

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
// protected:
  static NAN_METHOD(New);
  static NAN_METHOD(SetGlobal);
  static NAN_METHOD(GetGlobal);
  static NAN_METHOD(FromArray);
  static NAN_METHOD(ToArray);
  static NAN_METHOD(Lock);
  static NAN_METHOD(Unlock);
  static NAN_METHOD(Request);
  static NAN_METHOD(Respond);

  VmOne();
  ~VmOne();

  static void RunInThread(uv_async_t *handle);

// protected:
  Nan::Persistent<Value> global;
  uv_async_t async;
  uv_sem_t lockRequestSem;
  uv_sem_t lockResponseSem;
  uv_sem_t requestSem;
};

thread_local VmOne *threadVmOne = nullptr;

Handle<Object> VmOne::Initialize() {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("VmOne"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto, "getGlobal", GetGlobal);
  Nan::SetMethod(proto, "setGlobal", SetGlobal);
  Nan::SetMethod(proto, "toArray", ToArray);
  Nan::SetMethod(proto, "lock", Lock);
  Nan::SetMethod(proto, "unlock", Unlock);
  Nan::SetMethod(proto, "request", Request);
  Nan::SetMethod(proto, "respond", Respond);

  Local<Function> ctorFn = ctor->GetFunction();

  Local<Function> fromArrayFn = Nan::New<Function>(FromArray);
  ctorFn->Set(JS_STR("fromArray"), fromArrayFn);

  uintptr_t initFunctionAddress = (uintptr_t)Init;
  Local<Array> initFunctionAddressArray = Nan::New<Array>(2);
  initFunctionAddressArray->Set(0, Nan::New<Integer>((uint32_t)(initFunctionAddress >> 32)));
  initFunctionAddressArray->Set(1, Nan::New<Integer>((uint32_t)(initFunctionAddress & 0xFFFFFFFF)));
  ctorFn->Set(JS_STR("initFunctionAddress"), initFunctionAddressArray);

  return scope.Escape(ctorFn);
}

NAN_METHOD(VmOne::New) {
  Local<Object> vmOneObj = Local<Object>::Cast(info.This());

  VmOne *vmOne = new VmOne();
  vmOne->Wrap(vmOneObj);

  threadVmOne = vmOne; // XXX check for duplicates

  info.GetReturnValue().Set(vmOneObj);
}

NAN_METHOD(VmOne::GetGlobal) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
  Local<Value> globalValue = Nan::New(vmOne->global);
  info.GetReturnValue().Set(globalValue);
}

NAN_METHOD(VmOne::SetGlobal) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
  vmOne->global.Reset(info[0]);
}

NAN_METHOD(VmOne::FromArray) {
  Local<Array> array = Local<Array>::Cast(info[0]);

  Local<Function> vmOneConstructor = Local<Function>::Cast(info.This());
  Local<Value> argv[] = {
    array->Get(0),
    array->Get(1),
  };
  Local<Value> vmOneObj = vmOneConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();

  info.GetReturnValue().Set(vmOneObj);
}

NAN_METHOD(VmOne::ToArray) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());

  Local<Array> array = Nan::New<Array>(4);
  array->Set(0, Nan::New<Integer>((uint32_t)((uintptr_t)vmOne >> 32)));
  array->Set(1, Nan::New<Integer>((uint32_t)((uintptr_t)vmOne & 0xFFFFFFFF)));

  info.GetReturnValue().Set(array);
}

VmOne::VmOne() {
  uv_async_init(uv_default_loop(), &async, RunInThread);
  uv_sem_init(&lockRequestSem, 0);
  uv_sem_init(&lockResponseSem, 0);
  uv_sem_init(&requestSem, 0);
}

VmOne::~VmOne() {}

void VmOne::RunInThread(uv_async_t *handle) {
  Nan::HandleScope scope;

  VmOne *vmOne = threadVmOne;
  uv_sem_post(&threadVmOne->lockRequestSem);
  uv_sem_wait(&threadVmOne->lockResponseSem);
}

NAN_METHOD(VmOne::Lock) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());

  uv_async_send(&vmOne->async);
  uv_sem_wait(&vmOne->lockRequestSem);
}

NAN_METHOD(VmOne::Unlock) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());

  uv_sem_post(&vmOne->lockResponseSem);
}

NAN_METHOD(VmOne::Request) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
  uv_sem_wait(&vmOne->requestSem);
}

NAN_METHOD(VmOne::Respond) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
  uv_sem_post(&vmOne->requestSem);
}

void Init(Handle<Object> exports) {
  exports->Set(JS_STR("VmOne"), VmOne::Initialize());
}

}

#ifndef LUMIN
NODE_MODULE(NODE_GYP_MODULE_NAME, vmone::Init)
#else
extern "C" {
  void node_register_module_vm_one(Local<Object> exports, Local<Value> module, Local<Context> context) {
    vmone::Init(exports);
  }
}
#endif
