const {workerData, parentPort} = require('worker_threads');
const childVmOne = require('./build/Release/vm_one2.node');

const nativeVmOne = (() => {
  const exports = {};
  childVmOne.initChild(workerData.initFnAddress, exports);
  return exports.VmOne;
})();

const vmOne = nativeVmOne.fromArray(workerData.array);
vmOne.respond();
parentPort.on('message', m => {
  vmOne.handleRunInThread();
});

console.log('boot 4');

setInterval(() => {
  console.log('child interval');
}, 200);
