# Firmware

![OpenHAP Firmware](https://cdn.hackaday.io/images/6678741566922513774.png)

OpenHAP's firmware controls all the above

## Directory structure

* **www** - Contains html/css/js files that run the webserver on the device. Typically a web develoer edits this file.

* **main** - Contains the main codebase that controls all the functioning of an OpenHAP device. It utilises drivers and other libraries in the components/ folder to implement functionality such as webserver operations, Parsing JSON commands from client, editing OpenHAP behaviour. A firmware engineer typically edits files in this folder while communicating with the web developer handling the www/ folder and hardware developer handling the hardware/ folder.

* **components** - Contains lower level components such as hardware drivers and other softwares such as webserver. To extend openHAP, a firmware engineer typically edits or adds drivers here to suit measurement needs that may not yet be covered or met, as the hardware engineer makes necessary hardware modifications.
