-- Based on parts of telescope-fzf-native.nvim (MIT License)
-- Original author: Simon Hauser
--
local fuzzy_sorter = require("fzs_lib")
local sorters = require("telescope.sorters")

local get_fuzzy_sorter = function(_) --todo use opts - for what?
	-- local clear_filter_fun = function(self, prompt)
	--   local filter = '^(' .. self._delimiter .. '(%S+)' .. '[' .. self._delimiter .. '%s]' .. ')'
	--   local matched = prompt:match(filter)
	--
	--   if matched == nil then
	--     return prompt
	--   end
	--   return prompt:sub(#matched + 1, -1)
	-- end

	return sorters.Sorter:new({
		init = function(self)
			if self.filter_function then
				self.__highlight_prefilter = nil
			end
		end,
		destroy = function(_) end,
		start = function(_, _) end,
		discard = true,
		scoring_function = function(_, prompt, line)
			return fuzzy_sorter.get_score(line, prompt)
		end,
		highlighter = function(self, prompt, display)
			return fuzzy_sorter.get_pos(display, prompt)
		end,
	})
end

local fast_extend = function(opts, conf)
	local ret = {}
	ret.case_mode = vim.F.if_nil(opts.case_mode, conf.case_mode)
	ret.fuzzy = vim.F.if_nil(opts.fuzzy, conf.fuzzy)
	return ret
end

local wrap_sorter = function(conf)
	return function(opts)
		opts = opts or {}
		return get_fuzzy_sorter(fast_extend(opts, conf))
	end
end

return require("telescope").register_extension({
	setup = function(ext_config, config)
		local override_file = vim.F.if_nil(ext_config.override_file_sorter, true)
		local override_generic = vim.F.if_nil(ext_config.override_generic_sorter, true)

		local conf = {}
		conf.case_mode = vim.F.if_nil(ext_config.case_mode, "smart_case")
		conf.fuzzy = vim.F.if_nil(ext_config.fuzzy, true)

		if override_file then
			config.file_sorter = wrap_sorter(conf)
		end

		if override_generic then
			config.generic_sorter = wrap_sorter(conf)
		end
	end,
	exports = {
		fuzzy_sorter = function(opts)
			return get_fuzzy_sorter(opts or { case_mode = "smart_case", fuzzy = true })
		end,
	},
	health = function()
		local health = vim.health or require("health")
		local ok = health.ok or health.report_ok
		local warn = health.warn or health.report_warn
		local error = health.error or health.report_error

		local good = true
		local eq = function(expected, actual)
			if tostring(expected) ~= tostring(actual) then
				vim.notify(
					string.format("actual '%s' doesn't match '%s'", tostring(actual), tostring(expected)),
					vim.log.levels.WARNING
				)
				good = false
			end
		end

		local p = "fuzzy"
		eq(1 / 100, fuzzy_sorter.get_score("src/fuzzy.cpp", p))
		eq(-1, fuzzy_sorter.get_score("src/strict.cpp", p))
		eq(1 / 80, fuzzy_sorter.get_score("src/fiuzzay.h", p))

		if good then
			ok("lib working as expected")
		else
			error("lib not working as expected, please reinstall and open an issue if this error persists")
			return
		end

		local has, config = pcall(require, "telescope.config")
		if not has then
			error("unexpected: telescope configuration couldn't be loaded")
		end

		local test_sorter = function(name, sorter)
			good = true
			sorter:_init()
			local prompt = "fuzzy"
			eq(1 / 100, sorter:scoring_function(prompt, "src/fuzzy.cpp"))
			eq(-1, sorter:scoring_function(prompt, "lua/fzf.lua"))
			eq(-1, sorter:scoring_function(prompt, "src/lazzy.h"))
			eq(1 / 80, sorter:scoring_function(prompt, "fiuzzay"))
			sorter:_destroy()

			if good then
				ok(name .. " correctly configured")
			else
				warn(name .. " is not configured")
			end
		end
		test_sorter("file_sorter", config.values.file_sorter({}))
		test_sorter("generic_sorter", config.values.generic_sorter({}))
	end,
})
