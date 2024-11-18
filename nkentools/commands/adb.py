# adb.py

import subprocess
import argparse
import os
import sys

# Chemins par défaut vers le JDK et le SDK
jdk_path = "C:/Program Files/Java/jdk-17"
sdk_path = "C:/Users/teugu/AppData/Local/Android/Sdk"
adb_path = os.path.join(sdk_path, "platform-tools", "adb")


def description():
    """
    Ce script permet de lister les appareils connectés via ADB avec des informations détaillées.
    """
    print("""
    Ce script permet de lister les appareils connectés via ADB avec des informations détaillées.
    Il retourne l'ID de l'appareil et d'autres propriétés comme le modèle et le statut.
    """)


def help_command():
    """
    Affiche le message d'aide détaillé.
    """
    print("""
    Utilisation : ./nken adb <action> [options]
    
    Actions:
        list        : Affiche la liste des périphériques ADB connectés avec des informations détaillées.
        install     : Installe un APK sur un périphérique spécifique.
        
    Options:
        --id        : ID de l'appareil pour l'installation d'un APK (utilisé avec 'install').
        --apk       : Chemin vers l'APK à installer (utilisé avec 'install').
        --help      : Affiche cette aide.
        
    Exemples :
        ./nken adb list
        ./nken adb install --id <device_id> --apk <path_to_apk>
    """)


def list_devices():
    """
    Liste les appareils ADB connectés avec leurs informations détaillées.
    """
    try:
        command = [adb_path, "devices", "-l"]
        result = subprocess.run(command, check=True, capture_output=True, text=True)
        devices = result.stdout.strip().splitlines()[1:]  # Ignore la première ligne d'en-tête

        device_dict = {}

        for line in devices:
            if line:
                parts = line.split()
                device_id = parts[0]
                properties = {}

                for detail in parts[1:]:
                    if ':' in detail:
                        key, value = detail.split(':', 1)
                        properties[key] = value.strip()

                device_dict[device_id] = properties

        if device_dict:
            print("Appareils connectés via ADB :")
            for device_id, props in device_dict.items():
                print(f"\nID de l'appareil : {device_id}")
                for key, value in props.items():
                    print(f"  {key} : {value}")
        else:
            print("Aucun appareil ADB connecté.")

    except subprocess.CalledProcessError as e:
        print(f"Erreur lors de la récupération des périphériques ADB : {e}")
        sys.exit(1)
    

def install_apk(device_id, apk):
    """
    Installe un fichier APK sur un périphérique spécifié.
    """
    try:
        command = [adb_path, "-s", device_id, "install", apk]
        subprocess.run(command, check=True)
        print(f"APK installé avec succès sur le périphérique {device_id}.")
    except subprocess.CalledProcessError as e:
        print(f"Erreur lors de l'installation de l'APK sur le périphérique {device_id} : {e}")
        sys.exit(1)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Outil ADB pour gérer les périphériques et installer des APK.")
    parser.add_argument("action", choices=["list", "install"], help="Action à exécuter : 'list' ou 'install'")

    # Arguments optionnels pour l'action "install"
    parser.add_argument("--id", help="ID du périphérique pour l'installation de l'APK (utilisé avec 'install').")
    parser.add_argument("--apk", help="Chemin vers l'APK à installer (utilisé avec 'install').")
    
    args = parser.parse_args()
    
    if args.action == "list":
        list_devices()
    elif args.action == "install":
        if not args.id or not args.apk:
            print("Erreur : '--device_id' et '--apk' sont requis pour l'action 'install'.")
            help_command()
            sys.exit(1)
        install_apk(args.id, args.apk)
    else:
        help_command()
    
    sys.exit(0)
