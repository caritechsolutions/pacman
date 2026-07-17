#!/bin/python3

import argparse
import ast
import os
import re
import shutil
import subprocess
import sys

import yaml

script_path = os.path.dirname(os.path.abspath(__file__))
code_path = os.path.join(script_path, "../../../")
platform_path = os.path.join(code_path, "platform/")
buildroot_path = os.path.join(code_path, "buildroot/")
solution_path = os.path.join(code_path, "solution/")
gxtest_path = os.path.join(code_path, "gxtest/")
configs_directory = os.path.join(gxtest_path, "goxceed_conf/configs/")
modified_files_list_file = os.path.join(code_path, "modified_files_list_file")

project_paths = {
    "bsp": os.path.join(platform_path, "bsp/"),
    "ecos3.0": os.path.join(platform_path, "ecos3.0/"),
    "gxapi": os.path.join(platform_path, "gxapi/"),
    "gxavdev": os.path.join(platform_path, "gxavdev/"),
    "gxbus": os.path.join(platform_path, "gxbus/"),
    "gxcas": os.path.join(platform_path, "gxcas/"),
    "gxdownloader": os.path.join(platform_path, "gxdownloader/"),
    "gxfrontend": os.path.join(platform_path, "gxfrontend/"),
    "gxloader": os.path.join(platform_path, "gxloader/"),
    "gxlowpower": os.path.join(platform_path, "gxlowpower/"),
    "gxmisc": os.path.join(platform_path, "gxmisc/"),
    "gxosal": os.path.join(platform_path, "gxosal/"),
    "gxscpu": os.path.join(platform_path, "gxscpu/"),
    "gxsecure": os.path.join(platform_path, "gxsecure/"),
    "gxtools": os.path.join(platform_path, "gxtools/"),
    "optee": os.path.join(platform_path, "optee/"),
    "pctools": os.path.join(platform_path, "pctools/"),
    "pdmx": os.path.join(platform_path, "pdmx/"),
    "thirdparty": os.path.join(platform_path, "thirdparty/"),
    "buildroot": buildroot_path,
    "linux-4.9.y": os.path.join(buildroot_path, "kernel/4.9.y/"),
    "solution": solution_path,
    "gxtest": gxtest_path,
}


def list_files_in_directory():
    file_list = [
        file_name
        for file_name in os.listdir(configs_directory)
        if not file_name.startswith(".")
    ]
    sorted_file_list = sorted(file_list)

    for file in sorted_file_list:
        print(os.path.splitext(file)[0])


def parse_config(config_file):
    config_info_dict = dict.fromkeys(
        [
            "test_project",
            "arch",
            "chip",
            "board",
            "os",
            "flash_type",
            "frontend",
            "case",
        ],
    )
    flash_type_mapping = {"spinand": "spi_nand", "parallelnand": "parallel_nand"}

    config_file_name = os.path.basename(config_file)
    config_file_name_without_suffix = os.path.splitext(config_file_name)[0]
    if "@" in config_file_name_without_suffix:
        compile_info = config_file_name_without_suffix.split("@", len(config_info_dict) - 1)
    else:
        compile_info = config_file_name_without_suffix.split("_", len(config_info_dict) - 1)

    for key, value in zip(config_info_dict.keys(), compile_info):
        config_info_dict[key] = value

    config_info_dict["flash_type"] = flash_type_mapping.get(
        config_info_dict["flash_type"], config_info_dict["flash_type"]
    )

    with open(config_file, "r", encoding="utf-8") as file:
        config_info_dict.update(yaml.safe_load(file))

    return config_info_dict


def is_config_exsist(file_path, config_key):
    with open(file_path, "r") as file:
        for line in file:
            if not line.strip().startswith("#"):  # 检查是否为非注释行
                if re.search(f"{config_key}\s*=", line):  # 使用正则表达式匹配
                    return True
    return False


def modify_code(yaml_data):
    if "modify_info" in yaml_data:
        for modify_info in yaml_data["modify_info"]:
            file_path = project_paths.get(modify_info["project"]) + modify_info["file"]
            bak_file_path = file_path + "_backup"
            print(file_path)
            print(bak_file_path)
            shutil.copy(file_path, bak_file_path)

            with open(modified_files_list_file, "a", encoding="utf-8") as f:
                f.write(str([file_path, bak_file_path]) + "\n")

            if isinstance(modify_info["content"], list):
                config_changes = modify_info["content"]
            else:
                config_changes = [modify_info["content"]]

            for change in config_changes:
                change_key, change_value = change.split("=")
                change_key = change_key.strip()
                change_value = change_value.strip()
                if is_config_exsist(file_path, change_key):
                    sed_command = f"sed -i '/^\s*#/!s/{change_key}\s*=.*/{change_key}={change_value}/g' {file_path}"  # /^\s*#/! 排除注释行
                    print(sed_command)
                    ret = subprocess.run(sed_command, shell=True)
                    if ret.returncode != 0:
                        return False
                else:
                    with open(file_path, "a", encoding="utf-8") as f:
                        print(f" add {change_key}={change_value} to file_path")
                        f.write(
                            f"\n{change_key}={change_value}\n"
                        )  # 前面加换行, 解决文件末尾没有换行符的特殊文件中追加的内容和原始内容并到一行的问题.
    return True


def modify_select_case(original_select_case, specific_select_case):
    # TODO 暂未实现部分替换的效果果，先使用完全替换
    return specific_select_case


def compile_code(config_info, select_case):
    env = os.environ.copy()

    brmode = config_info.get("compile_info", {}).get("brmode")
    if brmode:
        env["brmode"] = brmode  # 设置 brmode 环境变量
    optimize = config_info.get("compile_info", {}).get("optimize")
    if optimize:
        env["optimize"] = optimize  # 设置 optimize 环境变量
    loglevel = config_info.get("compile_info", {}).get("loglevel")
    if loglevel:
        env["loglevel"] = loglevel  # 设置 loglevel 环境变量

    if select_case:
        modified_select_case = modify_select_case(
            config_info["compile_info"]["select_case"], select_case
        )
    else:
        modified_select_case = config_info["compile_info"]["select_case"]

    module_compile_config = (
        f"{gxtest_path}pctools/jenkins/scripts/module/module_compile_config"
    )
    makesh = f"{platform_path}../make.sh"

    if (
        config_info["test_project"] == "gxtest-bus"
        # or config_info["test_project"] == "gxtest-api"
        or config_info["test_project"] == "gxtest-goxceed"
        or config_info["test_project"] == "gxtest-chiptest"
    ):
        command = [
            "bash",
            module_compile_config,
            "--submode",
            config_info["test_project"],
            "--code_path",
            code_path,
            "--chip",
            config_info["chip"],
            "--select_case",
            modified_select_case,
            "--os",
            config_info["os"],
            "--flash_type",
            config_info["flash_type"],
        ]
        ret = subprocess.run(command, check=False)
        if ret.returncode != 0:
            print(f"【!!!compiled failed】【gxtest_build.py: 189】Command: {' '.join(command)}")
            return ret.returncode

        if config_info["test_project"] == "gxtest-bus":
            command = [
                "bash",
                makesh,
                config_info["test_project"],
                config_info["arch"],
                config_info["chip"],
                config_info["board"],
                config_info["os"],
                config_info["frontend"],
                config_info["flash_type"],
                "bin",
            ]
            ret = subprocess.run( command, env=env, check=False)
            if ret.returncode != 0:
                print(f"【!!!compiled failed】【gxtest_build.py: 207】Command: {' '.join(command)}")
            return ret.returncode

        if (
            config_info["test_project"] == "gxtest-goxceed"
            or config_info["test_project"] == "gxtest-chiptest"
        ):
            command = [
                "bash",
                makesh,
                config_info["test_project"],
                config_info["arch"],
                config_info["os"],
                config_info["chip"],
                config_info["board"],
                config_info["frontend"],
                config_info["flash_type"],
                "bin",
            ]
            ret = subprocess.run(command, env=env, check=False)
            if "test_info" in config_info and "cmd" in config_info["test_info"]:
                cmd_file = f"{gxtest_path}/stb/goxceed/output/cmd.txt"
                dir_path = os.path.dirname(cmd_file)
                if not os.path.exists(dir_path):
                    os.makedirs(dir_path)
                with open(cmd_file, "w", encoding="utf-8") as fd:
                    fd.write(config_info["test_info"]["cmd"])
            if ret.returncode != 0:
                print(f"【!!!compiled failed】【gxtest_build.py: 235】Command: {' '.join(command)}")
            return ret.returncode

    elif config_info["test_project"] == "solution":
        command = [
                "bash",
                module_compile_config,
                "--submode",
                config_info["test_project"],
                "--code_path",
                code_path,
                "--chip",
                config_info["chip"],
                "--scene",
                modified_select_case,
                "--os",
                config_info["os"],
                "--board",
                config_info["board"],
        ]
        ret = subprocess.run(command, check=False)
        if ret.returncode != 0:
            print(f"【!!!compiled failed】【gxtest_build.py: 257】Command: {' '.join(command)}")
            return ret.returncode

        command = [
            "bash",
            makesh,
            config_info["test_project"],
            config_info["arch"],
            config_info["os"],
        ]
        ret = subprocess.run(command, env=env, check=False)
        if ret.returncode != 0:
            print(f"【!!!compiled failed】【gxtest_build.py: 269】Command: {' '.join(command)}")
        return ret.returncode

    return 1


def clean():
    if os.path.exists(modified_files_list_file):
        with open(modified_files_list_file, "r", encoding="utf-8") as f:
            modified_files_list = f.readlines()
            for file_list in modified_files_list:
                shutil.move(
                    ast.literal_eval(file_list)[1], ast.literal_eval(file_list)[0]
                )
        os.remove(modified_files_list_file)


# 创建解析器对象
parser = argparse.ArgumentParser(description="")

# 添加命令行参数
parser.add_argument("command", type=str, nargs="?")
group = parser.add_mutually_exclusive_group()
group.add_argument("--clean", action="store_true", help="恢复对代码的修改")
group.add_argument(
    "-lc", "--list_configs", action="store_true", help="列出可选的 configs"
)
group.add_argument("--config", help="根据指定的 CONFIG 修改代码并编译出 bin")
parser.add_argument(
    "-sc",
    "--select_case",
    help="根据指定内容修改 configs 中的 select_case 相关内容, 在 --config 参数填写的情况下才能填写",
)

# 解析命令行参数
args = parser.parse_args()

if args.select_case and not args.config:
    parser.error("--select_case 在 --config 参数填写的情况下才能填写")

if args.list_configs:
    list_files_in_directory()
elif args.config:
    clean()
    config_info = parse_config(os.path.join(configs_directory, args.config + ".yml"))
    print(config_info)
    modify_code(config_info)
    ret = compile_code(config_info, args.select_case)
    sys.exit(ret)
elif args.clean:
    clean()
elif args.command in ("help", None):
    parser.print_help()
