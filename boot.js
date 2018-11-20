const {workerData, parentPort} = require('worker_threads');
console.log('boot 1');
const nativeVmOne = require('./build/Release/vm_one.node');

console.log('boot 2');

(() => {
  const vmOne = nativeVmOne.fromAddress(workerData.address);
  vmOne.setGlobal(global);
  vmOne.respond();
  parentPort.on('message', m => {
    try {
      eval(m.code);
    } catch(err) {
      console.warn(err.stack);
    }
    if (m.request) {
      vmOne.respond();
    }
  });
})();

// global.require = require;
