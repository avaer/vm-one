const fs = require('fs');
fs.tainted = true;

const vmOne = require('.');

const v = vmOne.make();
/* const g = v.getGlobal();
console.log('got global', g);
g.lol = 'zol';
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
