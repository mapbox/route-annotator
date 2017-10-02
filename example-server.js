'use strict';

const bindings = require('./index');
const express = require('express');
const async = require('async');

function main() {
  const argv = process.argv.slice(1);
  const argc = argv.length;

  if (argc < 2)
    return console.error(`Usage: ${__filename} FILE [PORT]`);

  const osmFile = argv[1];
  const port = argc == 3 ? argv[2] : 5000;

  const app = express();

  app.use((req, res, next) => {
    console.log(`${req.method}  ${req.url}  ${req.headers['user-agent']}`);
    next();
  });

  const annotator = new bindings.Annotator({coordinates: true});

  app.get('/nodelist/:nodelist', nodeListHandler(annotator));
  app.get('/coordlist/:coordlist', coordListHandler(annotator));

  annotator.loadOSMExtract(osmFile, (err) => {
    if (err)
      return console.error(err);

    app.listen(port, () => {
      console.log(`Listening on 0.0.0.0:${port}`);
    });
  });
}


function nodeListHandler(annotator) {
  return (req, res) => {
    const nodes = req.params.nodelist
                    .split(',')
                    .map(x => parseInt(x, 10));

    const invalid = (x) => !isFinite(x) || x === null;

    if (nodes.some(invalid))
      return res.sendStatus(400);

    annotator.annotateRouteFromNodeIds(nodes, (err, wayIds) => {
      if (err)
        return res.sendStatus(400);

      // TODO(daniel-j-h): standardize format for {wayId, tags}, accumulate and send back
      // wayIds.map(annotator.getAllTagsForWayId(wayId, (err, tags) => {}));
      res.json({'message': 'Danpat needs to standardize a format'});
    });
  };
}


function coordListHandler(annotator) {
  return (req, res) => {
    const coordinates = req.params.coordlist
                          .split(';')
                          .map(lonLat => lonLat
                                           .split(',')
                                           .map(x => parseFloat(x)));

    const invalid = (x) => !isFinite(x) || x === null;

    if (coordinates.some(lonLat => lonLat.some(invalid)))
      return res.sendStatus(400);

    annotator.annotateRouteFromLonLats(coordinates, (err, wayIds) => {
      if (err)
        return res.sendStatus(400);

      var response = {"way_indexes": [], "ways_seen": []};
      var way_indexes = {};

      async.each(wayIds, (way_id, next) => {
          annotator.getAllTagsForWayId(way_id, (err, tags) => {
              var wid = tags["_way_id"];
              if (!way_indexes.hasOwnProperty(wid)) {
                way_indexes[wid] = Object.keys(way_indexes).length;
                response.ways_seen.push(tags);
              }
              response.way_indexes.push(way_indexes[wid]);
              next();
          });
      }, (err, data) => {
          res.json(response);
      });
    });
  };
}


if (require.main === module) { main(); }
