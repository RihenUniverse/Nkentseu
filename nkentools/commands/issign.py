# issign.py

import subprocess
import argparse
import os
import sys
from pathlib import Path

# Chemins par défaut vers le JDK et le SDK
jdk_path = "C:/Program Files/Java/jdk-17"
sdk_path = "C:/Users/teugu/AppData/Local/Android/Sdk"


def description():
    """
    Ce script permet de vérifier si un fichier APK est signé correctement.
    Vous pouvez spécifier le chemin du fichier APK à vérifier.
    """
    print("""
    Ce script permet de vérifier si un fichier APK est signé correctement.
    Vous pouvez spécifier le chemin du fichier APK à vérifier.
    """)


def help_command():
    """
    Affiche le message d'aide détaillé.
    """
    print("""
    Utilisation : ./nken issign [options]
    
    Options:
        --apk    : Chemin vers le fichier APK à vérifier (obligatoire).
        
    Exemple :
        ./nken issign --apk myapp.apk
    """)


def issign(apk):
    """
    Vérifie si un fichier APK est signé correctement.
    
    :param apk: Chemin vers le fichier APK.
    """
    try:
        build_tools_path = os.path.join(sdk_path, "build-tools")
        latest_build_tool = sorted(os.listdir(build_tools_path))[-1]
        apksigner_path = os.path.join(build_tools_path, latest_build_tool, "apksigner.bat")

        command = [apksigner_path, "verify", apk]

        result = subprocess.run(command, check=True, capture_output=True, text=True)
        print(result.stdout)  # Affiche le résultat de la vérification
        print(f"Le fichier {apk} est signé correctement.")
    except subprocess.CalledProcessError as e:
        print(f"Le fichier {apk} n'est pas signé correctement : {e.stderr}")
        sys.exit(1)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Outil de vérification de signature APK pour Android.")

    parser.add_argument('--apk', required=True, help="Chemin vers le fichier APK à vérifier.")

    args = parser.parse_args()
    
    issign(args.apk)
    sys.exit(0)
