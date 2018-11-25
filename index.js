const path = require('path');
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

const vmOne = {
  make(options = {}) {
    const vmOne = new nativeVmOne();

    const worker = new Worker(childJsPath, {
      workerData: {
        initFnAddress: nativeVmOne.initFnAddress,
        array: vmOne.toArray(),
        initModule: options.initModule,
      },
    });
    worker.on('error', err => {
      console.warn(err.stack);
    });
    vmOne.request();
    nativeVmOne.dlclose(vmOne2SoPath); // so we can re-require the module from a different child

    vmOne.runSync = (jsString, arg, transferList) => {
      worker.postMessage({
        method: 'runSync',
        jsString,
        arg,
      }, transferList);
      return JSON.parse(vmOne.popResult());
    };
    vmOne.runAsync = (jsString, arg, transferList) => {
      let accept = null;
      let done = false;
      let result;
      const requestKey = vmOne.queueAsyncRequest(r => {
        if (accept) {
          accept(r);
        } else {
          done = true;
          result = r;
        }
      });
      worker.postMessage({
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
    };
    vmOne.postMessage = (m, transferList) => worker.postMessage(m, transferList);

    return vmOne;
  },
  fromArray(arg) {
    return new VmOne(arg);
  },
}

module.exports = vmOne;
