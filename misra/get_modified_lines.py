# 执行git diff命令
# 如果下一行开头是diff，执行解析函数

# 解析函数的逻辑：
# 依次读取每一行
# 第一行以diff --git开头，采用split函数，取最后一项为文件名
# 第二行进行split分割，若以index开头，说明此文件为已有文件。若以new开头，说明是新文件。新文件的第三行仍是index，跳过不处理。

# 跳过所有以+ - 开头的行
# 对以@开头的行，解析这几部分 "@@ -69,0 +70 @@"，得到+后面的内容，70说明改动发生在70行。如果该数字后面还有,和数字，则说明改动的行数为70开始的n行。
import os
import subprocess
from pre_commit import get_git_root_dir
class GitDiffInfo:
    def __init__(self, filename):
        self.filename = filename
        self.is_new_file = False
        self.changes = []

def parse_git_diff(diff_output):
    lines = diff_output.splitlines()
    # GitDiffInfo列表
    files = []
    info = None

    for i, line in enumerate(lines):
        if line.startswith("diff --git"):
            # 新文件的开始，创建新的 GitDiffInfo 对象
            if info:
                files.append(info)
            parts = line.split()
            full_path = parts[-1]
            # 提取第一个 / 后的内容
            filename = full_path.partition('/')[2]
            info = GitDiffInfo(filename)
            info.is_new_file = False
        elif line.startswith("index"):
            continue
        elif line.startswith("new file mode"):
            info.is_new_file = True
            continue
        elif line.startswith("@@"):
            parts = line.split()
            range_info = parts[2].split(',')
            start_line = int(range_info[0][1:])
            if len(range_info) > 1:
                num_lines = int(range_info[1])
            else:
                num_lines = 1
            info.changes.append((start_line, num_lines))

    if info:
        files.append(info)

    return files

def execute_git_diff():
    root_dir = get_git_root_dir(os.getcwd())
    git_opt = f"--git-dir={os.path.join(root_dir, '.git')}"
    against = "HEAD"
    try:
        # git  --git-dir rev-parse --verify HEAD，用来验证 HEAD 是否存在，返回 0 表示存在，返回 1 表示不存在。"
        result = subprocess.run(f"git -C {root_dir} {git_opt} rev-parse --verify {against}", shell=True, capture_output=True, text=True)
        # result = subprocess.run(['git', 'diff', '-U0', 'HEAD'], capture_output=True, text=True, check=True)
        if result.returncode != 0:
            # 仓库没有初始化
            # 4b825dc642cb6eb9a060e54bf8d69288fbee4904 是一个特殊的 Git 空树对象的 SHA-1 哈希值。
            against = "4b825dc642cb6eb9a060e54bf8d69288fbee4904"
    except Exception as e:
        print("Error:", e)
        against = "4b825dc642cb6eb9a060e54bf8d69288fbee4904"
    git_command = ["git",
                   "diff",
                   "--cached",
                   "-U0",
                   against
                   ]
    # print("git_command: ", git_command)
    # result = subprocess.run(['git', 'diff', '--cached', '-U0', against], capture_output=True, text=True, check=True)
    result = subprocess.run(git_command, capture_output=True, text=True, check=True)
    
    return result.stdout

    # try:
    #     result = subprocess.run(['git', 'diff', '-U0', 'HEAD'], capture_output=True, text=True, check=True)
    #     return result.stdout
    # except subprocess.CalledProcessError as e:
    #     print(f"Error executing git diff: {e}")
    #     return ""

def get_modified_lines():
    """
    Description:
        获取项目改动的行号。返回GitDiffInfo类的列表
    Return:
        每个GitDiffInfo：一个修改的文件名以及修改点列表。
        修改点：由一对数字表示，第一个数表示开的行号start，第二个数字num表示共有几行。因此一堆数字表示的修改行数范围为[start, start+num-1]
    """
    diff_output = execute_git_diff()
    # print(diff_output)
    if not diff_output:
        print("No modified files found.")
        return None

    files = parse_git_diff(diff_output)
    return files

if __name__ == "__main__":
    files = get_modified_lines()
    if not files:
        exit(0)
    for info in files:
        print(f"文件名: {info.filename}")
        print(f"是否为新文件: {info.is_new_file}")
        print("改动行数:")
        for start_line, num_lines in info.changes:
            print(f"  从第 {start_line} 行开始，共 {num_lines} 行")
