const SOCKET_URL = "ws://192.168.4.1:80/status.cgi";
// const SOCKET_URL = "ws://localhost:8999";

// _kaiote_handler.toast()
/***
 * Socket Management Library
 */
var websocketInst = new WebSocket(SOCKET_URL);
var tempe_data = {
  x: [],
  y: [],
  type: 'scatter'
}

var humidity_data = {
  x: [],
  y: [],
  type: 'scatter'
}

var pm25_data = {
  x: [],
  y: [],
  type: 'scatter'
}


var graphs = {
  load: function () {

    var Tdata = [tempe_data];
    var Hdata = [humidity_data];
    var Pdata = [pm25_data]

    Plotly.newPlot('temp-graph', Tdata, {
      xaxis: {
        title: 'Temperature'
      },
      displayModeBar: false,
    }, { showSendToCloud: false });

    Plotly.newPlot('humidity-graph', Hdata, {
      xaxis: {
        title: 'Humidity'
      },
      displayModeBar: false,
    }, { showSendToCloud: false });

    Plotly.newPlot('pm25-graph', Pdata, {
      xaxis: {
        title: 'Particulate Matter'
      },
      displayModeBar: false,
    }, { showSendToCloud: false });
  },
  keepUnderMin: function (data, threshold) {
    if (data.x.length > threshold) {
      data.x.shift()
      data.y.shift()
    }
    return data
  },

}


// graphs.temperature()

/**
 * Populate values on the required DOM Elements
 */
var socket_responses = {
  populate: function (responses) {
    // document.getElementById("pm25-val").innerHTML = responses.PM25 + "μg/m3"
    // document.getElementById("temp-val").innerHTML = responses.TEMP + "C"
    // document.getElementById("hum-val").innerHTML = responses.HUM + ""
    document.getElementById("main-bat-val").innerHTML = '<div class="progress" style="width:100%;">' +
      '<div class="progress-bar bg-success" role="progressbar" style="width: ' + responses.MAIN_BATT + '%" aria-valuenow="' + responses.MAIN_BATT + '" aria-valuemin="0" aria-valuemax="100"></div>' +
      '</div>';

    document.getElementById("sd-card-val").innerHTML = responses.SD_CARD + ""

    if (responses.SD_CARD == "DISCONNECTED") {
      document.getElementById("sd-card-val").style = "background-color:red; border-radius:5px; color:#ffffff; width:80%";
    } else {
      document.getElementById("sd-card-val").style = "background-color:green; border-radius:5px; color:#ffffff;width:80%";
    }

    var today = new Date();
    var time = today.getHours() + ":" + today.getMinutes() + ":" + today.getSeconds();

    tempe_data.x.push(time)
    tempe_data.y.push(responses.TEMP)

    humidity_data.x.push(time)
    humidity_data.y.push(responses.HUM)

    pm25_data.x.push(time)
    pm25_data.y.push(responses.PM25)

    tempe_data = graphs.keepUnderMin(tempe_data, 15)
    humidity_data = graphs.keepUnderMin(humidity_data, 15)
    pm25_data = graphs.keepUnderMin(pm25_data, 15)
    if (tempe_data.x.length <= 0) {
      _kaiote_handler.show("graph-loader")
    }
    if (tempe_data.x.length >= 5) {
      _kaiote_handler.playSound()
      _kaiote_handler.hide("graph-loader")
      graphs.load()
    }
  }
}


var socket = {
  init: function () {

  },
  onOpen: function (evt) {
    helpers.log(evt)
  },
  ony: function (evt) {
    helpers.log(evt)
  },
  onMessage: function (evt) {
    // helpers.log(evt)
    var responses = JSON.parse(evt)
    socket_responses.populate(responses)
  },
  onError: function (evt) {
    helpers.log(evt)
  },
  startMeasuring: function () {

    websocketInst.send(JSON.stringify({
      MEASUREMENT: 1
    }))

    _kaiote_handler.toast({
      type: "success",
      duration: 5000,
      message: "success"
    })


  }
}



/**
 * Web socket handling methods
 */
websocketInst.onopen = function (evt) {
  socket.onOpen(evt)
};

websocketInst.onmessage = function (evt) {
  socket.onMessage(evt.data)
};
websocketInst.onerror = function (evt) {
  socket.onError(evt)
};

websocketInst.ony = function (evt) {
  socket.ony(evt)
};
