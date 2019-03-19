


var urls = {
  "home": "index.html",
  "login": "index.html",
  "advanced": "advanced.html",
  "system": "system.html",
  "imaging": "imaging.html"
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
    if (LOG) {
      console.log(data)
    }

  }
}





