const http = require('http');
const https = require('https');
const {workerData} = require('worker_threads');

const {url, int32Array} = workerData;
const {buffer: sab} = int32Array;
const lengthArray = new Uint32Array(sab, Int32Array.BYTES_PER_ELEMENT, 1);
const resultArray = new Uint8Array(sab, Int32Array.BYTES_PER_ELEMENT*2);

(async () => {
  const res = await new Promise((accept, reject) => {
    const req = (/^https:/.test(url) ? https : http).request(url);
    req.on('response', res => {
      const bs = [];
      res.on('data', d => {
        bs.push(d);
      });
      res.on('end', () => {
        accept(Buffer.concat(bs));
      });
      res.on('error', reject);
    });
    req.on('error', reject);
    req.end();
  });
  if (res.status >= 200 && res.status < 300) {
    return await res.text();
  } else {
    throw new Error('request got invalid status code: ' + res.status);
  }
})()
  .then(result => {
    const s = result + '';
    const b = Buffer.from(s, 'utf8');
    lengthArray[0] = b.byteLength;
    resultArray.set(b);
    Atomics.store(int32Array, 0, 1);
  })
  .catch(err => {
    const s = err.stack || (err + '');
    const b = Buffer.from(s, 'utf8');
    lengthArray[0] = b.byteLength;
    resultArray.set(b);
    Atomics.store(int32Array, 0, 2);
  })
  .finally(() => {
    Atomics.notify(int32Array, 0);
  });
