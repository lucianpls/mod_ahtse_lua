/*
 * mod_ahtse_lua header file
 * Lucian Plesea
 * (C) 2016
 */

#if !defined(MOD_AHTSE_LUA)
#define MOD_AHTSE_LUA

#include <http_protocol.h>
#include <http_config.h>

#if defined(APLOG_USE_MODULE)
APLOG_USE_MODULE(ahtse_lua);
#endif

struct ahtse_lua_conf {
    const char *doc_path;
    const char *script_name;
    const char *func;
    apr_array_header_t *regexp;
};

extern module AP_MODULE_DECLARE_DATA ahtse_lua_module;

#endif