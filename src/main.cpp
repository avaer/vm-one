#include "vm.h"

namespace vmone {

NAN_METHOD(SetPrototype) {
  Local<Object> arg = Local<Object>::Cast(info[0]);
  Local<Value> prototype = info[1];

  arg->SetPrototype(Isolate::GetCurrent()->GetCurrentContext(), prototype);
}

void Init(Local<Object> exports) {
  exports->Set(JS_STR("VmOne"), VmOne::Initialize());
  Nan::SetMethod(exports, "setPrototype", SetPrototype);
}

}

#if !defined(ANDROID) && !defined(LUMIN)
NODE_MODULE(NODE_GYP_MODULE_NAME, vmone::Init)
#else
extern "C" {
  void node_register_module_vm_one(Local<Object> exports, Local<Value> module, Local<Context> context) {
    vmone::Init(exports);
  }
}
#endif
