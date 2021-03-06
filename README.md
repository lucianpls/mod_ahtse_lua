# mod_ahtse_lua [AHTSE](https://github.com/lucianpls/AHTSE)

Apache httpd content generator using [Lua](https://www.lua.org/)

A module using a Lua script file to respond to a get request.  Similar to the CGI mechanism, somewhat customized for AHTSE use.
The lua script file should define a handler function that takes three arguments and returns three values.

**Since it involves executing a script, the use of this module may increase server vulnerability**

### Inputs to Lua
* URL parameter string, or nil if there are no parameters
* A table of input headers
* A table of notes.  The *HTTPS* key is set if the matching apache environment variable is also set

### Outputs from Lua
* Content
* A table of output headers
* The http status code

The *Content-Type* header is properly handled, others might not work as expected

## Apache Directives

* AHTSE_lua_RegExp _regexp_  
  Regular expressions that have to match the request URL to activate the module.  May appear more than once.

* AHTSE_lua_Script _script_ _function_  
  The lua script to run and what function to call.  The script can be lua source or precompiled lua bytecode.  The default function name is _handler_

* AHTSE_lua_Redirect On  
  When the lua script returns a redirect status code and a Location header, issue an internal redirect to that location.  Default is to use the response as is.
  Recognized redirect codes are 301, 302, 307 and 308.

* AHTSE_lua_KeepAlive On  
  Preserve the initialized lua state between requests on the same input connection.  By default a new lua state is instantiated for each request.  This setting can improve performance, but should be used only when the lua state is not modified between requests.

## Lua

 * sample_script.lua  
   An example server side lua script, returns the query string wrapped in JSON

 * harness.lua  
   Executes a Lua script as if invoked by this module, for script testing
