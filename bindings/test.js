const pkg = require('./index');

const annotator = new pkg.Annotator(42);


console.log('Begin');

// Sync. member function call, blocks
//console.log(annotator.getValue());

// Async. member function call, does not block, pass callback
annotator.getValueAsync(function (err, result) {
  if (err)
    console.log(err);
  else
    console.log('Success: ' + result);
});

console.log('End');
