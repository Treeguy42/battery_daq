# battery_daq
Battery powered DAQ. Arduino Mega 2560 based data acquisition with csv output saved to SD card and broadcast over Bluetooth. Records 2 voltage, 1 current and 13 thermistor readings. 
Capable of sending battery power to apply load at arbitrary duty cycle.

See Arduino Thermistor based DAQ pdf for information on how this works and how to use it.

There are lazy fixes in the code. Do not use this for any reason other than learning!!

Code should be successfully reworked to operate off of REYAX RYLR896 LoRa tranceiver. It uses the AT commands only.

LoRa:
https://reyax.com/products/rylr896/

Datasheet:
https://reyax.com//upload/products_download/download_file/RYLR896_EN.pdf

AT Command Manual:
https://reyax.com//upload/products_download/download_file/LoRa%20AT%20Command%20RYLR40x_RYLR89x_EN.pdf

MAY require USB to TTL communication dongle I suggest:
https://www.amazon.com/dp/B07D6LLX19

But anything of that type, with a plastic cover over the chip components should do, especially if you can get two.

Program I used to to the USB to TTL comminication is HTERM:
https://www.der-hammer.info/pages/terminal.html

NOTE: These devices require Send on enter to be set to: CR-LF, use default baud of 115200 for this.
