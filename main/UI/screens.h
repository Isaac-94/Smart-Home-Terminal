#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>
#include <wifi.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct _objects_t
    {
        lv_obj_t *main;
        lv_obj_t *screen_settings;
        lv_obj_t *screen_networklist;
        lv_obj_t *scree_relays;
        lv_obj_t *settings_btn;
        lv_obj_t *list_relay_btn;
        lv_obj_t *img_weather;
        lv_obj_t *r1;
        lv_obj_t *obj0;
        lv_obj_t *obj1;
        lv_obj_t *relays_buttons;
        lv_obj_t *screen_wifi_credentials;
        lv_obj_t *wifi_password_field; 
    } objects_t;

    extern objects_t objects;

    enum ScreensEnum
    {
        SCREEN_ID_MAIN = 1,
        SCREEN_ID_SCREEN_SETTINGS = 2,
        SCREEN_ID_SCREEN_NETWORKLIST = 3,
        SCREEN_ID_SCREE_RELAYS = 4,
    };

    void show_networks(wifi_ap_record_t *aps, uint16_t count);

    void create_screen_main();
    void tick_screen_main();

    void create_screen_screen_settings();
    void tick_screen_screen_settings();

    void create_screen_screen_networklist();
    void tick_screen_screen_networklist();

    void create_screen_scree_relays();
    void tick_screen_scree_relays();

    void tick_screen_by_id(enum ScreensEnum screenId);
    void tick_screen(int screen_index);

    void create_screens();
    

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/