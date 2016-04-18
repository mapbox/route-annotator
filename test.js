const pkg = require('./index');

const annotator = new pkg.Annotator('build/monaco.osm.pbf');

const fst = annotator.annotateRouteFromNodeIds([1, 2, 3]);
console.log(fst);
