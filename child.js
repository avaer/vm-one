const {EventEmitter} = require('events');
const path = require('path');
const {workerData, parentPort} = require('worker_threads');

// latch parent VmOne

const vmOne = (() => {
  const exports = {};
  const childVmOneSoPath = require.resolve(path.join(__dirname, 'build', 'Release', 'vm_one2.node'));
  const childVmOne = require(childVmOneSoPath);
  childVmOne.initChild(workerData.initFnAddress, exports);
  delete require.cache[childVmOneSoPath]; // cannot be reused

  const nativeVmOne = exports.VmOne;
  const vmOne = nativeVmOne.fromArray(workerData.array);
  return vmOne;
})();

// global initialization

for (const k in EventEmitter.prototype) {
  global[k] = EventEmitter.prototype[k];
}
EventEmitter.call(global);

global.postMessage = (m, transferList) => parentPort.postMessage(m, transferList);

parentPort.on('message', m => {
  switch (m.method) {
    /* case 'lock': {
      vmOne.pushResult(global);
      break;
    } */
    case 'runSync': {
      let result;
      try {
        const fn = eval(`(function(arg) { ${m.jsString} })`);
        const resultValue = fn(m.arg);
        result = JSON.stringify(resultValue !== undefined ? resultValue : null);
      } catch(err) {
        console.warn(err.stack);
      }
      console.log('push', result);
      vmOne.pushResult(result);
      break;
    }
    case 'runAsync': {
      let result;
      try {
        const fn = eval(`(function(arg) { ${m.jsString} })`);
        const resultValue = fn(m.arg);
        result = JSON.stringify(resultValue !== undefined ? resultValue : null);
      } catch(err) {
        console.warn(err.stack);
      }
      vmOne.queueAsyncResponse(m.requestKey, result);
      break;
    }
    case 'postMessage': {
      try {
        global.emit('message', m.message);
      } catch(err) {
        console.warn(err.stack);
      }
      break;
    }
    default: throw new Error(`invalid method: ${JSON.stringify(m.method)}`);
  }
});

// run init module

if (workerData.initModule) {
  require(workerData.initModule);
}

// release lock

vmOne.respond();

/* setInterval(() => {
  console.log('child interval');
}, 200); */
