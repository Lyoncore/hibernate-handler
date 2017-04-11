#define RS9113_DRIVER_NAME "ven_rsi_sdio"
#define RS9113_MODE_OPTIONS "dev_oper_mode="
#define BUFFER_SZ 256

/*
 *   1 - Wi-Fi alone mode
 *   4 - BT alone mode
 *   8 - BT LE alone mode
 *   5 - Wi-Fi station + BT classic mode
 *   9 - Wi-Fi station + BT LE mode
 *   13 - Wi-Fi station + BT dual mode
 *   6 - Wi-Fi AP + BT classic mode
 *   14 - Wi-Fi AP + BT dual mode
 */
#define RS9113_MODE_WIFI_ALONE "1"
#define RS9113_MODE_BT_ALONE "4"
#define RS9113_MODE_BT_LE_ALONE "8"
#define RS9113_MODE_STA_BT_CLASSIC "5"
#define RS9113_MODE_STA_BT_LE "9"
#define RS9113_MODE_STA_BT_DUAL "13"
#define RS9113_MODE_AP_BT_CLASSIC "6"
#define RS9113_MODE_AP_BT_DUAL "14"
#define RS9113_DEFAULT_MODE RS9113_MODE_STA_BT_DUAL

int parse_config_file(const char *filename, char *options);
