const SOCKET_URL = "ws://192.168.4.1:80/infrared.cgi";
const LOG = true



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
        if (evt.IMAGE) {
            socket_responses.imaging(evt.IMAGE);
        }
        helpers.log(evt)


    },
    onError: function (evt) {
        helpers.log(evt)
    }

}


var socket_responses = {
    imaging: function (image) {
        // infrared-image
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



