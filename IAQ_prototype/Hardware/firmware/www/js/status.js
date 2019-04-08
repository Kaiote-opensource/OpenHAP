// const SOCKET_URL = "ws://192.168.4.1:80/status.cgi";
const SOCKET_URL = "ws://localhost:8999";

/***
 * Socket Management Library
 */
var websocketInst = new WebSocket(SOCKET_URL);

var device_time;
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
      // width: 350,
      height: 330,
      margin: {
        l: 45,
        r: 30,
        b: 60,
        t: 60,
        pad: 20
      },
      xaxis: {
        autorange: true,
        showgrid: false,
        zeroline: false,
        showline: false,
        autotick: true,
        ticks: '',
        showticklabels: false,
        fixedrange: true
      },
      displayModeBar: false,
    }, { showSendToCloud: false });

    Plotly.newPlot('humidity-graph', Hdata, {
      height: 330,
      margin: {
        l: 45,
        r: 30,
        b: 60,
        t: 60,
        pad: 20
      },

      xaxis: {
        autorange: true,
        showgrid: false,
        zeroline: false,
        showline: false,
        autotick: true,
        ticks: '',
        showticklabels: false,
        fixedrange: true
      },
      displayModeBar: false,
    }, { showSendToCloud: false });

    Plotly.newPlot('pm25-graph', Pdata, {
      height: 330,
      margin: {
        l: 45,
        r: 30,
        b: 60,
        t: 60,
        pad: 20
      },
      xaxis: {
        autorange: true,
        showgrid: false,
        zeroline: false,
        showline: false,
        autotick: true,
        ticks: '',
        showticklabels: false,
        fixedrange: true
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


/**
 * Populate values on the required DOM Elements
 */
var socket_responses = {
  socketResponses: function (responses) {
    /***
     * Start Measurement Response
     */
    try {
      if ('SET_TIME' in responses) {
        _kaiote_handler.toast({
          type: "success",
          duration: 2000,
          message: "Time Set"
        })

      }

      /**
       * Start Measurement Response
       */
      if ('START_MEASUREMENT' in responses) {
        _kaiote_handler.toast({
          type: "success",
          duration: 2000,
          message: "Success"
        })

      }
    } catch (error) {

    }
  },
  populate: function (responses) {
    // document.getElementById("pm25-val").innerHTML = responses.PM25 + "μg/m3"
    // document.getElementById("temp-val").innerHTML = responses.TEMP + "C"
    // document.getElementById("hum-val").innerHTML = responses.HUM + ""

    if (responses.RTC_BATT > 50) {

      document.getElementById("main-bat-val").innerHTML = '<div class="progress" style="width:100%;">' +
        '<div class="progress-bar bg-success" role="progressbar" style="width: '
        + responses.RTC_BATT +
        '%" aria-valuenow="' + responses.RTC_BATT +
        '" aria-valuemin="0" aria-valuemax="100">' +
        responses.RTC_BATT + "%" +
        '</div>' +
        '</div>';

    } else if (responses.RTC_BATT <= 25) {

      document.getElementById("main-bat-val").innerHTML = '<div class="progress" style="width:100%;">' +
        '<div class="progress-bar bg-danger" role="progressbar" style="width: ' +
        responses.RTC_BATT +
        '%" aria-valuenow="' +
        responses.RTC_BATT +
        '" aria-valuemin="0" aria-valuemax="100">' +
        responses.RTC_BATT + "%" +
        '</div>' +
        '</div>';

    } else {

      document.getElementById("main-bat-val").innerHTML = '<div class="progress" style="width:100%;">' +
        '<div class="progress-bar bg-warning" role="progressbar" style="width: ' +
        responses.RTC_BATT +
        '%" aria-valuenow="' +
        responses.RTC_BATT +
        '" aria-valuemin="0" aria-valuemax="100">' +
        responses.RTC_BATT + "%" +
        '</div>' +
        '</div>';
    }




    document.getElementById("device-time").innerHTML = _kaiote_handler.loadTimeFromUnixTime(responses.DEVICE_TIME)
    device_time = responses.DEVICE_TIME

    document.getElementById("sd-card-val").innerHTML = responses.SD_CARD + ""

    if (responses.SD_CARD == "DISCONNECTED") {

      document.getElementById("sd-card-val").innerHTML = '<button type="button" class="btn btn-danger">DISCONNECTED</button>';

    } else {

      document.getElementById("sd-card-val").innerHTML = '<button type="button" class="btn btn-success">CONNECTED</button>';

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


    if (tempe_data.x.length >= 1) {
      _kaiote_handler.playSound()
      _kaiote_handler.hide("graph-loader")
      graphs.load()
    }


  },

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
    var responses = JSON.parse(evt)
    socket_responses.populate(responses)
    socket_responses.socketResponses(responses)
  },
  onError: function (evt) {
    helpers.log(evt)
  },
  startMeasuring: function () {
    var t1 = _kaiote_handler.currentTime()
    var t2 = device_time
    var difference = _kaiote_handler.getTimeDifference(t1, t2)
    // console.log(_kaiote_handler.getTimeDifference(t1, t2))
    if (difference < 30) {
      websocketInst.send(JSON.stringify({
        START_MEASUREMENT: {
          VALUE: 1,
          UID: _kaiote_handler.uuidv4()
        }
      }))
    } else {
      _kaiote_handler.toast({
        type: "error",
        duration: 2000,
        message: "Time error! Failed"
      })
    }
  },
  sendTime() {
    websocketInst.send(JSON.stringify({
      SET_TIME: {
        VALUE: _kaiote_handler.currentTime(),
        UID: _kaiote_handler.uuidv4()
      }
    }))
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
