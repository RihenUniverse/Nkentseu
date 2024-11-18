import subprocess
import argparse
import sys
from pathlib import Path


premake_path = "./nkentools/premake/premake5.exe"


def description():
    """
    Affiche la description générale du script si aucune commande n'est fournie.
    """
    print("""
    Ce script génère des fichiers de projet en fonction du système de construction sélectionné.
    Les options disponibles sont :

    msvc    : Générer le projet pour Visual Studio 2022.
    make    : Générer le projet pour Make (gmake2).
    clang   : Générer le projet pour Clang (gmake2).
    gradle  : Générer le projet pour Gradle (mkdroid).
    mingw   : Générer le projet pour Mingw (gmake2).

    Utilisez `./nken gen --help` pour plus d'informations sur une commande.
    """)


def help_command():
    """
    Affiche l'aide générale et spécifique pour les commandes.
    """
    help_texts = {
        'msvc': """
        msvc : Utilise Visual Studio 2022 (MSVC) pour générer les fichiers de projet.
        Exemple d'utilisation : ./nken gen msvc
        """,
        'make': """
        make : Utilise Make (gmake2) pour générer les fichiers de projet.
        Exemple d'utilisation : ./nken gen make
        """,
        'clang': """
        clang : Utilise Clang (gmake2) pour générer les fichiers de projet.
        Exemple d'utilisation : ./nken gen clang
        """,
        'gradle': """
        gradle : Utilise Gradle (mkdroid) pour générer les fichiers de projet.
        Exemple d'utilisation : ./nken gen gradle
        """
    }

    print("""
    Ce script génère des fichiers de projet pour différents systèmes de construction.
    Les options disponibles sont :
    """)

    for command, description in help_texts.items():
        print(description)

    sys.exit(0)


def gen_command(command_names=None):
    """
    Gère la génération des commandes en fonction des systèmes de construction spécifiés.
    :param command_names: Liste des commandes passées en argument (par exemple, --msvc, --make).
    """
    if not command_names:
        print("Erreur : Aucune commande spécifiée.")
        description()
        sys.exit(1)

    for command in command_names:
        if command == 'msvc':
            print("Génération du projet pour Visual Studio 2022...")
            run_command([premake_path, "vs2022"])
        elif command == 'make':
            print("Génération du projet pour Make (gmake2)...")
            run_command([premake_path, "gmake2"])
        elif command == 'clang':
            print("Génération du projet pour Clang (gmake2)...")
            run_command([premake_path, "gmake2"])
        elif command == 'gradle':
            print("Génération du projet pour Gradle (mkdroid)...")
            run_command([premake_path, "mkdroid"])
        elif command == 'mingw':
            print("Génération du projet pour Mingw (gmake2)...")
            run_command([premake_path, "gmake2"])
        else:
            print(f"Commande inconnue : {command}")
            help_command()


def run_command(command):
    """
    Exécute une commande système et gère les erreurs.
    
    :param command: Liste des arguments de la commande à exécuter.
    """
    try:
        result = subprocess.run(command, check=True, capture_output=True, text=True)
        print(result.stdout)
    except subprocess.CalledProcessError as e:
        print(f"Erreur lors de l'exécution de la commande : {e.stderr} >> {e}")
        sys.exit(1)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Outil de compilation multi-plateforme.")
    parser.add_argument("action", choices=["msvc", "make", "clang", "gradle", "mingw"], help="Action à exécuter")
    
    args = parser.parse_args()
    
    # Appel de la fonction de génération avec l'action spécifiée
    gen_command([args.action])
    sys.exit(0)