#include <v8.h>
#include <nan.h>

#if _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

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
    static NAN_METHOD(GetGlobal);

    VmOne(/* Local<Object> globalInit, */Local<Function> handler, Local<String> dirname);
    ~VmOne();

    Environment *env;
    char argsString[4096];
    char *argv[3];
    Nan::Persistent<Context> context;
    Nan::Persistent<Function> handler;
};

}
