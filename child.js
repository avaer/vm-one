const path = require('path');
const {workerData, parentPort} = require('worker_threads');

// console.log('child require cache', Object.keys(require.cache));

const nativeVmOne = (() => {
  const exports = {};

  const childVmOneSoPath = require.resolve(path.join(__dirname, 'build', 'Release', 'vm_one2.node'));
  const childVmOne = require(childVmOneSoPath);
  childVmOne.initChild(workerData.initFnAddress, exports);
  delete require.cache[childVmOneSoPath]; // cannot be reused

  return exports.VmOne;
})();

const vmOne = nativeVmOne.fromArray(workerData.array);
vmOne.respond();
parentPort.on('message', m => {
  vmOne.handleRunInThread();
});

/* setInterval(() => {
  console.log('child interval');
}, 200); */
