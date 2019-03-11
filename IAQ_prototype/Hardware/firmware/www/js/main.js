// const SOCKET_URL = "wss://echo.websocket.org";
const SOCKET_URL = "ws://192.168.4.1:80";
const LOG = true

var urls = {
  "home": "dashboard.html",
  "login": "index.html",
  "advanced": "advanced.html",
  "system": "system.html"
}


var _kaiote_handler = {
  /**
   * Handle login on button click
   */
  login: function () {
    document.getElementById("login-loading").style.display = "inline-block";
    helpers.redirect("home")
  },
  logout: function () {
    helpers.redirect("login")
  },
  show_div: function (name) {
    // socket.name
  },

}



var helpers = {
  redirect: function (page) {
    window.location.replace(urls[page])
  },
  log(data) {
    if (LOG == true) {
      console.log(data)
    }

  }
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

  },
  onError: function (evt) {
    helpers.log(evt)
  }

}

/**
 * Web socket hanling methods
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



