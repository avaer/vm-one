const path = require('path');
const {Worker} = require('worker_threads');
const {VmOne: nativeVmOne} = require('./build/Release/vm_one.node');

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

    const worker = new Worker(path.join(__dirname, 'boot.js'), {
      workerData: {
        initFnAddress: nativeVmOne.initFnAddress,
        array: vmOne.toArray(),
      },
    });

    console.log('request 1');
    vmOne.request();
    console.log('request 2');

    vmOne.runSync = code => {
      worker.postMessage({
        code,
        request: true,
      });
      vmOne.request();
    };
    vmOne.runAsync = code => {
      worker.postMessage({
        code,
        request: false,
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
