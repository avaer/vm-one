const fs = require('fs');

const vmOne = require('.');

const v1 = vmOne.make();
console.log('example 1');
v1.getGlobal(g => {
  console.log('example 2', Object.keys(g));
});
console.log('example 3');

/* const v2 = vmOne.make();
console.log('timeout 2');
v2.getGlobal(g => {
  console.log('got global', Object.keys(g));
}); */
