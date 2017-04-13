var test = require('tape');
const bindings = require('../index');
const annotator = new bindings.Annotator();
var path = require('path');

test('load extract', function(t) {
  annotator.loadOSMExtract(path.join(__dirname,'data/winthrop.osm'), (err) => {
     if (err) throw err;
     t.end();
  });
});

test('annotate', function(t) {
    var nodes = [7.422155,43.7368838,7.4230139,43.7369751];
    annotator.annotateRouteFromNodeIds(nodes, (err, wayIds) => {
      if (err) throw err;
      t.end();
    });
});


// TEST AND EXAMPLE CODE TO USE CONGESTION LOOKUP
const congestionLookup = new bindings.Lookup(path.join(__dirname,'congestion/congestion.csv'));
congestionLookup.GetAnnotations([62444552,62444554,62444556], (err, resp)=> {
  console.log(resp);
});