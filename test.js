'use strict';

const bindings = require('./index');
const express = require('express');


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


  const annotator = new bindings.Annotator(osmFile);

  app.get('/nodelist/:nodelist', nodeListHandler(annotator));
  app.get('/coordlist/:coordlist', coordListHandler(annotator));

  app.listen(port, () => {
    console.log(`Listening on localhost:${port}`);
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

    const wayIds = annotator.annotateRouteFromNodeIds(nodes);
    res.send(wayIds);
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

    const wayIds = annotator.annotateRouteFromLonLats(coordinates);
    res.send(wayIds);
  };
}


if (require.main === module) { main(); }
