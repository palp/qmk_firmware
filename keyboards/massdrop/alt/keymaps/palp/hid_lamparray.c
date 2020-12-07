#ifdef HID_LAMPARRAY_ENABLE
#include "hid_lamparray.h"
#include "led_matrix.h"

bool hid_lamparray_auto_mode = true;
int process_count = 0;
uint32_t rgb_timer;

void map_keybindings(void) {
    uint8_t led_i[8];
    for (int i = 0; i < DRIVER_LED_TOTAL; i++) {
        led_keymap[i] = 0;
    }
    for (int col = 0; col < MATRIX_COLS; col++) {
        for (int row = 0; row < MATRIX_ROWS; row++) {
            int count = rgb_matrix_map_row_column_to_led(row, col, led_i);
            for (int l = 0; l < count; l++) {
                uint16_t key = keymaps[0][row][col];
                if (IS_ANY(key))
                    led_keymap[led_i[l]] = key & 0xFF;
            }
        }
    }
}

uint8_t get_led_binding(uint8_t lamp_id) {
    return led_keymap[lamp_id];
}

/*
void get_lamp_attributes(uint8_t lamp_id, uint8_t *responseBuffer) {
    hid_lamparray_attributes_response_report_storage_t *response = (hid_lamparray_attributes_response_report_storage_t*) responseBuffer;
    response->desc.report_id = hid_lamparray_attributes_response_report_id;
    response->desc.lamp_id = (0x00FF & lamp_id);
    response->desc.position.x = (uint32_t)(led_map[lamp_id].px * 25400);
    response->desc.position.y = (uint32_t)(led_map[lamp_id].py * 25400);
    response->desc.position.z = 0x000000FF;
    response->desc.update_latency = 0x00007148; // microseconds
    response->desc.lamp_purposes = 0x00000001;
    response->desc.level_counts.red = 0xFF;
    response->desc.level_counts.green = 0xFF;
    response->desc.level_counts.blue = 0xFF;
    response->desc.level_counts.intensity = 0x01;
    response->desc.is_programmable = 0x0001;
    response->desc.input_binding = get_led_binding(lamp_id);
}*/


void fill_lamp_attributes(hid_lamparray_attributes_response_report_t *report) {
    report->position.x = (uint32_t)(led_map[report->lamp_id].px * 25400);
    report->position.y = (uint32_t)(led_map[report->lamp_id].py * 25400);
    report->input_binding = get_led_binding(report->lamp_id);
    report->lamp_purposes = 0x02;
    if (g_led_config.flags[report->lamp_id] & LED_FLAG_KEYLIGHT)
        report->lamp_purposes = 0x01;
    if (g_led_config.flags[report->lamp_id] & LED_FLAG_MODIFIER)
        report->lamp_purposes = 0x01;
    if (g_led_config.flags[report->lamp_id] & LED_FLAG_INDICATOR)
        report->lamp_purposes = report->lamp_purposes & 0x08;
}

void hid_lamparray_recv(uint8_t *data, uint8_t length) {
    switch (*data) {
        case hid_lamparray_multi_update_report_id: {
            if (hid_lamparray_auto_mode)
                return;
            hid_lamparray_multi_update_report_t *report = (hid_lamparray_multi_update_report_t *)data;
            for (int i = 0; i < report->lamp_count; i++) {
                if (report->lamp_ids[i] < DRIVER_LED_TOTAL && report->lamp_ids[i] >= 0) {
                    rgb_matrix_set_color(report->lamp_ids[i], report->colors[i].red, report->colors[i].green, report->colors[i].blue);
                    process_count++;
                }
            }
            /*
            if (report->flags & 0x01) {
                rgb_matrix_update_pwm_buffers();
                process_count = 0;
            }*/
            if (rgb_timer == 0 || timer_elapsed32(rgb_timer) >= RGB_MATRIX_LED_FLUSH_LIMIT) {
                rgb_matrix_update_pwm_buffers();
                rgb_timer = timer_read32();
            }

        break;
        }
        case hid_lamparray_range_update_report_id: {
            if (hid_lamparray_auto_mode)
                return;
            hid_lamparray_range_update_report_t *report = (hid_lamparray_range_update_report_t *)data;
            for (int i = report->lamp_id_start; i <= report->lamp_id_end && i < DRIVER_LED_TOTAL; i++) {
                    rgb_matrix_set_color(i, report->color.red, report->color.green, report->color.blue);
                    process_count++;

            }/*
            if (report->flags & 0x01){
                rgb_matrix_update_pwm_buffers();
                process_count = 0;
            }*/
            if (rgb_timer == 0 || timer_elapsed32(rgb_timer) >= RGB_MATRIX_LED_FLUSH_LIMIT) {
                rgb_matrix_update_pwm_buffers();
                rgb_timer = timer_read32();
            }

            break;
        }
        case hid_lamparray_control_report_id: {
            hid_lamparray_control_report_t *report = (hid_lamparray_control_report_t *)data;
            if (hid_lamparray_auto_mode && report->autonomous_mode == 0) {
                //rgb_matrix_disable_noeeprom();
                //wait_ms(RGB_MATRIX_LED_FLUSH_LIMIT*2);
                hid_lamparray_auto_mode = false;
            } else if (!hid_lamparray_auto_mode && report->autonomous_mode == 1) {
                //rgb_matrix_enable_noeeprom();
                hid_lamparray_auto_mode = true;
            }
        break;
        }
    };
}
#endif
