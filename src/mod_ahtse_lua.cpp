#include "mod_ahtse_lua.h"
#include <apr_strings.h>

static int handler(request_rec *r)
{
    return DECLINED;
}

// Allow for one or more RegExp guard
// One of them has to match if the request is to be considered
static const char *set_regexp(cmd_parms *cmd, ahtse_lua_conf *c, const char *pattern)
{
    char *err_message = NULL;
    if (c->regexp == 0)
        c->regexp = apr_array_make(cmd->pool, 2, sizeof(ap_regex_t));
    ap_regex_t *m = (ap_regex_t *)apr_array_push(c->regexp);
    int error = ap_regcomp(m, pattern, 0);
    if (error) {
        int msize = 2048;
        err_message = (char *)apr_pcalloc(cmd->pool, msize);
        ap_regerror(error, m, err_message, msize);
        return apr_pstrcat(cmd->pool, "Bad Regular Expression ", err_message, NULL);
    }
    return NULL;
}

// Sets the script and possibly the function name to be called
static const char *set_script(cmd_parms *cmd, ahtse_lua_conf *c, const char *script, const char *func)
{
    c->script_name = apr_pstrcat(cmd->pool, script);
    c->func = (func) ? apr_pstrcat(cmd->pool, func) : "handler";
}

static void *create_dir_config(apr_pool_t *p, char *path)
{
    ahtse_lua_conf *c = reinterpret_cast<ahtse_lua_conf *>(apr_pcalloc(p, sizeof(ahtse_lua_conf)));
    c->doc_path = path;
    return c;
}

static void register_hooks(apr_pool_t *p) {
    ap_hook_handler(handler, NULL, NULL, APR_HOOK_MIDDLE);
}

static const command_rec cmds[] = {
    AP_INIT_TAKE12(
    "AHTSE_lua_Script",
    (cmd_func)set_script, // Callback
    0, // Self-pass argument
    ACCESS_CONF, // availability
    "TWMS configuration file"
    ),

    AP_INIT_TAKE1(
    "ATHSE_lua_RegExp",
    (cmd_func)set_regexp,
    0, // Self-pass argument
    ACCESS_CONF, // availability
    "Regular expression for URL matching.  At least one is required."),

    { NULL }
};

module AP_MODULE_DECLARE_DATA ahtse_lua_module = {
    STANDARD20_MODULE_STUFF,
    create_dir_config,
    NULL,
    NULL,
    NULL,
    cmds,
    register_hooks
};