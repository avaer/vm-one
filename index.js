const path = require('path');
const {VmOne, setPrototype} = require('./build/Release/vmOne.node');

const pushEnv = (name, value, cb) => {
  const del = !process.env.hasOwnProperty(name);
  const prev = process.env[name];
  process.env[name] = value;
  const result = cb();
  if (del) {
    delete process.env[name];
  } else {
    process.env[name] = prev;
  }
  return result;
}

let compiling = false;
const make = (globalInit = {}) => pushEnv('PKG_EXECPATH', 'PKG_INVOKE_NODEJS',
  () => new VmOne(globalInit, e => {
    if (e === 'compilestart') {
      compiling = true;
    } else if (e === 'compileend') {
      compiling = false;
    }
  }, (process.pkg ? path.dirname(process.execPath) : __dirname) + path.sep))

const isCompiling = () => compiling;

const vmOne = {
  make,
  isCompiling,
  setPrototype,
}

module.exports = vmOne;
