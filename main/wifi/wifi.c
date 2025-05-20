#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "wifi.h"
#include "lvgl.h"
// #include "esp_event_loop.h"
static const char *TAG = "WIFI";
bool ignore_next_disconnect = false;

//---------------------------------------------------------
// Callback de eventos: START | DISCONNECTED | GOT_IP
//---------------------------------------------------------
static void wifi_event_handler(void *arg,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    if (base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        // Arrancamos conexión solo si no venimos de un scan manual
        esp_wifi_connect();
    }
    else if (base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (ignore_next_disconnect)
        {
            ignore_next_disconnect = false; // Resetear el flag
            ESP_LOGI(TAG, "Desconexión intencional ignorada.");
            return;
        }
        else
        {
            ESP_LOGI(TAG, "Desconectado");
        }

        lv_async_call(on_wifi_fail, NULL);
    }
    else if (base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "IP obtenida: " IPSTR, IP2STR(&evt->ip_info.ip));
        lv_async_call(on_wifi_success, NULL);
    }
}

//---------------------------------------------------------
// Callback de scan done
//---------------------------------------------------------
static void scan_done_handler(void *arg,
                              esp_event_base_t base,
                              int32_t event_id,
                              void *event_data)
{
    uint16_t ap_num = 0;
    esp_wifi_scan_get_ap_num(&ap_num);
    ESP_LOGI(TAG, "Scan done: %d APs encontrados", ap_num);
    // Aquí puedes copiar resultados y llamar a un callback UI, por ej:
    // lv_async_call(on_scan_complete, &ap_num);
}

//---------------------------------------------------------
// Función de inicialización única
//---------------------------------------------------------
void wifi_init_core(void)
{
    //  NVS (solo una vez)
    ESP_ERROR_CHECK(nvs_flash_init());
    // TCP/IP y event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    //  Init driver Wi-Fi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //  Registrar handlers (solo una vez)
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &scan_done_handler, NULL, NULL));

    // Configurar modo STA (sin iniciar conexión aún)
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi core iniciado");
}

//---------------------------------------------------------
//  Función para iniciar un escaneo manual
//---------------------------------------------------------
esp_err_t wifi_start_scan(void)
{
    // Desconectar antes de scan (opcional, mejora fiabilidad)
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    // Parámetros de scan: NULL = default (pasivo, 0s, todos canales)
    esp_err_t err = esp_wifi_scan_start(NULL, true); //blocking
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Scan iniciado");
    }
    return err;
}

//---------------------------------------------------------
// Función para conectar a un SSID/Pass dado
//---------------------------------------------------------
esp_err_t wifi_connect_to_network(const char *ssid, const char *password)
{
    wifi_config_t sta_cfg = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char *)sta_cfg.sta.ssid, ssid, sizeof(sta_cfg.sta.ssid));
    strncpy((char *)sta_cfg.sta.password, password, sizeof(sta_cfg.sta.password));

    ESP_LOGI(TAG, "Configurando SSID=%s", ssid);
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_cfg));

    // Cancelar scan en curso si hay
    esp_wifi_scan_stop();
    // Iniciar conexión
    return esp_wifi_connect();
}