import subprocess
import argparse
import os
import sys
from pathlib import Path
import platform

# Détection du système d'exploitation
is_windows = platform.system() == "Windows"

# Définition des chemins dynamiquement
if is_windows:
    jdk_path = os.path.join("C:", "Program Files", "Java", "jdk-17")
    sdk_path = os.path.join("C:", "Users", os.getenv("USERNAME"), "AppData", "Local", "Android", "Sdk")
    ndk_path = os.path.join(sdk_path, "ndk", "25.1.8937393")  # Remplacez par la version de votre NDK
else:
    # Exemple pour Linux/macOS
    jdk_path = os.path.expanduser("~/java/jdk-17")
    sdk_path = os.path.expanduser("~/Android/Sdk")
    ndk_path = os.path.join(sdk_path, "ndk", "25.1.8937393")  # Remplacez par la version de votre NDK



def description():
    return "Commande pour créer un projet ou des fichiers, générer un keystore, etc."


def help_command():
    """Affiche l'aide pour une action spécifique."""
    for action_name in ["filesign", "class", "struct", "enum", "project"]:
        if action_name == "filesign":
            print("Commande : filesign")
            print("  Description : Crée un keystore pour signer des applications.")
            print("  Arguments requis :")
            print("    --keystore : Nom du fichier keystore à créer.")
            print("    --alias    : Alias de la clé.")
            print("    --storepass: Mot de passe du keystore.")
            print("    --keypass  : Mot de passe de la clé.")
            print("  Arguments facultatifs :")
            print("    --cn : Nom commun (CN) [défaut: 'Your Name'].")
            print("    --ou : Unité organisationnelle (OU) [défaut: 'Your Organization Unit'].")
            print("    --o : Organisation (O) [défaut: 'Your Organization'].")
            print("    --l : Localité (L) [défaut: 'Your City'].")
            print("    --s : État (S) [défaut: 'Your State'].")
            print("    --c : Pays (C) [défaut: 'Your Country'].")
            print("  Exemple : ./nken create filesign --keystore mon_keystore.jks --alias ma_cle --storepass mon_mot_de_passe --keypass mon_mot_de_passe --cn 'John Doe' --ou 'Dev Team' --o 'MyOrg' --l 'Paris' --s 'Ile-de-France' --c 'FR'")
            print()
        
        elif action_name == "class":
            print("Commande : class")
            print("  Description : Crée une classe dans un projet.")
            print("  Arguments requis :")
            print("    --project_name : Nom du projet.")
            print("    --group_name   : Nom du groupe.")
            print("    --class_name   : Nom de la classe.")
            print("    --visibility    : Visibilité de la classe (public, private, protected).")
            print("  Exemple : ./nken create class --project_name MonProjet --group_name MonGroupe --class_name MaClasse --visibility public")
            print()
        
        elif action_name == "struct":
            print("Commande : struct")
            print("  Description : Crée une structure dans un projet.")
            print("  Arguments requis :")
            print("    --project_name : Nom du projet.")
            print("    --group_name   : Nom du groupe.")
            print("    --struct_name  : Nom de la structure.")
            print("    --visibility    : Visibilité de la structure (public, private, protected).")
            print("  Exemple : ./nken create struct --project_name MonProjet --group_name MonGroupe --struct_name MaStructure --visibility private")
            print()
        
        elif action_name == "enum":
            print("Commande : enum")
            print("  Description : Crée une énumération dans un projet.")
            print("  Arguments requis :")
            print("    --project_name : Nom du projet.")
            print("    --group_name   : Nom du groupe.")
            print("    --enum_name    : Nom de l'énumération.")
            print("    --visibility    : Visibilité de l'énumération (public, private, protected).")
            print("  Exemple : ./nken create enum --project_name MonProjet --group_name MonGroupe --enum_name MonEnum --visibility protected")
            print()
        
        elif action_name == "project":
            print("Commande : project")
            print("  Description : Crée un projet.")
            print("  Arguments requis :")
            print("    --project_name : Nom du projet.")
            print("    --group_name   : Nom du groupe.")
            print("  Exemple : ./nken create project --project_name MonProjet --group_name MonGroupe")
            print()
        
        else:
            print("Commande inconnue. Veuillez utiliser 'help' pour voir la liste des commandes disponibles.")


def run_filesign(jdk_path, keystore, alias, storepass, keypass, cn, ou, o, l, s, c):
    try:
        keytool_path = os.path.join(jdk_path, "bin", "keytool.exe")
        
        # Construction de la commande keytool avec les valeurs dname par défaut ou fournies
        dname = f"CN={cn}, OU={ou}, O={o}, L={l}, S={s}, C={c}"
        
        command = [
            keytool_path,
            "-genkeypair",
            "-v",
            "-keystore", keystore,
            "-alias", alias,
            "-storepass", storepass,
            "-keypass", keypass,
            "-keyalg", "RSA",
            "-keysize", "2048",
            "-validity", "10000",
            "-dname", dname
        ]
        
        subprocess.run(command, check=True)
        print(f"Keystore créé avec succès : {keystore}")
    except subprocess.CalledProcessError as e:
        print(f"Erreur lors de la création du keystore : {e}")
        exit(1)

def run_class(args):
    """Génère une classe avec les arguments spécifiés."""
    print(f"Exécution de run_class avec les arguments :")
    print(f"  Projet : {args.project_name}")
    print(f"  Groupe : {args.group_name}")
    print(f"  Classe : {args.class_name}")
    print(f"  Visibilité : {args.visibility}")

def run_struct(args):
    """Génère une structure avec les arguments spécifiés."""
    print(f"Exécution de run_struct avec les arguments :")
    print(f"  Projet : {args.project_name}")
    print(f"  Groupe : {args.group_name}")
    print(f"  Structure : {args.struct_name}")
    print(f"  Visibilité : {args.visibility}")

def run_enum(args):
    """Génère une énumération avec les arguments spécifiés."""
    print(f"Exécution de run_enum avec les arguments :")
    print(f"  Projet : {args.project_name}")
    print(f"  Groupe : {args.group_name}")
    print(f"  Énumération : {args.enum_name}")
    print(f"  Visibilité : {args.visibility}")

def run_project(args):
    """Génère un projet avec les arguments spécifiés."""
    print(f"Exécution de run_project avec les arguments :")
    print(f"  Nom du projet : {args.project_name}")
    print(f"  Groupe : {args.group_name}")

def run_help():
    """Affiche l'aide pour chaque commande disponible."""
    print("Affichage de l'aide pour chaque commande disponible.")
    print("Actions disponibles :")
    print("  filesign  : Crée un keystore.")
    print("  class     : Crée une classe.")
    print("  struct    : Crée une structure.")
    print("  enum      : Crée une énumération.")
    print("  project   : Crée un projet.")
    print("  help      : Affiche cette aide.")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=description())
    
    # Définir l'action principale à réaliser
    parser.add_argument("action", choices=["filesign", "class", "struct", "enum", "project", "help"], help="Action à exécuter")

    # Arguments pour la création du keystore (utilisés uniquement avec l'action `filesign`)
    parser.add_argument("--keystore", help="Nom du fichier keystore à créer")
    parser.add_argument("--alias", help="Alias de la clé")
    parser.add_argument("--storepass", help="Mot de passe du keystore")
    parser.add_argument("--keypass", help="Mot de passe de la clé")
    
    # Arguments pour les informations d'identité (facultatifs)
    parser.add_argument("--cn", default="ttrs", help="Nom commun (CN)")
    parser.add_argument("--ou", default="Rihen", help="Unité organisationnelle (OU)")
    parser.add_argument("--o", default="Rihen", help="Organisation (O)")
    parser.add_argument("--l", default="Nkolmessen", help="Localité (L)")
    parser.add_argument("--s", default="Yaounde", help="État (S)")
    parser.add_argument("--c", default="Cameroun", help="Pays (C)")

    # Arguments pour les actions `class`, `struct`, `enum`, et `project`
    parser.add_argument("--project_name", help="Nom du projet")
    parser.add_argument("--group_name", help="Nom du groupe")
    
    # Arguments spécifiques pour les classes, structures et énumérations
    parser.add_argument("--class_name", help="Nom de la classe")
    parser.add_argument("--struct_name", help="Nom de la structure")
    parser.add_argument("--enum_name", help="Nom de l'énumération")
    parser.add_argument("--visibility", choices=["public", "private", "protected"], help="Visibilité de la classe/structure/énumération")

    args = parser.parse_args()

    # Vérifier les arguments nécessaires pour chaque action
    if args.action == "filesign":
        if not all([args.keystore, args.alias, args.storepass, args.keypass]):
            parser.error("L'action 'filesign' nécessite --keystore, --alias, --storepass et --keypass.")
        run_filesign(jdk_path, args.keystore, args.alias, args.storepass, args.keypass, args.cn, args.ou, args.o, args.l, args.s, args.c)

    elif args.action == "class":
        if not all([args.project_name, args.group_name, args.class_name, args.visibility]):
            parser.error("L'action 'class' nécessite --project_name, --group_name, --class_name et --visibility.")
        run_class(args)

    elif args.action == "struct":
        if not all([args.project_name, args.group_name, args.struct_name, args.visibility]):
            parser.error("L'action 'struct' nécessite --project_name, --group_name, --struct_name et --visibility.")
        run_struct(args)

    elif args.action == "enum":
        if not all([args.project_name, args.group_name, args.enum_name, args.visibility]):
            parser.error("L'action 'enum' nécessite --project_name, --group_name, --enum_name et --visibility.")
        run_enum(args)

    elif args.action == "project":
        if not all([args.project_name, args.group_name]):
            parser.error("L'action 'project' nécessite --project_name et --group_name.")
        run_project(args)

    elif args.action == "help":
        run_help()

    sys.exit(0)