#ifndef __TVCONFIG_API_H__
#define __TVCONFIG_API_H__

#define CC_CFG_KEY_STR_MAX_LEN                  (128)
#define CC_CFG_VALUE_STR_MAX_LEN                (512)


#define CFG_SECTION_TV                          "TV"
#define CFG_SECTION_ATV                         "ATV"
#define CFG_SECTION_SRC_INPUT                   "SourceInputMap"
#define CFG_SECTION_SETTING                     "SETTING"
#define CFG_SECTION_FBCUART                     "FBCUART"

#define CFG_BLUE_SCREEN_COLOR                   "tvin.bluescreen.color"

extern int tv_config_load(const char *file_name);
extern int tv_config_unload();
extern int tv_config_save();

//[TV] section

extern int config_set_str(const char *section, const char *key, const char *value);
extern const char *config_get_str(const char *section, const char *key, const char *def_value);
extern int config_get_int(const char *section, const char *key, const int def_value);
extern int config_set_int(const char *section, const char *key, const int value);


#endif //__TVCONFIG_API_H__
