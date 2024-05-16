#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "sdkconfig.h"
#include <inttypes.h>

char *TAG = "BLE Client Scan";
uint8_t ble_addr_type;
void ble_app_scan(void);

// Function to log UUIDs
void log_uuids(struct ble_hs_adv_fields *fields)
{
    // Print 16-bit UUIDs
    for (int i = 0; i < fields->num_uuids16; i++)
    {
        printf("16-bit UUID: 0x%04x\n", fields->uuids16[i].value);
    }

    // Print 32-bit UUIDs
    for (int i = 0; i < fields->num_uuids32; i++)
    {
        printf("32-bit UUID: 0x%" PRIx32 "\n", fields->uuids32[i].value);
    }

    // Print 128-bit UUIDs
    for (int i = 0; i < fields->num_uuids128; i++)
    {
        char uuid_str[BLE_UUID_STR_LEN];
        ble_uuid_to_str(&fields->uuids128[i].u, uuid_str);
        printf("128-bit UUID: %s\n", uuid_str);
    }
}

// BLE event handling
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_hs_adv_fields fields;

    switch (event->type)
    {
    case BLE_GAP_EVENT_DISC:
        ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
        if (fields.name_len > 0)
        {
            printf("Name: %.*s\n", fields.name_len, fields.name);
        }
        log_uuids(&fields); // Log the UUIDs

        break;
    default:
        break;
    }
    return 0;
}

void ble_app_scan(void)
{
    printf("Start scanning ...\n");

    struct ble_gap_disc_params disc_params;
    disc_params.filter_duplicates = 0;
    disc_params.passive = 0;
    disc_params.itvl = 0x64;   // Adjusted scan interval (100 in hexadecimal)
    disc_params.window = 0x63; // Adjusted scan window (99 in hexadecimal)
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    ble_gap_disc(ble_addr_type, BLE_HS_FOREVER, &disc_params, ble_gap_event, NULL);
}

// The application
void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type); // Determines the best address type automatically
    ble_app_scan();
}

// The infinite task
void host_task(void *param)
{
    nimble_port_run(); // This function will return only when nimble_port_stop() is executed
}

void app_main()
{
    nvs_flash_init();                               // 1 - Initialize NVS flash using
    nimble_port_init();                             // 3 - Initialize the controller stack
    ble_svc_gap_device_name_set("BLE-Scan-Client"); // 4 - Set device name characteristic
    ble_svc_gap_init();                             // 4 - Initialize GAP service
    ble_hs_cfg.sync_cb = ble_app_on_sync;           // 5 - Set application
    nimble_port_freertos_init(host_task);           // 6 - Set infinite task
}
