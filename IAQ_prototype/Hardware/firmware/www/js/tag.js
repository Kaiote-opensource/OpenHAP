const SOCKET_URL = "ws://192.168.4.1:80/tag.cgi";
// const SOCKET_URL = "ws://localhost:8999";
/***
 * Socket Management Library
 */

var websocketInst = new WebSocket(SOCKET_URL);

var tag = {
    value: [],
    selectedDevice: "F2416138F240",
    showDevice: function (id) {
        // console.log(id)
        this.selectedDevice = id
        this.displayAllDevices()
    },
    displayAllDevices: function () {
        var container = document.getElementById("tag-hub")
        container.innerHTML = ""
        for (let index = 0; index < this.value.length; index++) {
            const element = this.value[index];
            var div_cont = '<div class="col-md-4">' +
                '<div class="card" style="width: 100%;">' +
                '<div class="card-body">' +
                '<h5 class="card-title">' + element.MAC + '</h5>' +
                '<button class="btn btn-primary" onclick="tag.showDevice(\'' + element.MAC + '\')">view</button>';

            if (this.selectedDevice == element.MAC) {
                div_cont += '<div>'
            }

            if (this.selectedDevice != element.MAC) {
                div_cont += '<div style="display:none;">'
            }
            div_cont += '<br>'
            //table
            div_cont += '<div class="table-responsive">' +

                '<table class="table">' +
                '<tbody>' +
                '<tr>' +
                '<td>RSSI</td>' +
                '<td>' + element.RSSI + '</td>' +
                '</tr>' +
                '<tr>' +
                '<td>Battery Level</td>' +
                '<td>' + element.BATT_VOLTAGE + '</td>' +
                '</tr>' +
                '</tbody>' +
                '</table>' +
                '</div>' +
                //end of table

                '</div>' +
                '</div>' +
                '</div>' +
                '</div>';
            container.innerHTML += div_cont
        }
    },
    removeDevice: function () {

        for (let index = 0; index < this.value.length; index++) {
            const element = this.value[index];
            var t1 = _kaiote_handler.currentTime()
            var t2 = element.TIME
            var difference = _kaiote_handler.getTimeDifference(t1, t2)
            if (difference > 30) {
                this.value.splice(index, 1)
                continue;
            }
        }
    },
    addDevice: function (device) {
        /**
         * Check if device exists
         */
        var found = 0
        for (let index = 0; index < this.value.length; index++) {
            const element = this.value[index];
            if (element.MAC == device.MAC) {
                this.value[index] = device
                var found = 1
                break;
            }
        }
        if (found == 0) {
            this.value.push(device)
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
        tag.addDevice(response)
        tag.removeDevice()
        tag.displayAllDevices()
        // helpers.log(evt)
    },
    onError: function (evt) {
        helpers.log(response)
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

