# nts build
# nts run
# nts gen
# nts version
# nts di [extension] [project_name] [-all]
# nts gen build run
#!/usr/bin/env python3

import os, sys
import subprocess

"""source ~/.bash_env"""

def GetCommandPath(command):
    return f'./nkentools/commands/{command}.py'


def RunCommand(command, command_path, options):
    ret = 0

    
    args_command = ["py", command_path]
    if len(options) >= 1:
        for e in options:
            args_command.append(e)
    print("----------------------------------- Executing : {} -----------------------------------\n".format(command))
    ret = subprocess.call(args_command)
    print("\n----------------------------------- End executing : {} -----------------------------------\n".format(command))
    return ret


def RunCommands(argv):
    len_command = len(argv)
    i = 1
    command_separator = ";"
    run_command = False

    while i < len_command:
        command = ""
        options = []
        j = i
        
        while j < len_command:
            actual = argv[j]

            if actual == command_separator:
                j += 1
                break

            if command_separator in actual:
                actual_split = actual.split(command_separator)
                if actual_split[0] != "":
                    options.append(actual_split[0])

            if j == i:
                command = argv[j]
            else:
                # options = f"{options} {sys.argv[j]}"
                options.append(argv[j])
            
            j += 1
            if j >= len_command:
                break
        
        i = j

        if command == "?":
            command = "help"
            
        command_path = GetCommandPath(command)

        if os.path.isfile(command_path):
            if RunCommand(command, command_path, options) != 0:
                print("Error: command {} fail ".format(command))
                return
            run_command = True
        else:
            print("Error: {} is not a command tips [nken help] to get all command and information for it".format(command))
            print(f'Command "{command}" not found!')
            print(f'')
            print(f'Available Command:')
            for file in os.listdir('./Scripts/commands'):
                if file.endswith('.py') and file != "__init__.py":
                    print(f'- {file[0:-3]}')
            return
        
    if run_command == False:
        if RunCommand("help", GetCommandPath("help"), []) != 0:
                print("Error: command {} fail ".format("help"))
                return
        

RunCommands(sys.argv)