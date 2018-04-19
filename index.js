const {VmOne} = require('./build/Release/vmOne.node');

let compiling = false;
const make = (globalInit = {}) => new VmOne(globalInit, e => {
  if (e === 'compilestart') {
    compiling = true;
  } else if (e === 'compileend') {
    compiling = false;
  }
});
const isCompiling = () => compiling;

const vmOne = {
  make,
  isCompiling,
}

module.exports = vmOne;
