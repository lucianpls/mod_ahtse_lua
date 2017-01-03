# mod_ahtse_lua

Apache httpd content generator using Lua

A module using a Lua script file to respond to a get request.  Similar to the CGI mechanism, somewhat customized for AHTSE use.

The lua script file should provide a handler function that takes three arguments and returns three values

### Inputs
* URL parameters as a single string, or nil if there are no parameters
* An input header table
* A notes table.  The *HTTPS* key is set if the apache environment is also set

### Outputs
* Content as string
* A table of output headers, with string keys and strings or number values
* The http return code as number

The first and second return can also be nil. Content-Type header is properly handled, others might not work as expected

## Apache Directives

* ATHSE_lua_RegExp _regexp_
  Regular expressions that have to match the request URL to activate the module.  May appear more than once.

* AHTSE_lua_Script _script_ _function_
  The lua script to run and what function to call.  The script can be lua source or precompiled lua bytecode.  The default function name is _handler_
