const vmOne = require('.');

(async () => {
  {
    const v = vmOne.make();
    console.log('example 1');
    v.getGlobal(g => {
      console.log('example 2', Object.keys(g));
    });
    console.log('example 3');

    v.runSync(`
      console.log('example 4');
    `);
    await v.runAsync(`
      console.log('example 5');
    `);
    console.log('example 6');
  }

  {
    const v = vmOne.make();
    console.log('example 7');
    v.getGlobal(g => {
      console.log('example 8', Object.keys(g));
    });
    console.log('example 9');
  }

  process.exit();
})();
