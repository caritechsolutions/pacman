import os

import yaml


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
    compile_info = config_file_name_without_suffix.split("_", len(config_info_dict) - 1)

    for key, value in zip(config_info_dict.keys(), compile_info):
        config_info_dict[key] = value

    config_info_dict["flash_type"] = flash_type_mapping.get(
        config_info_dict["flash_type"], config_info_dict["flash_type"]
    )

    with open(config_file, "r") as file:
        config_info_dict.update(yaml.safe_load(file))

    return config_info_dict
