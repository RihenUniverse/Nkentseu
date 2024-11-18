import os
import sys
import shutil
from pathlib import Path
import fnmatch


def load_nkenclean_config():
    """
    Charge les extensions et dossiers supplémentaires à partir du fichier .nkenclean.
    """
    extra_extensions = []
    extra_folders = []
    config_path = Path(".nkenclean")
    
    if config_path.is_file():
        with config_path.open("r", encoding="utf-8") as f:
            for line in f:
                line = line.strip().lower()  # Convertir tout en minuscule pour comparaison
                if line.startswith("#") or not line:
                    continue  # Ignorer les lignes de commentaires et les lignes vides
                if "*" in line or line.startswith("."):  # Extensions ou motifs de fichiers
                    extra_extensions.append(line)
                else:
                    extra_folders.append(line.rstrip("/"))  # Retirer le / final des dossiers
    return extra_extensions, extra_folders


def load_project_list():
    """
    Charge les projets supplémentaires à partir du fichier ./nkentools/workspace/list_project.ntslp.
    """
    projects_path = Path("./nkentools/workspace/list_project.ntslp")
    project_list = {}
    
    if projects_path.is_file():
        with projects_path.open("r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if line.startswith("#") or not line:
                    continue  # Ignorer les lignes de commentaires et les lignes vides
                line = line.split('#', 1)[0].strip()  # Ignorer tout ce qui suit un '#' sur la même ligne
                if "=" in line:
                    project_name, project_path = line.split("=", 1)
                    project_list[project_name.strip()] = project_path.strip()
    return project_list


def description():
    """
    Ce script nettoie les fichiers et dossiers générés par la compilation dans les projets spécifiés.
    """
    print("""
    Ce script permet de supprimer les dossiers et fichiers temporaires créés par les environnements de développement.
    Il prend en charge des extensions et des dossiers spécifiques pour chaque projet configuré.
    """)


def help_command():
    """
    Affiche l'aide détaillée.
    """
    print("""
    Utilisation : ./nken clean
        
    Le fichier .nkenclean (s'il est présent) peut contenir des motifs d'extensions de fichiers (ex. *.tmp) 
    et des noms de dossiers (ex. temp), un par ligne. Les lignes vides et celles commençant par '#' seront ignorées.
    
    Exemples :
        ./nken clean
    """)


def clean_command(extra_extensions=None, extra_folders=None, project_list=None):
    """
    Nettoie les fichiers et dossiers temporaires dans les projets spécifiés.
    
    :param extra_extensions: Liste de motifs d'extensions supplémentaires à supprimer.
    :param extra_folders: Liste de noms de dossiers supplémentaires à supprimer.
    """
    extensions = set(extra_extensions)
    folders = set(extra_folders)

    for project_name, project_path in project_list.items():
        path = os.path.join(".", project_path)
        for root, dirs, files in os.walk(path):
            # Supprimer les dossiers correspondant aux motifs dans `folders`
            dirs_to_remove = [d for d in dirs if any(fnmatch.fnmatch(d.lower(), folder.lower()) for folder in folders)]
            for d in dirs_to_remove:
                dir_path = os.path.join(root, d)
                try:
                    print(f"============= [{project_name}] (Deleted folder) | \"{dir_path}\"")
                    shutil.rmtree(dir_path)
                except Exception as e:
                    print(f"Erreur lors de la suppression du dossier \"{dir_path}\": {e}")
                dirs.remove(d)  # Supprimer le dossier de la liste `dirs` pour éviter une descente récursive

            # Supprimer les fichiers correspondant aux motifs dans `extensions`
            files_to_remove = [f for f in files if any(fnmatch.fnmatch(f.lower(), ext.lower()) for ext in extensions)]
            for f in files_to_remove:
                file_path = os.path.join(root, f)
                try:
                    print(f"============= [{project_name}] (Deleted file) | \"{file_path}\"")
                    os.remove(file_path)
                except Exception as e:
                    print(f"Erreur lors de la suppression du fichier \"{file_path}\": {e}")


if __name__ == '__main__':
    # Charger les configurations supplémentaires depuis .nkenclean et list_project.ntslp
    file_extensions, file_folders = load_nkenclean_config()
    project_list = load_project_list()

    # Exécuter la commande de nettoyage avec les arguments facultatifs et ceux du fichier .nkenclean
    clean_command(extra_extensions=file_extensions, extra_folders=file_folders, project_list=project_list)

    sys.exit(0)
