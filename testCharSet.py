import subprocess
import chardet

git_command = ["git", "status"]

# 捕获二进制输出
result = subprocess.run(git_command, capture_output=True, check=True)
raw_output = result.stdout

# 使用 chardet 自动检测编码
detected = chardet.detect(raw_output)
encoding = detected["encoding"] or "utf-8"  # 默认回退到 UTF-8
print("编码格式" ,encoding)
# 尝试解码
try:
    decoded_output = raw_output.decode(encoding)
except UnicodeDecodeError:
    decoded_output = raw_output.decode(encoding, errors="replace")  # 替换非法字符
    print(f"解码时遇到非法字符，已替换。检测到的编码: {encoding}")

print(decoded_output)

print("----------------------")
git_command = [ "git", "diff", "--name-only", "HEAD^", "HEAD"]
subprocess.run(git_command)