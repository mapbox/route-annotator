var test = require('tape');
const bindings = require('../index');
const annotator = new bindings.Annotator({ coordinates: true });
const segmentmap = new bindings.SpeedLookup();
const path = require('path');

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

test('initialization failure', function(t) {
  t.throws(bindings.SpeedLookup, /Cannot call constructor/, "Check that lookup can't be constructed without new");
  t.end();
});

test('initialization failure with parameters', function(t) {
  t.throws(function() { var foo = new bindings.SpeedLookup("test"); }, /No types expected/, "Check that lookup can't be constructed with parameters");
  t.end();
});


test('check CSV loading', function(t) {
  t.throws(function(cb) { 
    var foo = new bindings.SpeedLookup(); 
    foo.loadCSV(1,(err) => {cb(err);});
  }, /and callback expected/, "Only understands strings and arrays of strings");

  t.throws(function(cb) { 
    var foo = new bindings.SpeedLookup(); 
    foo.loadCSV("testfile.csv");
  }, /and callback expected/, "Needs a callback function");

  t.throws(function(cb) { 
    var foo = new bindings.SpeedLookup();
    foo.loadCSV([],(err) => {cb(err);});
  }, /at least one filename/, "Doesn't know what to do with an empty array");
  t.end();
});

test('missing single files', function(t) {
  var foo = new bindings.SpeedLookup();
  foo.loadCSV("doesnotexist.csv",(err) => {
    t.ok(err, "Fails if the file does not exist");
    t.end();
  });
});

test('missing multifiles', function(t) {
  var foo = new bindings.SpeedLookup();
  foo.loadCSV([path.join(__dirname,'congestion/fixtures/congestion.csv'),
               "doesnotexist.csv"],(err) => {
    t.ok(err, "Fails if any of the files in the list does not exist");
    t.end();
  });
});

test('load valid CSV', function(t) {
  segmentmap.loadCSV(
    [path.join(__dirname,'congestion/fixtures/congestion.csv'),
     path.join(__dirname,'congestion/fixtures/congestion2.csv')]
    , (err) => {
    if (err) throw err;
    t.end();
  });
});

test('lookup node pair', function(t) {
  segmentmap.getRouteSpeeds([86909066,86909064,86909066,999], (err, resp)=> {
    if (err) { console.log(err); throw err; }
    t.same(resp, [79,80,4294967295], "Verify expected speed results");
    t.end();
  });
});

test('lookup node pair from second file', function(t) {
  segmentmap.getRouteSpeeds([90,91,92,93], (err, resp)=> {
    if (err) { console.log(err); throw err; }
    t.same(resp, [2,3,4], "Verify expected speed results");
    t.end();
  });
});

test('make sure that it works even if CSV is not loaded', function(t) {
  var speedlookup = new bindings.SpeedLookup();
  speedlookup.getRouteSpeeds([90,91,92,93], (err, resp)=> {
    if (err) { console.log(err); throw err; }
    t.ok(resp, "Return results even if CSV is not loaded");
    t.end();
  });
});

test('dont load CSV and see if you get the correct response (4 INVALID_SPEEDs)', function(t) {
  var speedlookup = new bindings.SpeedLookup();
  speedlookup.getRouteSpeeds([90,91,92,93], (err, resp)=> {
    if (err) { console.log(err); throw err; }
    t.same(resp, [ 4294967295, 4294967295, 4294967295 ], "Verify expected speed results");
    t.end();
  });
});

test('annotator without coordinates support - by default', function(t) {
  const coordsFalse = new bindings.Annotator();
  coordsFalse.loadOSMExtract(path.join(__dirname,'data/winthrop.osm'), (err) => {
    if (err) throw err;
    var coords = [[-120.1872774,48.4715898],[-120.1882910,48.4725110]];
    coordsFalse.annotateRouteFromLonLats(coords, (err) => {
      t.equals(err.toString(), 'Error: Annotator not created with coordinates support');
      t.end();
    });
  });
});

test('annotator without coordinates support - explicit', function(t) {
  const coordsFalse = new bindings.Annotator({ coordinates: false });
  coordsFalse.loadOSMExtract(path.join(__dirname,'data/winthrop.osm'), (err) => {
    if (err) throw err;
    var coords = [[-120.1872774,48.4715898],[-120.1882910,48.4725110]];
    coordsFalse.annotateRouteFromLonLats(coords, (err) => {
      t.equals(err.toString(), 'Error: Annotator not created with coordinates support');
      t.end();
    });
  });
});
