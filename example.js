const vmOne = require('.');

vmOne(`console.log(lol);`, {
  console,
  lol: 'zol',
});
