const path = require('path');
const {workerData, parentPort} = require('worker_threads');

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
  switch (m.method) {
    case 'lock': {
      vmOne.handleRunInThread();
      break;
    }
    case 'runSync': {
      try {
        eval(m.jsString);
      } catch(err) {
        console.warn(err.stack);
      }
      vmOne.respond();
      break;
    }
    case 'runAsync': {
      try {
        eval(m.jsString);
      } catch(err) {
        console.warn(err.stack);
      }
      vmOne.queueAsyncResponse(m.requestKey);
      break;
    }
    default: throw new Error(`invalid method: ${JSON.stringify(m.method)}`);
  }
});

/* setInterval(() => {
  console.log('child interval');
}, 200); */
