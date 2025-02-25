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
    misra_path = os.path.abspath(os.path.dirname(__file__))
    cppcheck_path = os.path.abspath(os.path.dirname(misra_path))
    #print("脚本文件所在路径:", cppcheck_dir_path)
    run_command("rm -rf " + cppcheck_path)

def uninstall():
    if os.name == "nt":
        print("Windows平台下需要卸载cppcheck，并情空安装路径")
    print("## Start Uninstalling cppcheck-misra")

    remove_cppcheck()
    print("## End Uninstall")

if __name__ == "__main__":
    uninstall()


