#include <stdint.h>

enum ReportIds
{
    REPORT_ID_KEYBOARD = 0x01,
    REPORT_ID_CONSUMER_CONTROL = 0x02,
};

const uint8_t hid_descriptor_keyboard_boot_mode[] = {
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x06, // Usage (Keyboard)
    0xa1, 0x01, // Collection (Application)

    0x85, REPORT_ID_KEYBOARD, // Report ID 1

    // Modifier byte

    0x75, 0x01, //   Report Size (1)5
    0x95, 0x08, //   Report Count (8)
    0x05, 0x07, //   Usage Page (Key codes)
    0x19, 0xe0, //   Usage Minimum (Keyboard LeftControl)
    0x29, 0xe7, //   Usage Maxium (Keyboard Right GUI)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //   Logical Maximum (1)
    0x81, 0x02, //   Input (Data, Variable, Absolute)

    // Reserved byte

    0x75, 0x01, //   Report Size (1)
    0x95, 0x08, //   Report Count (8)
    0x81, 0x03, //   Input (Constant, Variable, Absolute)

    // LED report + padding

    0x95, 0x05, //   Report Count (5)
    0x75, 0x01, //   Report Size (1)
    0x05, 0x08, //   Usage Page (LEDs)
    0x19, 0x01, //   Usage Minimum (Num Lock)
    0x29, 0x05, //   Usage Maxium (Kana)
    0x91, 0x02, //   Output (Data, Variable, Absolute)

    0x95, 0x01, //   Report Count (1)
    0x75, 0x03, //   Report Size (3)
    0x91, 0x03, //   Output (Constant, Variable, Absolute)

    // Keycodes

    0x95, 0x06, //   Report Count (6)
    0x75, 0x08, //   Report Size (8)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0xff, //   Logical Maximum (1)
    0x05, 0x07, //   Usage Page (Key codes)
    0x19, 0x00, //   Usage Minimum (Reserved (no event indicated))
    0x29, 0xff, //   Usage Maxium (Reserved)
    0x81, 0x00, //   Input (Data, Array)

    0xc0, // End collection

    // start of consumer control Media keys
    0x05, 0x0C,                       // Usage Page (Consumer)
    0x09, 0x01,                       // Usage (Consumer Control)
    0xA1, 0x01,                       // Collection (Application)
    0x85, REPORT_ID_CONSUMER_CONTROL, //   Report ID (2)
    0x19, 0x00,                       //   Usage Minimum (Unassigned)
    // 0x2A, 0xFF, 0x02,                 //   Usage Maximum (0x02FF)
    0x29, 0xFF, /* usage maximum (3ff) */
    0x15, 0x00,                       //   Logical Minimum (0)
    // 0x26, 0xFF, 0x7F,                 //   Logical Maximum (32767)
    0x25, 0xFF, // logical maximum (3ff)
    0x95, 0x01, //   Report Count (1)
    0x75, 0x10, //   Report Size (16)
    0x81, 0x00, //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,       // End Collection
};