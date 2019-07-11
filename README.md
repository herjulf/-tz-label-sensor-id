Intro
-----
Printing the eui64 label on 6mm tape for RS sensornodes.
The sensor node should be connected via the USB-TTL cable.
node should be flashed with firmware diag-rss2.avr-rss2
which printouts the addr in response to the eui64 command.

This process is handled by tty_talk command.

The PT2430PC TZ-printer should be connected to USB. It should
give a device address typically /dev/usb/lp1

Tape
----
TZ 6mm tape. TZe-211. Black-On-White.

Install
-------
make builds a static version of pt1230. See github ref.
ghostscript for converting arbitrary text.
Linux static binary for tty_talk and pt1230 is provided
	
Usage
------
sudo ./sensor-id-labelprint-6mm.sh

Issues
------
It's possible device protection needs to be altered.
sudo chown root:lpadmin /dev/usb/lp1

Sometimes the device /dev/usb/lp1 seems to be locked
by Ubuntu (16.04 LTS). Reboot seems to cure.


To apply label
--------------
Tweezers can help to place at exact position.

References
 ----------
https://github.com/cbdevnet/pt1230

https://github.com/herjulf/tty_talk
