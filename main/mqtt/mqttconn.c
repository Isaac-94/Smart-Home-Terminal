#include "mqttconn.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "MQTT_CONN";
static esp_mqtt_client_handle_t mqtt_client = NULL;

extern const uint8_t ca_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t ca_cert_pem_end[] asm("_binary_ca_cert_pem_end");

// Callback para manejar eventos MQTT
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Conectado al broker");
        esp_mqtt_client_subscribe(client, MQTT_SUB_TOPIC, 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Desconectado del broker");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "Mensaje recibido");
        printf("Tópico: %.*s\n", event->topic_len, event->topic);
        printf("Datos: %.*s\n", event->data_len, event->data);

        // Copiar los datos recibidos a una nueva cadena
        char *payload = malloc(event->data_len + 1);
        if (payload == NULL)
        {
            ESP_LOGE(TAG, "Error al asignar memoria para el payload");
            break;
        }
        memcpy(payload, event->data, event->data_len);
        payload[event->data_len] = '\0'; // Asegurar terminación nula

        // Llamar a la función asincrónica con la copia del payload
        lv_async_call(relay_state_change, payload);
        break;
    default:
        ESP_LOGI(TAG, "Evento MQTT no manejado: %d", event->event_id);
        break;
    }
}

// Inicializa y arranca el cliente MQTT
void mqtt_app_start(void)
{
    ESP_LOGI(TAG, "Initialize MQTT");

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .broker.verification.certificate = (const char *)ca_cert_pem_start,
        .broker.verification.skip_cert_common_name_check = true,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

// Publica un mensaje en un tópico específico
esp_err_t mqtt_publish(const char *topic, const char *data, int qos, int retain)
{
    if (mqtt_client == NULL)
    {
        ESP_LOGE(TAG, "Cliente MQTT no inicializado");
        return ESP_FAIL;
    }

    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, data, 0, qos, retain);
    if (msg_id == -1)
    {
        ESP_LOGE(TAG, "Error al publicar el mensaje");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Mensaje publicado, ID: %d", msg_id);
    return ESP_OK;
}

// Encola un mensaje MQTT para su publicación
void publicar_mensaje_mqtt(const char *topico, const char *mensaje)
{
    int msg_id = esp_mqtt_client_enqueue(mqtt_client, topico, mensaje, 0, 1, 0, true);
    if (msg_id == -1)
    {
        ESP_LOGW(TAG, "Error al encolar el mensaje MQTT");
    }
    else
    {
        ESP_LOGI(TAG, "Mensaje MQTT encolado con ID: %d", msg_id);
    }
}