"""
    uninstall the cppcheck-misra
    1. remove the cppcheck-misra from the system
    2. remove the pre-commit

"""

import os
from tools  import *
import sys
# 卸载pre-commit hook
def remove_pre_commit_hook():
    run_command("pre-commit clean")
    run_command("pre-commit uninstall")

# 删除cppcheck-misra
def remove_cppcheck():
    script_path = sys.argv[0]
    abs_script_path = os.path.abspath(script_path)
    dir_path = os.path.dirname(abs_script_path)
    cppcheck_dir_path = os.path.dirname(dir_path)
    #print("脚本文件所在路径:", cppcheck_dir_path)
    run_command("rm -rf " + cppcheck_dir_path)
    #run_command("source ~/.bashrc")

def uninstall():
    if os.name == "nt":
        log_error("Uninstall Failed: Uninstall by [misra --uninstall_misra] is not supported at Windows.\n\rPlease use [cppcheck-2.16.0-x64-Setup.msi] to uninstall it manually!")
        exit(1)
    log_success("## Start Uninstalling cppcheck-misra")
    remove_pre_commit_hook()
    remove_cppcheck()
    log_success("## End Uninstall")

if __name__ == "__main__":
    uninstall()


