const fs = require('fs');
fs.tainted = true;

const vmOne = require('.');

const v = vmOne.make();
setTimeout(() => {
  // v.lock();
  console.log('timeout 1');
  console.log('timeout 2');
  const g = v.getGlobal(g => {
    console.log('got global', Object.keys(g));
  });
  console.log('timeout 3');
  // console.log('got global 2');
}, 1000);
/* g.lol = 'zol';
g.callback = object => {
  console.log('check 3', object, !(object instanceof Object));
};

const result = v.run(`
  const fs = require('fs');
  console.log('check 0', lol === 'zol');
  console.log('check 1', fs.tainted === undefined);
  console.log('check 2', fs.readFileSync('./boot.js').buffer instanceof ArrayBuffer);
  callback({});
`);
*/
