var urls = {
  "home": "index.html",
  "login": "index.html",
  "advanced": "advanced.html",
  "system": "system.html",
  "imaging": "imaging.html"
}

const LOG = true

/**
 * Toggle Mobile Menu
 */
function openMenu() {

  if (document.getElementById("mobile-menu").style.display == "none") {

    document.getElementById("mobile-menu").style.display = "block";
  } else {
    document.getElementById("mobile-menu").style.display = "none";
  }

}

var _kaiote_handler = {
  /**
   * Handle login on button click
   */
  _toast_options: {
    type: "success",
    duration: 4000,
    message: "",
  },
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
  toast: function (options = this._toast_options) {
    _kaiote_handler.show("graph-loader")
    if (options.type == "success") {
      document.body.innerHTML += '<div id="toasting" class="toast toast-success">' +
        '<p>' + options.message + '</p>' +
        '</div>';
    }

    if (options.type == "error") {
      document.body.innerHTML += '<div id="toasting" class="toast toast-error">' +
        '<p>' + options.message + '</p>' +
        '</div>';
    }

    setTimeout(function () {
      var elem = document.querySelector('#toasting');
      elem.parentNode.removeChild(elem);
      _kaiote_handler.hide("graph-loader")
    }, options.duration)


  },
  show: function (name) {
    document.getElementById(name).style.display = "inline";
  },
  hide: function (name) {
    document.getElementById(name).style.display = "none";
  },
  playSound: function () {
    var audio = new Audio('./resources/beep4.mp3');
    audio.play();
  },
  currentTime: function () {
    var date = new Date();
    return date.getTime() / 1000 | 0
  },
  loadTimeFromUnixTime: function (UNIX_timestamp) {
    var a = new Date(UNIX_timestamp * 1000);
    var months = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December'];
    var year = a.getFullYear();
    var month = months[a.getMonth()];

    function formattedDay() {
      if (a.getDate() < 10) {
        return '0' + a.getDate()
      }
      return a.getDate()
    }

    function formattedSeconds() {
      if (a.getSeconds() < 10) {
        return '0' + a.getSeconds()
      }
      return a.getSeconds()
    }

    function formattedMinutes() {
      if (a.getMinutes() < 10) {
        return '0' + a.getMinutes()
      }
      return a.getMinutes()
    }

    function formattedHours() {
      if (a.getHours() < 10) {
        return '0' + a.getHours()
      }
      return a.getHours()
    }

    var date = formattedDay()
    var hour = formattedHours();
    var min = formattedMinutes();
    var sec = formattedSeconds();
    var time = date + ' ' + month + ' ' + year + ' ' + hour + ':' + min + ':' + sec;
    return time;
  },
  uuidv4: function () {
    return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function (c) {
      var r = Math.random() * 16 | 0, v = c == 'x' ? r : (r & 0x3 | 0x8);
      return v.toString(16);
    });
  },
  getTimeDifference: function (t1, t2) {
    var a = new Date(t1 * 1000);
    var b = new Date(t2 * 1000);
    var diff = (a.getTime() - b.getTime()) / 1000;
    return Math.abs(diff);
  }


}

/***
 * 
 * Manage Storage On the 
 */
var storage = {
  set: function (key, data) {
    localStorage.setItem(key, JSON.stringify(data));
  },
  remove: function () { },
  update: function () { },
}


// ** Update data section (Called from the onclick)
var helpers = {
  redirect: function (page) {
    window.location.replace(urls[page])
  },

  log: function (data) {
    if (LOG) {
      console.log(data)
    }
  },

}





