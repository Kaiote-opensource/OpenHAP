const SOCKET_URL = "ws://192.168.4.1:80/status.cgi";
const LOG = true

var sample = {
  "HUM": 67,
  "TEMP": 99,
  "PM2.5": 8,
  "TIME": 19,
  "RTC_BATT": 10,
  "MAIN BATT_BATT": 68,
  "SD CARD": "DISCONNECTED"
}

console.log(JSON.parse(sample))
/***
 * Socket Management Library
 */
var websocketInst = new WebSocket(SOCKET_URL);

var socket = {
  init: function () {

  },
  onOpen: function (evt) {
    helpers.log(evt)
  },
  onClose: function (evt) {
    helpers.log(evt)
  },
  onMessage: function (evt) {
    helpers.log(evt)
    var responses = JSON.parse(evt)
    document.getElementById("pm25-val").innerHTML = responses.PM25 + "μg/m3"
    document.getElementById("temp-val").innerHTML = responses.TEMP + "C"
    document.getElementById("hum-val").innerHTML = responses.HUM + ""
    document.getElementById("main-bat-val").innerHTML = responses.MAIN_BATT + ""
    document.getElementById("sd-card-val").innerHTML = responses.SD_CARD + ""
  },
  onError: function (evt) {
    helpers.log(evt)
  },
  startMeasuring: function () {
    websocketInst.send(JSON.stringify({
      MEASUREMENT: true
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

websocketInst.onclose = function (evt) {
  socket.onClose(evt)
};
