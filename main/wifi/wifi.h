#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"
#include "esp_wifi_types.h" // for wifi_ap_record_t
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* WiFi credentials - change to your own */
#define WIFI_SSID "ss"
#define WIFI_PASS "sss"
    extern bool ignore_next_disconnect;
// Scan configuration
#define MAX_AP_RECORDS 20
#define TOP_N_AP 10

    void wifi_init_core(void);
    /**
     * @brief WiFi event handler for connection and IP events.
     *
     * @param arg       User data (not used).
     * @param base      Event base.
     * @param id        Event ID.
     * @param data      Event data.
     */

    /**
     * @brief Initialize WiFi in station mode for scanning only.
     *        Registers handlers for scan done and IP events.
     */
    void wifi_init_scan(void);

    /**
     * @brief Initialize WiFi in station mode to connect to an AP.
     *        Uses WIFI_SSID and WIFI_PASS.
     */
    void wifi_init_sta(void);

    /**
     * @brief Perform a blocking WiFi scan and retrieve the top N networks sorted by RSSI.
     *
     * @param ap_list    Output array of at least TOP_N_AP elements.
     * @param ap_count   Pointer to uint16_t where the number of returned APs will be stored.
     * @return esp_err_t ESP_OK on success, error code otherwise.
     */
    esp_err_t wifi_scan_get_top_n(wifi_ap_record_t *ap_list, uint16_t *ap_count);

    esp_err_t wifi_connect_to_network(const char *ssid, const char *password);

    void on_wifi_success(void *user_data);

    /**
     * @brief Ejecutada desde el contexto de LVGL ante fallo de conexión.
     *        Debería cargar una pantalla de error.
     *
     * @param user_data  Puede contener mensaje de fallo o ser NULL.
     */
    void on_wifi_fail(void *user_data);

    esp_err_t wifi_start_scan(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_H
