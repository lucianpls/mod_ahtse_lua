-- A lua script that executes a mod_ahtse_lua script from the command line and prints out the result
-- Usage: lua harness.lua script.lua URI QUERY_STRING
query = arg[3]
headers = nil
notes = { ["URI"] = arg[2] }

dofile(arg[1])

response, oheaders, code = handler(query, headers, notes)

print("CODE: " .. code)
print("Response Headers:")
for key,value in pairs(oheaders)
do
  print(key, value)
end
print()
print("response:")
print(response)
