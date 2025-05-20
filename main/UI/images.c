#include "images.h"

const ext_img_desc_t images[11] = {
    { "cloudy", &img_cloudy },
    { "sunny", &img_sunny },
    { "fog", &img_fog },
    { "flurries", &img_flurries },
    { "changesflurries", &img_changesflurries },
    { "Partlycloud", &img_partlycloud },
    { "ntPartlycloud", &img_nt_partlycloud },
    { "nt_sunny", &img_nt_sunny },
    { "rain", &img_rain },
    { "snow", &img_snow },
    { "storm", &img_storm },
};
