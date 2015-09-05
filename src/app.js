



var gpsRetry = 0;

var departures = {
  departure_list: [],
  servertime: 0,
  servertime_text: "",

  loadDepartures: function(stopId, directionId) {
      var httpstring = "http://api.vasttrafik.se/bin/rest.exe/v1/departureBoard?" +
                "authKey=" + authKey + "&format=json" +
                  "&id="+stopId;
      if (directionId !== "") httpstring += "&direction="+directionId;
      httpstring += "&timeSpan=60&maxDeparturesPerLine=20";

      console.log("Get departures: " + httpstring);

      var response = HTTPGET(httpstring);
      // TODO: handle no response?

      var json = JSON.parse(response);

      if (json.DepartureBoard.servertime!==undefined && json.DepartureBoard.servertime!==null) this.servertime_text = json.DepartureBoard.servertime;

      if (json.DepartureBoard.Departure===null || json.DepartureBoard.Departure===undefined) {
        // No departures
        console.log("No departures found");
        return;
      }

      //console.log("Found departures: " + JSON.stringify(json.DepartureBoard.Departure));
      this.departure_list = this.departure_list.concat(json.DepartureBoard.Departure);
      //console.log("Departures found so far: " + JSON.stringify(this.departure_list));
      return;

  },

  getDepartures: function(stopID, direction) {
    var i;
    this.departure_list = [];

    console.log("Get departures for stopId " + stopID + ", directions: " + JSON.stringify(direction));

    if (direction.length === 0) {
      console.log("Get departures without direction");
      this.loadDepartures(stopID, "");
    } else {
      for (i=0; i<direction.length; i++) {
        this.loadDepartures(stopID, direction[i]);
      }
    }


    this.servertime = timeToMinutes(this.servertime_text);

    if (this.departure_list.length == 0) {
      // No departures
      MessageQueue.sendAppMessage({"KEY_DEPARTUREBOARD_STATUS": 2});
      return;
    }

    var departure;

    //Calculate times before sorting
    for(i=0; i<this.departure_list.length;i++) {
      departure = this.departure_list[i];

     if (departure.rtTime)
        this.departure_list[i].calcTime = timeToMinutes(departure.rtTime) - this.servertime - 1;
      else
        this.departure_list[i].calcTime = timeToMinutes(departure.time) - this.servertime - 1;

      //dirty fix for midnight handling
      if (this.departure_list[i].calcTime<-10) this.departure_list[i].calcTime+=1440;
    }

    //Sort by time first, then line. Make an option?
    this.departure_list.sort(function(a, b) {
        var x = parseInt(a.sname); var y = parseInt(b.sname);
        if (a.calcTime<b.calcTime) return -1;
        if (a.calcTime>b.calcTime) return 1;
        return ((x < y) ? -1 : ((x > y) ? 1 : 0));
    });

    var stop = trimStopName(this.departure_list[0].stop,22);
    MessageQueue.sendAppMessage({"KEY_DEPARTUREBOARD_NAME": stop});
    //MessageQueue.sendAppMessage({"KEY_DEPARTUREBOARD_SIZE": departures.length});
    for(i=0;i<this.departure_list.length;i++) {
        departure = this.departure_list[i];

        var time = "";
        if (departure.rtTime) {
          if (departure.calcTime === 0 || departure.calcTime === -1) time = "now";
          else  time = String(departure.calcTime) + " min";
        } else {
            time = "~" + String(departure.calcTime) + " min";
        }

        MessageQueue.sendAppMessage({"KEY_DEPARTUREBOARD_ROUTENR": departure.sname,
          "KEY_DEPARTUREBOARD_DIRECTION": departure.direction,
          "KEY_DEPARTUREBOARD_TIME": time,
          "KEY_DEPARTUREBOARD_FGCOLOR": GColorFromHex(departure.fgColor.substr(1)),
          "KEY_DEPARTUREBOARD_BGCOLOR": GColorFromHex(departure.bgColor.substr(1))
          });

      console.log(departure.sname + "  " + departure.direction + ": " + time );
    }
    MessageQueue.sendAppMessage({"KEY_DEPARTUREBOARD_COMPLETE": this.servertime_text});

  },
};

function getStopByName(inputName) {
  var httpstring = "http://api.vasttrafik.se/bin/rest.exe/v1/location.name?" +
            "authKey=" + authKey + "&format=json" +
            //"&originCoordLat=57.719556&originCoordLong=12.902289" +
            "&input=" + inputName;
  console.log('Request named stops:' + httpstring);
  var response = HTTPGET(httpstring);
  //Convert to JSON
  var json = JSON.parse(response);
  if (json.LocationList.StopLocation===null || json.LocationList.StopLocation===undefined ) {
    console.log("No stop found called: " + inputName);
    return false;
  }

  var stop = json.LocationList.StopLocation[0];

  return stop;
}

var favoriteStops = {

  getTimestamp: function() {
    var t = localStorage.getItem('timestamp');
    if (t===null || t===undefined) t=0;
    return t;
  },

  setTimestamp: function() {
    localStorage.setItem('timestamp',Math.round(Date.now() / 1000));
  },

  getNrFavorites: function() {
    var n = localStorage.getItem('nrFavorites');
    if (n>0) return n;
    else return 0;
  },

  getNrDirections: function(stop) {
    var n = localStorage.getItem('nrDirections_'+stop);
    if (n>0) return n;
    else return 0;
  },

  seralize: function() {
    var t = "";
    var n = this.getNrFavorites();
    var nd;
    var temp;
    for (var i=0; i < n ; i++) {
      nd = this.getNrDirections(i);
      temp = localStorage.getItem('favoritestop_'+i);
      if (temp!==null && temp!==undefined && temp.length>0) {
        t += 'favoritestop_' + i + '=' + temp + '&';
        for (var j=0; j < nd; j++) {
          temp = localStorage.getItem('favoritedirection_'+i+'_'+j);
          if (temp!==null && temp!==undefined && temp.length>0) {
            t += 'favoritedirection_' + i + '_' + j + '=' + temp + '&';
          }
        }
      }
    }

    if (t.length===0) return t;
    return t.substring(0,t.length-1);
  },

  saveConfiguration: function(config) {
    this.setTimestamp();
    var ns = 0;
    var nd = 0;
    var s;

    for (var i=0; i<10; i++) {
      if (config['favoritestop_'+i]!==null && config['favoritestop_'+i]!==undefined && config['favoritestop_'+i].length>0) {
        s = getStopByName(config['favoritestop_'+i]);
        if (s!==false) {
          localStorage.setItem('favoritestop_'+i,s.name);
          localStorage.setItem('favoritestopid_'+i,s.id);
          for (var j=0; j<=2; j++) {
            if (config['favoritedirection_'+i+'_'+j]!==null && config['favoritedirection_'+i+'_'+j]!==undefined && config['favoritedirection_'+i+'_'+j].length>0) {
              s = getStopByName(config['favoritedirection_'+i+'_'+j]);
              if (s!==false) {
                localStorage.setItem('favoritedirection_'+i+'_'+j,s.name);
                localStorage.setItem('favoritedirectionid_'+i+'_'+j,s.id);
                nd++;
              }
            }
          }
          localStorage.setItem('nrDirections_'+ns,nd);
          ns++;
          nd = 0;
        }
      }
    }
    localStorage.setItem('nrFavorites',ns);

    this.sendList();
  },

    sendList: function() {
      console.log("Start sending favorites");
      var n, nd;
      MessageQueue.sendAppMessage({"KEY_FAVORITES_INIT": 1});
      n = this.getNrFavorites();
      for (var i=0; i<n; i++) {
        nd = this.getNrDirections(i);
        MessageQueue.sendAppMessage({"KEY_FAVORITES_NAME":trimStopName(localStorage.getItem('favoritestop_'+i),17) });
        for (var j=0; j<nd; j++) {
          MessageQueue.sendAppMessage({"KEY_FAVORITES_DIRECTION":"» " + trimStopName(localStorage.getItem('favoritedirection_'+i+'_'+j),19) });
        }
      }
      MessageQueue.sendAppMessage({"KEY_FAVORITES_COMPLETE": this.getTimestamp()});
      console.log("Completed sending favorites");
    },

    getDepartures: function(i) {
      console.log("Get departureboard for favorite " + i);
      var dirs = [];
      if (i<this.getNrFavorites()) {
        for (var j=0; j<this.getNrDirections(i); j++) {
          dirs = dirs.concat(localStorage.getItem('favoritedirectionid_'+i+'_'+j));
        }
        departures.getDepartures(localStorage.getItem('favoritestopid_'+i), dirs);
      }
    }
};

var locationOptions = {
  enableHighAccuracy: true,
  maximumAge: 30000,
  timeout: 60000
};

function getNearbyStations(pos) {
  var httpstring = "http://api.vasttrafik.se/bin/rest.exe/v1/location.nearbystops?" +
            "authKey=" + authKey + "&format=json" +
            "&originCoordLat=" + pos.coords.latitude + "&originCoordLong=" + pos.coords.longitude +
            "&maxNo=40&maxDist=3000";
  console.log('Request nearby stops:' + httpstring);
    var response = HTTPGET(httpstring);
    //Convert to JSON
    var json = JSON.parse(response);

  if (!json.LocationList.StopLocation) {
    console.log("No nearby stops");
    MessageQueue.sendAppMessage({"KEY_NEARBY_STATUS":4});
    return;
  }
  var stops = json.LocationList.StopLocation;
  var checkDuplicates = [];

  var temp;
  for (var i=0; i<stops.length; i++) {
    if (stops[i].name === null || stops[i].name === undefined) {
      console.log("Stop without name");
      stops.splice(i,1);
    }
    else if (checkDuplicates.indexOf(stops[i].name)>=0) {
      // Only interested in each stop once
      temp = stops.splice(i,1);
      i--;
    }
    else {
      checkDuplicates.push(stops[i].name);
      stops[i].calcDist = ( getDistance(pos.coords.latitude,pos.coords.longitude,parseFloat(stops[i].lat),parseFloat(stops[i].lon)));
      //stops[i].calcDist = ( getDistance(57.719556,12.902289,parseFloat(stops[i].lat),parseFloat(stops[i].lon)));
      //console.log("Stop: " + stops[i].name + ". Calculated distance:" + stops[i].calcDist);
    }
  }
  /** Shall already be received in order from api
  stops.sort(function(a, b) {
    return ((a.calcDist < b.calcDist) ? -1 : ((a.calcDist > b.calcDist) ? 1 : 0));
  });
  */

  MessageQueue.sendAppMessage({"KEY_NEARBY_INIT":1});

  for (i=0; i<stops.length && i<10; i++) {
    //console.log("Nearby "+i+": " + JSON.stringify(stops[i]));
    MessageQueue.sendAppMessage({"KEY_NEARBY_NAME":trimStopName(stops[i].name,17),
      "KEY_NEARBY_DISTANCE": (Math.round(stops[i].calcDist/100)*100) + "m",
      "KEY_NEARBY_ID":stops[i].id
      });
  }
  MessageQueue.sendAppMessage({"KEY_NEARBY_COMPLETE":i+1});

}

function locationNotAvailable(err) {
  if (gpsRetry<3) {
    console.log("No gps position, retrying...");
    gpsRetry++;
    MessageQueue.sendAppMessage({"KEY_NEARBY_STATUS":2});
    navigator.geolocation.getCurrentPosition(getNearbyStations,locationNotAvailable, locationOptions);
  }
  else {
    MessageQueue.sendAppMessage({"KEY_NEARBY_STATUS":3});
    console.log("No gps position available");
  }
}

Pebble.addEventListener("ready",
  function(e) {
    MessageQueue.sendAppMessage({"KEY_PHONE_STARTUP":favoriteStops.getTimestamp()});
    //Watchapp opened, send stop list
    //favoriteStations.sendList();

    //Get nearby stations
    gpsRetry = 0;
    navigator.geolocation.getCurrentPosition(getNearbyStations,locationNotAvailable, locationOptions);
  }
);

Pebble.addEventListener("appmessage",
  function(e) {
    console.log('Received message: ' + JSON.stringify(e.payload));
    if (e.payload.KEY_REQUEST_FAVORITE_DEPARTUREBOARD !== null && e.payload.KEY_REQUEST_FAVORITE_DEPARTUREBOARD !==undefined) {
      favoriteStops.getDepartures(e.payload.KEY_REQUEST_FAVORITE_DEPARTUREBOARD);
    }
    if (e.payload.KEY_REQUEST_NEARBY_DEPARTUREBOARD !== null && e.payload.KEY_REQUEST_NEARBY_DEPARTUREBOARD!==undefined) {
      console.log("Get departures for stopID: " + e.payload.KEY_REQUEST_NEARBY_DEPARTUREBOARD);
      departures.getDepartures(e.payload.KEY_REQUEST_NEARBY_DEPARTUREBOARD,[]);
    }
    if (e.payload.KEY_REQUEST_NEARBY_STOPS !== null && e.payload.KEY_REQUEST_NEARBY_STOPS!==undefined) {
      gpsRetry = 0;
      navigator.geolocation.getCurrentPosition(getNearbyStations,locationNotAvailable, locationOptions);
    }
    if (e.payload.KEY_REQUEST_SETTINGS !== null && e.payload.KEY_REQUEST_SETTINGS!==undefined) {
      //if (e.payload.KEY_REQUEST_SETTINGS < favoriteStations.settings_timestamp) {
        favoriteStops.sendList();
      //}
    }
  }
);

Pebble.addEventListener("showConfiguration",
  function(e) {
    Pebble.openURL("http://davidsvantesson.github.io/vasttid.html?"+favoriteStops.seralize());
  }
);

Pebble.addEventListener("webviewclosed",
  function(e) {
    var fullConfiguration = JSON.parse(decodeURIComponent(e.response));
    console.log("Configuration window returned: " + JSON.stringify(fullConfiguration));
    if (fullConfiguration['save']==1) favoriteStops.saveConfiguration(fullConfiguration);
  }
);
