-- mkdroid.lua
-- Android Makefile Premake module for managing project configurations


premake.modules.mkdroid = {}
local mkdroid = premake.modules.mkdroid
local p = premake

-- Define version information
mkdroid._VERSION = "1.0.0-alpha3"  -- Current development version
mkdroid.CONFIG_OPTION = "MKA_CONFIG"  -- Configuration option name used for mkdroid builds
mkdroid.project = {}
mkdroid.workspace = {}
mkdroid.android = {}

-- Custom action to export project configurations in mkdroid-compatible format
newaction {
	trigger = "mkdroid",
	description = "Export project information as mkdroid tables",

	onStart = function()
	end,

	onWorkspace = function(wks)
		-- Generate workspace-level files for mkdroid
		premake.escaper(mkdroid.escaper)
		premake.generate(wks, solution.appFile(wks), mkdroid.workspace.generate_appmk)
		premake.generate(wks, solution.androidFile(wks), mkdroid.workspace.generate)
	end,

	onProject = function(prj)
		-- Generate project-level files, accessing workspace directly from project
		local wks = prj.workspace
		premake.escaper(mkdroid.escaper)
		premake.generate(prj, solution.prjFile(prj), 
			function()
				mkdroid.project.generate(wks, prj)  -- Pass workspace and project to generator
			end
		)
	end,	

	onCleanSolution = function(sln)
		-- Clean up generated workspace-level files
		premake.clean.file(sln, solution.appFile(sln))
		premake.clean.file(sln, solution.androidFile(sln))
	end,
  
	onCleanProject = function(prj)
	  	-- Clean up generated project-level files
	  	premake.clean.file(prj, solution.prjFile(prj))
	end,

	execute = function()
		-- Execute any additional actions during generation if needed
	end,

	onEnd = function()
		-- Any actions to perform after generation
	end
}

-- Function to escape values for safe inclusion in generated files
function mkdroid.escaper(value)
	value = value:gsub('\\', '\\\\')
	value = value:gsub('"', '\\"')
	return value
end

-- Function to print defined variables for a given project or workspace
function mkdroid.printVariables(prj_name)
	-- Print workspace-level variables if they exist
	if solution.variables.values.workspace then
		for name, value in pairs(solution.variables.values.workspace) do
			p.w("%s := %s", name, value)
		end
	end
  
	-- Print project-level variables if they exist and are not the workspace
	if prj_name ~= "workspace" and solution.variables.values[prj_name] then
		for name, value in pairs(solution.variables.values[prj_name]) do
			p.w("%s := %s", name, value)
		end
	end
end

-- Additional mkdroid modules
include "mkdroid_android.lua"
include "mkdroid_sln.lua"
include "mkdroid_prj.lua"
include "mkdroid_wks.lua"

return m
