# mod_ahtse_lua

Apache httpd content generator using Lua

A module using Lua to respond to a get request.  Similar to the CGI mechanism, but customized for AHTSE use.

## Apache Directives

* ATHSE_lua_RegExp _regexp_
  Regular expressions that have to match the request URL to activate the module.  May appear more than once.

* AHTSE_lua_Script _script_ _function_
  The lua script to run and what function to call.  The default function name is _handler_
