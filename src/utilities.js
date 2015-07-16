
// From https://gist.github.com/matthewtole/49da38377b0b0a423407
  function GColorFromHex(hex) {
    var hexNum = parseInt(hex, 16);
    var a = 192;
    var r = (((hexNum >> 16) & 0xFF) >> 6) << 4;
    var g = (((hexNum >>  8) & 0xFF) >> 6) << 2;
    var b = (((hexNum >>  0) & 0xFF) >> 6) << 0;
    return a + r + g + b;
  }
