Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
  
  //Publish phone ready event
  Pebble.sendAppMessage({'AppKeyJSReady': 1});
});

Pebble.addEventListener('appmessage', function(e) {
  var dict = e.payload;

  console.log('Got message: ' + JSON.stringify(dict));
  // Create the request
  var request = new XMLHttpRequest();
  
  request.onload = function() {
    console.log('Got response: ' + this.responseText);
    try {
      var json = JSON.parse(this.responseText);
      Pebble.sendAppMessage(json);
    } catch(err) {
      Pebble.sendAppMessage({'AppKeyResponseFailure': 1});
      console.log('Error parsing JSON response!');
    }
  };
  
  request.open('POST', 'https://secure.vilant.com/test.php');
  request.setRequestHeader("Content-type", "application/json");
  request.send(JSON.stringify(dict));
  
});