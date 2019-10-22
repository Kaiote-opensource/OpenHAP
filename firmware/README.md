# Firmware

![OpenHAP Firmware](https://cdn.hackaday.io/images/6678741566922513774.png)

OpenHAP's firmware controls all the above

## Directory structure

* **www** - Contains HTML/CSS/JS files that run frontend webpage that is used to configure an OpenHAP device before measurement start, as well as verify everything is okay. All of the communication between client frontend(Mobile/Desktop) and backend(On the OpenHAP device) occurs over websockets([RFC6455](https://tools.ietf.org/html/rfc6455) to support bidirectional communication. Typically a front-end web develoer edits the files in this folder.

* **main** - Contains the main codebase that controls all the functioning of an OpenHAP device. It utilises drivers and other libraries in the components/ folder to implement functionality such as webserver operations, Parsing JSON commands over websockets from mobile/desktop client(s), editing OpenHAP behaviour. A firmware engineer typically edits files in this folder while communicating with the web developer handling the www/ folder and hardware developer handling the hardware/ folder.

* **components** - Contains lower level components such as hardware drivers and other software such as an ANSI-C webserver. To extend openHAP, a firmware engineer typically edits or adds drivers here to suit measurement needs that may not yet be covered or met, as the hardware engineer makes necessary hardware modifications.
