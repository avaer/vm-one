const {VmOne} = require('./build/Release/vmOne.node');

const vmSymbol = Symbol();

let compiling = false;
function vmOne(jsString = '', globalInit = {}, fileName = 'script', lineOffset = 0, colOffset = 0) {
  let vm = globalInit[vmSymbol];
  if (!vm) {
    vm = new VmOne(globalInit, e => {
      if (e === 'compilestart') {
        compiling = true;
      } else if (e === 'compileend') {
        compiling = false;
      }
    });
    globalInit[vmSymbol] = vm;
  }

  return vm.run(jsString, fileName, lineOffset, colOffset);
}
vmOne.isCompiling = () => compiling;

module.exports = vmOne;
