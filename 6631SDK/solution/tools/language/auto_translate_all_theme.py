#!/usr/bin/env python3
import os
import sys
import subprocess

def run_auto_translate(directory, input_string):
    """
    遍历指定目录下的所有子目录，如果子目录包含名为 "language" 的子目录，
    则将 "language" 目录的路径和用户提供的字符串作为参数传递给 autoTranslate.py 脚本。
    """
    for root, dirs, files in os.walk(directory):
        if 'language' in dirs:
            language_dir = os.path.join(root, 'language')
            print(f"找到 language 目录: {language_dir}")
            try:
                # 调用 autoTranslate.py 脚本
                script_dir = os.path.dirname(os.path.abspath(__file__))
                script_path = os.path.join(script_dir, 'autoTranslate.py')
                subprocess.run(['python3', script_path, language_dir, input_string], check=True)
            except subprocess.CalledProcessError as e:
                print(f"执行 autoTranslate.py 失败: {e}")
            except FileNotFoundError:
                print("找不到 autoTranslate.py 脚本，请确认脚本路径是否正确。")

def read_strings_from_file(file_path):
    """
    从指定的文件中读取字符串，每行一个词条。
    """
    try:
        with open(file_path, 'r', encoding='utf-8') as file:
            return [line.strip() for line in file if line.strip()]
    except FileNotFoundError:
        print(f"找不到文件: {file_path}")
        sys.exit(1)
    except Exception as e:
        print(f"读取文件时发生错误: {e}")
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("用法: python3 auto_translate_all_theme.py <目录路径> -e <字符串> 或 python3 auto_translate_all_theme.py <目录路径> -f <文件名>")
        sys.exit(1)

    directory = sys.argv[1]
    option = sys.argv[2]

    if option == '-e' and len(sys.argv) == 4:
        input_string = sys.argv[3]
        run_auto_translate(directory, input_string)
    elif option == '-f' and len(sys.argv) == 4:
        file_path = sys.argv[3]
        strings = read_strings_from_file(file_path)
        for input_string in strings:
            run_auto_translate(directory, input_string)
    else:
        print("用法: python3 auto_translate_all_theme.py <目录路径> -e <字符串> ")
        print("    : python3 auto_translate_all_theme.py <目录路径> -f <文件名> ")
        sys.exit(1)

