/*
 * Copyright (C) 2014 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at
 * contact@bluekitchen-gmbh.com
 *
 */

// #define BTSTACK_FILE__ "hog_keyboard_demo.c"

// *****************************************************************************
/* EXAMPLE_START(hog_keyboard_demo): HID Keyboard LE
 */
// *****************************************************************************

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "hog_keyboard_demo.h"

#include "btstack.h"

#include "ble/gatt-service/battery_service_server.h"
#include "ble/gatt-service/device_information_service_server.h"
#include "ble/gatt-service/hids_device.h"
#include "btstack_hid.h"
#include "keyboard.h"
#include "hid_descriptor.h"
#include "pico/cyw43_arch.h"

// static btstack_timer_source_t heartbeat;
static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;
static uint8_t battery = 100;
static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;
static uint8_t protocol_mode = 1;
static btstack_timer_source_t typing_timer;

static bool timer_created = false;
static bool sample = false;
static bool play_pause = false;
static hids_device_report_t hid_reports_generic_storage[2];

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

const uint8_t adv_data[] = {
    // Flags general discoverable, BR/EDR not supported
    0x02,
    BLUETOOTH_DATA_TYPE_FLAGS,
    0x06,
    // Name
    0x0d,
    BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME,
    'H',
    'I',
    'D',
    ' ',
    'K',
    'e',
    'y',
    'b',
    'o',
    'a',
    'r',
    'd',
    // 16-bit Service UUIDs
    0x03,
    BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS,
    ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE & 0xff,
    ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE >> 8,
    // Appearance HID - Keyboard (Category 15, Sub-Category 1)
    0x03,
    BLUETOOTH_DATA_TYPE_APPEARANCE,
    0xC1,
    0x03,
};

const uint8_t adv_data_len = sizeof(adv_data);

// HID Report sending
static void send_report(int modifier, int keycode)
{
    uint8_t report[] = {modifier, 0, keycode, 0, 0, 0, 0, 0};
    uint8_t status_report;
    switch (protocol_mode)
    {
    case 0:
        hids_device_send_boot_keyboard_input_report(con_handle, report, sizeof(report));
        break;
    case 1:
        // hids_device_send_input_report(con_handle, report, sizeof(report));
        status_report = hids_device_send_input_report_for_id(con_handle, REPORT_ID_KEYBOARD, report, sizeof(report));
        // uint8_t hids_device_send_input_report_for_id(hci_con_handle_t con_handle, uint16_t report_id, const uint8_t * report, uint16_t report_len);
        printf("Status keyboard: %d\n", status_report);
        break;
    default:
        break;
    }
}

static void send_report_media(int keycode)
{
    uint8_t report[] = {keycode, 0};
    uint8_t status = hids_device_send_input_report_for_id(con_handle, REPORT_ID_CONSUMER_CONTROL, report, sizeof(report));
    printf("Status keyboard: %d\n", status);
    // hids_device_request_can_send_now_event(con_handle);
}

#define TYPING_PERIOD_MS 1000

static void update_battery()
{
    if (battery <= 0)
    {
        battery = 0;
    }
    else
    {
        battery--;
    }

    battery_service_server_set_battery_value(battery);
}

/**
 * This function sends key up (0,0) and the pressed key
 */
static void timer_handler(btstack_timer_source_t *ts)
{
    printf("Timer Handler - app connected init\n");
    update_battery();
    if (!sample)
    {
        send_report(0, 34); // send number 55
        send_report(0, 0);  // simulate keyup
        sample = true;
    }
    if (!play_pause)
    {
        send_report_media(205); // send play pause
        send_report_media(0);   // simulate keyup
        play_pause = true;
        send_report_media(226); // send play pause
        send_report_media(0);   // simulate keyup
    }

    printf("restart timer\n");
    btstack_run_loop_set_timer(ts, TYPING_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
    printf("Timer Handler - app connected end\n");
}

static void reset_timer()
{
    btstack_run_loop_set_timer(&typing_timer, TYPING_PERIOD_MS);
    btstack_run_loop_add_timer(&typing_timer);
}

static void setup_timer(void)
{
    if (!timer_created)
    {
        timer_created = true;
        // typing_timer.process = &timer_handler;
        btstack_run_loop_set_timer_handler(&typing_timer, timer_handler);
        btstack_run_loop_set_timer(&typing_timer, TYPING_PERIOD_MS);
        btstack_run_loop_add_timer(&typing_timer);
    }
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET)
        return;

    switch (hci_event_packet_get_type(packet))
    {
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        con_handle = HCI_CON_HANDLE_INVALID;
        printf("Disconnected\n");
        break;
    case HCI_EVENT_HIDS_META:
        switch (hci_event_hids_meta_get_subevent_code(packet))
        {
        case HIDS_SUBEVENT_INPUT_REPORT_ENABLE:
            con_handle = hids_subevent_input_report_enable_get_con_handle(packet);
            printf("Report Characteristic Subscribed %u\n", hids_subevent_input_report_enable_get_enable(packet));
            setup_timer();
            break;
        case HIDS_SUBEVENT_BOOT_KEYBOARD_INPUT_REPORT_ENABLE:
            con_handle = hids_subevent_boot_keyboard_input_report_enable_get_con_handle(packet);
            printf("Boot Keyboard Characteristic Subscribed %u\n", hids_subevent_boot_keyboard_input_report_enable_get_enable(packet));
            break;
        case HIDS_SUBEVENT_PROTOCOL_MODE:
            protocol_mode = hids_subevent_protocol_mode_get_protocol_mode(packet);
            printf("Protocol Mode: %s mode\n", hids_subevent_protocol_mode_get_protocol_mode(packet) ? "Report" : "Boot");
            break;
        case HIDS_SUBEVENT_CAN_SEND_NOW:
            printf("hid subevent can send now!");
            // send_report(send_modifier, send_keycode);
            // setup_timer();
            // reset_timer();
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

static void sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET)
        return;

    switch (hci_event_packet_get_type(packet))
    {
    case SM_EVENT_REENCRYPTION_STARTED:
        printf("SM_EVENT_REENCRYPTION_STARTED\n");
        break;
    case SM_EVENT_REENCRYPTION_COMPLETE:
        printf("SM_EVENT_REENCRYPTION_COMPLETE\n");
        break;
    case SM_EVENT_IDENTITY_RESOLVING_STARTED:
        printf("SM_EVENT_IDENTITY_RESOLVING_STARTED!\n");
        break;
    case SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED:
        printf("SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED!\n");
        break;
    case SM_EVENT_IDENTITY_RESOLVING_FAILED:
        printf("SM_EVENT_IDENTITY_RESOLVING_FAILED!\n");
        break;
    case SM_EVENT_PAIRING_STARTED:
        printf("paring started!\n");
        break;
    case SM_EVENT_PAIRING_COMPLETE:

        switch (sm_event_pairing_complete_get_status(packet))
        {
        case ERROR_CODE_SUCCESS:
            printf("Pairing complete, success\n");
            break;
        case ERROR_CODE_CONNECTION_TIMEOUT:
            printf("Pairing failed, timeout\n");
            break;
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
            printf("Pairing failed, disconnected\n");
            break;
        case ERROR_CODE_AUTHENTICATION_FAILURE:
            printf("Pairing failed, reason = %u\n", sm_event_pairing_complete_get_reason(packet));
            break;
        default:
            break;
        }
        break;
    case SM_EVENT_JUST_WORKS_REQUEST:
        printf("Just Works requested\n");
        sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
        break;
    case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
        printf("Confirming numeric comparison: %" PRIu32 "\n", sm_event_numeric_comparison_request_get_passkey(packet));
        sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
        break;
    case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
        printf("Display Passkey: %" PRIu32 "\n", sm_event_passkey_display_number_get_passkey(packet));
        break;
    default:
        break;
    }
}

int btstack_main(void)
{
    l2cap_init();

    // setup SM: Display only
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING | SM_AUTHREQ_MITM_PROTECTION);

    // setup ATT server
    att_server_init(profile_data, NULL, NULL);

    // setup battery service
    battery_service_server_init(battery);

    // setup device information service
    device_information_service_server_init();

    // setup HID Device service
    hids_device_init_with_storage(22, hid_descriptor_keyboard_boot_mode, sizeof(hid_descriptor_keyboard_boot_mode), 2, hid_reports_generic_storage);

    for (int i = 0; i < 2; i++)
    {
        printf("report %d\n", i);
        printf("id %d\n", hid_reports_generic_storage[i].id);
        printf("type %d\n", hid_reports_generic_storage[i].type);
        printf("size %d\n", hid_reports_generic_storage[i].size);
    }

    // setup advertisements
    uint16_t adv_int_min = 0x0030;
    uint16_t adv_int_max = 0x0030;
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(adv_data_len, (uint8_t *)adv_data);
    gap_advertisements_enable(1);

    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for SM events
    sm_event_callback_registration.callback = &sm_packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    // register for HIDS
    hids_device_register_packet_handler(packet_handler);

    // turn on!
    hci_power_control(HCI_POWER_ON);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    return 0;
}
/* EXAMPLE_END */
