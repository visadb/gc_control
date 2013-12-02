Overview
--------

This project contains software for using a Teensy++ 2.0 as a programmable
Gamecube controller. Currently only the buttons of the controller are
supported, not joysticks or shoulder button sliders.

See [a demo in Youtube](http://www.youtube.com/watch?v=K-2K3p3ANhw).

This project needs Linux for compiling and running the client. It should not be
too difficult to support Windows and OSX -- pull requests are welcome. ;)

The software is divided in two parts:

1. Software for Teensy++ 2.0 microcontroller board. This makes the Teensy act
   as though it was a Gamecube controller. It listens to button state commands
   through Teensy's USB interface.
2. Client software that sends button state commands to Teensy through USB.
   Supports direct keyboard input (poorly) and macros.

Required hardware:

1. A Teensy++ 2.0 microcontroller board with header pins soldered
2. A breadboard for easy connections.
3. A Mini-USB cable for connecting the Teensy++ to a host computer
4. A Gamecube controller cable (eg. from a extension cable or a controller)

Setting up the Teensy++ 2.0 board
---------------------------------

1. Install pre-requisities: make, avr-gcc and
   [`teensy_loader_cli`](http://www.pjrc.com/teensy/loader_cli.html)
2. Plug the Teensy++ 2.0 to a USB port and press the on-board reset button.
3. Run `make upload` in the top-level directory of the project. This
   will compile `gc_control.hex` and try to upload it to the
   connected Teensy.
4. Connect the Gamecube controller cable's data line to the D0 pin of Teensy.
   See [this article](http://tk-421projects.blogspot.fi/2012/12/gamecube-controller-protocol.html)
   for pinout of the connector.
5. Connect the Gamecube controller cable's ground line to the ground of the
   Teensy through a small resistor, eg. 1 Ohm. You can also try to connect the
   ground directly, but this might blow something due to a possibly large
   potential difference between the ground of Gamecube/Wii and the ground of
   Teensy/host-computer.

That's it, the Teensy should now be talking to Gamecube/Wii and pretending to
be a controller with no buttons pressed and joysticks in neutral position.

Setting up the client software
------------------------------

1. Install pre-requisities: gcc and `libusb-dev`
2. Compile the client by running `make` in the `client` directory.
3. See `client/example_macro.gcmacro` and write your macro.
4. Plug the Teensy++ 2.0 to a USB port
5. Run `gc_control_client path/to/your_macro` to start controlling the
   Gamecube/Wii.

USB protocol description
------------------------

If you want to write your own client, the USB protocol is really simple. Just
open up the USB device by vendorid:productid and find the HID OUT endpoint
whose usage page and usage can be found in `usb_rawhid_settings.h`. Then send
two-byte HID reports with button states like this:

* Byte 0: `always_zero always_zero always_zero start Y X B A`
* Byte 1: `always_one L R Z D-Up D-Down D-Right D-Left`

To represent a pressed state, set the corresponding bit (to one).
