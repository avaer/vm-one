const path = require('path');
const {EventEmitter} = require('events');
const {Worker} = require('worker_threads');
const {VmOne: nativeVmOne} = require(path.join(__dirname, 'build', 'Release', 'vm_one.node'));
const vmOne2SoPath = require.resolve(path.join(__dirname, 'build', 'Release', 'vm_one2.node'));
const childJsPath = path.join(__dirname, 'child.js');

/* let compiling = false;
const make = () => new VmOne(e => {
  if (e === 'compilestart') {
    compiling = true;
  } else if (e === 'compileend') {
    compiling = false;
  }
}, __dirname + path.sep);
const isCompiling = () => compiling; */

class Vm extends EventEmitter {
  constructor(options = {}) {
    super();

    const instance = new nativeVmOne();

    const worker = new Worker(childJsPath, {
      workerData: {
        initFunctionAddress: nativeVmOne.initFunctionAddress,
        array: instance.toArray(),
        initModule: options.initModule,
        args: options.args,
      },
    });
    worker.on('message', m => {
      this.emit('message', m);
    });
    worker.on('error', err => {
      console.warn(err.stack);
    });
    instance.request();
    nativeVmOne.dlclose(vmOne2SoPath); // so we can re-require the module from a different child

    this.instance = instance;
    this.worker = worker;
  }

  runSync(jsString, arg, transferList) {
    this.worker.postMessage({
      method: 'runSync',
      jsString,
      arg,
    }, transferList);
    return JSON.parse(this.instance.popResult());
  }
  runAsync(jsString, arg, transferList) {
    let accept = null;
    let done = false;
    let result;
    const requestKey = this.instance.queueAsyncRequest(r => {
      if (accept) {
        accept(r);
      } else {
        done = true;
        result = r;
      }
    });
    this.worker.postMessage({
      method: 'runAsync',
      jsString,
      arg,
      requestKey,
    }, transferList);
    return new Promise((accept2, reject2) => {
      if (done) {
        accept2(result);
      } else {
        accept = accept2;
      }
    });
  }
  postMessage(message, transferList) {
    this.worker.postMessage({
      method: 'postMessage',
      message,
    }, transferList);
  }
}

const vmOne = {
  make(options = {}) {
    return new Vm(options);
  },
  fromArray(arg) {
    return new VmOne(arg);
  },
}

module.exports = vmOne;
