import os
from pathlib import Path
import shutil
import sys
import argparse

from misra.tools import *

PYTHON_EXE = "python"
# PYTHON_EXE = "python3"

def ensure_directory_exists(directory_path):
    directory = Path(directory_path)
    if not directory.exists():
        directory.mkdir(parents=True, exist_ok=True)
def main():
    parser = argparse.ArgumentParser(description="Dscription:Install Cppcheck with Misra-C2012 support\n"
                                     "usage: python/python3 install_win.py [D:/]\n"
                                      "note: you must use python 3!" )
    parser.add_argument('--install', type=str, required=True, help='The [absolute path] where you want to install cppcheck-misra.')
    args = parser.parse_args()
    if os.name != 'nt':
        log_error("This script only works on Windows.")
        return
    cppcheck_dir = None
    if args.install:
        cppcheck_dir = args.install
        cppcheck_dir = cppcheck_dir.replace("\\", "/")
    else:
        log_error("Please specify the path to install cppcheck-misra")
    
    
    
    
    # Modify misra configuration
    place_holder = "REPLACE_ME"
    replace_path = os.path.dirname(cppcheck_dir)
    # misra.json
    misra_json_in = Path("conf/misra.json.in")
    misra_json_out = Path(os.path.join("misra", "misra.json"))
    with misra_json_in.open('r') as f:
        content = f.read().replace(place_holder, replace_path)
        content = content.replace("PYTHON_EXE", PYTHON_EXE)
    with misra_json_out.open('w') as f:
        f.write(content)
    # pre-commit-config
    pre_commit_config_in = Path("conf/.pre-commit-config.yaml.in")
    pre_commit_config_out = Path(os.path.join("misra", ".pre-commit-config.yaml"))
    with pre_commit_config_in.open('r') as f:
        content = f.read().replace(place_holder, replace_path)
        content = content.replace("PYTHON_EXE", PYTHON_EXE)
    with pre_commit_config_out.open('w') as f:
        f.write(content)
    # misra.sh / misra.bat
    misra_bat_in = Path("conf/misra.bat.in")
    misra_bat_out = Path(os.path.join("misra", "misra.bat"))
    with misra_bat_in.open('r') as f:
        content = f.read().replace(place_holder, replace_path)
        content = content.replace("PYTHON_EXE", PYTHON_EXE)
    with misra_bat_out.open('w') as f:
        f.write(content)
    # Copy misra files to the appropriate location
    shutil.copytree("misra", os.path.join(cppcheck_dir,"misra"), dirs_exist_ok=True)
    log_success("## install dependencies")
    run_command("pip3 install pygments elementpath pre-commit")
    log_success("## modify misra config")


    log_success("## Config git template dir")
    command_result = run_command("git config --global init.templatedir")
    if command_result:
        git_template_dir = command_result.rstrip('\n')
    else:
        git_template_dir = os.path.join(Path.home(), ".git-template")
    run_command(f"git config --global init.templateDir {git_template_dir}")
    ensure_directory_exists(git_template_dir)
    run_command(f"pre-commit init-templatedir {git_template_dir}")


    cppcheck_misra_bat = os.path.join(cppcheck_dir, "misra/misra.bat")
    new_cppcheck_bat = os.path.join(cppcheck_dir, "misra.bat")
    os.chmod(os.path.join(cppcheck_dir, "misra/.pre-commit-config.yaml"), 0o755)
    if os.name == "nt":
        os.chmod(cppcheck_misra_bat, 0o755)
        #os.symlink(os.path.join(cppcheck_dir,"misra/misra.bat"), os.path.join(bin_dir, "misra"))
    try:
        shutil.copy(cppcheck_misra_bat, new_cppcheck_bat)
    except FileNotFoundError:
        print("源文件不存在")
    except PermissionError:
        print("没有权限移动文件")
    # Set global git template directory
    output = run_command(f"git config --global init.templateDir").strip('\n')
    if output == git_template_dir:
        log_success("## Set global git template directory successfully")
    else:
        log_error("## Failed to set global git template directory")
        log_error("Install Failed")
        exit(1)


    # log_warning(f"## add {bin_dir} to your PATH")
    # if os.name == 'nt':
    #     subprocess.run(['setx', "PATH", bin_dir], check=True)
    #     log_success(f"## ADD {bin_dir} to PATH")
    # else:
    #     log_error("ERROR: Unsupported OS：{os.name}")
    
    # # Verify the PATH variable
    # log_warning(f"Current PATH:\n {os.environ['PATH']}")
    log_success("## Cppcheck-Misra Install Successful!")

if __name__ == "__main__":
    main()


