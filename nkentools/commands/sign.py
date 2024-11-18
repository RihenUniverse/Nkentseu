# sign.py

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
    Ce script permet de signer un fichier APK en utilisant le JDK et le SDK Android.
    Vous pouvez spécifier le chemin vers le fichier APK, le keystore, l'alias de clé et les mots de passe.
    """
    print("""
    Ce script permet de signer un fichier APK en utilisant le JDK et le SDK Android.
    Vous pouvez spécifier le chemin vers le fichier APK, le keystore, l'alias de clé et les mots de passe.
    """)


def help_command():
    """
    Affiche le message d'aide détaillé.
    """
    print("""
    Utilisation : ./nken sign [options]
    
    Options:
        --apk          : Chemin vers le fichier APK à signer (obligatoire).
        --keystore     : Chemin vers le keystore à utiliser pour signer l'APK (obligatoire).
        --alias        : Alias de la clé dans le keystore (obligatoire).
        --storepass    : Mot de passe du keystore (obligatoire).
        --keypass      : Mot de passe de la clé (facultatif, si différent du storepass).
        
    Exemple :
        ./nken sign --apk myapp.apk --keystore mykeystore.jks --alias mykeyalias --storepass mystorepass --keypass mykeypass
    """)


def sign(apk, keystore, alias, storepass, keypass):
    """
    Signe un fichier APK avec les informations de keystore et de clé fournies.
    
    :param apk: Chemin vers le fichier APK.
    :param keystore: Chemin vers le keystore.
    :param alias: Alias de la clé dans le keystore.
    :param storepass: Mot de passe du keystore.
    :param keypass: Mot de passe de la clé (facultatif).
    """
    try:
        build_tools_path = os.path.join(sdk_path, "build-tools")
        latest_build_tool = sorted(os.listdir(build_tools_path))[-1]
        apksigner_path = os.path.join(build_tools_path, latest_build_tool, "apksigner.bat")

        command = [
            apksigner_path,
            "sign",
            "--ks", keystore,
            "--ks-key-alias", alias,
            "--ks-pass", f"pass:{storepass}",
            "--key-pass", f"pass:{keypass}" if keypass else f"pass:{storepass}",
            apk
        ]

        result = subprocess.run(command, check=True, capture_output=True, text=True)
        print(f"Le fichier {apk} a été signé avec succès :\n{result.stdout}")
    except subprocess.CalledProcessError as e:
        print(f"Erreur lors de la signature du fichier : {e.stderr}")
        sys.exit(1)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Outil de signature APK pour Android.")

    parser.add_argument('--apk', required=True, help="Chemin vers le fichier APK à signer.")
    parser.add_argument('--keystore', required=True, help="Chemin vers le keystore.")
    parser.add_argument('--alias', required=True, help="Alias de la clé dans le keystore.")
    parser.add_argument('--storepass', required=True, help="Mot de passe du keystore.")
    parser.add_argument('--keypass', help="Mot de passe de la clé (facultatif, si différent du storepass).")

    args = parser.parse_args()

    sign(args.apk, args.keystore, args.alias, args.storepass, args.keypass)
    sys.exit(0)
