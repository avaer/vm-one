#include <v8.h>
#include <uv.h>
#include <nan.h>

#include <dlfcn.h>
#include <deque>
#include <map>
#include <mutex>

using namespace v8;
using namespace node;

#define JS_STR(...) Nan::New<v8::String>(__VA_ARGS__).ToLocalChecked()
#define JS_INT(val) Nan::New<v8::Integer>(val)
#define JS_NUM(val) Nan::New<v8::Number>(val)
#define JS_FLOAT(val) Nan::New<v8::Number>(val)
#define JS_BOOL(val) Nan::New<v8::Boolean>(val)

namespace vmone {

void Init(Handle<Object> exports);
void RunInMainThread(uv_async_t *handle);

uv_async_t async;
std::map<int, Nan::Persistent<Function>> asyncFns;
std::deque<std::pair<int, std::string>> asyncQueue;
std::mutex asyncMutex;

class VmOne : public ObjectWrap {
public:
  static Handle<Object> Initialize();
// protected:
  static NAN_METHOD(New);
  // static NAN_METHOD(GetGlobal);
  static NAN_METHOD(FromArray);
  static NAN_METHOD(ToArray);
  static NAN_METHOD(Dlclose);
  static NAN_METHOD(Request);
  static NAN_METHOD(Respond);
  // static NAN_METHOD(PushGlobal);
  static NAN_METHOD(PushResult);
  static NAN_METHOD(PopResult);
  static NAN_METHOD(QueueAsyncRequest);
  static NAN_METHOD(QueueAsyncResponse);

  VmOne(VmOne *ovmo = nullptr);
  ~VmOne();

// protected:
  std::string result;
  uv_sem_t *lockRequestSem;
  uv_sem_t *lockResponseSem;
  uv_sem_t *requestSem;
  VmOne *oldVmOne;
};

Handle<Object> VmOne::Initialize() {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("VmOne"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto, "toArray", ToArray);
  // Nan::SetMethod(proto, "getGlobal", GetGlobal);
  Nan::SetMethod(proto, "request", Request);
  Nan::SetMethod(proto, "respond", Respond);
  // Nan::SetMethod(proto, "pushGlobal", PushGlobal);
  Nan::SetMethod(proto, "pushResult", PushResult);
  Nan::SetMethod(proto, "popResult", PopResult);
  Nan::SetMethod(proto, "queueAsyncRequest", QueueAsyncRequest);
  Nan::SetMethod(proto, "queueAsyncResponse", QueueAsyncResponse);

  Local<Function> ctorFn = ctor->GetFunction();
  ctorFn->Set(JS_STR("fromArray"), Nan::New<Function>(FromArray));
  ctorFn->Set(JS_STR("dlclose"), Nan::New<Function>(Dlclose));

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

NAN_METHOD(VmOne::Dlclose) {
  if (info[0]->IsString()) {
    Local<String> soPathString = Local<String>::Cast(info[0]);
    String::Utf8Value soPathUtf8Value(soPathString);
    void *handle = dlopen(*soPathUtf8Value, RTLD_LAZY);

    if (handle) {
      while (dlclose(handle) == 0) {}
    } else {
      Nan::ThrowError("VmOne::Dlclose: failed to open handle to close");
    }
  } else {
    Nan::ThrowError("VmOne::Dlclose: invalid arguments");
  }
}

VmOne::VmOne(VmOne *ovmo) {
  if (!ovmo) {
    lockRequestSem = new uv_sem_t();
    uv_sem_init(lockRequestSem, 0);
    lockResponseSem = new uv_sem_t();
    uv_sem_init(lockResponseSem, 0);
    requestSem = new uv_sem_t();
    uv_sem_init(requestSem, 0);
  } else {
    Local<Context> localContext = Isolate::GetCurrent()->GetCurrentContext();

    localContext->AllowCodeGenerationFromStrings(true);
    // ContextEmbedderIndex::kAllowWasmCodeGeneration = 34
    localContext->SetEmbedderData(34, Nan::New<Boolean>(true));

    /* uv_async_t lol;
    uv_async_init(uv_default_loop(), &lol, RunInMainThread); */

    lockRequestSem = ovmo->lockRequestSem;
    lockResponseSem = ovmo->lockResponseSem;
    requestSem = ovmo->requestSem;
    oldVmOne = ovmo;
  }
}

VmOne::~VmOne() {
  if (!oldVmOne) {
    uv_sem_destroy(lockRequestSem);
    delete lockRequestSem;
    uv_sem_destroy(lockResponseSem);
    delete lockResponseSem;
    uv_sem_destroy(requestSem);
    delete requestSem;
  }
}

/* NAN_METHOD(VmOne::PushGlobal) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
  vmOne->oldVmOne->result.Reset(Isolate::GetCurrent()->GetCurrentContext()->Global());

  uv_sem_post(vmOne->lockRequestSem);
  uv_sem_wait(vmOne->lockResponseSem);

  vmOne->oldVmOne->result.Reset();
} */

NAN_METHOD(VmOne::PushResult) {
  if (info[0]->IsString()) {
    VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
    Local<String> stringValue = Local<String>::Cast(info[0]);
    String::Utf8Value utf8Value(stringValue);
    vmOne->oldVmOne->result = std::string(*utf8Value, utf8Value.length());

    uv_sem_post(vmOne->lockRequestSem);
    uv_sem_wait(vmOne->lockResponseSem);

    vmOne->oldVmOne->result.clear();
  } else {
    Nan::ThrowError("VmOne::PushResult: invalid arguments");
  }
}

NAN_METHOD(VmOne::PopResult) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());

  uv_sem_wait(vmOne->lockRequestSem);
  Local<String> result = JS_STR(vmOne->result);
  uv_sem_post(vmOne->lockResponseSem);

  info.GetReturnValue().Set(result);
}

NAN_METHOD(VmOne::QueueAsyncRequest) {
  if (info[0]->IsFunction()) {
    Local<Function> localFn = Local<Function>::Cast(info[0]);

    int requestKey = rand();
    {
      std::lock_guard<std::mutex> lock(asyncMutex);

      asyncFns.emplace(requestKey, localFn);
    }

    info.GetReturnValue().Set(JS_INT(requestKey));
  } else {
    Nan::ThrowError("VmOne::QueueAsyncRequest: invalid arguments");
  }
}

NAN_METHOD(VmOne::QueueAsyncResponse) {
  if (info[0]->IsNumber() && info[1]->IsString()) {
    int requestKey = info[0]->Int32Value();
    String::Utf8Value utf8Value(info[1]);

    {
      std::lock_guard<std::mutex> lock(asyncMutex);

      asyncQueue.emplace_back(requestKey, std::string(*utf8Value, utf8Value.length()));
    }

    uv_async_send(&async);
  } else {
    Nan::ThrowError("VmOne::QueueAsyncResponse: invalid arguments");
  }
}

NAN_METHOD(nop) {}
void RunInMainThread(uv_async_t *handle) {
  Nan::HandleScope scope;

  {
    std::lock_guard<std::mutex> lock(asyncMutex);

    for (auto iter = asyncQueue.begin(); iter != asyncQueue.end(); iter++) {
      const int &requestKey = iter->first;
      const std::string &requestResult = iter->second;

      {
        Nan::HandleScope scope;

        Local<Object> asyncObj = Nan::New<Object>();
        AsyncResource asyncResource(Isolate::GetCurrent(), asyncObj, "VmOne::RunInMainThread");

        Nan::Persistent<Function> &fn = asyncFns[requestKey];
        Local<Function> localFn = Nan::New(fn);

        Local<Value> argv[] = {
          JS_STR(requestResult),
        };
        asyncResource.MakeCallback(localFn, sizeof(argv)/sizeof(argv[0]), argv);
      }

      asyncFns.erase(requestKey);
    }
    asyncQueue.clear();
  }
}

/* NAN_METHOD(VmOne::GetGlobal) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
  Local<Function> cb = Local<Function>::Cast(info[0]);

  {
    Nan::HandleScope scope;

    Local<Function> postMessageFn = Local<Function>::Cast(info.This()->Get(JS_STR("postMessage")));
    Local<Object> messageObj = Nan::New<Object>();
    messageObj->Set(JS_STR("method"), JS_STR("lock"));
    Local<Value> argv[] = {
      messageObj,
    };
    postMessageFn->Call(Nan::Null(), sizeof(argv)/sizeof(argv[0]), argv);
  }

  uv_sem_wait(vmOne->lockRequestSem);

  {
    Nan::HandleScope scope;

    Local<Value> argv[] = {
      Nan::New(vmOne->result),
    };
    cb->Call(Nan::Null(), sizeof(argv)/sizeof(argv[0]), argv);
  }

  uv_sem_post(vmOne->lockResponseSem);
} */

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

void RootInit(Handle<Object> exports) {
  uv_async_init(uv_default_loop(), &async, RunInMainThread);

  Init(exports);
}

}

#ifndef LUMIN
NODE_MODULE(NODE_GYP_MODULE_NAME, vmone::RootInit)
#else
extern "C" {
  void node_register_module_vm_one(Local<Object> exports, Local<Value> module, Local<Context> context) {
    vmone::RootInit(exports);
  }
}
#endif
