import os
from pathlib import Path
import shutil
from  tools import *
misra_dir = os.path.dirname(__file__)
cppcheck_dir = os.path.dirname(misra_dir)
bin_dir = os.path.join(cppcheck_dir, "bin")
def remove_cppcheck():
    if not os.path.exists(cppcheck_dir):
        return
        
    if os.path.exists(cppcheck_dir):
        shutil.rmtree(cppcheck_dir)

    log_success(f"{cppcheck_dir} removed!.")
    log_success("Please remove the cppcheck from your PATH environment variable by yourself.")

def main():
    remove_cppcheck()

if __name__ == "__main__":
    main()
