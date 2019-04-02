var urls = {
  "home": "index.html",
  "login": "index.html",
  "advanced": "advanced.html",
  "system": "system.html",
  "imaging": "imaging.html"
}

const LOG = false

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





