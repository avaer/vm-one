const {workerData, parentPort} = require('worker_threads');
const childVmOne = require('./build/Release/vm_one2.node');

const nativeVmOne = (() => {
  const exports = {};
  childVmOne.initChild(workerData.initFnAddress, exports);
  return exports.VmOne;
})();

console.log('from array', workerData.array);
const vmOne = nativeVmOne.fromArray(workerData.array);
console.log('boot 1');
// vmOne.setGlobal();
console.log('boot 2');
vmOne.respond();
console.log('boot 3');
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

console.log('boot 4');

setInterval(() => {
  console.log('child interval');
}, 200);

// global.require = require;
