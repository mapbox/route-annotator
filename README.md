# Route Annotator

This is a HTTP service written in C++.

It loads an OSM map into RAM and indexes all the connected nodes.

Then, you can query it for sequences of nodes, and get back the tags that are on the ways those connected nodes belong to.

## Building

Run:

```
./deps.sh
make
```

## Getting the node sequence to query with

OSRM (on the feature/expose_node_ids branch) is able to return not just coordinates along a route, but OSM node ids as well.

First, query a route (this example is across San Francisco, east to west):

```
curl 'http://localhost:5000/route/v1/driving/-122.40726470947266,37.80313821864871;-122.48657226562499,37.76922210201122' > response.json
```

Then, extract the list of nodeids from that:

```
cat response.json | node -e '
  var c=[]; 
  process.stdin.on("data",function(chunk){c.push(chunk);}); 
  process.stdin.on("end",function() { 
    var input=c.join(),j=JSON.parse(input),nodes=[]; 
    j.routes[0].legs[0].steps.map(function(step,idx) { 
       step.nodes.map(function(nodeid,nodeidx) { 
          if (!(idx>0&&nodeidx==0)) nodes.push(nodeid); 
        }); 
    }); 
    console.log(nodes.join(",")); 
  });' > nodelist.txt`
```

## Running the route annotator

After building, run:

`./build/Release/./osm-infod http://localhost:5000 san-francisco-bay_california.osm.pbf`

**IMPORTANT NOTE**  The PBF file you use to serve metadata should match the one imported into OSRM - the assumption is that the OSM node IDs will match.


## Querying route metadata

The URL format is:

  http://localhost:5000/nodelist/1,2,3,4,5,6

So, using the node sequence from above, you can query:

  `curl http://localhost:5000/nodelist/$(cat nodelist.txt)`

And you'll get a response like:

```
time curl 'http://localhost:5000/nodelist/65297808,65297808,65297806,65328739,65293860,65336067,65336069,65371307,65322658,2064190371,2064190375,2064190386,65312651'
{"2064190371,2064190375":{"foot":"yes","highway":"residential","name":"Stockton Street","sidewalk":"both","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Stockton","tiger:name_type":"St","tiger:zip_left":"94133","tiger:zip_right":"94133"},"2064190375,2064190386":{"foot":"yes","highway":"residential","name":"Stockton Street","sidewalk":"both","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Stockton","tiger:name_type":"St","tiger:zip_left":"94133","tiger:zip_right":"94133"},"2064190386,65312651":{"foot":"yes","highway":"residential","name":"Stockton Street","sidewalk":"both","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Stockton","tiger:name_type":"St","tiger:zip_left":"94133","tiger:zip_right":"94133"},"65293860,65336067":{"highway":"residential","lanes":"2","name":"Greenwich Street","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Greenwich","tiger:name_type":"St","tiger:zip_left":"94123","tiger:zip_right":"94123"},"65297806,65328739":{"highway":"residential","lanes":"2","name":"Greenwich Street","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Greenwich","tiger:name_type":"St","tiger:zip_left":"94123","tiger:zip_right":"94123"},"65297808,65297806":{"highway":"residential","lanes":"2","name":"Child Street","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Child","tiger:name_type":"St","tiger:zip_left":"94133","tiger:zip_left_1":"94133","tiger:zip_right":"94133"},"65322658,2064190371":{"foot":"yes","highway":"residential","name":"Stockton Street","sidewalk":"both","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Stockton","tiger:name_type":"St","tiger:zip_left":"94133","tiger:zip_right":"94133"},"65328739,65293860":{"highway":"residential","lanes":"2","name":"Greenwich Street","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Greenwich","tiger:name_type":"St","tiger:zip_left":"94123","tiger:zip_right":"94123"},"65336067,65336069":{"highway":"residential","lanes":"2","name":"Greenwich Street","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Greenwich","tiger:name_type":"St","tiger:zip_left":"94123","tiger:zip_right":"94123"},"65336069,65371307":{"foot":"yes","highway":"residential","name":"Stockton Street","sidewalk":"both","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Stockton","tiger:name_type":"St","tiger:zip_left":"94133","tiger:zip_right":"94133"},"65371307,65322658":{"foot":"yes","highway":"residential","name":"Stockton Street","sidewalk":"both","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Stockton","tiger:name_type":"St","tiger:zip_left":"94133","tiger:zip_right":"94133"}}
real	0m0.011s
user	0m0.003s
sys	0m0.003s
```

The current response structure is:

```
{ "nodeA,nodeB" : { "key" : "value" , "key" : "value", ... },
  "nodeB,nodeC" : { "key" : "value" , ... }
}
```

# TODO
  - Consolidate consecutive node id pairs that are on the same way and only report tags once (include the range of node indexes that are part of each way in the response)
  - Figure out how to handle node pairs that belong to multiple ways
  - Perhaps implement an RTree, and allow the matching to be done by lon/lat instead of requiring node ids
  - Gracefully handle missing pairs, so that we can honour a request from a .OSM file that is slightly out of sync with the one we're serving
  - Startup progress indicator
  - Parallelize OSM parsing at the start
  - Improve the `nodepair_t` hash function to improve performance
  - Figure out why we can only handl 123 characters on the URL
