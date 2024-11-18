-- mkdroid_wks.lua

local mkdroid = premake.modules.mkdroid
local pworkspace = premake.workspace
local workspace = mkdroid.workspace
local project = premake.project
local make = premake.make
local p = premake

-- Generate the Application.mk build script for a given workspace/solution.
-- This script sets up configurations, ABIs, STL versions, and toolchains based on the solution's configurations.
-- Parameters:
--   - sln: The solution (workspace) for which to generate the Application.mk file.
-- Example usage:
--   mkdroid.workspace.generate_appmk(mySolution)
function mkdroid.workspace.generate_appmk(sln)
    premake.eol("\n")
    workspace.slnBuildScript(sln)
    p.w('MKA_HELP := true')
    
    for cfg in pworkspace.eachconfig(sln) do
        p.w('')
        p.x('ifeq ($(%s),%s)', mkdroid.CONFIG_OPTION, cfg.shortname)
        workspace.slnOptim(sln, cfg)
        workspace.slnAbi(sln, cfg)
        workspace.slnPlatform(sln, cfg)
        workspace.slnStl(sln, cfg)
        workspace.slnToolchainVersion(sln, cfg)
        p.w('  MKA_HELP := false')
        p.w('endif')
    end

    workspace.slnPremakeHelp(sln)
end

-- Generate the main build script for a workspace.
-- This script includes paths to all projects within the workspace.
-- Parameters:
--   - sln: The solution (workspace) for which to generate the main build file.
-- Example usage:
--   mkdroid.workspace.generate(mySolution)
function mkdroid.workspace.generate(sln)
    premake.eol("\n")
    local curpath = 'SOLUTION_'..sln.name:upper()..'_PATH'
    p.w('%s := $(call my-dir)', curpath)
    p.w('')

    local includedFiles = {}  -- Table pour suivre les fichiers inclus

    -- Inclusion des projets
    for prj in pworkspace.eachproject(sln) do
        local prjpath = premake.filename(prj, solution.prjFile(prj))
        local prjrelpath = path.getrelative(sln.location, prjpath)
        if not includedFiles[prjrelpath] then
            p.x('include $(%s)/%s', curpath, prjrelpath)
            includedFiles[prjrelpath] = true  -- Marquer comme inclus
        end
    end

    -- Inclusion par configuration
    for cfg in pworkspace.eachconfig(sln) do
        local existingmklist = {}
        local moduleslist = {}

        -- Agrégation des chemins Android.mk uniques et des imports de modules pour chaque projet par configuration.
        for prj in pworkspace.eachproject(sln) do
            for prjcfg in project.eachconfig(prj) do
                if prjcfg.shortname == cfg.shortname then
                    for _, mkpath in ipairs(prj.local_includes) do
                        local mkrelpath = path.getrelative(sln.location, mkpath)
                        if not table.contains(existingmklist, mkrelpath) and not includedFiles[mkrelpath] then
                            table.insert(existingmklist, mkrelpath)
                        end
                    end
                    for _, mod in ipairs(prj.local_importModules) do
                        if not table.contains(moduleslist, mod) then
                            table.insert(moduleslist, mod)
                        end
                    end
                end
            end
        end

        if #existingmklist > 0 or #moduleslist > 0 then
            p.w('')
            p.x('ifeq ($(%s),%s)', mkdroid.CONFIG_OPTION, cfg.shortname)
            for _, mkpath in ipairs(existingmklist) do
                p.x('  include $(%s)/%s', curpath, mkpath)
                includedFiles[mkpath] = true  -- Marquer comme inclus
            end
            for _, mod in ipairs(moduleslist) do
                p.x('  $(call import-module,%s)', mod)
            end
            p.w('endif')
        end
    end
end

-- Define the application build script path for the solution.
-- Parameters:
--   - sln: The solution (workspace) for which to set the build script path.
-- Example usage:
--   mkdroid.workspace.slnBuildScript(mySolution)
function mkdroid.workspace.slnBuildScript(sln)
    p.x('APP_BUILD_SCRIPT := $(call my-dir)/%s', solution.androidFile(sln))
end

-- Generate help messages for the build configuration in case of missing configuration.
-- Parameters:
--   - sln: The solution (workspace) for which to display help.
-- Example usage:
--   mkdroid.workspace.slnPremakeHelp(mySolution)
function mkdroid.workspace.slnPremakeHelp(sln)
    p.w('')
    p.w('ifeq ($(MKA_HELP),true)')
    p.w('  $(info )')
    p.w('  $(info Usage:)')
    p.w('  $(info $()  ndk-build %s=<config>)', mkdroid.CONFIG_OPTION)
    p.w('  $(info )')
    p.w('  $(info CONFIGURATIONS:)')
    for cfg in pworkspace.eachconfig(sln) do
        p.w('  $(info $()  %s)', cfg.shortname)
    end
    p.w('  $(info )')
    p.w('  $(info For more ndk-build options, see https://developer.android.com/ndk/guides/ndk-build.html)')
    p.w('  $(info )')
    p.w('  $(error Set %s and try again)', mkdroid.CONFIG_OPTION)
    p.w('endif')
end

-- Ensure that a specific configuration option is consistent across all projects.
-- Parameters:
--   - sln: The solution (workspace).
--   - cfg: The configuration to check.
--   - option: The option to aggregate and validate.
-- Returns: The aggregated option value, or nil if it's set to "default".
-- Example usage:
--   local opt = agregateOption(mySolution, config, "optimize")
local function agregateOption(sln, cfg, option)
    local first = true
    local val
    for prj in pworkspace.eachproject(sln) do
        for prjcfg in project.eachconfig(prj) do
            if prjcfg.shortname == cfg.shortname then
                if first then
                    first = false
                    val = prjcfg[option]
                else
                    if prjcfg[option] ~= val then
                        error("Value for "..option.." must be the same on every project for configuration "..cfg.longname.." in solution "..sln.name)
                    end
                end
            end
        end
    end
    if val == "default" then
        return nil
    end
    return val
end

-- Configure optimization level in the Application.mk for a given configuration.
-- Parameters:
--   - sln: The solution (workspace).
--   - cfg: The configuration to set optimization for.
-- Example usage:
--   mkdroid.workspace.slnOptim(mySolution, config)
function mkdroid.workspace.slnOptim(sln, cfg)
    local optim = agregateOption(sln, cfg, "optimize")
    if optim == p.OFF or optim == "Debug" then
        p.w('  APP_OPTIM := debug')
    elseif optim ~= nil then
        p.w('  APP_OPTIM := release')
    end
end

-- Set the ABI (Application Binary Interface) for the configuration.
-- Parameters:
--   - sln: The solution (workspace).
--   - cfg: The configuration to set the ABI for.
-- Example usage:
--   mkdroid.workspace.slnAbi(mySolution, config)
function mkdroid.workspace.slnAbi(sln, cfg)
    local abi = agregateOption(sln, cfg, "mka_abi")
    if abi then
        p.w('  APP_ABI := %s', abi)
    end
end

-- Set the platform API level for the configuration.
-- Parameters:
--   - sln: The solution (workspace).
--   - cfg: The configuration to set the platform API level for.
-- Example usage:
--   mkdroid.workspace.slnPlatform(mySolution, config)
function mkdroid.workspace.slnPlatform(sln, cfg)
    local plat = agregateOption(sln, cfg, "mka_platform")
    if plat then
        p.w('  APP_PLATFORM := android-%s', plat)
    end
end

-- Set the STL (Standard Template Library) type for the configuration.
-- Parameters:
--   - sln: The solution (workspace).
--   - cfg: The configuration to set the STL type for.
-- Example usage:
--   mkdroid.workspace.slnStl(mySolution, config)
function mkdroid.workspace.slnStl(sln, cfg)
    local stl = agregateOption(sln, cfg, "mka_stl")
    if stl then
        p.w('  APP_STL := %s', stl)
    end
end

-- Set the NDK toolchain version for the configuration.
-- Parameters:
--   - sln: The solution (workspace).
--   - cfg: The configuration to set the toolchain version for.
-- Example usage:
--   mkdroid.workspace.slnToolchainVersion(mySolution, config)
function mkdroid.workspace.slnToolchainVersion(sln, cfg)
    local toolchain = agregateOption(sln, cfg, "mka_toolchain")
    if toolchain then
        p.w('  NDK_TOOLCHAIN_VERSION := %s', toolchain)
    end
end

return mkdroid.workspace
