const path = require('path');
const test = require('tape');
const async = require('async');

const bindings = require('../index');
const MONACO_FILE = path.join(__dirname,'data/monaco.extract.osm');
const BALTIMORE_FILE = path.join(__dirname,'data/baltimore.osm.pbf');
const TAGS_FILE = path.join(__dirname, 'data/tags'); // data/tags => 'maxspeed'

test('loadOSMExtract errors', function(t) {
    const annotator = new bindings.Annotator();
    t.throws(function() {
        annotator.loadOSMExtract([], (err) => {});
    }, 'Empty array of osm files');
    t.throws(function() {
        annotator.loadOSMExtract([[MONACO_FILE]]);
    }, 'Nested array of files');
    t.throws(function() {
        annotator.loadOSMExtract([path.join(__dirname,'data/paris.extract.osm.pbf'), [MONACO_FILE]]);
    }, 'Array containing first a string then another array');
    t.throws(function() {
        annotator.loadOSMExtract('./fake-file');
    }, 'Non-existent file');
    t.throws(function() {
        annotator.loadOSMExtract(['./fake-file']);
    }, 'Non-existent file in an array');
    t.throws(function() {
        annotator.loadOSMExtract([MONACO_FILE]);
    }, 'no callback provided');
    t.throws(function() {
        annotator.loadOSMExtract((err) => {});
    }, 'only callback is provided');
    t.throws(function() {
        annotator.loadOSMExtract([MONACO_FILE], [TAGS_FILE], (err) => {});
    }, 'tags file provided as array');
    t.end();
});

test('load extract as string with tags', function(t) {
    const annotator = new bindings.Annotator({ coordinates: true });
    annotator.loadOSMExtract([MONACO_FILE], TAGS_FILE, (err) => {
       if (err) throw err;
       t.end();
    });
});

test('load multiple extracts in array with tags', function(t) {
    const annotator = new bindings.Annotator();
    annotator.loadOSMExtract([MONACO_FILE, path.join(__dirname,'data/paris.extract.osm.pbf')], TAGS_FILE, (err) => {
       t.ifError(err);
       t.end();
    });
});

test('load extract with tags', function(t) {
    // data/multiple-tags => 'maxspeed, tunnel'
    const annotator = new bindings.Annotator();
    annotator.loadOSMExtract(MONACO_FILE, path.join(__dirname, 'data/multiple-tags'), (err) => {
       if (err) throw err;
       t.end();
    });
});

test('only one nodeId in array', function(t) {
    const annotator = new bindings.Annotator();
    var nodes = [2109571486];
    t.throws(function() { annotator.annotateRouteFromNodeIds(nodes, (err, wayIds) => { })});
    t.end();
});

test('annotate by node on data filtered by tag', function(t) {
    const annotator = new bindings.Annotator();
    // data/multiple-tags => 'maxspeed, tunnel'
    annotator.loadOSMExtract(MONACO_FILE, path.join(__dirname, 'data/multiple-tags'), (err) => {
        t.error(err, 'Load monaco');
        var q = async.queue((task, callback) => {
            annotator.annotateRouteFromNodeIds(task.nodes, (err, wayIds) => {
                t.error(err, 'No error annotating');
                callback(wayIds);
            });
        });
        // Get annotations with tags maxspeed
        var NodesWithMaxspeed = {nodes: [1079045359,1079045454]};
        q.push(NodesWithMaxspeed, (ids) => {
            t.equals(ids.length, 1, 'One way id returned');
            annotator.getAllTagsForWayId(ids[0], (err, tags) => {
                t.error(err, 'No error');
                t.ok(tags.maxspeed, 'Has maxspeed tag');
                t.equal(tags.maxspeed, '50', 'Got correct maxspeed');
                t.equal(tags._way_id, '93091314', 'Got correct _way_id attribute on match');
            });
        });
        // Get annotations with tags tunnel
        var NodesWithTunnel = {nodes: [1347559127,1868280229,1868280238,1868280241]};
        q.push(NodesWithTunnel, (ids) => {
            t.equals(ids.length, 3);
            annotator.getAllTagsForWayId(ids[0], (err, tags) => {
                console.log(tags);
                t.error(err, 'No error');
                t.ok(tags.tunnel, 'Has tunnel tag');
                // TODO: decide whether tag filter should limit what gets indexed
                // t.equals(tags.name, 'Tunnel Rocher Canton', 'has correct name');
                t.equal(tags._way_id, '80378486', 'Got correct _way_id attribute on match');
            });
        });
        // Get no annotations back
        var NodesWithoutMaxspeedOrTunnel = {nodes: [25193333,1204288453,3854490634]};
        q.push(NodesWithoutMaxspeedOrTunnel, (ids) => {
            t.equals(ids.length, 2, 'One way id returned');
            t.ok(ids.every(i => i == null), 'Way ids are null');
        });
        q.drain = () => {
            t.end();
        };
    });
});

test('annotate by node on data filtered by tag', function(t) {
    const annotator = new bindings.Annotator();
    // data/multiple-tags => 'maxspeed, tunnel'
    annotator.loadOSMExtract(BALTIMORE_FILE, path.join(__dirname, 'data/multiple-tags'), (err) => {
        t.error(err, 'Load monaco');
        var q = async.queue((task, callback) => {
            annotator.annotateRouteFromNodeIds(task.nodes, (err, wayIds) => {
                t.error(err, 'No error annotating');
                callback(wayIds);
            });
        });
        // Get annotations with tags maxspeed
        var NodesWithMaxspeed = {nodes: [49461073,2494188158]};
        q.push(NodesWithMaxspeed, (ids) => {
            t.equals(ids.length, 1, 'One way id returned');
            annotator.getAllTagsForWayId(ids[0], (err, tags) => {
                t.error(err, 'No error');
                t.ok(tags.maxspeed, 'Has maxspeed tag');
                t.equal(tags.maxspeed, '40', 'Got correct maxspeed');
                t.equal(tags._way_id, '326116174', 'Got correct _way_id attribute on match');
            });
        });
        // Get annotations with tags maxspeed
        NodesWithMaxspeed = {nodes: [2719002521,2719002531]};
        q.push(NodesWithMaxspeed, (ids) => {
            t.equals(ids.length, 1, 'One way id returned');
            annotator.getAllTagsForWayId(ids[0], (err, tags) => {
                t.error(err, 'No error');
                t.ok(tags.maxspeed, 'Has maxspeed tag');
                t.equal(tags.maxspeed, '88', 'Got correct maxspeed');
                t.equal(tags._way_id, '266323344', 'Got correct _way_id attribute on match');
            });
        });
        // Get no annotations back
        var NodesWithoutMaxspeedOrTunnel = {nodes: [25193333,1204288453,3854490634]};
        q.push(NodesWithoutMaxspeedOrTunnel, (ids) => {
            t.equals(ids.length, 2, 'One way id returned');
            t.ok(ids.every(i => i == null), 'Way ids are null');
        });
        q.drain = () => {
            t.end();
        };
    });
});

