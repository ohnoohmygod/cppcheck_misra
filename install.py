import os
import subprocess
import shutil
from pathlib import Path
import argparse
from misra.tools import *
def ensure_directory_exists(directory_path):
    directory = Path(directory_path)
    if not directory.exists():
        directory.mkdir(parents=True, exist_ok=True)


def main():
    parser = argparse.ArgumentParser(description="Dscription:Install Cppcheck with Misra-C2012 support")
    parser.add_argument('--path', type=str, required=False, help='Default: $HOME/cppcheck.  The [absolute path] where you want to install cppcheck-misra.')
    args = parser.parse_args()
    # 卸载已有的cppcheck
    target_version = "Cppcheck 2.14.0"
    current_platform = os.name
    installed_version = None
    
    
    home_dir = str(Path.home())
    local_dir = os.path.join(home_dir, ".local")
    cppcheck_dir = local_dir
    if args.path:
        cppcheck_dir = args.path

    
    # Check if cppcheck is installed
    try:
        output = run_command("cppcheck --version")
        if output:
            installed_version = output.strip()
    except FileNotFoundError:
        installed_version = None
    # cppcheck is already installed
    if installed_version:
        log_warning(f"## {installed_version} is alreay installed!")
        log_warning(f"Please to remove cppcheck before this install")
        #remove_cppcheck()
        if current_platform == 'posix': # Linux/MacOS
            output = run_command("which cppcheck")
        elif current_platform == 'nt': # Windows
            output = run_command("where cppcheck")
        else:
            log_error("Unsupported platform: {},only support for Ubuntu and Windows".format(current_platform))
            log_error("Install Failed")
            exit(1)
        if output:
            log_error(f"Another coocheck is found at {output}")
            log_error(f"Please remove it manually before you install this cppcheck-misra!")
            log_error(f"Insatall failed: Have another cppcheck installed!")
            exit(1)
        else:
            log_error(f"Remove success!")
    # install directory for cppcheck
    # some directiries

    # ensure_directory_exists(bin_dir)
    
    ensure_directory_exists(cppcheck_dir)
    if current_platform == "posix":
        log_success("## Compile cppcheck-2.14, it will take a while")
        tar_file = "cppcheck-2.14.0.tar.gz"
        try:
            # Extract the cppcheck then compile cppcheck
            shutil.unpack_archive(tar_file, extract_dir=".")
            os.chdir("cppcheck-2.14.0")
            if current_platform == 'posix':
                make_tool = "make"
            run_command(f"{make_tool} clean")
            make_command = (
                f"{make_tool} "
                f"DESTDIR={cppcheck_dir} "
                f"FILESDIR=/cppcheck "  #install -d ${DESTDIR}${FILESDIR}
                f"PREFIX=/cppcheck " # BIN=$(DESTDIR)$(PREFIX)/bin
                f"CFGDIR=cppcheck/bin/cfg/ "
                f"> /dev/null "
                f" install -j4"
            )
            # print(make_command)
            run_command(make_command)
            # run_command("make install")
            os.chdir("..")
            shutil.rmtree("cppcheck-2.14.0")
        except Exception as e:
            log_error(f"Compilation of cppcheck failed: {e}")


    log_success("## install dependencies")
    run_command("pip3 install pygments elementpath pre-commit")
    log_success("## modify misra config")


    # Modify misra configuration
    place_holder = "REPLACE_ME"
    # misra.json
    misra_json_in = Path("conf/misra.json.in")
    misra_json_out = Path(os.path.join("misra", "misra.json"))
    with misra_json_in.open('r') as f:
        content = f.read().replace(place_holder, cppcheck_dir)
    with misra_json_out.open('w') as f:
        f.write(content)
    # pre-commit-config
    pre_commit_config_in = Path("conf/.pre-commit-config.yaml.in")
    pre_commit_config_out = Path(os.path.join("misra", ".pre-commit-config.yaml"))
    with pre_commit_config_in.open('r') as f:
        content = f.read().replace(place_holder, cppcheck_dir)
    with pre_commit_config_out.open('w') as f:
        f.write(content)
    # misra.sh / misra.bat
    if current_platform == "posix":
        misra_sh_in = Path("conf/misra.sh.in")
        misra_sh_out = Path(os.path.join("misra", "misra.sh"))
        with misra_sh_in.open('r') as f:
            content = f.read().replace(place_holder, cppcheck_dir)
        with misra_sh_out.open('w') as f:
            f.write(content)
    elif current_platform == "nt":
        misra_bat_in = Path("conf/misra.bat.in")
        misra_bat_out = Path(os.path.join("misra", "misra.bat"))
        with misra_bat_in.open('r') as f:
            content = f.read().replace(place_holder, cppcheck_dir)
        with misra_bat_out.open('w') as f:
            f.write(content)

    # Copy misra files to the appropriate location
    shutil.copytree("misra", os.path.join(cppcheck_dir,"cppcheck/misra"), dirs_exist_ok=True)
    


    log_success("## Config git template dir")
    git_template_dir = run_command("git config --global init.templatedir").rstrip('\n')
    if not git_template_dir:
        git_template_dir = os.path.join(Path.home(), ".git-template")
        run_command("git config --global init.templateDir {git_template_dir}")
    ensure_directory_exists(git_template_dir)
    run_command(f"pre-commit init-templatedir {git_template_dir}")

    # Ensure that cppcheck/bin is in the PATH
    bin_dir = os.path.join(cppcheck_dir, "cppcheck/bin")
    os.chmod(os.path.join(cppcheck_dir, "cppcheck/misra/.pre-commit-config.yaml"), 0o755)
    if os.name == "posix":
        os.chmod(os.path.join(cppcheck_dir, "cppcheck/misra/misra.sh"), 0o755)
        os.symlink(os.path.join(cppcheck_dir,"cppcheck/misra/misra.sh"), os.path.join(bin_dir, "misra"))
    elif os.name == "nt":
        os.chmod(os.path.join(cppcheck_dir, "cppcheck/misra/misra.bat"), 0o755)
        os.symlink(os.path.join(cppcheck_dir,"cppcheck/misra/misra.bat"), os.path.join(bin_dir, "misra"))

    # Set global git template directory
    output = run_command(f"git config --global init.templateDir").strip('\n')
    if output == git_template_dir:
        log_success("## Set global git template directory successfully")
    else:
        log_error("## Failed to set global git template directory")
        log_error("Install Failed")
        exit(1)


    log_error(f"!!! Please add {bin_dir} to your PATH !!!")
    # path_env_var = os.environ.get('PATH')
    # if str(bin_dir) not in path_env_var:
    #     if current_platform == 'posix':
    #         #print("#####22222222222")
    #         bashrc_file = os.path.join(Path.home(), '.bashrc')

    #         with open(bashrc_file, 'a') as f:
    #             f.write(f'export PATH={bin_dir}\n')
    #         run_command("source ~/.bashrc")
    #     elif current_platform == 'nt':
    #         subprocess.run(['setx', "PATH", bin_dir], check=True)
    #     log_success(f"## ADD {bin_dir} to PATH")
    # else:
    #     log_success(f"## PATH already contains {bin_dir}")
    
    # Verify the PATH variable
    # log_warning(f"Current PATH:\n {os.environ['PATH']}")
    log_success("## Cppcheck-Misra Install Successful!")

if __name__ == "__main__":
    main()