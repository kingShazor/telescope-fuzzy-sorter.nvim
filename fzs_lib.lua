local ffi = require 'ffi'
local library_path = (function()
  local dirname = string.sub(debug.getinfo(1).source, 2, #'/fzs_lib.lua' * -1)
  if package.config:sub(1, 1) == '\\' then
    return dirname .. '../build/libfuzzy_sorter.dll'
  else
    return dirname .. '/build/libfuzzy_sorter.so'
  end
end)()
local native = ffi.load(library_path)

ffi.cdef [[
  typedef struct {
    uint32_t *data;
    size_t size;
  } fzs_position_t;

  fzs_position_t *fzs_get_positions(const char *text, const char *pattern);
  int32_t fzs_get_score(const char *text, const char *pattern);
]]

local fzs = {}

fzs.get_score = function(input, pattern)
  return native.fzs_get_score(input, pattern)
end

fzs.dir = '~/.config/nvim/lua/plugins'
fzs.name = 'fuzzy_sorter'
fzs.lazy = false -- direkt beim Start laden

fzs.get_pos = function(input, pattern)
  local pos = native.fzs_get_positions(input, pattern)
  if pos == nil then
    return
  end

  local res = {}
  for i = 1, tonumber(pos.size) do
    res[i] = pos.data[i - 1] + 1
  end

  return res
end

return fzs
