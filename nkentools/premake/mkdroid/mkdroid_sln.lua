-- mkdroid_sln.lua

local api = premake.api

-- Premake5 script pour configurer le workspace et les projets Android
solution = {}

-- solution project
solution.project = {}

-- solution properties
solution.properties 			              = {}

solution.properties.jdk 		            = ""
solution.properties.sdk 		            = {}
solution.properties.sdk.path 		        = ""
solution.properties.sdk.version         = {}
solution.properties.sdk.version.compile = ""
solution.properties.sdk.version.target  = ""
solution.properties.sdk.version.min     = ""
solution.properties.ndk                 = {}
solution.properties.ndk.path 		        = ""
solution.properties.ndk.version         = ""
solution.properties.cmake 		          = ""
solution.properties.application 	      = {}
solution.properties.application.id 	    = ""
solution.properties.application.ApkName = ""
solution.properties.application.root    = ""
solution.properties.gradle              = {}
solution.properties.gradle.pack         = ""
solution.properties.gradle.version      = ""

-- solution premake5 path
solution.premake = {}
solution.premake.path = ""
solution.premake.mkdroidPath = "mkdroid"

-- outpout dir
solution.outputdir = {}
solution.outputdir.code     =   "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
solution.outputdir.bin      =   "%{wks.location}/build/bin/"
solution.outputdir.obj      =   "%{wks.location}/build/obj/"
solution.outputdir.bindir   =   solution.outputdir.bin .. solution.outputdir.code
solution.outputdir.objdir   =   solution.outputdir.obj .. solution.outputdir.code

-- solution platform
solution.platforms = {}

-- solution variables

solution.variables          = {}
solution.variables.values   = {}

-- Fonction pour ajouter une variable à un projet donné
function solution.variables.add(prj_name, name, value)
  -- Si le projet n'existe pas encore dans la table, on le crée
  if not solution.variables.values[prj_name] then
    solution.variables.values[prj_name] = {}
  end
  -- Ajouter la variable avec son nom et sa valeur
  solution.variables.values[prj_name][name] = value
end

-- Fonction pour appeler une variable pour un projet donné
function solution.variables.call(prj_name, name)
  -- Vérifie si le projet existe et si la variable existe pour ce projet
  if not solution.variables.values[prj_name] then
      error("Projet '" .. prj_name .. "' non défini dans les variables")
  end
  
  if not solution.variables.values[prj_name][name] then
      error("Variable '" .. name .. "' non définie pour le projet '" .. prj_name .. "'")
  end
  
  -- Retourne la valeur de la variable sous la forme d'un format attendu
  return "$(" .. name .. ")"
end

-- solution file
function solution.appFile(sln)
	return sln.name .. ".app.mk"
end
  
function solution.androidFile(sln)
	return sln.name .. ".mk"
end
  
  
function solution.prjFile(prj)
	return prj.name .. ".mk"
end

-- Linker flags for optimizing builds
solution.ldflags = {
	flags = {
		LinkTimeOptimization = "-flto",
	}
}
  
-- Compiler flags for C and C++ compilation settings
solution.cflags = {
	flags = {
		FatalCompileWarnings = "-Werror",
		ShadowedVariables = "-Wshadow",
		UndefinedIdentifiers = "-Wundef",
		DefineExeception = "-fexceptions",
		UnuseFunction = "-Wunused-function",
		LinkTimeOptimization = "-flto",
	},
	warnings = {
		Extra = "-Wall -Wextra",
		Off = "-w",
	}
}

-- C++ language standard flags for different C++ versions
solution.cppflags = {
	flags = {
		["C++11"] = "-std=c++11",
		["C++14"] = "-std=c++14",
		["C++17"] = "-std=c++17",
		["C++20"] = "-std=c++20",
	}
}

--[[ how to use it
  buildoptions {
      mkdroid.cflags.flags.FatalCompileWarnings,
      mkdroid.cflags.flags.ShadowedVariables,
      mkdroid.cflags.flags.UndefinedIdentifiers,
      mkdroid.cflags.flags.DefineExeception,
      mkdroid.cflags.flags.UnuseFunction,
  }

  -- Ajout des C++ flags pour Android
  buildoptions {
      mkdroid.cppflags.flags["C++17"]  -- Exemple : utiliser le standard C++17
  }

  -- Ajout des options de lien
  linkoptions {
      mkdroid.ldflags.flags.LinkTimeOptimization
  }
]]

-- Configuration de l'ABI supporté
solution.abi = {
  flags = {
    armeabi_v7a = "armeabi-v7a",
    arm64_v8a = "arm64-v8a",
    x86 = "x86",
    x86_64 = "x86_64",
    all = "all",
  },
  allowed = function(value)
    local selectedAbis = {}
    local validAbis = { "armeabi-v7a", "arm64-v8a", "x86", "x86_64", "all" }

    -- Processus de validation des ABI
    for _, abi in ipairs(string.explode(value, ' ')) do
      if abi ~= "" then
        if not table.contains(validAbis, abi) then
          error("abi: '" .. abi .. "' is not a valid ABI. Valid options are: " .. table.concat(validAbis, ', '))
        end
        table.insert(selectedAbis, abi)
      end
    end

    -- Vérification que des ABI valides ont été spécifiés
    if #selectedAbis == 0 then
      error("abi: At least one valid ABI must be specified.")
    end

    -- Retourner le résultat
    return table.contains(selectedAbis, "all") and "all" or table.implode(selectedAbis, '', '', ' ')
  end
}

api.register {
  name = "local_abi",
  scope = "config",
  kind = "list:string", -- La version Android sera un string (par exemple, "android-30")

  allowed = function(value)
    local selectedAbis = {}
    local validAbis = {
      "armeabi-v7a",
      "arm64-v8a",
      "x86",
      "x86_64",
      "all"
    }

    for _, abi in ipairs(string.explode(value, ' ')) do
      if abi ~= "" then
        if not table.contains(validAbis, abi) then
          error("abi: '" .. abi .. "' is not a valid ABI. Valid options are: " .. table.concat(validAbis, ', '))
        end
        table.insert(selectedAbis, abi)
      end
    end

    if #selectedAbis == 0 then
      error("abi: At least one valid ABI must be specified.")
    end

    return table.contains(selectedAbis, "all") and "all" or table.implode(selectedAbis, '', '', ' ')
  end
}

-- Configuration de la version de la plateforme NDK (API Android)
-- Définition des versions Android
solution.platform = {
  flags = {
      android16 = "android-16",
      android17 = "android-17",
      android18 = "android-18",
      android19 = "android-19",
      android20 = "android-20",
      android21 = "android-21",
      android22 = "android-22",
      android23 = "android-23",
      android24 = "android-24",
      android25 = "android-25",
      android26 = "android-26",
      android27 = "android-27",
      android28 = "android-28",
      android29 = "android-29",
      android30 = "android-30",
      android31 = "android-31",
      android32 = "android-32",
      android33 = "android-33",
      android34 = "android-34",
  },
  allowed = function(value)
      local minApi = 16
      local maxApi = 34

      -- Si la valeur est "default", on retourne "android-30" par défaut
      if value == "default" then
          return "android-30"
      -- Vérification si la valeur est un nombre valide
      elseif tonumber(value) then
          local apiValue = tonumber(value)
          if apiValue >= minApi and apiValue <= maxApi then
              return "android-" .. value
          else
              error("platform: Version " .. value .. " is not supported. Use a value between " .. minApi .. " and " .. maxApi .. ".")
          end
      else
          error("platform: Invalid version value. Use a number between " .. minApi .. " and " .. maxApi .. ".")
      end
  end
}

-- Enregistrement de l'API pour android_version
api.register {
  name = "local_platform",
  scope = "config",
  kind = "string" -- La version Android sera un string (par exemple, "android-30")
}

-- Configuration de la bibliothèque STL NDK
solution.stl = {
  flags = {
    default = "default",
    cpp_static = "c++_static",
    cpp_shared = "c++_shared"
  }
}

api.register {
  name = "local_stl",
  scope = "config",
  kind = "string"
}

-- Configuration de la version du toolchain NDK
solution.toolchain = {
  flags = {
    default = "default",
    clang = "clang"
  }
}

api.register {
  name = "local_toolchain",
  scope = "config",
  kind = "string",
}

api.register {
  name = "local_includes",
  scope = "project",
  kind = "list:file",
  tokens = true,
}

-- Configuration pour l'importation de modules dans Android.mk
api.register {
  name = "local_importModules",
  scope = "project",
  kind = "list:string",
  tokens = true,
}

-- Configuration pour les liens statiques dans Android.mk
api.register {
  name = "local_staticLinks",
  scope = "config",
  kind = "list:string",
}

-- Configuration pour les liens dynamiques dans Android.mk
api.register {
  name = "local_sharedLinks",
  scope = "config",
  kind = "list:string",
}



--[[ ## android feature
  feature:reqGlEsVersion=0x30002
  feature:android.hardware.audio.low_latency
  feature:android.hardware.audio.output
  feature:android.hardware.audio.pro
  feature:android.hardware.bluetooth
  feature:android.hardware.bluetooth_le
  feature:android.hardware.camera
  feature:android.hardware.camera.any
  feature:android.hardware.camera.ar
  feature:android.hardware.camera.autofocus
  feature:android.hardware.camera.capability.manual_post_processing
  feature:android.hardware.camera.capability.manual_sensor
  feature:android.hardware.camera.capability.raw
  feature:android.hardware.camera.flash
  feature:android.hardware.camera.front
  feature:android.hardware.camera.level.full
  feature:android.hardware.faketouch
  feature:android.hardware.fingerprint
  feature:android.hardware.location
  feature:android.hardware.location.gps
  feature:android.hardware.location.network
  feature:android.hardware.microphone
  feature:android.hardware.nfc
  feature:android.hardware.nfc.any
  feature:android.hardware.nfc.ese
  feature:android.hardware.nfc.hce
  feature:android.hardware.nfc.hcef
  feature:android.hardware.nfc.uicc
  feature:android.hardware.opengles.aep
  feature:android.hardware.ram.normal
  feature:android.hardware.screen.landscape
  feature:android.hardware.screen.portrait
  feature:android.hardware.sensor.accelerometer
  feature:android.hardware.sensor.barometer
  feature:android.hardware.sensor.compass
  feature:android.hardware.sensor.gyroscope
  feature:android.hardware.sensor.hifi_sensors
  feature:android.hardware.sensor.light
  feature:android.hardware.sensor.proximity
  feature:android.hardware.sensor.stepcounter
  feature:android.hardware.sensor.stepdetector
  feature:android.hardware.telephony
  feature:android.hardware.telephony.gsm
  feature:android.hardware.telephony.ims
  feature:android.hardware.touchscreen
  feature:android.hardware.touchscreen.multitouch
  feature:android.hardware.touchscreen.multitouch.distinct
  feature:android.hardware.touchscreen.multitouch.jazzhand
  feature:android.hardware.usb.accessory
  feature:android.hardware.usb.host
  feature:android.hardware.vulkan.compute
  feature:android.hardware.vulkan.level=1
  feature:android.hardware.vulkan.version=4198400
  feature:android.hardware.wifi
  feature:android.hardware.wifi.aware
  feature:android.hardware.wifi.direct
  feature:android.hardware.wifi.rtt
  feature:android.software.activities_on_secondary_displays
  feature:android.software.app_widgets
  feature:android.software.autofill
  feature:android.software.backup
  feature:android.software.cant_save_state
  feature:android.software.companion_device_setup
  feature:android.software.connectionservice
  feature:android.software.cts
  feature:android.software.device_admin
  feature:android.software.file_based_encryption
  feature:android.software.freeform_window_management
  feature:android.software.home_screen
  feature:android.software.input_methods
  feature:android.software.ipsec_tunnels
  feature:android.software.live_wallpaper
  feature:android.software.managed_users
  feature:android.software.midi
  feature:android.software.picture_in_picture
  feature:android.software.print
  feature:android.software.secure_lock_screen
  feature:android.software.securely_removes_users
  feature:android.software.sip
  feature:android.software.sip.voip
  feature:android.software.verified_boot
  feature:android.software.voice_recognizers
  feature:android.software.webview
  feature:android.sofware.nfc.beam
  feature:com.google.android.feature.DPS
  feature:com.google.android.feature.TURBO_PRELOAD
  feature:com.nxp.mifare
  feature:com.samsung.android.api.version.2402
  feature:com.samsung.android.api.version.2403
  feature:com.samsung.android.api.version.2501
  feature:com.samsung.android.api.version.2502
  feature:com.samsung.android.api.version.2601
  feature:com.samsung.android.api.version.2701
  feature:com.samsung.android.api.version.2801
  feature:com.samsung.android.api.version.2802
  feature:com.samsung.android.api.version.2803
  feature:com.samsung.android.api.version.2901
  feature:com.samsung.android.api.version.2902
  feature:com.samsung.android.api.version.2903
  feature:com.samsung.android.authfw
  feature:com.samsung.android.bio.face
  feature:com.samsung.android.camerasdkservice
  feature:com.samsung.android.cameraxservice
  feature:com.samsung.android.knox.knoxsdk
  feature:com.samsung.android.sdk.camera.processor
  feature:com.samsung.android.sdk.camera.processor.effect
  feature:com.samsung.feature.SAMSUNG_EXPERIENCE
  feature:com.samsung.feature.aodservice_v07
  feature:com.samsung.feature.aremoji.v2
  feature:com.samsung.feature.arruler
  feature:com.samsung.feature.audio_listenback
  feature:com.samsung.feature.device_category_phone
  feature:com.samsung.feature.galaxyfinder_v7
  feature:com.samsung.feature.ipsgeofence=1
  feature:com.samsung.feature.samsung_experience_mobile
  feature:com.samsung.feature.samsungpositioning
  feature:com.samsung.feature.samsungpositioning.snlp
  feature:com.sec.android.mdm
  feature:com.sec.android.secimaging
  feature:com.sec.android.smartface.smart_stay
  feature:com.sec.feature.cocktailpanel
  feature:com.sec.feature.cover
  feature:com.sec.feature.cover.clearsideviewcover
  feature:com.sec.feature.cover.flip
  feature:com.sec.feature.cover.ledbackcover
  feature:com.sec.feature.cover.nfcledcover
  feature:com.sec.feature.cover.sview
  feature:com.sec.feature.edge_v03
  feature:com.sec.feature.fingerprint_manager_service
  feature:com.sec.feature.motionrecognition_service
  feature:com.sec.feature.nfc_authentication
  feature:com.sec.feature.nfc_authentication_cover
  feature:com.sec.feature.nsflp=521
  feature:com.sec.feature.overlaymagnifier
  feature:com.sec.feature.people_edge_notification
  feature:com.sec.feature.saccessorymanager
  feature:com.sec.feature.sensorhub=32
  feature:com.sec.feature.slocation=3
  feature:com.sec.feature.spen_usp=40
  feature:com.sec.feature.support_mst
  feature:com.sec.feature.usb_authentication
  feature:com.sec.feature.wirelesscharger_authentication
]]
