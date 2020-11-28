#include "hid_lamparray.h"
#include "led_matrix.h"

bool hid_lamparray_auto_mode = true;

hid_lamparray_attributes_report_storage_t hid_lamparray_attributes_report = { .desc = {
    .report_id = hid_lamparray_attributes_report_id,
    .lamp_count = DRIVER_LED_TOTAL,
    .bounding_box = {
        .x = 322580,
        .y = 111760,
        .z = 31750
    },
    .array_kind = 0x01, // keyboard
    .min_update_interval = (RGB_MATRIX_LED_FLUSH_LIMIT * 1000)
}};

uint8_t get_led_binding(uint8_t lamp_id) {
    for (int col = 0; col <= MATRIX_COLS; col++) {
        for (int row = 0; row <= MATRIX_ROWS; row++) {
            if (g_led_config.matrix_co[row][col] == lamp_id)
                return keymaps[0][row][col];
        }
    }
    return 0;
}

void get_lamp_attributes(uint8_t lamp_id, uint8_t *responseBuffer) {
    hid_lamparray_attributes_response_report_storage_t *response = (hid_lamparray_attributes_response_report_storage_t*) responseBuffer;
    response->desc.report_id = hid_lamparray_attributes_response_report_id;
    response->desc.lamp_id = lamp_id;
    response->desc.is_programmable = true;
    response->desc.position.x = led_map[lamp_id].px * 25400,
    response->desc.position.y = led_map[lamp_id].py * 25400,
    response->desc.position.z = 0;
    response->desc.level_counts.red = 255;
    response->desc.level_counts.green = 255;
    response->desc.level_counts.blue = 255;
    response->desc.level_counts.intensity = 1;
    response->desc.update_latency = 1000;
    response->desc.lamp_purposes = 0x01;
    response->desc.input_binding = get_led_binding(lamp_id);
    response->desc.is_programmable = true;
}

void hid_lamparray_recv(uint8_t *data, uint8_t length) {
    switch (*data) {
        case hid_lamparray_multi_update_report_id: {
            if (hid_lamparray_auto_mode)
                return;
            hid_lamparray_multi_update_report_t *report = (hid_lamparray_multi_update_report_t *)data;
            for (int i = 0; i < report->lamp_count; i++) {
                if (report->lamp_ids[i] < DRIVER_LED_TOTAL && report->lamp_ids[i] > 0)
                    rgb_matrix_set_color(report->lamp_ids[i], report->colors[i].red, report->colors[i].green, report->colors[i].blue);
            }
            if (report->flags & 0x01)
                rgb_matrix_update_pwm_buffers();
        break;
        }
        case hid_lamparray_range_update_report_id: {
            if (hid_lamparray_auto_mode)
                return;
            hid_lamparray_range_update_report_t *report = (hid_lamparray_range_update_report_t *)data;
            for (int i = report->lamp_id_start; i <= report->lamp_id_end && i < DRIVER_LED_TOTAL; i++) {
                    rgb_matrix_set_color(i, report->color.red, report->color.green, report->color.blue);
            }
            if (report->flags & 0x01)
                rgb_matrix_update_pwm_buffers();
            break;
        }
        case hid_lamparray_control_report_id: {
            hid_lamparray_control_report_t *report = (hid_lamparray_control_report_t *)data;
            if (hid_lamparray_auto_mode && report->autonomous_mode == 0) {
                hid_lamparray_auto_mode = false;
                rgb_matrix_disable_noeeprom();
                rgb_matrix_set_color_all(0,0,0);
                rgb_matrix_update_pwm_buffers();
            } else if (!hid_lamparray_auto_mode && report->autonomous_mode == 1) {
                hid_lamparray_auto_mode = true;
                rgb_matrix_enable_noeeprom();
            }
        break;
        }
    };
}
