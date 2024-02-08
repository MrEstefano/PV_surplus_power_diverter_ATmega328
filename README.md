# MK2 solar_surplus_power_diverter_ATmega328
PV_surplus_power_diverter_ATmega328 code written by Robin at https://mk2pvrouter.co.uk/index.html (uploading functional version, for personal lookup reason)
for those who still own analogue elec. meter, follow the link https://youtu.be/flaBkkdfaq0
for those who have digital power meter follow Robin's tutorial
Code uploaded to ATmega328 merges transmit data into one string with start and begining mark, this string is sent to ESP8266 via Software serial in period of 9 seconds,
Code which is flashed on ESp8266 deconstruct data and manipulates them for updating Firebase real time database in 18 seconds interwal.
Web application hosted by Firebase is collecting data freom database and displays them in tabs, gauges, graphs and table, see the format below
![image](https://github.com/MrEstefano/PV_surplus_power_diverter_ATmega328/assets/79326044/3a05e52f-3fef-41e3-90b5-53a37b65971f)

