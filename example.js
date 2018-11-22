const vmOne = require('.');

(async () => {
  {
    const v = vmOne.make();
    console.log('example 1');
    let result = v.runSync(`
      console.log('example 2');

      global.lol = 'zol';
      return 'woot';
    `);
    console.log('example 3', result);
    result = await v.runAsync(`
      console.log('example 4');

      global.lol = 'zol2';
      return 'toot';
    `);
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

  process.exit();
})();
