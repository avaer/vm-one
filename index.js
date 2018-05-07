const path = require('path');
const {VmOne} = require('./build/Release/vmOne.node');

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
}

module.exports = vmOne;
