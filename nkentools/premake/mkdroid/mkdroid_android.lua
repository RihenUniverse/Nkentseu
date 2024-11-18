local make = premake.make
local project = premake.project
local api = premake.api
local mkdroid = premake.modules.mkdroid
local p = premake

function mkdroid.android.generate(wks, prj)
    -- Vérifier si le type de projet est 'ConsoleApp'
    if prj.kind == "ConsoleApp" then
        -- Chemin source (dossier 'android' dans le module 'mkdroid')
        local sourceDir = path.join("nkentools/premake/mkdroid", "android")
        
        -- Chemin destination (dossier du projet)
        local destDir = path.join(prj.location, "platforms", "android")
        
        -- Appeler la fonction de copie
        mkdroid.android.copyDirectory(wks, prj, sourceDir, destDir)
    else
        print("Le type de projet n'est pas 'ConsoleApp'. Aucune copie ne sera effectuée.")
    end
end

-- Fonction récursive pour copier un dossier et son contenu
function mkdroid.android.copyDirectory(wks, prj, srcDir, destDir)
    -- Créer le dossier de destination s'il n'existe pas
    os.mkdir(destDir)
    -- Utiliser os.matchfiles pour obtenir tous les fichiers et sous-dossiers
    local files = os.matchfiles(srcDir .. "/**")
    
    for _, file in ipairs(files) do
        -- Chemin relatif à partir du dossier source
        local relPath = path.getrelative(srcDir, file)
        local destPath = path.join(destDir, relPath)

        -- Créer les dossiers nécessaires dans le chemin de destination
        os.mkdir(path.getdirectory(destPath))

        -- Copier le fichier
        local success = mkdroid.android.copyFile(wks, prj, file, destPath, relPath)
        if success then
            --print("Fichier copié de", file, "à", destPath)
        else
            --print("Erreur de copie de", file, "à", destPath)
        end
    end
end

-- Fonction pour copier un fichier
function mkdroid.android.copyFile(wks, prj, src, dest, relPath)
    local srcFile = io.open(src, "rb")
    if not srcFile then
        print("Erreur : Impossible d'ouvrir le fichier source", src)
        return false
    end

    -- Lire le contenu source
    local content = srcFile:read("*a")
    srcFile:close()

    -- Modifier le contenu si le fichier est un fichier spécifique
    local fileName = path.getname(src)
    if fileName == "settings.gradle" then
        content = mkdroid.android.modifySettingsGradle(wks, prj, content)
    elseif fileName == "local.properties" then
        content = mkdroid.android.modifyLocalProperties(wks, prj, content)
    elseif fileName == "gradle.properties" then
        content = mkdroid.android.modifyGradleProperties(wks, prj, content)
    elseif fileName == "build.gradle" then
        -- Vérifier si c'est le fichier build.gradle à la racine ou celui dans le dossier app
        if relPath == "build.gradle" then
            content = mkdroid.android.modifyRootBuildGradle(wks, prj, content)
        elseif relPath == "app/build.gradle" then
            content = mkdroid.android.modifyAppBuildGradle(wks, prj, content)
        end
    elseif fileName == "gradle-wrapper.properties" then
        content = mkdroid.android.modifyWrapperProperties(wks, prj, content)
    elseif fileName == "AndroidManifest.xml" then
        content = mkdroid.android.modifyManifest(wks, prj, content)
    end

    -- Écrire le contenu modifié ou non modifié dans le fichier de destination
    local destFile = io.open(dest, "wb")
    if not destFile then
        print("Erreur : Impossible d'ouvrir le fichier de destination", dest)
        return false
    end
    destFile:write(content)
    destFile:close()

    return true
end

-- Fonction pour convertir un chemin en fonction du système d'exploitation
function mkdroid.android.convertPathForOS(path)
    -- Déterminer si le système d'exploitation actuel est Windows
    local isWindows = os.target() == "windows"

    -- Vérifier le style de chemin d'entrée
    local isWindowsPath = path:find("\\") ~= nil
    local isUnixPath = path:find("/") ~= nil

    if isWindows then
        if isUnixPath then
            -- Convertir les barres obliques (/) en doubles barres inversées (\\) pour Windows
            return path:gsub("/", "\\\\")
        elseif isWindowsPath then
            -- Convertir les simples barres inversées (\) en doubles barres inversées (\\) pour Windows
            return path:gsub("\\([^\\])", "\\\\%1"):gsub("\\$", "\\\\")
        end
    else
        if isWindowsPath then
            -- Convertir les barres inversées (\) en barres obliques (/) pour Unix
            return path:gsub("\\", "/")
        end
    end

    -- Si aucun changement n'est nécessaire, retourner le chemin tel quel
    return path
end

-- Fonction de modification pour le fichier settings.gradle
function mkdroid.android.modifySettingsGradle(wks, prj, content)
    local projectName = prj.name
    content = content:gsub('"{1}"', '"' .. projectName .. '"')
    return content
end

-- Fonction de modification pour le fichier local.properties
function mkdroid.android.modifyLocalProperties(wks, prj, content)
    content = content:gsub('{1}', mkdroid.android.convertPathForOS(solution.properties.sdk.path))
    content = content:gsub('{2}', mkdroid.android.convertPathForOS(solution.properties.jdk))
    return content
end

-- Fonction de modification pour le fichier gradle.properties
function mkdroid.android.modifyGradleProperties(wks, prj, content)
    local relative = path.getrelative(prj.location .. "/platforms/android/app", wks.location)
    local outputdir_bin = "../../../../../../bin/android" .. '/' .. prj.name .. '/'
    --local outputdir_bin = relative .. "/build/bin/android" .. '/' .. prj.name
    local outputdir_obj = relative .. "/build/obj/android" .. '/' .. prj.name

    content = content:gsub('{1}', solution.properties.ndk.version)
    content = content:gsub('{2}', prj.name)
    content = content:gsub('{3}', outputdir_obj)
    content = content:gsub('{4}', outputdir_bin)
    return content
end

-- Fonction de modification pour le fichier build.gradle (racine)
function mkdroid.android.modifyRootBuildGradle(wks, prj, content)
    content = content:gsub("{1}", solution.properties.gradle.version)
    content = content:gsub("{2}", solution.properties.gradle.version)
    return content
end

-- Fonction de modification pour le fichier gradle-wrapper.properties
function mkdroid.android.modifyWrapperProperties(wks, prj, content)
    content = content:gsub('{1}', solution.properties.gradle.pack)
    return content
end

-- Fonction de modification pour le fichier app/build.gradle
function mkdroid.android.modifyAppBuildGradle(wks, prj, content)
    content = content:gsub("{1}", solution.properties.application.id)
    content = content:gsub("{2}", solution.properties.sdk.version.compile)
    content = content:gsub("{3}", solution.properties.ndk.version)
    content = content:gsub("{1}", solution.properties.application.id)
    content = content:gsub("{4}", solution.properties.sdk.version.min)
    content = content:gsub("{5}", solution.properties.sdk.version.target)
    content = content:gsub("{6}", "1_8")
    content = content:gsub("{7}", "1_8")
    -- content = content:gsub("{8}", prj.name)
    --content = content:gsub("{9}", "implementation 'com.android.support:appcompat-v7:28.0.0'") -- implementation 'androidx.appcompat:appcompat:1.3.1' // Remplacez par la dernière version
    content = content:gsub("{9}", "implementation 'androidx.appcompat:appcompat:1.3.1'") -- implementation 'androidx.appcompat:appcompat:1.3.1' // Remplacez par la dernière version

    local relative = path.getrelative(prj.location .. "/platforms/android/app", wks.location)

    content = content:gsub("{10}", relative .. "/" .. wks.name .. ".mk")
    
    return content
end

-- Fonction de modification pour le fichier AndroidManifest.xml
function mkdroid.android.modifyManifest(wks, prj, content)
    -- Remplacement des placeholders par des valeurs spécifiques
    content = content:gsub('{1}', '1.0')                     -- Version de l'application
    content = content:gsub('{2}', 'utf-8')                   -- Encodage
    content = content:gsub('{3}', '1')                       -- Version code
    content = content:gsub('{4}', '1.0')                     -- Version name
    content = content:gsub('{5}', 'false')                   -- Allow backup
    content = content:gsub('{6}', 'false')                   -- Full backup content
    content = content:gsub('{7}', prj.name)                  -- Nom du projet
    content = content:gsub('{8}', prj.name)                  -- Nom de l'application
    content = content:gsub('{9}', 'true')                    -- Accélération matérielle
    content = content:gsub('{10}', prj.name)   -- Nom de la bibliothèque
    content = content:gsub('{11}', 'nkentseu_android_entry')                 -- Nom du projet pour la bibliothèque

    -- Ajout de la version OpenGL ou Vulkan spécifiée
    local api_type = "opengl"  -- Remplacez par "vulkan" si vous voulez utiliser Vulkan
    local version = "3.0"      -- Remplacez par la version souhaitée pour OpenGL ES ou Vulkan
    
    -- Ajout de la fonctionnalité OpenGL ou Vulkan au contenu
    content = content:gsub('{12}', mkdroid.android.feature_list(api_type, version))

    content = content:gsub('{13}', solution.properties.application.id)
    
    return content
end

-- Fonction pour définir les caractéristiques en fonction de l'API et de la version (OpenGL ou Vulkan)
function mkdroid.android.feature_list(api_type, version)
    local feature = "\t"  -- Initialisation de la chaîne de caractéristiques
    
    if api_type == "opengl" then
        -- Convertir la version d'OpenGL en hexadécimal, comme "0x00030000" pour OpenGL ES 3.0
        local major, minor = version:match("(%d+)%.(%d+)")
        if major and minor then  -- Vérification que les valeurs ont été extraites correctement
            local gl_version_hex = string.format("0x%04x%04x", tonumber(major), tonumber(minor) * 10000)
            feature = feature .. '<uses-feature android:glEsVersion="' .. gl_version_hex .. '" android:required="true" />'
        else
            error("Invalid OpenGL version format: " .. version)  -- Gestion d'erreur pour un format de version invalide
        end
        
    elseif api_type == "vulkan" then
        -- Pour Vulkan, convertir la version, par exemple, "1.1.0" en "4198400"
        local major, minor, patch = version:match("(%d+)%.(%d+)%.?(%d*)")  -- Le patch est optionnel

        major = mkdroid.android.calcul_version(major, 4194304)
        minor = mkdroid.android.calcul_version(minor, 65536)
        patch = mkdroid.android.calcul_version(patch, 1)

        local vulkan_version_int = major + minor + patch

        feature = feature .. '<uses-feature android:name="android.hardware.vulkan.version" android:version="' .. vulkan_version_int .. '" android:required="true" />'
    else
        error("Unsupported API type: " .. api_type)  -- Gestion d'erreur pour un type d'API non supporté
    end

    return feature
end

-- Fonction utilitaire pour calculer une partie de la version en fonction du coefficient
function mkdroid.android.calcul_version(number_version, coef)
    -- Vérifier que number_version et coef sont bien définis et sont des nombres
    if number_version == nil or coef == nil then
        return 0 -- Retourne 0 en cas de valeur indéfinie pour l'un ou l'autre
    end

    -- Convertir number_version en nombre si c'est une chaîne de caractères
    local number = tonumber(number_version)
    if not number then
        return 0 -- Retourne 0 si number_version ne peut pas être converti en nombre
    end

    return number * coef
end
