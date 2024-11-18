import os
import sys
import importlib
from pathlib import Path


commands_dir = "./nkentools/commands"


def get_module_command(command_name):
    """
    Tente d'importer un module de commande spécifique.

    Args:
        command_name (str): Nom du module de la commande.

    Returns:
        tuple: (bool, module) Retourne un booléen indiquant si le module existe, et le module lui-même ou None.
    """
    try:
        # Création du nom du module à partir du chemin relatif
        command_module = importlib.import_module(f"{command_name}")
        return True, command_module
    except ModuleNotFoundError:
        return False, None


def description():
    return "Commande d'aide pour comprendre comment les autres commandes fonctionnent"


def help_command(command_name):
    """
    Affiche l'aide d'une commande spécifique.

    Args:
        command_name (str): Nom de la commande.
    """
    exists, command_module = get_module_command(command_name)

    if not exists:
        print(f"Commande inconnue : '{command_name}'")
        return

    if not hasattr(command_module, "help_command"):
        print(f"La commande '{command_name}' n'a pas d'aide disponible.")
    else:
        command_module.help_command()


def run(command_names=None):
    """
    Affiche l'aide générale ou l'aide pour des commandes spécifiques.

    Args:
        command_names (list): Liste des noms de commandes.
    """
    if command_names is None:
        command_names = []

    print("**Aide du shell nkentseu**")
    print("---------------------")

    if not command_names or command_names[0] in ["", "?", "help"]:
        print("**Commandes disponibles :**")
        # Liste les commandes disponibles
        for file in os.listdir(commands_dir):
            if file.endswith(".py") and file != "__init__.py":
                command_name = file[:-3]
                exists, command_module = get_module_command(command_name)

                if exists and hasattr(command_module, "description"):
                    print(f" - {command_name} : {command_module.description()}")
    else:
        for command_name in command_names:
            help_command(command_name)

    print("\n\n**Utilisation :**")
    print(" - `./nken commande [arguments]`")
    print(" - `./nken commande [arguments] ; commande [arguments]`")
    print("**Aide pour une commande spécifique :**")
    print(" - `./nken help commande`")



if __name__ == '__main__':
    run(sys.argv[1:])
    sys.exit(0)
