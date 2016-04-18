const pkg = require('./index');
const assert = require('assert');

const annotator = new pkg.Annotator('build/monaco.osm.pbf');

const fst = annotator.annotateRouteFromNodeIds([1, 2, 3]);
assert.equal(fst.length, 2, 'There are n-1 ways between n nodes')
console.log(fst);

const snd = annotator.annotateRouteFromLonLats([[1, 2], [3, 4]]);
assert.equal(snd.length, 1, 'There are n-1 ways between n coordinates')
console.log(snd);

const thd = annotator.getAllTagsForWayId(1);
console.log(thd)
