const SOCKET_URL = "ws://192.168.4.1:80";
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

    document.getElementById("pm25-val").innerHTML = evt["PM2.5"] + "μg/m3"
    document.getElementById("temp-val").innerHTML = evt.TEMP + "C"
    document.getElementById("hum-val").innerHTML = evt.HUM + ""
  },
  onError: function (evt) {
    helpers.log(evt)
  },
  startMeasuring: function () {
    websocketInst.send({
      MEASUREMENT: true
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

websocketInst.onclose = function (evt) {
  socket.onClose(evt)
};
