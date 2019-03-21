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





