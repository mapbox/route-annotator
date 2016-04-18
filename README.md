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

## Building the Node.js Bindings

```
./deps.sh
npm install
npm test
```

## Getting the node sequence to query with

OSRM (on the feature/expose_node_ids branch) is able to return not just coordinates along a route, but OSM node ids as well.

First, query a route (this example is across San Francisco, east to west):

```
curl 'http://localhost:5000/route/v1/driving/-122.40726470947266,37.80313821864871;-122.48657226562499,37.76922210201122?geometries=geojson' > response.json
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
  });' > nodelist.txt
```

OR extract the list of coordinates with:

```
cat response.json | node -e '
  var c=[]; 
  process.stdin.on("data",function(chunk){c.push(chunk);}); 
  process.stdin.on("end",function() { 
    var input=c.join(),j=JSON.parse(input),coords=[]; 
    j.routes[0].legs[0].steps.map(function(step,idx) { 
       step.geometry.coordinates(function(lonlat,coordidx) { 
          if (!(idx>0&&coordidx==0)) coords.push(lonlat); 
        }); 
    }); 
    console.log(coords.join(";")); 
  });' > coordlist.txt
```

## Running the route annotator

After building, run:

`./build/Release/./osm-infod http://localhost:5000 san-francisco-bay_california.osm.pbf`

**IMPORTANT NOTE**  The PBF file you use to serve metadata should match the one imported into OSRM - the assumption is that the OSM node IDs will match.


## Querying route metadata

**NOTE**: querying by node ID is faster - if you supply coordinates, they need to be internally mapped to nodes.

For querying by node ID, the URL format is:

  `http://localhost:5000/nodelist/1,2,3,4,5,6`

For queryinb by coordinates, the URL format is:

  `http://localhost:5000/coordlist/lon,lat;lon,lat;lon,lat`

So, using the node sequence from above, you can query:

  `curl http://localhost:5000/coordlist/$(cat coordlist.txt)`

OR

  `curl http://localhost:5000/nodelist/$(cat nodelist.txt)`

And you'll get a response like:

```
$ time curl "http://localhost:5000/coordlist/$(cat coordlist.txt)"
{"way_indexes":[null,0,1,1,1,1,2,2,2,2,2,2,3,3,3,4,5,5,5,6,6,6,7,7,8,9,9,10,11,12,12,12,12,13,13,13,14,15,15,16,16,16,16,17,17,18,19,20,21,21,21,21,22,22,23,23,23,23,23,23,24,24,25,25,25,25,25,26,27,28,29,30,30,31,32,32,32,32,32,32,33,33,33,33,33,33,33,33,34,34,35,35,36,37,37,37,37,37,37,38,39,39,39,39,40,41,41,41,41,41,41,42,42,42,42,43,43,43,43,43,43,43,43,43,43,43,44,44,44,44,45,45,45,45,46,46,47,48,49,50,51,51,51,51,51,51,51,51,51,52,52,52,53,53,53,54,54,54,54,54,54,54,54,54,54,54,54,54,54,54,54,54,55,56,56,56,57,57,58,58,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,59,60,60,60,60,60,60,60,60,60,61,61,61,61,61,61,61,61,61,null,61,61,null],
"ways_seen":[{"_way_id":"8916490","highway":"residential","lanes":"2","name":"Child Street","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Child","tiger:name_type":"St","tiger:zip_left":"94133","tiger:zip_left_1":"94133","tiger:zip_right":"94133"},{"_way_id":"28808571","highway":"residential","lanes":"2","name":"Greenwich Street","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Greenwich","tiger:name_type":"St","tiger:zip_left":"94123","tiger:zip_right":"94123"},{"_way_id":"148847000","foot":"yes","highway":"residential","name":"Stockton Street","sidewalk":"both","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Stockton","tiger:name_type":"St","tiger:zip_left":"94133","tiger:zip_right":"94133"},{"_way_id":"48211489","foot":"yes","highway":"residential","name":"Stockton Street","sidewalk":"both","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Stockton","tiger:name_type":"St","tiger:zip_left":"94133","tiger:zip_right":"94133","trolley_wire":"yes"},{"_way_id":"206629822","highway":"tertiary","lanes":"2","name":"Stockton Street","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Stockton","tiger:name_type":"St","tiger:zip_left":"94133","tiger:zip_right":"94133","trolley_wire":"yes"},{"_way_id":"28841114","highway":"tertiary","lanes":"2","name":"Stockton Street","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Stockton","tiger:name_type":"St","tiger:zip_left":"94133","tiger:zip_right":"94133","trolley_wire":"yes"},{"_way_id":"87376660","cycleway":"lane","highway":"primary","lanes":"4","lcn_ref":"10","name":"Broadway","name_1":"Broadway","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Broadway","tiger:name_base_1":"Broadway","tiger:name_type":"St","tiger:zip_left":"94133","tiger:zip_right":"94133"},{"_way_id":"26938247","highway":"primary","lanes":"2","lcn_ref":"10","maxspeed":"35 mph","name":"Broadway","name_1":"Broadway","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Broadway","tiger:name_base_1":"Broadway","tiger:name_type":"St","tiger:zip_left":"94133","tiger:zip_right":"94133"},{"_way_id":"133376103","highway":"primary","lanes":"2","lcn_ref":"210","maxspeed":"35 mph","name":"Broadway","name_1":"Broadway","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Broadway","tiger:name_base_1":"Broadway","tiger:name_type":"St","tiger:zip_left":"94133","tiger:zip_right":"94133"},{"_way_id":"8921175","highway":"primary","layer":"-1","lcn_ref":"210","maxspeed":"40 mph","name":"Robert C Levy Tunnel","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Robert C Levy","tiger:name_type":"Tunl","tunnel":"yes"},{"_way_id":"26938401","cutting":"yes","highway":"primary","lanes":"2","lcn_ref":"210","maxspeed":"40 mph","name":"Broadway","name_1":"Broadway","oneway":"yes","tiger:cfcc":"A41;A45","tiger:county":"San Francisco, CA","tiger:name_base":"Robert C Levy;Broadway","tiger:name_base_1":"Broadway","tiger:name_type":"Tunl;St"},{"_way_id":"87297169","cutting":"yes","highway":"primary","lanes":"2","lcn_ref":"210","maxspeed":"35 mph","name":"Broadway","name_1":"Broadway","oneway":"yes","tiger:cfcc":"A41;A45","tiger:county":"San Francisco, CA","tiger:name_base":"Robert C Levy;Broadway","tiger:name_base_1":"Broadway","tiger:name_type":"Tunl;St"},{"_way_id":"85644934","highway":"primary","lanes":"2","lcn_ref":"210","name":"Broadway","name_1":"Broadway","oneway":"yes","tiger:cfcc":"A41;A45","tiger:county":"San Francisco, CA","tiger:name_base":"Robert C Levy;Broadway","tiger:name_base_1":"Broadway","tiger:name_type":"Tunl;St"},{"_way_id":"27050852","highway":"primary","lanes":"5","lanes:backward":"2","lanes:forward":"3","name":"Broadway","name_1":"Broadway","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Robert C Levy;Broadway","tiger:name_base_1":"Broadway","tiger:name_type":"Tunl;St","tiger:zip_left":"94109;94115","tiger:zip_right":"94109;94115","turn:lanes:backward":"|","turn:lanes:forward":"||"},{"_way_id":"85644923","highway":"primary","name":"Broadway","name_1":"Broadway","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Robert C Levy;Broadway","tiger:name_base_1":"Broadway","tiger:name_type":"Tunl;St","tiger:zip_left":"94109;94115","tiger:zip_right":"94109;94115"},{"_way_id":"85644928","highway":"tertiary","lanes":"4","lanes:backward":"2","lanes:forward":"2","name":"Broadway","name_1":"Broadway","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Robert C Levy;Broadway","tiger:name_base_1":"Broadway","tiger:name_type":"St","tiger:zip_left":"94109;94115","tiger:zip_right":"94109;94115","turn:lanes:backward":"|","turn:lanes:forward":"|"},{"_way_id":"85562549","highway":"residential","maxspeed":"25 mph","name":"Gough Street","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Gough","tiger:name_type":"St","tiger:zip_left":"94123","tiger:zip_right":"94123"},{"_way_id":"398681180","highway":"residential","maxspeed":"25 mph","name":"Gough Street","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Gough","tiger:name_type":"St","tiger:zip_left":"94123","tiger:zip_right":"94123"},{"_way_id":"224199016","highway":"residential","maxspeed":"25 mph","name":"Gough Street","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Gough","tiger:name_type":"St","tiger:zip_left":"94123","tiger:zip_right":"94123","trolley_wire":"yes"},{"_way_id":"26944132","highway":"tertiary","lanes":"2","maxspeed":"25 mph","name":"Gough Street","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Gough","tiger:name_type":"St","tiger:reviewed":"no","tiger:zip_left":"94123","tiger:zip_right":"94123","turn:lanes":"left;through|right;through"},{"_way_id":"27398072","highway":"tertiary","lanes":"2","maxspeed":"25 mph","name":"Gough Street","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Gough","tiger:name_type":"St","tiger:reviewed":"no","tiger:zip_left":"94123","tiger:zip_right":"94123"},{"_way_id":"27398071","highway":"secondary","lanes":"3","maxspeed":"25 mph","name":"Gough Street","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Gough","tiger:name_type":"St","tiger:zip_left":"94123","tiger:zip_right":"94123"},{"_way_id":"254306573","highway":"secondary","lanes":"3","maxspeed":"25 mph","name":"Gough Street","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Gough","tiger:name_type":"St","tiger:zip_left":"94123","tiger:zip_right":"94123"},{"_way_id":"84654229","highway":"primary","lanes":"3","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd"},{"_way_id":"84733284","highway":"primary","lanes":"3","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd"},{"_way_id":"84733309","highway":"primary","lanes":"3","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd"},{"_way_id":"8919251","cutting":"yes","highway":"primary","lanes":"3","layer":"-1","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","surface":"paved","tiger:cfcc":"A45","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd"},{"_way_id":"398846302","cutting":"yes","highway":"primary","lanes":"3","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","surface":"paved","tiger:cfcc":"A45","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd","tunnel":"yes"},{"_way_id":"398846305","cutting":"yes","highway":"primary","lanes":"3","layer":"-1","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","surface":"paved","tiger:cfcc":"A45","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd"},{"_way_id":"84733296","highway":"primary","lanes":"3","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","tiger:cfcc":"A45","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd"},{"_way_id":"84733287","highway":"primary","lanes":"3","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","tiger:cfcc":"A45","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd"},{"_way_id":"23880808","highway":"primary","lanes":"4","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd"},{"_way_id":"397082296","highway":"primary","lanes":"4","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd","turn:lanes":"|||merge_to_left"},{"_way_id":"84733316","highway":"primary","lanes":"3","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd"},{"_way_id":"23887553","highway":"primary","lanes":"3","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd"},{"_way_id":"86336709","highway":"primary","lanes":"3","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd"},{"_way_id":"23848469","highway":"primary","lanes":"3","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","tiger:cfcc":"A45","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd","tiger:zip_left":"94118"},{"_way_id":"23848418","highway":"primary_link","lanes":"1","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","tiger:cfcc":"A45","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd"},{"_way_id":"396968573","highway":"primary_link","lanes":"1","lcn_ref":"55","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","tiger:cfcc":"A45","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd","trolley_wire":"yes"},{"_way_id":"133856232","highway":"primary_link","lanes":"2","lcn_ref":"55","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","tiger:cfcc":"A45","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd","trolley_wire":"yes"},{"_way_id":"396968570","highway":"primary_link","lanes":"3","lcn_ref":"55","maxspeed":"35 mph","name":"Geary Boulevard","oneway":"yes","tiger:cfcc":"A45","tiger:county":"San Francisco, CA","tiger:name_base":"Geary","tiger:name_type":"Blvd","trolley_wire":"yes"},{"_way_id":"23848314","highway":"primary_link","lanes":"2","lcn_ref":"55","maxspeed":"35 mph","oneway":"yes","trolley_wire":"yes"},{"_way_id":"214037668","highway":"primary_link","lanes":"2","lcn_ref":"55","oneway":"yes","trolley_wire":"yes"},{"_way_id":"27167652","highway":"primary","lanes":"3","lcn_ref":"55","name":"Masonic Avenue","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Masonic","tiger:name_type":"Ave","tiger:zip_left":"94117","tiger:zip_right":"94117","trolley_wire":"yes"},{"_way_id":"396969676","highway":"primary","lanes":"4","lcn_ref":"55","name":"Masonic Avenue","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Masonic","tiger:name_type":"Ave","tiger:zip_left":"94117","tiger:zip_right":"94117","trolley_wire":"yes","turn:lanes":"left|||"},{"_way_id":"396969674","highway":"primary","lanes":"3","lcn_ref":"55","name":"Masonic Avenue","oneway":"yes","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Masonic","tiger:name_type":"Ave","tiger:zip_left":"94117","tiger:zip_right":"94117","trolley_wire":"yes"},{"_way_id":"24335286","highway":"primary","lanes":"5","lanes:backward":"2","lanes:forward":"3","lcn_ref":"55","name":"Masonic Avenue","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Masonic","tiger:name_type":"Ave","tiger:zip_left":"94117","tiger:zip_right":"94117","trolley_wire":"yes"},{"_way_id":"396973462","highway":"primary","lanes":"4","lanes:backward":"2","lanes:forward":"2","lcn_ref":"55","name":"Masonic Avenue","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Masonic","tiger:name_type":"Ave","tiger:zip_left":"94117","tiger:zip_right":"94117","trolley_wire":"yes"},{"_way_id":"101153936","highway":"primary","lanes":"4","lanes:backward":"2","lanes:forward":"2","lcn_ref":"20;55","name":"Masonic Avenue","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Masonic","tiger:name_type":"Ave","tiger:zip_left":"94117","tiger:zip_right":"94117"},{"_way_id":"101153941","highway":"primary","lanes":"4","lanes:backward":"2","lanes:forward":"2","lcn_ref":"20;55","name":"Masonic Avenue","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Masonic","tiger:name_type":"Ave","tiger:zip_left":"94117","tiger:zip_right":"94117"},{"_way_id":"101153938","highway":"primary","lanes":"4","lanes:backward":"2","lanes:forward":"2","lcn_ref":"55","name":"Masonic Avenue","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Masonic","tiger:name_type":"Ave","tiger:zip_left":"94117","tiger:zip_right":"94117"},{"_way_id":"86331264","highway":"secondary","lanes":"4","lanes:backward":"2","lanes:forward":"2","maxspeed":"35 mph","name":"Fulton Street","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Fulton","tiger:name_type":"St","tiger:zip_left":"94117","tiger:zip_right":"94121","trolley_wire":"yes"},{"_way_id":"254750798","highway":"secondary","lanes":"4","lanes:backward":"2","lanes:forward":"2","maxspeed":"35 mph","name":"Fulton Street","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Fulton","tiger:name_type":"St","tiger:zip_left":"94117","tiger:zip_right":"94121","trolley_wire":"yes","turn:lanes:forward":"|right"},{"_way_id":"396980731","highway":"secondary","lanes":"4","lanes:backward":"2","lanes:forward":"2","maxspeed":"35 mph","name":"Fulton Street","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Fulton","tiger:name_type":"St","tiger:zip_left":"94117","tiger:zip_right":"94121","trolley_wire":"yes"},{"_way_id":"162799061","highway":"secondary","lanes":"4","lanes:backward":"2","lanes:forward":"2","maxspeed":"35 mph","name":"Fulton Street","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"Fulton","tiger:name_type":"St","tiger:zip_left":"94117","tiger:zip_right":"94121","trolley_wire":"yes"},{"_way_id":"396977055","highway":"tertiary","lanes":"3","lanes:backward":"2","lanes:forward":"1","lcn_ref":"330","name":"8th Avenue","placement:backward":"left_of:1","tiger:county":"San Francisco, CA","turn:lanes:backward":"left|"},{"_way_id":"30903947","highway":"tertiary","lanes":"3","lanes:backward":"1","lanes:forward":"2","lcn_ref":"330","name":"8th Avenue","placement:backward":"left_of:1","tiger:county":"San Francisco, CA","turn:lanes:forward":"left|right"},{"_way_id":"255166633","cycleway":"lane","highway":"tertiary","lanes":"4","lanes:backward":"2","lanes:forward":"2","lcn_ref":"30","maxspeed":"25 mph","name":"John F Kennedy Drive","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"John F Kennedy","tiger:name_type":"Dr"},{"_way_id":"396977056","cycleway":"lane","highway":"tertiary","lanes":"4","lanes:backward":"2","lanes:forward":"2","lcn_ref":"30","maxspeed":"25 mph","name":"John F Kennedy Drive","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"John F Kennedy","tiger:name_type":"Dr"},{"_way_id":"27031304","cycleway":"lane","highway":"tertiary","lanes":"2","lanes:backward":"1","lanes:forward":"1","lcn_ref":"30","maxspeed":"25 mph","name":"John F Kennedy Drive","tiger:cfcc":"A41","tiger:county":"San Francisco, CA","tiger:name_base":"John F Kennedy","tiger:name_type":"Dr"},{"_way_id":"302406180","highway":"unclassified","lcn_ref":"75","name":"Transverse Drive","tiger:county":"San Francisco, CA"},{"_way_id":"158602262","highway":"unclassified","lcn_ref":"34","maxspeed":"25 mph","name":"Middle Drive West","tiger:county":"San Francisco, CA"}]}
real	0m0.023s
user	0m0.005s
sys	0m0.006s
```

The current response structure is:

```
{ "way_indexes" : [ null, 0, 1, 2, 2, 3, ... ],
  "ways_seen" : [ {"highway" : "primary", ... }, {"higway" : "motorway", ...} , ... ]
}
```

The `way_indexes` values are indexes into the `ways_seen` array.  There is one entry in `way_indexes` for each sequential pair of coordinates/nodes supplied in the original request.

# TODO
  - [x] Consolidate consecutive node id pairs that are on the same way and only report tags once (include the range of node indexes that are part of each way in the response)
  - [ ] Figure out how to handle node pairs that belong to multiple ways
  - [x] Perhaps implement an RTree, and allow the matching to be done by lon/lat instead of requiring node ids (DONE: you can now search by coordinate)
  - [x] Gracefully handle missing pairs, so that we can honour a request from a .OSM file that is slightly out of sync with the one we're serving
  - [ ] Startup progress indicator
  - [ ] Parallelize OSM parsing at the start
  - [x] Improve the `nodepair_t` hash function to improve performance
  - [x] Figure out why we can only handle 123 characters on the URL (it was `int` vs `ulong_long`)
