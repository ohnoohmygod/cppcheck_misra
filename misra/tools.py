import subprocess
from termcolor import colored
import yaml
def log_success(message):
    print(colored(message, 'green'))

def log_error(message):
    print(colored(message, 'red'))

def log_warning(message):
    print(colored(message, 'yellow'))

def run_command(command):
    try:
        result = subprocess.run(command, shell=True, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        return result.stdout
    except subprocess.CalledProcessError as e:
        log_error(f"Command '{command}' failed with error: {e}")
        return None
    
def get_git_root_dir(current_path):
    """
    current_path 为 执行git commit命令的路径
    返回项目的根目录，使用 命令git rev-parse --show-toplevel 获取
    """
    try:
        # 使用 subprocess.run 获取 git 根目录
        result = subprocess.run(
            ["git", "-C", current_path, "rev-parse", "--show-toplevel"],
            capture_output=True,
            text=True,
            check=True
        )
        root_dir = result.stdout.strip()
        return root_dir
    except subprocess.CalledProcessError:
        print("\033[31m Not a Git repository.\033[0m")
        return None
    except Exception as e:
        print("Error:", e)
        return None
def load_config(config_file = "config.yaml"):
    """
    加载配置文件，返回一个字典
    """
    with open(config_file, 'r') as f:
        config = yaml.safe_load(f)
    return config