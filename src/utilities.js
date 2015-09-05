
function HTTPGET(url) {
    var req = new XMLHttpRequest();
    req.open("GET", url, false);
    req.send(null);
    return req.responseText;
}

function timeToMinutes(time) {
       var c = time.split(':');
       if (c.length==2) {
         return parseInt(c[0]) * 60 + parseInt(c[1]);
       } else {
         return 0;
       }
}

function trimStopName(stopName, max) {
  stopName = stopName.split(',')[0];  // remove information about kommun

  return stopName.substr(0,max);
}



// From https://gist.github.com/matthewtole/49da38377b0b0a423407
  function GColorFromHex(hex) {
    var hexNum = parseInt(hex, 16);
    var a = 192;
    var r = (((hexNum >> 16) & 0xFF) >> 6) << 4;
    var g = (((hexNum >>  8) & 0xFF) >> 6) << 2;
    var b = (((hexNum >>  0) & 0xFF) >> 6) << 0;
    return a + r + g + b;
  }
