#include <string.h>
#include <wifi.h>
#include "esp_wifi.h"
#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "esp_mac.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"
#include "mqttconn.h"

#include <string.h>

#define MSGBOX_TIMEOUT_MS 3000


    objects_t objects;
lv_obj_t *tick_value_change_obj;

// Guardar el listado y la SSID seleccionada
lv_obj_t *wifi_list;
static char current_ssid[33] = " "; // máximo 32 + '\0'
static char wifi_password[64] = " "; // Contraseña por defecto

// Buttons Events Handlers
static void event_handler_cb_main_settings_btn(lv_event_t *e)
{
    lv_event_code_t event = lv_event_get_code(e);

    if (event == LV_EVENT_RELEASED)
    {
        lv_scr_load(objects.screen_settings);
    }
}
// evento boton relays
static void event_handler_cb_main_relays_btn(lv_event_t *e)
{
    lv_event_code_t event = lv_event_get_code(e);

    if (event == LV_EVENT_RELEASED)
    {
        lv_scr_load(objects.scree_relays);
    }
}

// evento boton volver a la pantalla principal
static void event_handler_cb_main_back(lv_event_t *e)
{
    lv_event_code_t event = lv_event_get_code(e);

    if (event == LV_EVENT_RELEASED)
    {
        lv_scr_load(objects.main);
    }
}

// evento boton volver a la pantalla set
static void event_handler_cb_main_back_set(lv_event_t *e)
{
    lv_event_code_t event = lv_event_get_code(e);

    if (event == LV_EVENT_RELEASED)
    {
        lv_scr_load(objects.screen_settings);
    }
}

// evento boton R1
static void r1_event_handler(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);
    publicar_mensaje_mqtt("rele/r1", state ? "ON" : "OFF");
}

// evento boton Wifi
static void event_handler_cb_settings_wifi_btn(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
        return;

    // 1) Mostrar “Buscando redes…”
    lv_obj_t *loading_label = lv_label_create(lv_scr_act());
    lv_label_set_text(loading_label, "Buscando redes...");
    lv_obj_center(loading_label);
    lv_refr_now(NULL);

    // 2) Desconectar y escanear
    // ESP_ERROR_CHECK(esp_wifi_disconnect());
    ignore_next_disconnect = true;
    ESP_ERROR_CHECK(wifi_start_scan());

    // Obtener resultados
    wifi_ap_record_t aps[TOP_N_AP];
    uint16_t ap_count = TOP_N_AP;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, aps));

    // Poblar la lista **antes** de cambiar de pantalla**
    if (ap_count > 0)
    {
        show_networks(aps, ap_count);
    }
    else
    {
        show_networks(NULL, 0);
    }

    // 5) Cargar la pantalla de lista y eliminar mensaje
    lv_scr_load(objects.screen_networklist);
    lv_obj_del(loading_label);

    lv_refr_now(NULL);
}

static void wifi_network_selected_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    const char *ssid = lv_list_get_btn_text(wifi_list, btn);

    // Guarda la SSID
    strncpy(current_ssid, ssid, sizeof(current_ssid) - 1);
    current_ssid[sizeof(current_ssid) - 1] = '\0';

    // Carga la pantalla de credenciales
    lv_scr_load(objects.screen_wifi_credentials);
}

void show_networks(wifi_ap_record_t *aps, uint16_t count)
{
    lv_obj_clean(wifi_list);

    if (count == 0 || aps == NULL)
    {
        lv_list_add_text(wifi_list, "No networks found");
        return;
    }

    for (int i = 0; i < count; i++)
    {
        // Cada botón tiene icono y texto SSID
        lv_obj_t *btn = lv_list_add_btn(wifi_list, LV_SYMBOL_WIFI, (char *)aps[i].ssid);
        lv_obj_add_event_cb(btn, wifi_network_selected_cb, LV_EVENT_CLICKED, NULL);
    }
}

static void mbox_success_screen_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    lv_scr_load(objects.main);
}

void on_wifi_success(void *user_data)
{
    LV_UNUSED(user_data);

    /*Crear msgbox sin botones */
    static const char *btns[] = {""}; // cadena terminadora para lv_msgbox_add_btns
    lv_obj_t *mbox = lv_msgbox_create(lv_scr_act(), "Exito",
                                      "Conectado a Wi-Fi", btns, false);
    lv_obj_center(mbox);
    /*Programar borrado automático tras MSGBOX_TIMEOUT_MS */
    lv_obj_del_delayed(mbox, MSGBOX_TIMEOUT_MS); // :contentReference[oaicite:1]{index=1}

    /*Crear temporizador para cambiar pantalla */
    lv_timer_t *t = lv_timer_create(mbox_success_screen_cb,
                                    MSGBOX_TIMEOUT_MS, NULL);
    lv_timer_set_repeat_count(t, 1);
}

void on_wifi_fail(void *user_data)
{
    LV_UNUSED(user_data);

    /* 1) Crear msgbox sin botones */
    static const char *btns[] = {""};
    lv_obj_t *mbox = lv_msgbox_create(lv_scr_act(), "Error",
                                      "No se pudo conectar a la red", btns, false);
    lv_obj_center(mbox);

    /* 2) Programar borrado automático tras MSGBOX_TIMEOUT_MS */
    lv_obj_del_delayed(mbox, MSGBOX_TIMEOUT_MS); // :contentReference[oaicite:3]{index=3}

    /* 3) Crear temporizador para regresar a ajustes */
    lv_timer_t *t = lv_timer_create(mbox_success_screen_cb,
                                    MSGBOX_TIMEOUT_MS, NULL);
    lv_timer_set_repeat_count(t, 1);
}

void wifi_credentials_save_cb(lv_event_t *e)
{
    ignore_next_disconnect = false;
    if (lv_event_get_code(e) != LV_EVENT_READY)
        return;

    // 1) Recuperar el textarea asociado al teclado
    lv_obj_t *kb = lv_event_get_target(e);
    lv_obj_t *ta = lv_keyboard_get_textarea(kb);
    if (!ta)
    {
        printf("ERROR: No hay textarea asociado al teclado\n");
        return;
    }
        const char *pass = lv_textarea_get_text(ta);
    strncpy(wifi_password, pass, sizeof(wifi_password) - 1);
    wifi_password[sizeof(wifi_password) - 1] = '\0';

   
    printf("DEBUG: Attempting Wi-Fi connect to SSID=\"%s\", PASS=\"%s\"\n",
           current_ssid, wifi_password);

    wifi_config_t cfg = {.sta = {.threshold.authmode = WIFI_AUTH_WPA2_PSK}};
    strncpy((char *)cfg.sta.ssid, current_ssid, sizeof(cfg.sta.ssid));
    strncpy((char *)cfg.sta.password, wifi_password, sizeof(cfg.sta.password));

    esp_err_t err = esp_wifi_set_config(ESP_IF_WIFI_STA, &cfg);
    if (err != ESP_OK)
    {
        printf("ERROR: esp_wifi_set_config failed: %d\n", err);
        return;
    }

    err = esp_wifi_connect();
    /*lv_obj_t *connecting_label = lv_label_create(lv_scr_act());
    lv_label_set_text(connecting_label, "Conectando...");
    lv_obj_center(connecting_label);
    lv_refr_now(NULL); */

    if (err == ESP_OK)
    {
        printf("Intentando conectar al Wi-Fi\n");
    }
    else
    {
        printf("ERROR: esp_wifi_connect returned %d\n", err);
    }
}

void wifi_credentials_cancel_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CANCEL)
    {
        // Regresar a la pantalla de selección de red
        lv_scr_load(objects.screen_settings);
    }
}

void relay_state_change(void *user_data)
{
    const char *estado_mqtt = (const char *)user_data;

    /* 1) Crear msgbox sin botones */
    static const char *btns[] = {""};
    lv_obj_t *mbox = lv_msgbox_create(lv_scr_act(), "MSJ:",
                                      "Mensaje recibido", btns, false);
    lv_obj_center(mbox);

    /* 2) Programar borrado automático tras MSGBOX_TIMEOUT_MS */
    lv_obj_del_delayed(mbox, MSGBOX_TIMEOUT_MS); // :contentReference[oaicite:3]{index=3}

    /* 3) Crear temporizador para regresar a ajustes */
    lv_timer_t *t = lv_timer_create(mbox_success_screen_cb,
                                    MSGBOX_TIMEOUT_MS, NULL);
    lv_timer_set_repeat_count(t, 1);

    if (estado_mqtt != NULL)
    {
        if (strcmp(estado_mqtt, "ON") == 0)
        {
            lv_obj_add_state(objects.r1, LV_STATE_CHECKED); // Encender el interruptor
        }
        else if (strcmp(estado_mqtt, "OFF") == 0)
        {
            lv_obj_clear_state(objects.r1, LV_STATE_CHECKED); // Apagar el interruptor
        }
        
    }
    free(user_data); // Liberar la memoria asignada
}

void create_screen_main()
{
    
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    {
        lv_obj_t *parent_obj = obj;
        {
            // List_relay_btn
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.list_relay_btn = obj;
            lv_obj_set_pos(obj, 13, 177);
            lv_obj_set_size(obj, 78, 41);
            lv_obj_add_event_cb(obj, event_handler_cb_main_relays_btn, LV_EVENT_ALL, NULL);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "LIST");
                }
            }
        }
        {
            // settings_btn
            lv_obj_t *obj = lv_btn_create(parent_obj);
            objects.settings_btn = obj;
            lv_obj_set_pos(obj, 261, 177);
            lv_obj_set_size(obj, 45, 41);
            lv_obj_add_event_cb(obj, event_handler_cb_main_settings_btn, LV_EVENT_ALL, NULL);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "SET");
                }
            }
        }
        {
            // img_weather
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.img_weather = obj;
            lv_obj_set_pos(obj, 140, 16);
            lv_obj_set_size(obj, 130, 130);
            lv_img_set_src(obj, &img_sunny);
        }
        {
            // R1
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.r1 = obj;
            lv_obj_set_pos(obj, 26, 31);
            lv_obj_set_size(obj, 65, 35);
            lv_obj_add_event_cb(obj, r1_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 51, 8);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "R1");
        }
        {
            lv_obj_t *obj = lv_switch_create(parent_obj);
            lv_obj_set_pos(obj, 26, 103);
            lv_obj_set_size(obj, 65, 35);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 51, 81);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "R2");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 175, 138);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
            lv_obj_set_style_translate_y(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "9");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 218, 138);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "°C");
        }
       /* {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj1 = obj;
            lv_obj_set_pos(obj, 176, 138);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "1");
        }*/
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 145, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Text");
        }
    }

}



void create_screen_screen_settings()
{
    
    lv_obj_t *obj = lv_obj_create(0);
    objects.screen_settings = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            lv_obj_set_pos(obj, 19, 32);
            lv_obj_set_size(obj, 100, 50);
            lv_obj_add_event_cb(obj, event_handler_cb_settings_wifi_btn, LV_EVENT_ALL, NULL);

            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Wifi");
                }
            }
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            lv_obj_set_pos(obj, 190, 32);
            lv_obj_set_size(obj, 100, 50);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Button");
                }
            }
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            lv_obj_set_pos(obj, 200, 170);
            lv_obj_set_size(obj, 100, 50);
            lv_obj_add_event_cb(obj, event_handler_cb_main_back, LV_EVENT_ALL, NULL);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Volver");
                }
            }
        }
    }

    
}


void create_screen_screen_networklist()
{
    lv_obj_t *obj = lv_obj_create(0);
    objects.screen_networklist = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    {
        lv_obj_t *parent_obj = obj;

        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            lv_obj_set_pos(obj, 200, 170);
            lv_obj_set_size(obj, 100, 50);
            lv_obj_add_event_cb(obj, event_handler_cb_main_back_set, LV_EVENT_ALL, NULL);

            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Salir");
                }
            }
        }
        {
            wifi_list = lv_list_create(parent_obj);
            lv_obj_set_pos(wifi_list, 16, 12);
            lv_obj_set_size(wifi_list, 285, 143);
        }
    }

    
}


void create_screen_scree_relays()
{
    lv_obj_t *obj = lv_obj_create(0);
    objects.scree_relays = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 10, 20);
            lv_obj_set_size(obj, 300, 200);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // relays_buttons
            lv_obj_t *obj = lv_btnmatrix_create(parent_obj);
            objects.relays_buttons = obj;
            lv_obj_set_pos(obj, 20, 0);
            lv_obj_set_size(obj, 281, 177);
            static const char *map[12] = {
                "R3",
                "R4",
                "R5",
                "\n",
                "R6",
                "R7",
                "R8",
                "\n",
                "R9",
                "R10",
                "R11",
                NULL,
            };
            static lv_btnmatrix_ctrl_t ctrl_map[9] = {
                1 | LV_BTNMATRIX_CTRL_CHECKABLE,
                1 | LV_BTNMATRIX_CTRL_CHECKABLE,
                1 | LV_BTNMATRIX_CTRL_CHECKABLE,
                1 | LV_BTNMATRIX_CTRL_CHECKABLE,
                1 | LV_BTNMATRIX_CTRL_CHECKABLE,
                1 | LV_BTNMATRIX_CTRL_CHECKABLE,
                1 | LV_BTNMATRIX_CTRL_CHECKABLE,
                1 | LV_BTNMATRIX_CTRL_CHECKABLE,
                1 | LV_BTNMATRIX_CTRL_CHECKABLE,
            };
            lv_btnmatrix_set_map(obj, map);
            lv_btnmatrix_set_ctrl_map(obj, ctrl_map);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_ON);
            lv_obj_set_scroll_dir(obj, LV_DIR_TOP);
        }
        {
            lv_obj_t *obj = lv_btn_create(parent_obj);
            lv_obj_set_pos(obj, 200, 180);
            lv_obj_set_size(obj, 100, 50);
            lv_obj_add_event_cb(obj, event_handler_cb_main_back, LV_EVENT_ALL, NULL);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Volver");
                }
            }
        }
    }

    
}


void create_screen_screen_wifi_credentials()
{
    // Crear pantalla principal
    lv_obj_t *obj = lv_obj_create(NULL);
    objects.screen_wifi_credentials = obj;
    lv_obj_set_size(obj, 320, 240);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);

    // Estilo para el campo de contraseña
    static lv_style_t style_field;
    lv_style_init(&style_field);
    lv_style_set_bg_opa(&style_field, LV_OPA_50);
    lv_style_set_bg_color(&style_field, lv_color_hex(0xFFFFFF));
    lv_style_set_border_width(&style_field, 2);
    lv_style_set_border_color(&style_field, lv_color_hex(0x000000));
    lv_style_set_radius(&style_field, 10);
    lv_style_set_pad_all(&style_field, 10);

    // Campo de texto para la contraseña
    lv_obj_t *pwd_field = lv_textarea_create(obj);
    lv_obj_set_size(pwd_field, 280, 50);
    lv_obj_align(pwd_field, LV_ALIGN_TOP_MID, 0, 5);
    lv_textarea_set_placeholder_text(pwd_field, "Ingrese Password WiFi");
    lv_textarea_set_password_mode(pwd_field, true);
    lv_textarea_set_one_line(pwd_field, true);

    // --- TECLADO GRANDE (3/4 de pantalla) ---
    lv_obj_t *kb = lv_keyboard_create(obj);
    lv_keyboard_set_textarea(kb, pwd_field);
    lv_obj_set_size(kb, 320, 180); // 240px * 0.75 = 180px
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);

    // lv_obj_add_event_cb(pwd_field, pwd_field_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(kb, wifi_credentials_save_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(kb, wifi_credentials_cancel_cb, LV_EVENT_CANCEL, NULL);
}

void create_screens()
{

    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    create_screen_main();
    create_screen_screen_settings();
    create_screen_screen_networklist();
    create_screen_scree_relays();
    create_screen_screen_wifi_credentials();
}
