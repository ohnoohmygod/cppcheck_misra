import sys
import os
import subprocess

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
    
def commit_msg():
    args = sys.argv[1:]
    if len(args) == 0:
        print("Can not find edit message file!")
        sys.exit(1)
    # 传入的参数是 commit-editmsg的路径
    COMMIT_EDITMSG = args[0]
    
    project_root = get_git_root_dir(os.getcwd())
    cppcheck_flag_path = os.path.join(project_root, 'cppcheck_results/flag.txt')
    cppcheck_title = '[CPPCHECK] '
    # 追加写入是否经过cppcheck检查，依据为是否有flag.txt文件
    with open(COMMIT_EDITMSG, 'a') as f:
        f.write('\n')
        content = ''
        try:
            with open(cppcheck_flag_path, 'r') as sum_f:
                content = "!!! CHECKED !!!"
        except FileNotFoundError:
            content = "!!! NO CHECK !!!"

        f.write(cppcheck_title)
        f.write(content)
        f.write('\n')
    return 0
if __name__ == '__main__':
    
    sys.exit(commit_msg())


            


    