


function HTTPGET(url) {
    var req = new XMLHttpRequest();
    req.open("GET", url, false);
    req.send(null);
    return req.responseText;
}

function timeToMinutes(time) {
       var c = time.split(':');
       return parseInt(c[0]) * 60 + parseInt(c[1]);
}

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
    
      this.servertime_text = json.DepartureBoard.servertime;
    
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

    console.log("Got departures:" + JSON.stringify(this.departure_list));
    
    this.servertime = timeToMinutes(this.servertime_text);
      
    if (this.departure_list.length === 0) {
      // No departures
      MessageQueue.sendAppMessage({"KEY_DEPARTUREBOARD_STATUS": 2})
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

    var stop = this.departure_list[0].stop.split(',')[0];
    MessageQueue.sendAppMessage({"KEY_DEPARTUREBOARD_NAME": stop});
    //MessageQueue.sendAppMessage({"KEY_DEPARTUREBOARD_SIZE": departures.length});
    for(i=0;i<this.departure_list.length;i++) {
        departure = this.departure_list[i];
      
        var time = "";
        if (departure.rtTime) {
          if (departure.calcTime === 0 || departure.calcTime === -1) time = "nu";
          else  time = String(departure.calcTime) + " min";
        } else {
            time = "~" + String(departure.calcTime) + " min";
        }

        MessageQueue.sendAppMessage({"KEY_DEPARTUREBOARD_ROUTENR": departure.sname});
        MessageQueue.sendAppMessage({"KEY_DEPARTUREBOARD_DIRECTION": departure.direction});
        MessageQueue.sendAppMessage({"KEY_DEPARTUREBOARD_TIME": time});
        MessageQueue.sendAppMessage({"KEY_DEPARTUREBOARD_FGCOLOR": GColorFromHex(departure.fgColor.substr(1))});
        MessageQueue.sendAppMessage({"KEY_DEPARTUREBOARD_BGCOLOR": GColorFromHex(departure.bgColor.substr(1))});
      
      console.log(departure.sname + "  " + departure.direction + ": " + time );
    }
    MessageQueue.sendAppMessage({"KEY_DEPARTUREBOARD_COMPLETE": this.servertime_text});

  },
};

var favoriteStops = {
  settings_timestamp: 6,    // Timestamp for latest setting done, seconds since 1 January 1970 00:00:00 UTC
  
  stops: [
    {stopName: "Borås resecentrum", stopId: "9021014082017000", 
     directionId: ["9021014082500000","9022014082053001"],
     directionName: ["Byttorpsklint", "Fjällgatan"] },
    {stopName: "Delsjömotet", stopId: "9022014002043002", 
     directionId: ["9021014082017000"], 
     directionName: ["Borås resecentrum"]},
    {stopName: "Fjällkroken", stopId: "9021014082689000", 
     directionId: ["9021014082017000"], 
     directionName: ["Borås resecentrum"]},
    {stopName: "Fjällgatan", stopId: "9021014082053000", 
     directionId: ["9021014082017000"], 
     directionName: ["Borås resecentrum"]},
    {stopName: "Södra Torget", stopId: "9022014082087020", 
     directionId: [],
     directionName: []},
    {stopName: "Korsvägen", stopId: "9021014003980000", 
     directionId: ["9021014082017000","9022014012153001","9021014012110000"],
     directionName: ["Borås resecentrum", "Bergfotsgatan", "Mölndal centrum"]},
    
  ],
    
    sendList: function() {
      console.log("Start sending favorites");
      MessageQueue.sendAppMessage({"KEY_FAVORITES_INIT": 1});
      for (var i=0; i<this.stops.length; i++) {
        MessageQueue.sendAppMessage({"KEY_FAVORITES_NAME":this.stops[i].stopName});
        for (var j=0; j<this.stops[i].directionName.length; j++) {
          MessageQueue.sendAppMessage({"KEY_FAVORITES_DIRECTION":"» " + this.stops[i].directionName[j]});
        }
      }
      MessageQueue.sendAppMessage({"KEY_FAVORITES_COMPLETE": this.settings_timestamp}); 
      console.log("Completed sending favorites");
    },
    
    getDepartures: function(i) {
      console.log("Get departureboard for favorite " + i);
      if (i<this.stops.length) {
        departures.getDepartures(this.stops[i].stopId, this.stops[i].directionId);
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
            //"&originCoordLat=57.719556&originCoordLong=12.902289" +
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
    MessageQueue.sendAppMessage({"KEY_NEARBY_NAME":stops[i].name});
    MessageQueue.sendAppMessage({"KEY_NEARBY_DISTANCE": (Math.round(stops[i].calcDist/100)*100) + "m"});
    MessageQueue.sendAppMessage({"KEY_NEARBY_ID":stops[i].id});
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
    MessageQueue.sendAppMessage({"KEY_PHONE_STARTUP":favoriteStops.settings_timestamp});
    //Watchapp opened, send stop list
    //favoriteStations.sendList();
    
    //Get nearby stations
    gpsRetry = 0;
    console.log("Try to get gps position...");
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