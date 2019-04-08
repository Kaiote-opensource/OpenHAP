// const SOCKET_URL = "ws://192.168.4.1:80/infared.cgi";
const SOCKET_URL = "ws://localhost:8999";
//My Password : $2y$10$Cn8pGbZtpRL8l7supw6FR.ubwn5gu/m4Z5Qvl8lQASv1/mR55Do1i
// Oyoundis Password : $2y$10$IJe3riEyQdac99pbupozFOh620HBO1fFyxmsFKSiXRFwPn6Tax4DG
/***
 * Socket Management Library
 */
var websocketInst = new WebSocket(SOCKET_URL);

var infared_responses = {
    imageFloating: [],
    imageData: function () {
        return this.dataSplit(this.imageFloating);
    },
    imaging: function () {
        var data = [{
            z: this.imageData().reverse(),
            colorscale: 'Bluered',
            type: 'heatmap',
            zsmooth: 'best',
        }];
        var layout = {
            hoverlabel: {
                bgcolor: 'white',
                font: {
                    color: 'white',
                    size: '20px'
                }
            }
        };
        Plotly.newPlot('infrared-image', data, layout);

    },
    dataSplit: function (imgData) {
        var holder = []
        for (let i = 0; i < 768;) {
            var out = []
            for (let index = i; index < (i + 32); index++) {
                const element = imgData[index];
                out.push(element)
            }
            holder.push(out)
            i += 32
        }
        return holder;
    }
}


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
        infared_responses.imageFloating = response.CAMERA_DATA
        document.getElementById("conversion-frame-rate").innerHTML = response.CONVERSION_FRAMERATE + ""
        document.getElementById("t-min").innerHTML = infared_responses.imageFloating.reduce(function (a, b) {
            return Math.min(a, b);
        }) + ""
        document.getElementById("t-max").innerHTML = infared_responses.imageFloating.reduce(function (a, b) {
            return Math.max(a, b);
        }) + ""
        // console.log(infared_responses.imageFloating)
        infared_responses.imaging();
        helpers.log(evt)
    },
    onError: function (evt) {
        helpers.log(evt)
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



