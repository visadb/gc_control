#ifndef usb_rawhid_settings_h__
#define usb_rawhid_settings_h__

/**************************************************************************
 *
 *  Configurable Options
 *
 **************************************************************************/

// You can change these to give your code its own name.
#define STR_MANUFACTURER    L"Visa"
#define STR_PRODUCT         L"Teensy Raw HID"

// These 4 numbers identify your device.  Set these to
// something that is (hopefully) not used by any others!
#define VENDOR_ID           0x16C0
#define PRODUCT_ID          0x0480
#define RAWHID_USAGE_PAGE   0xFFEE // recommended: 0xFF00 to 0xFFFF
#define RAWHID_USAGE        0x0200 // recommended: 0x0100 to 0xFFFF

// These determine the bandwidth that will be allocated
// for your communication.  You do not need to use it
// all, but allocating more than necessary means reserved
// bandwidth is no longer available to other USB devices.
#define RAWHID_TX_SIZE      64 // transmit packet size
#define RAWHID_TX_INTERVAL  2  // max # of ms between transmit packets
#define RAWHID_RX_SIZE      64 // receive packet size
#define RAWHID_RX_INTERVAL  8  // max # of ms between receive packets

#endif
