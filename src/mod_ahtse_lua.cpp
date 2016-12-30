#include "mod_ahtse_lua.h"
#include <apr_strings.h>
#include <http_log.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static bool our_request(request_rec *r) {
    if (r->method_number != M_GET) return false;
    apr_array_header_t *regexp_table = ((ahtse_lua_conf *)
            ap_get_module_config(r->per_dir_config, &ahtse_lua_module))->regexp;

    if (!regexp_table) return false;
    char *url_to_match = apr_pstrcat(r->pool, r->uri, r->args ? "?" : NULL, r->args, NULL);
    for (int i = 0; i < regexp_table->nelts; i++) {
        ap_regex_t *m = &APR_ARRAY_IDX(regexp_table, i, ap_regex_t);
        if (!ap_regexec(m, url_to_match, 0, NULL, 0))
            return true;
    }
    return false;
}

#define error_from_lua(S) static_cast<const char *>(apr_pstrcat(r->pool, S, lua_tostring(L, -1), NULL))

static int handler(request_rec *r)
{
    if (!our_request(r))
        return DECLINED;

    ahtse_lua_conf * c = (ahtse_lua_conf *)
        ap_get_module_config(r->per_dir_config, &ahtse_lua_module);

    lua_State *L = NULL;

    // Initialize Lua script, push the function to be called
    try {
        L = luaL_newstate();
        if (!L)
            throw "Lua state allocation error";
        // Some magic, this can't err
        luaL_openlibs(L);
        if (LUA_OK != luaL_loadbuffer(L, reinterpret_cast<const char *>(c->script), c->script_len, c->func)
            || LUA_OK != lua_pcall(L, 0, 0, 0))
            throw error_from_lua("Lua initialization script ");

        if (LUA_TFUNCTION != lua_getglobal(L, c->func))
            throw "Lua function not found";

    }
    catch (const char *msg) {
        if (L)
            lua_close(L);
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, msg);
        return HTTP_INTERNAL_SERVER_ERROR;
        L = NULL;
    }

    int status = OK;

    // Now execute the lua script and send the result, if any
    try {
        // The function takes one parameter, the arguments for the request, or nil
        if (r->args)
            lua_pushstring(L, r->args);
        else
            lua_pushnil(L);

        // returns content, headers and code
        int err = lua_pcall(L, 1, 3, 0);
        if (LUA_OK != err)
            throw error_from_lua("Lua execution error ");

        // Get the return code
        int isnum;
        status = static_cast<int>(lua_tointegerx(L, -1, &isnum));
        lua_pop(L, 1); // Remove the code

        if (!isnum)
            throw "Lua third return should be an http numeric status code";

        // 200 means all OK
        if (HTTP_OK == status)
            status = OK;
        
        if (status != OK) // Can't set headers or content
            throw reinterpret_cast<const char *>(NULL);

        int type = lua_type(L, -1);
        if (type != LUA_TTABLE || type != LUA_TNIL)
            throw "Lua second return should be nil or table";

        // No table, no headers
        if (type == LUA_TTABLE) {
            lua_pushnil(L); // First key
            while (lua_next(L, -2)) {
                // Key at -2, value at -1
                if (!(lua_isstring(L, -1) && lua_isstring(L, -2)))
                    throw "Lua header table non-string key or value found";
                const char * key = lua_tostring(L, -2);
                const char * val = lua_tostring(L, -1);
                apr_table_set(r->headers_out, key, val);
                lua_pop(L, 1); // Pop the key
            }
        }

        lua_pop(L, 1);

        const char *result = NULL;
        size_t size = 0;

        // Content, could be nil
        type = lua_type(L, -1);

        if (type != LUA_TNIL)
            result = lua_tolstring(L, -1, &size);

        lua_pop(L, 1);

        if (size) { // Got this far, send the result if any
            ap_set_content_length(r, size);
            ap_rwrite(result, size, r);
        }
    }
    catch (const char *msg) {
        if (msg) { // No message means early exit
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, msg);
            status = HTTP_INTERNAL_SERVER_ERROR;
        }
    }

    lua_close(L);
    return status;
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
    // Read the script file, makes it faster if the file is small and self-contained
    apr_finfo_t status;
    if (APR_SUCCESS != apr_stat(&status, script, APR_FINFO_SIZE, cmd->temp_pool))
        return apr_pstrcat(cmd->pool, "Can't stat ", script, NULL);
    c->script_len = static_cast<apr_size_t>(status.size);
    c->script = apr_palloc(cmd->pool, c->script_len);
    if (!c->script)
        return "Can't allocate memory for lua script";
    apr_file_t *thefile;
    if (APR_SUCCESS != apr_file_open(&thefile, script, APR_FOPEN_READ | APR_FOPEN_BINARY, 0, cmd->temp_pool))
        return apr_pstrcat(cmd->pool, "Can't open ", script, NULL);

    apr_size_t bytes_read;
    if (APR_SUCCESS != apr_file_read_full(thefile, c->script, c->script_len, &bytes_read)
        || bytes_read != c->script_len)
        return apr_pstrcat(cmd->pool, "Can't read ", script, NULL);

    // Get the function name too, if present, otherwise use "handler"
    c->func = (func) ? apr_pstrcat(cmd->pool, func) : "handler";
    return NULL;
}

static void *create_dir_config(apr_pool_t *p, char *path)
{
    ahtse_lua_conf *c = (ahtse_lua_conf *)(apr_pcalloc(p, sizeof(ahtse_lua_conf)));
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