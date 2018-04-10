const {VmOne} = require('./build/Release/vmOne.node');

let compiling = false;
function vmOne(jsString = '', globalInit = {}, fileName = 'script', lineOffset = 0, colOffset = 0) {
  const vm = new VmOne(globalInit, e => {
    if (e === 'compilestart') {
      compiling = true;
    } else if (e === 'compileend') {
      compiling = false;
    }
  });
  return vm.run(jsString, fileName, lineOffset, colOffset);
}
vmOne.isCompiling = () => compiling;

module.exports = vmOne;
