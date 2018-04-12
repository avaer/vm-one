#include <v8.h>
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
  protected:
    static NAN_METHOD(New);
    static NAN_METHOD(Run);

    VmOne(Local<Object> globalInit, Local<Function> handler);
    ~VmOne();

    Nan::Persistent<Context> context;
    Nan::Persistent<Function> handler;
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
  if (info[0]->IsObject() && info[1]->IsFunction()) {
    Local<Object> global = Local<Object>::Cast(info[0]);
    Local<Function> handler = Local<Function>::Cast(info[1]);

    Local<Object> vmOneObj = Local<Object>::Cast(info.This());

    VmOne *vmOne = new VmOne(global, handler);
    vmOne->Wrap(vmOneObj);

    info.GetReturnValue().Set(vmOneObj);
  } else {
    Nan::ThrowError("invalid arguments");
  }
}

NAN_METHOD(VmOne::Run) {
  if (info[0]->IsString()) {
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
    Local<Context> localContext = Nan::New(vmOne->context);
    Local<Function> handler = Nan::New(vmOne->handler);

    {
      Context::Scope scope(localContext);
      Nan::TryCatch tryCatch;

      {
        Local<Value> argv[] = {
          JS_STR("compilestart"),
        };
        handler->Call(Nan::Null(), sizeof(argv)/sizeof(argv[0]), argv);
      }

      ScriptOrigin scriptOrigin(
        resourceName,
        lineOffset,
        colOffset
      );
      MaybeLocal<Script> scriptMaybe = Script::Compile(localContext, src, &scriptOrigin);

      {
        Local<Value> argv[] = {
          JS_STR("compileend"),
        };
        handler->Call(Nan::Null(), sizeof(argv)/sizeof(argv[0]), argv);
      }

      if (!scriptMaybe.IsEmpty()) {
        Local<Script> script = scriptMaybe.ToLocalChecked();
        MaybeLocal<Value> result = script->Run(localContext);

        if (!tryCatch.HasCaught()) {
          info.GetReturnValue().Set(result.ToLocalChecked());
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

void copyObject(Local<Object> src, Local<Object> dst, Local<Context> context) {
  Local<Array> propertyNames = src->GetPropertyNames(
    context,
    KeyCollectionMode::kOwnOnly,
    PropertyFilter::ALL_PROPERTIES,
    IndexFilter::kIncludeIndices
  ).ToLocalChecked();
  size_t numPropertyNames = propertyNames->Length();
  for (size_t i = 0; i < numPropertyNames; i++) {
    Local<Value> key = propertyNames->Get(i);

    MaybeLocal<Value> srcDescriptorMaybe = key->IsName() ? src->GetOwnPropertyDescriptor(context, Local<Name>::Cast(key)) : MaybeLocal<Value>();

    if (!srcDescriptorMaybe.IsEmpty()) {
      Local<Object> srcDescriptor = Local<Object>::Cast(srcDescriptorMaybe.ToLocalChecked());

      Local<Value> value = srcDescriptor->Get(JS_STR("value"));

      if (value->StrictEquals(src)) {
        value = dst;
      }

      Local<Value> get = srcDescriptor->Get(JS_STR("get"));
      Local<Value> set = srcDescriptor->Get(JS_STR("set"));
      Local<Value> enumerable = srcDescriptor->Get(JS_STR("enumerable"));
      Local<Value> configurable = srcDescriptor->Get(JS_STR("configurable"));
      Local<Value> writable = srcDescriptor->Get(JS_STR("writable"));

      if (!value->IsUndefined()) {
        if (writable->IsBoolean()) {
          PropertyDescriptor dstDescriptor(value, writable->BooleanValue());

          if (enumerable->IsBoolean()) {
            dstDescriptor.set_enumerable(enumerable->BooleanValue());
          }
          if (configurable->IsBoolean()) {
            dstDescriptor.set_configurable(configurable->BooleanValue());
          }

          dst->DefineProperty(context, Local<Name>::Cast(key), dstDescriptor);
        } else {
          PropertyDescriptor dstDescriptor(value);

          if (enumerable->IsBoolean()) {
            dstDescriptor.set_enumerable(enumerable->BooleanValue());
          }
          if (configurable->IsBoolean()) {
            dstDescriptor.set_configurable(configurable->BooleanValue());
          }

          dst->DefineProperty(context, Local<Name>::Cast(key), dstDescriptor);
        }
      } else if (!get->IsUndefined() || !set->IsUndefined()) {
        PropertyDescriptor dstDescriptor(get, set);

        if (enumerable->IsBoolean()) {
          dstDescriptor.set_enumerable(enumerable->BooleanValue());
        }
        if (configurable->IsBoolean()) {
          dstDescriptor.set_configurable(configurable->BooleanValue());
        }

        dst->DefineProperty(context, Local<Name>::Cast(key), dstDescriptor);
      } else {
        PropertyDescriptor dstDescriptor;

        if (enumerable->IsBoolean()) {
          dstDescriptor.set_enumerable(enumerable->BooleanValue());
        }
        if (configurable->IsBoolean()) {
          dstDescriptor.set_configurable(configurable->BooleanValue());
        }

        dst->DefineProperty(context, Local<Name>::Cast(key), dstDescriptor);
      }
    } else {
      Local<Value> value = src->Get(key);

      if (value->StrictEquals(src)) {
        value = dst;
      }

      dst->Set(context, key, value);
    }
  }
}

VmOne::VmOne(Local<Object> globalInit, Local<Function> handler) {
  this->handler.Reset(handler);

  Local<Context> localContext = Context::New(Isolate::GetCurrent());
  Local<Object> contextGlobal = localContext->Global();

  copyObject(globalInit, contextGlobal, localContext);

  Local<Context> topContext = Isolate::GetCurrent()->GetCurrentContext();
  localContext->SetSecurityToken(topContext->GetSecurityToken());
  localContext->AllowCodeGenerationFromStrings(true);
  // ContextEmbedderIndex::kAllowWasmCodeGeneration = 34
  localContext->SetEmbedderData(34, Nan::New<Boolean>(true));
  // ContextEmbedderIndex::kEnvironment = 32
  Environment *env = (Environment *)topContext->GetAlignedPointerFromEmbedderData(32);
  localContext->SetAlignedPointerInEmbedderData(32, env);

  context.Reset(localContext);
}
VmOne::~VmOne() {}

void Init(Handle<Object> exports) {
  exports->Set(JS_STR("VmOne"), VmOne::Initialize());
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)

}
