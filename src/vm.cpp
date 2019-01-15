#include "vm.h"

namespace vmone {

Handle<Object> VmOne::Initialize() {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("VmOne"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto, "run", Run);
  Nan::SetMethod(proto, "getGlobal", GetGlobal);

  Local<Function> ctorFn = ctor->GetFunction();

  return scope.Escape(ctorFn);
}

NAN_METHOD(VmOne::New) {
  if (/*info[0]->IsObject() && */info[0]->IsFunction() && info[1]->IsString()) {
    // Local<Object> global = Local<Object>::Cast(info[0]);
    Local<Function> handler = Local<Function>::Cast(info[0]);
    Local<String> dirname = Local<String>::Cast(info[1]);

    Local<Object> vmOneObj = Local<Object>::Cast(info.This());

    VmOne *vmOne = new VmOne(/* global, */handler, dirname);
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
    Local<Function> handler;
    if (!vmOne->handler.IsEmpty()) {
      handler = Nan::New(vmOne->handler);
    }
    Local<Value> exception;

    {
      Context::Scope scope(localContext);
      Nan::TryCatch tryCatch;

      if (!handler.IsEmpty()) {
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

      if (!handler.IsEmpty()) {
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
          exception = tryCatch.Exception();
        }
      } else {
        exception = tryCatch.Exception();
      }
    }
    if (!exception.IsEmpty()) {
      Nan::ThrowError(exception);
    }
  } else {
    Nan::ThrowError("invalid arguments");
  }
}

NAN_METHOD(VmOne::GetGlobal) {
  VmOne *vmOne = ObjectWrap::Unwrap<VmOne>(info.This());
  Local<Context> localContext = Nan::New(vmOne->context);
  Local<Object> global = localContext->Global();

  info.GetReturnValue().Set(global);
}

/* void copyObject(Local<Object> src, Local<Object> dst, Local<Context> context) {
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
} */

VmOne::VmOne(/* Local<Object> globalInit, */Local<Function> handler, Local<String> dirname) {
  // create context
  Isolate *isolate = Isolate::GetCurrent();
  Local<Context> localContext = Context::New(isolate);
  Local<Object> contextGlobal = localContext->Global();

  Local<Context> topContext = Isolate::GetCurrent()->GetCurrentContext();
  localContext->SetSecurityToken(topContext->GetSecurityToken());
  localContext->AllowCodeGenerationFromStrings(true);
  // ContextEmbedderIndex::kAllowWasmCodeGeneration = 34
  localContext->SetEmbedderData(34, Nan::New<Boolean>(true));

  // flag hack
#if _WIN32
  HMODULE allowNativesSyntaxHandle = GetModuleHandle(nullptr);
  FARPROC allowNativesSyntaxAddress = GetProcAddress(allowNativesSyntaxHandle, "?FLAG_allow_natives_syntax@internal@v8@@3_NA");
#else
  void *allowNativesSyntaxHandle = dlopen(NULL, RTLD_LAZY);
  void *allowNativesSyntaxAddress = dlsym(allowNativesSyntaxHandle, "_ZN2v88internal25FLAG_allow_natives_syntaxE");
#endif
  bool *flag = (bool *)allowNativesSyntaxAddress;
  *flag = true;

  // create new environment
#if _WIN32
  HMODULE getCurrentPlatformHandle = GetModuleHandle(nullptr);
  FARPROC getCurrentPlatformAddress = GetProcAddress(getCurrentPlatformHandle, "?GetCurrentPlatform@V8@internal@v8@@SAPEAVPlatform@3@XZ");
#else
  void *getCurrentPlatformHandle = dlopen(NULL, RTLD_LAZY);
  void *getCurrentPlatformAddress = dlsym(getCurrentPlatformHandle, "_ZN2v88internal2V818GetCurrentPlatformEv");
#endif
  Platform *(*GetCurrentPlatform)(void) = (Platform *(*)(void))getCurrentPlatformAddress;
  MultiIsolatePlatform *platform = (MultiIsolatePlatform *)GetCurrentPlatform();
  IsolateData *isolate_data = CreateIsolateData(isolate, uv_default_loop(), platform);

  int i = 0;

  char *binPathArg = argsString + i;
  const char *binPathString = "node";
  strncpy(binPathArg, binPathString, sizeof(argsString) - i);
  i += strlen(binPathString) + 1;

  char *jsPathArg = argsString + i;
  String::Utf8Value dirnameValue(dirname);
  std::string dirnameString(*dirnameValue, dirnameValue.length());
  strncpy(jsPathArg, dirnameString.c_str(), sizeof(argsString) - i);
  i += dirnameString.length();

  char *jsPathArg2 = argsString + i;
  const char *jsFilePath = "boot.js";
  strncpy(jsPathArg2, jsFilePath, sizeof(argsString) - i);
  i += sizeof(jsFilePath);

  char *allowNativesSynax = argsString + i;
  strncpy(allowNativesSynax, "--allow_natives_syntax", sizeof(argsString) - i);
  i += strlen(allowNativesSynax) + 1;

  int argc = sizeof(argv)/sizeof(argv[0]);
  argv[0] = binPathArg;
  argv[1] = jsPathArg;
  argv[2] = allowNativesSynax;

  env = CreateEnvironment(isolate_data, localContext, argc, argv, argc, argv);
  LoadEnvironment(env);

  // copyObject(globalInit, contextGlobal, localContext);

  /* // ContextEmbedderIndex::kEnvironment = 32
  Environment *env = (Environment *)topContext->GetAlignedPointerFromEmbedderData(32);
  localContext->SetAlignedPointerInEmbedderData(32, env); */

  if (!handler.IsEmpty()) {
    this->handler.Reset(handler);
  }
  context.Reset(localContext);
}
VmOne::~VmOne() {
  FreeEnvironment(env);
}

}
