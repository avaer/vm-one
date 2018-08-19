const path = require('path');
const {VmOne, setPrototype} = require('./build/Release/vm_one.node');

let compiling = false;
const make = (globalInit = {}) => new VmOne(globalInit, e => {
  if (e === 'compilestart') {
    compiling = true;
  } else if (e === 'compileend') {
    compiling = false;
  }
}, __dirname + path.sep);
const isCompiling = () => compiling;

const vmOne = {
  make,
  isCompiling,
  setPrototype,
}

module.exports = vmOne;
