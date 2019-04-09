const SOCKET_URL = "ws://192.168.4.1:80/infared.cgi";
// const SOCKET_URL = "ws://localhost:8999";
/***
 * Socket Management Library
 */
var websocketInst = new WebSocket(SOCKET_URL);


function _base64ToArrayBuffer(base64) {
    var binary_string = window.atob(base64);
    var len = binary_string.length;
    var bytes = new Uint8Array(len);
    for (var i = 0; i < len; i++) {
        bytes[i] = parseFloat(binary_string.charCodeAt(i));
    }
    return bytes.buffer;
}

var infared_responses = {
    imageFloating: [],
    bufferIn: "",
    bufferOut: "",
    imageData: function () {
        return this.dataSplit(this.imageFloating);
    },
    imaging: function () {
        var data = [{
            z: this.imageData().reverse(),
            colorscale: 'YIOrRd',
            type: 'heatmap',
            zsmooth: 'best',
        }];
        var layout = {
            margin: {
                l: 45,
                r: 30,
                b: 60,
                t: 60,
                pad: 20
            },
            hoverlabel: {
                bgcolor: 'white',
                font: {
                    family: 'Courier New, monospace',
                    size: 16,
                    color: '#ffffff'
                },
            }
        };
        Plotly.newPlot('infrared-image', data, layout);

    },
    dataSplit: function (imgData) {
        var holder = []
        for (let i = 0; i < 768;) {
            var out = []
            for (let index = i; index < (i + 32); index++) {
                const element = Number(imgData[index]).toFixed(4);
                // if( element < 1){
                //     out.push(30)
                //     continue;
                // }
                out.push(element)
            }
            holder.push(out)
            i += 32
        }
        return holder;
    },
    manageBuffer: function (response) {

        if (response.CAMERA_CONT != null || response.CAMERA_CONT != undefined) {
            this.bufferIn += response.CAMERA_CONT
        }

        if (response.CAMERA_END != null || response.CAMERA_END != undefined) {
            this.bufferIn += response.CAMERA_END
            this.bufferOut = this.bufferIn
            this.bufferIn = ""
        }

    }
}


var socket = {
    init: function () {

    },
    onOpen: function (evt) {
        helpers.log(evt)
        console.trace()
    },
    onClose: function (evt) {
        helpers.log(evt)
    },
    onMessage: function (evt) {

        var response = JSON.parse(evt)

        infared_responses.manageBuffer(response)

        infared_responses.imageFloating = Array.prototype.slice.call(new Float32Array(_base64ToArrayBuffer(infared_responses.bufferOut)));
        helpers.log(infared_responses.imageFloating)

        document.getElementById("conversion-frame-rate").innerHTML = response.CONVERSION_FRAMERATE + ""
        document.getElementById("t-min").innerHTML = infared_responses.imageFloating.reduce(function (a, b) {
            return Number(Math.min(a, b)).toFixed(2);
        }) + ""
        document.getElementById("t-max").innerHTML = infared_responses.imageFloating.reduce(function (a, b) {
            return Number(Math.max(a, b)).toFixed(2)
        }) + ""


        infared_responses.imaging();
        // helpers.log(evt)
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
    helpers.log(evt.data)
};
websocketInst.onerror = function (evt) {
    socket.onError(evt)
};

websocketInst.onclose = function (evt) {
    socket.onClose(evt)
};



