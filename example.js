const vmOne = require('.');

(async () => {
  {
    const v = vmOne.make({
      initModule: './example-module.js',
    });
    v.on('message', m => {
      console.log('parent got message', m);
    });

    console.log('example 1');

    const int32Array = Int32Array.from([1, 2, 3]);
    const int32SharedArray = new Int32Array(new SharedArrayBuffer(3 * Int32Array.BYTES_PER_ELEMENT));
    int32SharedArray[0] = 7;
    let result = v.runSync(`
      console.log('example 2', arg.int32Array, arg.int32SharedArray);

      global.lol = 'zol';
      global.on('message', m => {
        console.log('child got message', m);

        global.postMessage(m);
      });
      return 'woot';
    `, {
      int32Array,
      int32SharedArray,
    }, [int32Array.buffer]);

    console.log('example 3', result);

    v.postMessage({
      lol: 'zol',
    });

    console.log('example 4');

    result = await v.runAsync(`
      console.log('example 5', arg.int32SharedArray);

      global.lol = 'zol2';
      return 'toot';
    `, {
      int32SharedArray,
    });

    console.log('example 6', result);
  }

  {
    const v = vmOne.make();

    console.log('example 7');

    let result = v.runSync(`
      console.log('example 8');

      global.lol = 'zol';
      return 'woot';
    `);

    console.log('example 9', result);
  }

  setTimeout(() => {
    process.exit();
  }, 100);
})();
