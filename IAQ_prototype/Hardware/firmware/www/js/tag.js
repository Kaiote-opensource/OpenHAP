const SOCKET_URL = "ws://192.168.4.1:80/tag.cgi";
// const SOCKET_URL = "ws://localhost:8999";
/***
 * Socket Management Library
 */

var websocketInst = new WebSocket(SOCKET_URL);

var lib = new LocalStorageDB('documents');

// Check if the database was just created. Useful for initial database setup


var tag = {
    value: function () {
        return lib.get()['devices'];
    },
    playFrequency: 1000,
    selectedDevice: null,
    activeDevice: null,
    showDevice: function (id) {
        this.selectedDevice = id
        this.findDevice(id)
        this.displayAllDevices()
        // this.getDistanceSeverity()
    },
    displayAllDevices: function () {
        var container = document.getElementById("tag-hub")
        container.innerHTML = ""
        for (let index = 0; index < this.value().length; index++) {
            const element = this.value()[index];
            var power_since_time_on = _kaiote_handler.toHHMMSS(element.TIME / 10)
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
                '<td>' + element.RSSI + ' dBm </td>' +
                '</tr>' +
                '<tr>' +
                '<td>Battery Level</td>' +
                '<td>' + element.BATT_VOLTAGE + ' mV</td>' +
                '</tr>' +
                '<tr>' +
                '<td>Time Since Power On</td>' +
                '<td>' + power_since_time_on + '</td>' +
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

    },
    findDevice: function (id) {
        for (let index = 0; index < this.value().length; index++) {
            const element = this.value()[index];
            if (element.MAC == id) {
                this.activeDevice = element
                break;
            }
        }
    },
    addDevice: function (device) {
        // Initialise. If the database doesn't exist, it is created
        var found = 0
        //         console.log(this.value())
        if (this.value() != undefined) {
            for (let index = 0; index < this.value().length; index++) {
                const element = this.value()[index];
                if (element.MAC == device.MAC) {
                    //                     lib.update(device, 'devices', element.id)
                    //                     found = 1
                    console.log(element)
                }
            }
        }

        //         if (found == 0) {
        //             lib.create('devices', device)
        //         }

    },
    playInterval: null,
    doPlayInterval: function () {
        this.playInterval = setInterval(function () {
            _kaiote_handler.playSound()
        }, this.playFrequency)
    },
    getDistanceSeverity: function (data = null) {
        if (data !== null && this.selectedDevice !== null) {
            this.activeDevice = data
        }

        if (this.activeDevice !== null) {
            var RSSI = 100 - (parseFloat(this.activeDevice.RSSI) + 90)
            var range = RSSI * 10
            console.log(range)
            clearInterval(this.playInterval)
            this.playFrequency = range
            this.doPlayInterval()
        }
    }
}

var socket = {
    init: function () {

    },
    onOpen: function (evt) {
        helpers.log(evt)
        // console.trace()
    },
    onClose: function (evt) {
        helpers.log(evt)
    },
    onMessage: function (evt) {
        var response = JSON.parse(evt)

        try {
            if ('SET_TIME' in response) {
                _kaiote_handler.toast({
                    type: "success",
                    duration: 2000,
                    message: "Time Set"
                })

            }

            /**
             * Start Measurement Response
             */
            if ('START_MEASUREMENT' in response) {
                _kaiote_handler.toast({
                    type: "success",
                    duration: 2000,
                    message: "Success"
                })

            }
        } catch (error) {

        }
        // console.log(response.MAC)
        // response.LAST_CHECK = _kaiote_handler.currentTime()
        tag.addDevice(response)
        // tag.removeDevice()
        tag.displayAllDevices()
        // tag.getDistanceSeverity(response)

        if (response.MAC = tag.selectedDevice) {
            var audio = new Audio('./resources/notification.mp3')
            audio.play()
        }

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

