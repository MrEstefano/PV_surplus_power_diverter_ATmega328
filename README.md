# MK2 solar_surplus_power_diverter_ATmega328
PV_surplus_power_diverter_ATmega328 code written by Robin Robin Emley at https://mk2pvrouter.co.uk/index.html. All credits to Robin (uploading functional version, for personal lookup reason)
The changes where done to utilize dataloging to real time data bsae at Firebase
For those who still own analogue elec. meter, follow the link https://youtu.be/flaBkkdfaq0
For those who have digital power meter follow Robin's tutorial
Code uploaded to ATmega328 merges transmit data into one string with start and begining mark, this string is sent to ESP8266 via Software serial in period of 9 seconds,
Code which is flashed on ESp8266 deconstruct data and manipulates them for updating Firebase real time database in 18 seconds interwal.
Web application hosted by Firebase is collecting data freom database and displays them in tabs, gauges, graphs and table, see the format below.
![image](https://github.com/MrEstefano/PV_surplus_power_diverter_ATmega328/assets/79326044/c62f30c2-f8bb-4ed0-a265-b2cdd118d011)
![image](https://github.com/MrEstefano/PV_surplus_power_diverter_ATmega328/assets/79326044/ba9d38ee-1ae7-4c44-847b-f726c1f949c0)
![image](https://github.com/MrEstefano/PV_surplus_power_diverter_ATmega328/assets/79326044/cade0df9-35ce-4c31-86d9-bf71bfe2d40c)
![diverter](https://github.com/user-attachments/assets/d5266805-36e3-484a-acde-77d8a0ae5a7d)

