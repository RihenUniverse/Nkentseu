# build.py

import subprocess
import argparse
import os
import sys
from pathlib import Path
import platform


# Détection de la plateforme
system = platform.system()


# Chemins pour Windows
if system == "Windows":
    gradle_build_path = os.path.join(os.getcwd(), "nkentools", "android", "gradlew.bat")
    msvc_build_path = os.path.join("C:\\Program Files", "Microsoft Visual Studio", "2022", "Community", "MSBuild", "Current", "Bin", "MSBuild.exe")
    clang_build_path = os.path.join("C:\\Program Files", "LLVM", "bin", "clang++.exe")
    mingw_build_path = os.path.join("C:\\Program Files", "MinGW", "bin", "g++.exe")  # Chemin pour g++
    make_build_path = os.path.join("C:\\Program Files", "MinGW", "bin", "make.exe")  # Chemin pour Make
    jdk_path = os.path.join("C:", "Program Files", "Java", "jdk-17")
    sdk_path = os.path.join("C:", "Users", os.getenv("USERNAME"), "AppData", "Local", "Android", "Sdk")
    ndk_path = os.path.join(sdk_path, "ndk", "25.1.8937393")  # Remplacez par la version de votre NDK
    ndk_build_path = os.path.join(ndk_path, "ndk-build.cmd")  # Chemin pour ndk-build
# Chemins pour Linux
else:
    gradle_build_path = os.path.join(".", "nkentools", "android", "gradlew")  # Utilisation de `./` pour Linux
    msvc_build_path = None  # MSVC n'est pas disponible sous Linux
    clang_build_path = os.path.join("/usr", "bin", "clang++")  # Chemin typique de Clang sous Linux
    mingw_build_path = os.path.join("/usr", "bin", "g++")  # Chemin pour g++
    make_build_path = os.path.join("/usr", "bin", "make")  # Chemin typique pour Make sous Linux
    clang_cpp_compiler = os.path.join("/usr", "bin", "clang++")  # Chemin pour clang++
    jdk_path = os.path.expanduser("~/java/jdk-17")
    sdk_path = os.path.expanduser("~/Android/Sdk")
    ndk_path = os.path.join(sdk_path, "ndk", "25.1.8937393")  # Remplacez par la version de votre NDK
    ndk_build_path = os.path.join(ndk_path, "ndk-build")  # Chemin pour ndk-build


def description():
    print("""
    Ce script permet de compiler des projets avec plusieurs outils de compilation (Gradle, MSBuild, Clang, MinGW, Make).
    Vous pouvez spécifier le type de build et le chemin du projet.
    """)


def help_command():
    print("""
    Utilisation : ./nken build [options]
    
    Options:
        action                      : Système de build à utiliser [gradle|msvc|clang|mingw|make|ndk] (obligatoire).
        --path                      : Chemin vers la solution/projet à compiler (obligatoire).
        --config                    : Type de build [debug|release], par défaut "debug".
        --android                   : Version de android pris en charge (19+) [uniquement pour le ndk-build]
        
    Exemples :
        ./nken build gradle --path ./myproject --config release
        ./nken build msvc --path ./myproject.sln --config debug
        ./nken build make --path ./myproject --config release
        ./nken build ndk --path ./myproject --config release --android 21
    """)


def build_ndk(solution_path_with_file, androidVersion, config):
    """
    Compile un projet Android en utilisant ndk-build.

    :param solution_path_with_file: Chemin complet vers le fichier solution.mk (inclut son nom).
    :param ndk_build_path: Chemin vers l'exécutable ndk-build.
    :param config: Type de build ('debug' ou 'release').
    """
    if config not in ['debug', 'release']:
        raise ValueError("Le type de build doit être 'debug' ou 'release'.")

    # Séparer le chemin du fichier .mk
    solution_path = Path(solution_path_with_file).parent
    solution_file = Path(solution_path_with_file).name

    # Vérification du chemin de ndk-build
    ndk_build = Path(ndk_build_path).resolve()
    if not ndk_build.exists() or not ndk_build.is_file():
        raise FileNotFoundError(f"Le chemin ndk-build spécifié est invalide : {ndk_build}")

    # Vérification de l'existence du fichier .mk
    solution_mk = solution_path / solution_file
    if not solution_mk.exists() or not solution_mk.is_file():
        raise FileNotFoundError(f"Le fichier {solution_file} n'existe pas dans {solution_path}")

    # Construction de la commande
    command = [
        str(ndk_build),
        f"NDK_PROJECT_PATH={solution_path.resolve()}",
        f"APP_BUILD_SCRIPT={solution_mk.resolve()}",
        f"APP_PLATFORM=android-{androidVersion}",
        f"MKA_CONFIG={config}"
    ]

    # Exécution de la commande
    try:
        result = subprocess.run(
            command,
            check=True,
            capture_output=True,
            text=True,
            cwd=solution_path
        )
        print(f"Compilation {config} réussie :\n{result.stdout}")
    except subprocess.CalledProcessError as e:
        print(f"Erreur lors de la compilation {config} :\n{e.stderr}\nCommande : {' '.join(command)}")
        exit(1)


def build_gradle(solution_path, config):
    """
    Compile un projet Android en utilisant Gradle Wrapper.
    
    :param solution_path: Chemin vers le répertoire du projet Android.
    :param config: Type de build ('debug' ou 'release').
    """
    if config not in ['debug', 'release']:
        raise ValueError("Le type de build doit être 'debug' ou 'release'.")
    
    build_path = Path(gradle_build_path)
    if not build_path.exists() or not build_path.is_file():
        raise FileNotFoundError(f"Le fichier Gradle Wrapper n'a pas été trouvé : {build_path}")

    command = [
        str(build_path), '-p', str(Path(solution_path).resolve()),
        f"assemble{config.capitalize()}", f"-PMKA_CONFIG={config}"
    ]

    try:
        result = subprocess.run(command, check=True, capture_output=True, text=True, cwd=solution_path)
        print(f"Compilation {config} réussie :\n{result.stdout}")
    except subprocess.CalledProcessError as e:
        print(f"Erreur lors de la compilation {config} : {e.stderr} : {e}")
        exit(1)


def build_msvc(solution_path, config):
    """
    Compile un projet Visual Studio en utilisant MSBuild.
    
    :param solution_path: Chemin vers le fichier .sln du projet Visual Studio.
    :param config: Type de build ('debug' ou 'release').
    """
    build_path = msvc_build_path
    if not Path(build_path).exists():
        raise FileNotFoundError(f"MSBuild introuvable : {build_path}")

    command = [build_path, solution_path, f'/property:Configuration={config}']

    try:
        result = subprocess.run(command, check=True, capture_output=True, text=True)
        print(f"Compilation {config} réussie :\n{result.stdout}")
    except subprocess.CalledProcessError as e:
        print(f"Erreur lors de la compilation {config} : {e.stderr}")
        exit(1)


def build_clang(solution_path, config):
    """
    Compile un projet avec Clang.
    
    :param solution_path: Chemin vers le fichier source ou le répertoire du projet.
    :param config: Type de build ('debug' ou 'release').
    """
    build_path = Path(clang_build_path)
    if not build_path.exists():
        raise FileNotFoundError(f"Clang introuvable : {build_path}")

    command = [str(build_path), solution_path, "-o", "output"]

    if config == "debug":
        command.append("-g")
    elif config == "release":
        command.append("-O3")

    try:
        result = subprocess.run(command, check=True, capture_output=True, text=True)
        print(f"Compilation {config} réussie :\n{result.stdout}")
    except subprocess.CalledProcessError as e:
        print(f"Erreur lors de la compilation {config} : {e.stderr}")
        exit(1)


def build_mingw(solution_path, config):
    """
    Compile un projet avec MinGW.
    
    :param solution_path: Chemin vers le fichier source ou le répertoire du projet.
    :param config: Type de build ('debug' ou 'release').
    """
    build_path = Path(mingw_build_path)
    if not build_path.exists():
        raise FileNotFoundError(f"MinGW introuvable : {build_path}")

    command = [str(build_path), solution_path, "-o", "output"]

    if config == "debug":
        command.append("-g")
    elif config == "release":
        command.append("-O3")

    try:
        result = subprocess.run(command, check=True, capture_output=True, text=True)
        print(f"Compilation {config} réussie :\n{result.stdout}")
    except subprocess.CalledProcessError as e:
        print(f"Erreur lors de la compilation {config} : {e.stderr}")
        exit(1)


def build_make(solution_path, config):
    """
    Compile un projet en utilisant Make.
    
    :param solution_path: Chemin vers le répertoire contenant le Makefile.
    :param config: Type de build ('debug' ou 'release').
    """
    build_path = Path(make_build_path)
    if not build_path.exists():
        raise FileNotFoundError(f"Make introuvable : {build_path}")

    command = [str(build_path)]
    
    # Ajouter la configuration de build comme variable d'environnement si nécessaire
    if config == "debug":
        command.append("DEBUG=1")
    elif config == "release":
        command.append("RELEASE=1")

    try:
        result = subprocess.run(command, check=True, capture_output=True, text=True, cwd=solution_path)
        print(f"Compilation {config} réussie :\n{result.stdout}")
    except subprocess.CalledProcessError as e:
        print(f"Erreur lors de la compilation {config} : {e.stderr}")
        exit(1)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Outil de compilation multi-plateforme.")
    
    parser.add_argument("action", choices=["gradle", "msvc", "clang", "mingw", "make", "ndk"], help="Action à exécuter")
    parser.add_argument('--path', default='./', help="Chemin vers la solution/projet à compiler.")
    parser.add_argument('--config', default='debug', choices=['debug', 'release'], help="Type de build (par défaut : debug).")
    parser.add_argument('--android', default='21', help="Type de android version (par défaut : 21).")

    args = parser.parse_args()

    if args.action == 'gradle':
        build_gradle(args.path, args.config)
    elif args.action == 'ndk':
        build_ndk(args.path, args.android, args.config)
    elif args.action == 'msvc':
        build_msvc(args.path, args.config)
    elif args.action == 'clang':
        build_clang(args.path, args.config)
    elif args.action == 'mingw':
        build_mingw(args.path, args.config)
    elif args.action == 'make':
        build_make(args.path, args.config)
