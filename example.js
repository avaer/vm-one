const vmOne = require('.');

(() => {
  const v = vmOne.make();
  console.log('example 1');
  v.getGlobal(g => {
    console.log('example 2', Object.keys(g));
  });
  console.log('example 3');
})();

(() => {
  const v = vmOne.make();
  console.log('example 4');
  v.getGlobal(g => {
    console.log('example 5', Object.keys(g));
  });
  console.log('example 6');
})();
