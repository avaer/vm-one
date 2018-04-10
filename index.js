const {VmOne} = require('./build/Release/vmOne.node');

module.exports = (jsString = '', globalInit = {}, fileName = 'script', lineOffset = 0, colOffset = 0) => {
  const vm = new VmOne(globalInit);
  return vm.run(jsString, fileName, lineOffset, colOffset);
};
