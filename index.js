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
  make() {
    const vmOne = new nativeVmOne();

    const worker = new Worker(childJsPath, {
      workerData: {
        initFnAddress: nativeVmOne.initFnAddress,
        array: vmOne.toArray(),
      },
    });
    worker.on('error', err => {
      console.warn(err.stack);
    });
    vmOne.request();
    nativeVmOne.dlclose(vmOne2SoPath); // so we can re-require the module from a different child

    vmOne.runSync = jsString => {
      console.log('run sync 1');
      worker.postMessage({
        method: 'runSync',
        jsString,
      });
      console.log('run sync 2');
      return JSON.parse(vmOne.popResult());
    };
    vmOne.runAsync = jsString => {
      let accept = null;
      let done = false;
      let result;
      console.log('run async 1');
      const requestKey = vmOne.queueAsyncRequest(r => {
        console.log('run async 2', !!accept, !!done, r);
        if (accept) {
          accept(r);
        } else {
          done = true;
          result = r;
        }
      });
      console.log('run async 3', requestKey);
      worker.postMessage({
        method: 'runAsync',
        jsString,
        requestKey,
      });
      console.log('run async 4');
      return new Promise((accept2, reject2) => {
        console.log('run async 5', !!accept, !!done, result);
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
