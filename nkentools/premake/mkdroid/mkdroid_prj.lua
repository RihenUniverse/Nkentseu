-- mkdroid_prj.lua
-- Generator for Android.mk project files
-- Author: Bastien Brunnenstein

local make = premake.make
local project = premake.project
local api = premake.api
local mkdroid = premake.modules.mkdroid
local p = premake
local api = premake.api

function mkdroid.project.generate(wks, prj)
    local can_enter = false
    premake.eol("\n")

    -- Générer les entêtes du projet
    mkdroid.project.header(prj, wks)

    -- Préparer la liste des fichiers sources
    local rootFiles, cfgFiles = mkdroid.project.prepareSrcFiles(prj)
    mkdroid.project.srcFiles(rootFiles)
    
    -- Gérer les configurations spécifiques
    for cfg in project.eachconfig(prj) do
        p.w('')
        p.x('ifeq ($(%s),%s)', mkdroid.CONFIG_OPTION, cfg.shortname)

        mkdroid.project.includes(prj, cfg, wks)
        mkdroid.project.cppFeatures(prj, cfg)
        mkdroid.project.cfgSrcFiles(cfgFiles[cfg])
        mkdroid.project.ldFlags(prj, cfg)
        mkdroid.project.cFlags(prj, cfg)

        -- Gestion de la configuration ABI
        mkdroid.project.abi(prj, cfg)

        -- Version de la plateforme Android
        mkdroid.project.platform(prj, cfg)

        -- STL
        mkdroid.project.stl(prj, cfg)

        -- Toolchain
        mkdroid.project.toolchain(prj, cfg)

        -- Modules externes importés
        mkdroid.project.importModules(prj, cfg)

        -- Liens statiques et dynamiques
        -- mkdroid.project.staticLinks(prj, cfg)
        -- mkdroid.project.sharedLinks(prj, cfg)
        -- mkdroid.project.dependencies(prj, cfg)
        mkdroid.project.unique_dependencies(prj, cfg)

        -- Toujours à la fin
        mkdroid.project.kind(prj, cfg)

        p.w('endif')
        can_enter = true
    end

    -- Générer des fichiers pour ConsoleApp
    if can_enter == true and prj.kind == premake.CONSOLEAPP then
        mkdroid.android.generate(wks, prj)
    end
end

function mkdroid.project.header(prj, wks)
    p.w('LOCAL_PATH := $(call my-dir)')
    p.w('include $(CLEAR_VARS)')
    p.w('LOCAL_MODULE := %s', prj.name)
    if prj.targetname then
        p.w('LOCAL_MODULE_FILENAME := %s', prj.targetname)
    end
    p.w('')

    -- Récupérer le chemin du workspace et du projet
    local workspacePath = wks.location
    local projectPath = prj.location

    -- Définir la variable WORKSPACE_PATH
    local workspaceRelativePath
    if workspacePath ~= projectPath then
        -- Calculer le chemin relatif du workspace par rapport au projet
        workspaceRelativePath = path.getrelative(projectPath, workspacePath)
    else
        -- Si les chemins sont identiques, utiliser "./"
        workspaceRelativePath = "./"
    end

    mkdroid.printVariables(prj.name);

    p.w('')
end

function mkdroid.project.kind(prj, cfg)
    if cfg.kind == premake.STATICLIB then
        p.w('  include $(BUILD_STATIC_LIBRARY)')
    else -- cfg.kind == premake.SHAREDLIB
        p.w('  include $(BUILD_SHARED_LIBRARY)')
    end
end

function mkdroid.project.includes(prj, cfg, wks)
    local all_includes = {}
    local seen_includes = {}  -- Table pour suivre les répertoires d'inclusion ajoutés

    -- Fonction pour vérifier si le chemin contient une variable sous la forme $(nom_variable)
    local function contains_variable(dir)
        return dir:match("%$%(.+%)") ~= nil
    end

    -- Fonction pour vérifier si le chemin est relatif
    local function is_relative_path(dir)
        return not dir:match("^%w%:") and not dir:match("^/")  -- Vérifie si le chemin n'est pas absolu
    end

    -- Fonction pour vérifier si le chemin est dans le même workspace
    local function is_in_same_workspace(dir)
        -- Remplacez cette logique par celle qui vérifie si le chemin est dans le même workspace
        return dir:match("^" .. wks.location) ~= nil  -- Exemple : vérifie si le chemin commence par le chemin du workspace
    end

    -- Fonction pour obtenir le chemin relatif par rapport au répertoire du projet
    local function relative_path(dir)
        if contains_variable(dir) then
            return dir  -- Si le chemin contient une variable, le retourner tel quel
        elseif is_relative_path(dir) then
            return "$(LOCAL_PATH)/" .. path.getrelative(prj.location, dir)  -- Ajouter LOCAL_PATH si le chemin est relatif
        elseif is_in_same_workspace(dir) then
            return "$(LOCAL_PATH)/" .. path.getrelative(prj.location, dir)  -- Ajouter LOCAL_PATH si le chemin est dans le même workspace
        else
            return dir  -- Si le chemin est absolu, le retourner tel quel
        end
    end

    -- Fonction pour ajouter les répertoires d'inclusion sans doublons
    local function add_includes(dirs)
        if dirs then
            for _, dir in ipairs(dirs) do
                local rel_path = relative_path(dir)
                if not seen_includes[rel_path] then
                    table.insert(all_includes, rel_path)
                    seen_includes[rel_path] = true  -- Marquer le chemin comme ajouté
                end
            end
        end
    end

    -- Ajouter les répertoires d'inclusion du projet, de la configuration et de mka
    add_includes(prj.includedirs)
    add_includes(cfg.includedirs)
    add_includes(cfg.includes)

    -- Vérifier si des inclusions ont été ajoutées et les afficher
    if #all_includes > 0 then
        p.w('\tLOCAL_C_INCLUDES := %s', table.concat(all_includes, " \\\n                    "))
    end
end

function mkdroid.project.cppFeatures(prj, cfg)
    local features = {}

    if cfg.rtti == p.ON then
        table.insert(features, "rtti")
    end

    if cfg.exceptionhandling == p.ON then
        table.insert(features, "exceptions")
    end

    if #features > 0 then
        p.w('  LOCAL_CPP_FEATURES := %s', table.implode(features, '', '', ' '))
    end
end

function mkdroid.project.abi(prj, cfg)
    -- Vérifiez si local_abi est une table ou une chaîne
    local abi = ""
    if type(cfg.local_abi) == "table" then
        -- Si c'est une table, concaténez les valeurs en une chaîne
        abi = table.concat(cfg.local_abi, ' ')
    else
        -- Sinon, utilisez la valeur directement ou "all" par défaut
        abi = cfg.local_abi or "all"
    end

    -- Enregistrement de la valeur dans p.w
    p.w('  LOCAL_ABI := %s', abi)
end

function mkdroid.project.platform(prj, cfg)
    local platform = cfg.local_platform or "default"  -- Utilisation de la version de la plateforme Android
    p.w('  LOCAL_PLATFORM := %s', platform)
end

function mkdroid.project.stl(prj, cfg)
    local stl = cfg.local_stl or "default"  -- STL NDK
    p.w('  LOCAL_STL := %s', stl)
end

function mkdroid.project.toolchain(prj, cfg)
    local toolchain = cfg.local_toolchain or "default"  -- Version du toolchain
    p.w('  LOCAL_TOOLCHAIN := %s', toolchain)
end

function mkdroid.project.importModules(prj, cfg)
    if cfg.local_importModules and #cfg.local_importModules > 0 then
        -- Ajouter le chemin /Android.mk à chaque module
        local modules_with_path = {}
        for _, module in ipairs(cfg.local_importModules) do
            -- Extraire le nom du module (dernière partie après le dernier '/')
            local module_name = module:match("([^/]+)$")
            -- Construire le chemin complet
            local complete_path = module .. "/" .. module_name .. ".mk"
            table.insert(modules_with_path, complete_path)
        end
        
        -- Concaténer les modules avec le chemin ajouté
        p.w('  LOCAL_IMPORT_MODULES := %s', table.concat(modules_with_path, " \\\n                    "))
    end
end

function mkdroid.project.staticLinks(prj, cfg)
    if #cfg.local_staticLinks > 0 then
        p.w('  LOCAL_STATIC_LIBRARIES := %s', table.concat(cfg.local_staticLinks, " \\\n                    "))
    end
end

function mkdroid.project.sharedLinks(prj, cfg)
    if #cfg.local_sharedLinks > 0 then
        p.w('  LOCAL_SHARED_LIBRARIES := %s', table.concat(cfg.local_sharedLinks, " \\\n                    "))
    end
end

-- Préparation des fichiers source pour chaque configuration
function mkdroid.project.prepareSrcFiles(prj)
    local root = {}
    local configs = {}
    
    for cfg in project.eachconfig(prj) do
        configs[cfg] = {}
    end

    local tr = project.getsourcetree(prj)
    if not tr then
        print("L'arbre source est vide")
        return root, configs
    elseif #tr.children == 0 then
        print("L'arbre source n'a pas de noeuds.")
        return root, configs
    end

    premake.tree.traverse(tr, {
        onbranch = function(node, depth)
            --print("Branch:", node.name)
        end,
        onleaf = function(node, depth)
            --print("Leaf trouvé:", node.abspath)
            -- Identifier les configurations contenant ce fichier
            local incfg = {}
            local inall = true
            for cfg in project.eachconfig(prj) do
                local filecfg = premake.fileconfig.getconfig(node, cfg)
                if filecfg and not filecfg.flags.ExcludeFromBuild then
                    incfg[cfg] = filecfg
                else
                    inall = false
                end
            end

            -- Autoriser certains fichiers spécifiques
            if not path.iscppfile(node.abspath) and
                path.getextension(node.abspath) ~= "arm" and
                path.getextension(node.abspath) ~= "neon" then
                return
            end

            local filename = project.getrelative(prj, node.abspath)

            if inall then
                table.insert(root, filename)
            else
                for cfg in project.eachconfig(prj) do
                    if incfg[cfg] then
                        table.insert(configs[cfg], filename)
                    end
                end
            end
        end
    })

    return root, configs
end

function mkdroid.project.srcFiles(srcFiles)
    if srcFiles and #srcFiles > 0 then
        -- Afficher les fichiers sources avec un formatage approprié
        p.w('LOCAL_SRC_FILES := %s', table.concat(srcFiles, " \\\n                    "))
    end
end

function mkdroid.project.cfgSrcFiles(files)
    if #files > 0 then
        -- p.w('  LOCAL_SRC_FILES += %s', table.implode(table.translate(files, p.esc), '', '', ' '))
        p.w('LOCAL_SRC_FILES := %s', table.concat(table.translate(files, p.esc), " \\\n                    "))
    end
end

function mkdroid.project.dependencies(prj, cfg)
    local staticdeps = {}
    local shareddeps = {}

    local dependencies = premake.config.getlinks(cfg, "dependencies", "object")
    
    for _, dep in ipairs(dependencies) do
        if dep.kind == premake.STATICLIB then
            table.insert(staticdeps, dep.filename)
        else
            table.insert(shareddeps, dep.filename)
        end
    end
    
    for _, v in ipairs(cfg.local_staticLinks) do
        table.insert(staticdeps, v)
    end
    
    for _, v in ipairs(cfg.local_sharedLinks) do
        table.insert(shareddeps, v)
    end
    
    if #staticdeps > 0 then
        p.w('  LOCAL_STATIC_LIBRARIES := %s', table.concat(staticdeps, " \\\n                    "))
    end
    
    if #shareddeps > 0 then
        --p.w('  LOCAL_SHARED_LIBRARIES := %s', table.implode(shareddeps, '', '', ' '))
        p.w('  LOCAL_SHARED_LIBRARIES := %s', table.concat(shareddeps, " \\\n                    "))
    end
end

function mkdroid.project.unique_dependencies(prj, cfg)
    local staticdeps = {}
    local shareddeps = {}

    -- Récupérer les dépendances
    local dependencies = premake.config.getlinks(cfg, "dependencies", "object")

    -- Ajouter les dépendances à la liste appropriée
    for _, dep in ipairs(dependencies) do
        if dep.kind == premake.STATICLIB then
            if not staticdeps[dep.filename] then
                staticdeps[dep.filename] = true  -- Utiliser un tableau comme un ensemble pour éviter les doublons
            end
        else
            if not shareddeps[dep.filename] then
                shareddeps[dep.filename] = true
            end
        end
    end

    -- Ajouter les liens locaux
    for _, v in ipairs(cfg.local_staticLinks) do
        if not staticdeps[v] then
            staticdeps[v] = true
        end
    end

    for _, v in ipairs(cfg.local_sharedLinks) do
        if not shareddeps[v] then
            shareddeps[v] = true
        end
    end

    -- Afficher les bibliothèques statiques
    if next(staticdeps) then
        local staticList = {}
        for k in pairs(staticdeps) do
            table.insert(staticList, k)
        end
        p.w('  LOCAL_STATIC_LIBRARIES := %s', table.concat(staticList, " \\\n                    "))
    end

    -- Afficher les bibliothèques partagées
    if next(shareddeps) then
        local sharedList = {}
        for k in pairs(shareddeps) do
            table.insert(sharedList, k)
        end
        p.w('  LOCAL_SHARED_LIBRARIES := %s', table.concat(sharedList, " \\\n                    "))
    end
end

function mkdroid.project.ldFlags(prj, cfg)
    local flags = {}

    for _, dir in ipairs(premake.config.getlinks(cfg, "system", "directory")) do
        table.insert(flags, '-L' .. premake.quoted(dir))
    end
    
    for _, name in ipairs(premake.config.getlinks(cfg, "system", "basename")) do
        table.insert(flags, "-l" .. name)
    end
    
    if #flags > 0 then
        p.w('  LOCAL_LDLIBS := %s', table.implode(table.translate(flags, p.esc), '', '', ' '))
    end
    
    flags = premake.config.mapFlags(cfg, solution.ldflags)
    
    for _, opt in ipairs(cfg.linkoptions) do
        table.insert(flags, opt)
    end
    
    if #flags > 0 then
        p.w('  LOCAL_LDFLAGS := %s', table.implode(table.translate(flags, p.esc), '', '', ' '))
    end
end

function mkdroid.project.cFlags(prj, cfg)
    local flags = premake.config.mapFlags(cfg, solution.cflags)

    for _, opt in ipairs(cfg.buildoptions) do
        table.insert(flags, opt)
    end

    if #flags > 0 then
        p.w('  LOCAL_CFLAGS := %s', table.implode(table.translate(flags, p.esc), '', '', ' '))
    end
end