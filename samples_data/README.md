# Samples

Real world data captured by OpenHAP and associated simple data analysis examples.

## CSV data fields

* **Time**- Absolute time of measurement in Unix time(Elapsed time in seconds since the unix epoch 00:00:00 UTC - 1 January 1970). A good tool to decode this non-programatically is https://www.epochconverter.com/.

* **Mean temp**- Mean of all 24x32 pixel temperatures in OpenHAP's thermal imager's field of view(FOV).

* **Max temp**- Maximum of all 24x32 pixel infared temperatures in OpenHAP's thermal imager's field of view(FOV).

* **Min temp**- Minimum of all 24x32 pixel temperatures in OpenHAP's thermal imager's field of view(FOV).

* **Temp variance**- Variance of all 24x32 pixel temperatures in OpenHAP's thermal imager's  field of view(FOV).

* **Temp variance**- square root of the Temperature variance above.

* **Humidity**- Ambient humidity at the time of measurement.

* **Temperature**- Ambient temperature at the time of measurement.

* **PM 1**- Amount of PM 1 measured within a one second interval based on the particulate matter sensor's active pump characteristics such volume of air swept per unit time.

* **PM 2.5**- Amount of PM 2.5 measured within a one second interval based on the particulate matter sensor's active pump characteristics such volume of air swept per unit time.

* **PM 10**- Amount of PM 10 measured within a one second interval based on the particulate matter sensor's active pump characteristics such volume of air swept per unit time.

* **Bluetooth tag ID_count**- Number of pings a specific bluetooth tag, identified by its MAC address, makes to an OpenHAP unit within a measurement period.

* **Bluetooth tag ID_mean_dBm**- Average signal streangth in decibel-milliWatts(dBm) of all pings received from a specific bluetooth tag, within a measurement period.
