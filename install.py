import os
import shutil
from pathlib import Path
import argparse
import sys
from misra.tools import *

#PYTHON_EXE = "python"
# 如果是特定的python版本， 可以指定
PYTHON_EXE = sys.executable
def checkPythonVersion():
    global PYTHON_EXE
    PYTHON_EXE = "python3"
    resString = run_command(PYTHON_EXE + " --version" )
    if (resString != None):
        print(resString)
        return
    else:
        PYTHON_EXE = "python"
        resString = run_command(PYTHON_EXE + " --version" )
        if (resString != None):
            version = resString.strip()[1].split('.')[0]
            if version == '2':
                print("Need python version >= 3.x")
                exit(1)
            else:
                print(resString)
                return
        else:
            print("依赖命令python/python3")
            print("如果指定python路径， 请更改PYTHON_EXE")
            exit(1)
        

def ensure_directory_exists(directory_path):
    directory = Path(directory_path)
    if not directory.exists():
        directory.mkdir(parents=True, exist_ok=True)


def main():
    parser = argparse.ArgumentParser(description="Description:Install Cppcheck with Misra-C2012 support")
    parser.add_argument('--path', type=str, required=False, help='The [absolute path] where you want to install cppcheck-misra.  Default: $HOME/cppcheck.')
    args = parser.parse_args()
    current_platform = os.name
    if current_platform != 'posix':
        return
    # 卸载已有的cppcheck
    target_version = "cppcheck-2.16.0"
    installed_version = None
    
    
    home_dir = str(Path.home())
    local_dir = os.path.join(home_dir, ".local")
    cppcheck_dir = local_dir
    if args.path:
        cppcheck_dir = args.path

    
    # Check whtere cppcheck is installed
    try:
        output = run_command("cppcheck --version")
        if output:
            installed_version = output.strip()
    except FileNotFoundError:
        installed_version = None
    # cppcheck is already installed: Need to uninstall manually
    # cppcheck is already installed: Need to uninstall manually
    if installed_version:
        print(f"## {installed_version} is alreay installed!")
        print(f"Please to remove cppcheck before this install")
        #remove_cppcheck()
        if current_platform == 'posix': # Linux/MacOS
            output = run_command("which cppcheck")
        else:
            print("Install Failed: Unsupported platform: {},only support for Linux".format(current_platform))
            #print("Install Failed: Unsupported platform: {},only support for Linux".format(current_platform))
            exit(1)
        if output:
            print(f"Another coocheck is found at {output}")
            print(f"Please remove it manually before you install this cppcheck-misra!After you remove, run this script again!")
            print(f"Insatall failed: Have another cppcheck installed!")
            exit(1)
    
    # extract cppcheck source code and compile it
    ensure_directory_exists(cppcheck_dir)
    if current_platform == "posix":
        print("## Compiling cppcheck ... it will take a while")
        tar_file = "cppcheck-2.16.0.tar.gz"
        try:
            # Extract the cppcheck then compile cppcheck
            shutil.unpack_archive(tar_file, extract_dir=".")
            #os.chdir("cppcheck-2.16.0")
            os.chdir("cppcheck-2.16.0")
            if current_platform == 'posix':
                make_tool = "make"
            run_command(f"{make_tool} clean")
            make_command = (
                f"{make_tool} "
                f"DESTDIR={cppcheck_dir} "
                f"FILESDIR=/cppcheck "  #install -d ${DESTDIR}${FILESDIR}
                f"PREFIX=/cppcheck " # BIN=$(DESTDIR)$(PREFIX)/bin
                f"CFGDIR=/cppcheck/bin/cfg/ "
                f"> /dev/null "
                f" install -j4"
            )
            # print(make_command)
            run_command(make_command)
            # run_command("make install")
            os.chdir("..")
            shutil.rmtree("cppcheck-2.16.0")
            #shutil.rmtree("cppcheck-2.16.0")
        except Exception as e:
            print(f"Compilation of cppcheck failed: {e}")
            exit(1)
    #将cfg目录移动到bin目录下，不知为何上面的配置CFGDIR没有起作用，所以这里手动移动
    cfg_path = os.path.join(cppcheck_dir, "cppcheck/cfg")
    shutil.move(cfg_path, os.path.join(cppcheck_dir, "cppcheck/bin/cfg"))

    print("## install dependencies")
    run_command("pip3 install pygments elementpath termcolor")
    


    # Modify misra configuration
    print("## Misra configing")
    place_holder = "REPLACE_ME"
    # misra.json
    misra_json_in = Path("conf/misra.json.in")
    misra_json_out = Path(os.path.join("misra", "misra.json"))
    with misra_json_in.open('r') as f:
        content = f.read().replace(place_holder, cppcheck_dir)
    with misra_json_out.open('w') as f:
        f.write(content)

    # # pre-commit-config
    # pre_commit_config_in = Path("conf/.pre-commit-config.yaml.in")
    # pre_commit_config_out = Path(os.path.join("misra", ".pre-commit-config.yaml"))
    # with pre_commit_config_in.open('r') as f:
    #     content = f.read().replace(place_holder, cppcheck_dir)
    #     content = content.replace("PYTHON_EXE", PYTHON_EXE)
    # with pre_commit_config_out.open('w') as f:
    #     f.write(content)


    # misra.sh
    misra_sh_in = Path("conf/misra.sh.in")
    misra_sh_out = Path(os.path.join("misra", "misra.sh"))
    with misra_sh_in.open('r') as f:
        content = f.read().replace(place_holder, cppcheck_dir)
        content = content.replace("PYTHON_EXE", PYTHON_EXE)
    with misra_sh_out.open('w') as f:
        f.write(content)

    # 拷贝misra到安装目录下
    if os.path.exists(os.path.join(cppcheck_dir,"cppcheck/misra")):
        shutil.rmtree(os.path.join(cppcheck_dir,"cppcheck/misra"))
    shutil.copytree("misra", os.path.join(cppcheck_dir,"cppcheck/misra"), dirs_exist_ok=True)
    
    # Ensure that cppcheck/bin is in the PATH
    bin_dir = os.path.join(cppcheck_dir, "cppcheck/bin")
    # os.chmod(os.path.join(cppcheck_dir, "cppcheck/misra/.pre-commit-config.yaml"), 0o755)
    os.chmod(os.path.join(cppcheck_dir, "cppcheck/misra/misra.sh"), 0o755)
    os.symlink(os.path.join(cppcheck_dir,"cppcheck/misra/misra.sh"), os.path.join(bin_dir, "misra"))
    
    # 复制 full_check.sh / full_check.bat 到cppcheck_dir/misra文件夹中
    full_check_in = Path("conf/full_check.sh.in")
    full_check_out = Path(os.path.join(cppcheck_dir, "cppcheck", "misra", "full_check.sh"))
    shutil.copy(full_check_in, full_check_out)
    # 复制 incre_check.sh / incre_check.bat 到cppcheck_dir/misra文件夹中
    incre_check_in = Path("conf/incre_check.sh.in")
    incre_check_out = Path(os.path.join(cppcheck_dir, "cppcheck", "misra", "incre_check.sh"))
    shutil.copy(incre_check_in, incre_check_out)

    print(f"!!! Please add {bin_dir} to your PATH !!!")
    print("## Cppcheck-Misra Install Successful!")

if __name__ == "__main__":
    
    # checkPythonVersion()
    main()