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
    vmOne.request();
    nativeVmOne.dlclose(vmOne2SoPath); // so we can re-require the module from a different child

    vmOne.runSync = jsString => {
      worker.postMessage({
        method: 'runSync',
        jsString,
      });
      vmOne.request();
    };
    vmOne.runAsync = jsString => {
      let accept = null;
      let done = false;
      console.log('run async 1');
      const requestKey = vmOne.queueAsyncRequest(() => {
        console.log('run async 2', !!accept, !!done);
        if (accept) {
          accept();
        } else {
          done = true;
        }
      });
      console.log('run async 3', requestKey);
      worker.postMessage({
        method: 'runAsync',
        jsString,
        requestKey,
      });
      console.log('run async 4');
      return new Promise((a, r) => {
        console.log('run async 5', !!accept, !!done);
        if (done) {
          a();
        } else {
          accept = a;
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
