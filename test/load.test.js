var test = require('tape');
const bindings = require('../index');
const annotator = new bindings.Annotator();
const segmentmap = new bindings.Lookup();
var path = require('path');

test('load extract', function(t) {
  annotator.loadOSMExtract(path.join(__dirname,'data/winthrop.osm'), (err) => {
     if (err) throw err;
     t.end();
  });
});

test('annotate by node', function(t) {
    //var nodes = [-120.1872774,48.4715898,-120.1882910,48.4725110];
    //var nodes = [7.422155,43.7368838,7.4230139,43.7369751];
    var nodes = [50253600,50253602,50137292];
    annotator.annotateRouteFromNodeIds(nodes, (err, wayIds) => {
      if (err) throw err;
      t.same(wayIds, [0,0], "Verify got two hits on the first way");
      annotator.getAllTagsForWayId(wayIds[0], (err, tags) => {
        if (err) throw err;
        t.equal(tags._way_id,'6091729',"Got correct _way_id attribute on match");
        t.end();
      });
    });
});

test('annotate by coordinate', function(t) {
    var coords = [[-120.1872774,48.4715898],[-120.1882910,48.4725110]];
    annotator.annotateRouteFromLonLats(coords, (err, wayIds) => {
      if (err) throw err;
      t.same(wayIds, [0], "Verify got two hits on the first way");
      annotator.getAllTagsForWayId(wayIds[0], (err, tags) => {
        if (err) throw err;
        t.equal(tags._way_id,'6091729',"Got correct _way_id attribute on match");
        t.end();
      });
    });
});

test('load CSV', function(t) {
  segmentmap.loadCSV(path.join(__dirname,'congestion/fixtures/congestion.csv'), (err) => {
    if (err) throw err;
    t.end();
  });
});

test('lookup node pair', function(t) {
  segmentmap.GetAnnotations([86909066,86909064,86909066,999], (err, resp)=> {
    if (err) { console.log(err); throw err; }
    t.same(resp, [79,80,4294967295], "Verify expected speed results");
    t.end();
  });

});