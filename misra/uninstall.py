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
        
    # 删除环境变量
    if os.name == "posix":
        variable_name = "PATH={bin_dir}"
        bashrc_path = os.path.expanduser("~/.bashrc")
        try:
            with open(bashrc_path, 'r') as file:
                lines = file.readlines()
            with open(bashrc_path, 'w') as file:
                for line in lines:
                    if not line.startswith(f"export {variable_name}"):
                        file.write(line)
            run_command("source ~/.bashrc")
        except FileNotFoundError:
            print("~/.bashrc don't exist")
    elif os.name == 'nt':
        import winreg
        # 打开Windows注册表
        reg = winreg.ConnectRegistry(None, winreg.HKEY_LOCAL_MACHINE)

        # 打开环境变量键
        key = winreg.OpenKey(reg, r'SYSTEM/CurrentControlSet/Control/Session Manager/Environment', 0, winreg.KEY_ALL_ACCESS)

        # 获取当前环境变量值
        value, type = winreg.QueryValueEx(key, 'Path')
        new_value = ""
        # 将路径添加到环境变量中
        value_list = value.split(';')
        for ele in value_list:
            if ele != bin_dir:
                new_value += ele + ';'

        # 更新环境变量值
        winreg.SetValueEx(key, 'Path', 0, type, new_value)

        # 关闭键和注册表
        winreg.CloseKey(key)
        winreg.CloseKey(reg)
    if os.path.exists(cppcheck_dir):
        shutil.rmtree(cppcheck_dir)

def main():
    remove_cppcheck()

if __name__ == "__main__":
    main()
