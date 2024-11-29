"""
    uninstall the cppcheck-misra
    1. remove the cppcheck-misra from the system
    2. remove the pre-commit

"""

import os
from tools  import *
# 卸载pre-commit hook
def remove_pre_commit_hook():
    run_command("pre-commit clean")
    run_command("pre-commit uninstall")

# 删除cppcheck-misra
def remove_cppcheck():
    run_command("rm -rf ../../cppcheck")
    run_command("source ~/.bashrc")

def uninstall():
    log_success("## Start Uninstalling cppcheck-misra")
    remove_pre_commit_hook()
    remove_cppcheck()
    log_success("## End Uninstall Successfully")

if __name__ == "__main__":
    uninstall()


