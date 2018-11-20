#include <v8.h>
#include <uv.h>
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

  VmOne(VmOne *oldVmOne = nullptr);
  ~VmOne();

  static void RunInThread(uv_async_t *handle);

// protected:
  Nan::Persistent<Value> *global;
  uv_async_t *async;
  uv_sem_t *lockRequestSem;
  uv_sem_t *lockResponseSem;
  uv_sem_t *requestSem;
  bool owner;
};

thread_local VmOne *threadVmOne = nullptr;
void Init(Handle<Object> exports);

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

  uintptr_t initFnAddress = (uintptr_t)vmone::Init;
  Local<Array> initFnAddressArray = Nan::New<Array>(2);
  initFnAddressArray->Set(0, Nan::New<Integer>((uint32_t)(initFnAddress >> 32)));
  initFnAddressArray->Set(1, Nan::New<Integer>((uint32_t)(initFnAddress & 0xFFFFFFFF)));
  ctorFn->Set(JS_STR("initFnAddress"), initFnAddressArray);

  return scope.Escape(ctorFn);
}

NAN_METHOD(VmOne::New) {
  Local<Object> vmOneObj = Local<Object>::Cast(info.This());

  VmOne *oldVmOne;
  if (info[0]->IsArray()) {
    std::cout << "new from array" << std::endl;
    Local<Array> array = Local<Array>::Cast(info[0]);
    uint32_t a = array->Get(0)->Uint32Value();
    uint32_t b = array->Get(1)->Uint32Value();
    uintptr_t c = ((uintptr_t)a << 32) | (uintptr_t)b;
    oldVmOne = reinterpret_cast<VmOne *>(c);
  } else {
    std::cout << "new from scratch" << std::endl;
    oldVmOne = nullptr;
  }

  VmOne *vmOne = oldVmOne ? new VmOne(oldVmOne) : new VmOne();
  vmOne->Wrap(vmOneObj);

  if (!oldVmOne) {
    threadVmOne = vmOne; // XXX check for duplicates
  }

  info.GetReturnValue().Set(vmOneObj);
}

NAN_METHOD(VmOne::GetGlobal) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
  Local<Value> globalValue = Nan::New(*vmOne->global);
  info.GetReturnValue().Set(globalValue);
}

NAN_METHOD(VmOne::SetGlobal) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
  vmOne->global->Reset(info[0]);
}

NAN_METHOD(VmOne::FromArray) {
  Local<Array> array = Local<Array>::Cast(info[0]);

  Local<Function> vmOneConstructor = Local<Function>::Cast(info.This());
  Local<Value> argv[] = {
    array,
  };
  Local<Value> vmOneObj = vmOneConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();

  info.GetReturnValue().Set(vmOneObj);
}

NAN_METHOD(VmOne::ToArray) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());

  Local<Array> array = Nan::New<Array>(2);
  array->Set(0, Nan::New<Integer>((uint32_t)((uintptr_t)vmOne >> 32)));
  array->Set(1, Nan::New<Integer>((uint32_t)((uintptr_t)vmOne & 0xFFFFFFFF)));

  info.GetReturnValue().Set(array);
}

VmOne::VmOne(VmOne *oldVmOne) {
  if (!oldVmOne) {
    global = new Nan::Persistent<Value>();
    async = new uv_async_t();
    uv_async_init(uv_default_loop(), async, RunInThread);
    lockRequestSem = new uv_sem_t();
    uv_sem_init(lockRequestSem, 0);
    lockResponseSem = new uv_sem_t();
    uv_sem_init(lockResponseSem, 0);
    requestSem = new uv_sem_t();
    uv_sem_init(requestSem, 0);
    std::cout << "init parent with request sem" << (void *)requestSem << std::endl;
    owner = true;
  } else {
    global = oldVmOne->global;
    async = oldVmOne->async;
    lockRequestSem = oldVmOne->lockRequestSem;
    lockResponseSem = oldVmOne->lockResponseSem;
    requestSem = oldVmOne->requestSem;
    std::cout << "init child with request sem" << (void *)requestSem << std::endl;
    owner = false;
  }
}

VmOne::~VmOne() {
  if (owner) {
    uv_close((uv_handle_t *)async, nullptr);
    delete async;
    uv_sem_destroy(lockRequestSem);
    delete lockRequestSem;
    uv_sem_destroy(lockResponseSem);
    delete lockResponseSem;
    uv_sem_destroy(requestSem);
    delete requestSem;
  }
}

void VmOne::RunInThread(uv_async_t *handle) {
  Nan::HandleScope scope;

  VmOne *vmOne = threadVmOne;
  uv_sem_post(threadVmOne->lockRequestSem);
  uv_sem_wait(threadVmOne->lockResponseSem);
}

NAN_METHOD(VmOne::Lock) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());

  uv_async_send(vmOne->async);
  uv_sem_wait(vmOne->lockRequestSem);
}

NAN_METHOD(VmOne::Unlock) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());

  uv_sem_post(vmOne->lockResponseSem);
}

NAN_METHOD(VmOne::Request) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
  std::cout << "request " << (void *)vmOne->requestSem << std::endl;
  uv_sem_wait(vmOne->requestSem);
}

NAN_METHOD(VmOne::Respond) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
  std::cout << "respond " << (void *)vmOne->requestSem << std::endl;
  uv_sem_post(vmOne->requestSem);
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
