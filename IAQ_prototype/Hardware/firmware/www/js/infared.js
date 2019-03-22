const SOCKET_URL = "ws://192.168.4.1:80/infared.cgi";
// const SOCKET_URL = "ws://localhost:8999";

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
        var response = JSON.parse(evt)
        document.getElementById("conversion-frame-rate").innerHTML = response.CONVERSION_FRAMERATE + ""
        document.getElementById("t-min").innerHTML = response.TMIN + ""
        document.getElementById("t-max").innerHTML = response.TMAX + ""
        socket_responses.imaging(response.IMAGE);
        helpers.log(evt)
    },
    onError: function (evt) {
        helpers.log(evt)
    }

}


var socket_responses = {
    imaging: function (image) {
        document.getElementById("infrared-image").src = "data:image/png;base64," + image;
      
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



