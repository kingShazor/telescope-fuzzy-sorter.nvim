-- Based on parts of telescope-fzf-native.nvim (MIT License)
-- Original author: Simon Hauser
--
local fuzzy_sorter = require("fzs_lib")
local sorters = require("telescope.sorters")

local get_fuzzy_sorter = function() --todo use opts - for what?
	return sorters.Sorter:new({
		init = function(self)
			if self.filter_function then
				self.__highlight_prefilter = nil
			end
		end,
		destroy = nil,
		start = nil,
		discard = true,
		scoring_function = function(_, prompt, line)
			return fuzzy_sorter.get_score(line, prompt)
		end,
		highlighter = function(_, prompt, display)
			return fuzzy_sorter.get_pos(display, prompt)
		end,
	})
end

return require("telescope").register_extension({
	setup = function(ext_config, config)
		local override_file = vim.F.if_nil(ext_config.override_file_sorter, true)
		local override_generic = vim.F.if_nil(ext_config.override_generic_sorter, true)

		-- conf.case_mode = vim.F.if_nil(ext_config.case_mode, "smart_case")
		-- conf.fuzzy = vim.F.if_nil(ext_config.fuzzy, true)

		if override_file then
			config.file_sorter = get_fuzzy_sorter
		end

		if override_generic then
			config.generic_sorter = get_fuzzy_sorter
		end
	end,
	exports = {
		fuzzy_sorter = get_fuzzy_sorter,
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
