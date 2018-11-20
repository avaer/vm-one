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
  static NAN_METHOD(HandleRunInThread);

  VmOne(VmOne *ovmo = nullptr);
  ~VmOne();

  static void RunInThread(uv_async_t *handle);

// protected:
  Nan::Persistent<Value> global;
  Nan::Persistent<Value> securityToken;
  uv_async_t *async;
  uv_sem_t *lockRequestSem;
  uv_sem_t *lockResponseSem;
  uv_sem_t *requestSem;
  VmOne *oldVmOne;
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
  Nan::SetMethod(proto, "toArray", ToArray);
  Nan::SetMethod(proto, "lock", Lock);
  Nan::SetMethod(proto, "unlock", Unlock);
  Nan::SetMethod(proto, "setGlobal", SetGlobal);
  Nan::SetMethod(proto, "getGlobal", GetGlobal);
  Nan::SetMethod(proto, "request", Request);
  Nan::SetMethod(proto, "respond", Respond);
  Nan::SetMethod(proto, "handleRunInThread", HandleRunInThread);

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
    Local<Array> array = Local<Array>::Cast(info[0]);
    uint32_t a = array->Get(0)->Uint32Value();
    uint32_t b = array->Get(1)->Uint32Value();
    uintptr_t c = ((uintptr_t)a << 32) | (uintptr_t)b;
    oldVmOne = reinterpret_cast<VmOne *>(c);
  } else {
    oldVmOne = nullptr;
  }

  VmOne *vmOne = oldVmOne ? new VmOne(oldVmOne) : new VmOne();
  vmOne->Wrap(vmOneObj);

  if (!oldVmOne) {
    threadVmOne = vmOne; // XXX check for duplicates
  }

  info.GetReturnValue().Set(vmOneObj);
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

VmOne::VmOne(VmOne *ovmo) {
  if (!ovmo) {
    securityToken.Reset(Isolate::GetCurrent()->GetCurrentContext()->GetSecurityToken());

    async = nullptr;
    lockRequestSem = new uv_sem_t();
    uv_sem_init(lockRequestSem, 0);
    lockResponseSem = new uv_sem_t();
    uv_sem_init(lockResponseSem, 0);
    requestSem = new uv_sem_t();
    uv_sem_init(requestSem, 0);
  } else {
    Local<Value> oldSecurityToken = Nan::New(ovmo->securityToken);
    Local<Context> localContext = Isolate::GetCurrent()->GetCurrentContext();

    localContext->SetSecurityToken(oldSecurityToken);
    localContext->AllowCodeGenerationFromStrings(true);
    // ContextEmbedderIndex::kAllowWasmCodeGeneration = 34
    localContext->SetEmbedderData(34, Nan::New<Boolean>(true));

    async = ovmo->async = new uv_async_t();
    uv_async_init(uv_default_loop(), async, RunInThread);
    lockRequestSem = ovmo->lockRequestSem;
    lockResponseSem = ovmo->lockResponseSem;
    requestSem = ovmo->requestSem;
    oldVmOne = ovmo;
  }
}

VmOne::~VmOne() {
  if (!oldVmOne) {
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

NAN_METHOD(VmOne::HandleRunInThread) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
  vmOne->oldVmOne->global.Reset(Isolate::GetCurrent()->GetCurrentContext()->Global());
  uv_sem_post(vmOne->lockRequestSem);

  uv_sem_wait(vmOne->lockResponseSem);

  vmOne->oldVmOne->global.Reset();
}

void VmOne::RunInThread(uv_async_t *handle) {
  Nan::HandleScope scope;

  VmOne *vmOne = threadVmOne;
  vmOne->oldVmOne->global.Reset(Isolate::GetCurrent()->GetCurrentContext()->Global());
  uv_sem_post(threadVmOne->lockRequestSem);

  uv_sem_wait(threadVmOne->lockResponseSem);

  vmOne->oldVmOne->global.Reset();
}

NAN_METHOD(VmOne::Lock) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());

  // uv_async_send(vmOne->async);
  // uv_sem_wait(vmOne->lockRequestSem);
}

NAN_METHOD(VmOne::Unlock) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());

  // uv_sem_post(vmOne->lockResponseSem);
}

NAN_METHOD(VmOne::SetGlobal) {
  // VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
  // vmOne->global.Reset(Isolate::GetCurrent()->GetCurrentContext()->Global());
}

NAN_METHOD(VmOne::GetGlobal) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
  Local<Function> cb = Local<Function>::Cast(info[0]);

  {
    Nan::HandleScope scope;

    Local<Function> postMessageFn = Local<Function>::Cast(info.This()->Get(JS_STR("postMessage")));
    postMessageFn->Call(Nan::Null(), 0, nullptr);
  }

  // uv_async_send(vmOne->async);
  uv_sem_wait(vmOne->lockRequestSem);

  {
    Nan::HandleScope scope;

    Local<Value> argv[] = {
      Nan::New(vmOne->global),
    };
    cb->Call(Nan::Null(), sizeof(argv)/sizeof(argv[0]), argv);
  }

  uv_sem_post(vmOne->lockResponseSem);
}

NAN_METHOD(VmOne::Request) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
  uv_sem_wait(vmOne->requestSem);
}

NAN_METHOD(VmOne::Respond) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
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
