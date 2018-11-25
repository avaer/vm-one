const vmOne = require('.');

(async () => {
  {
    const v = vmOne.make({
      initModule: './example-module.js',
    });
    console.log('example 1');
    const int32Array = Int32Array.from([1, 2, 3]);
    const int32SharedArray = new Int32Array(new SharedArrayBuffer(3 * Int32Array.BYTES_PER_ELEMENT));
    int32SharedArray[0] = 7;
    let result = v.runSync(`
      console.log('example 2', arg.int32Array, arg.int32SharedArray);

      global.lol = 'zol';
      return 'woot';
    `, {
      int32Array,
      int32SharedArray,
    }, [int32Array.buffer]);
    console.log('example 3', result);
    result = await v.runAsync(`
      console.log('example 4', arg.int32SharedArray);

      global.lol = 'zol2';
      return 'toot';
    `, {
      int32SharedArray,
    });
    console.log('example 5', result);
  }

  {
    const v = vmOne.make();
    console.log('example 6');
    let result = v.runSync(`
      console.log('example 7');

      global.lol = 'zol';
      return 'woot';
    `);
    console.log('example 8', result);
  }

  setTimeout(() => {
    process.exit();
  }, 100);
})();
