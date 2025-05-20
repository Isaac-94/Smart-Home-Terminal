# Smart Home Terminal

Terminal embebido desarrollado para el control y monitoreo de una casa inteligente. Utiliza un ESP32 con pantalla SPI (ILI9341) y una interfaz gráfica creada con **LVGL**.

## 🚀 Características principales

- Interfaz táctil intuitiva basada en **LVGL**
- Conexión Wi-Fi a redes disponibles desde la interfaz
- Escaneo de redes Wi-Fi y conexión dinámica
- Visualización del estado de dispositivos (relés)
- Lectura y visualización del clima actual (vía sensores o futura API)
- Navegación por pantallas para configuración, estado y control
- Control modular de dispositivos de domótica

## 🔧 Tecnologías utilizadas

- **ESP-IDF 5.3**
- **LVGL v8.3**
- **ESP32-S3**
- Pantalla SPI: **ILI9341** (240x320)
- Touch: **XPT2046**
- Lenguaje C

## 🗂️ Estructura del proyecto

```
Smart_Home_app/
├── main/
│   ├── UI/                # Código de pantallas LVGL, imagenes, etc
│   ├── wifi/              # Funciones de control de WIFI
│   ├── Smart_Home_app.c   # Código fuente principal y config LCD
│   └── CMakeLists.txt
├── README.md
├── .gitignore
└── CMakeLists.txt
```

## 🛠️ Compilación y carga

1. Instalar [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
2. Clonar el proyecto:
   ```bash
   git clone https://github.com/tu-usuario/Smart_Home_app.git
   cd Smart_Home_app
   ```
3. Compilar y flashear:
   ```bash
   idf.py build
   idf.py -p COMx flash
   ```

## 📋 Pendientes / Futuras mejoras

- Integración con plataforma IoT externa
- Control vía MQTT
- Soporte para más sensores
- Almacenamiento de configuración en NVS
- Separar configuración LCD de main.

## 📷 Interfaz actual
![Pantalla de conexión Wi-Fi](/assets/wifi.jpg)
![Pantalla main](/assets/main.jpg)

## 📄 Licencia

Este proyecto está en desarrollo. Podés usarlo para fines educativos o personales.
