# Distributed Real-Time System on CAN, using FreeRTOS

ESP-IDF version: 5.2.1 \
Python version: 3.12.1 \
Recommended IDE: VS Code  

The `assets` folder contains the `.txt` files for the automatic code generation and the `.vscode` folder for the dependencies. DO NOT change these files. Any changes that need to be made to the code should be made in the final generated code. 

Example code for the two processors and two tasks are given in the `examples/two-processors-two-tasks` folder. That folder contains an example config.json file and the corresponding esp-idf projets.

The following steps should be used for running the code. 

## Setting up 
- Install Python version 3.12.1
- Install esp-idf version 5.2.1. Follow the instructions here: [ESP-IDF Installation Guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/linux-macos-setup.html)

## Create esp-idf projects 
- Create as many new esp-idf projects as many processors you would like to use. Run this command for creating the project:
`idf.py create-new-project processor1` where, `processor1` is the name of the project. 
- Set the target to ESP32 in each project by running the command `idf.py set-target esp32` while creating each project. 

## Set up your esp-idf project
- If you are using VS Code (which is recommended for this project), you have to setup the sources by including the `.vscode` folder. Copy paste the `assets/.vscode` folder to each of your esp-idf project directories and change the following lines in the `settings.json` file to reflect your local paths:

```json
"idf.customExtraPaths": "/home/sohamc1909/.espressif/tools/xtensa-esp-elf-gdb/12.1_20231023/xtensa-esp-elf-gdb/bin:/home/sohamc1909/.espressif/tools/riscv32-esp-elf-gdb/12.1_20231023/riscv32-esp-elf-gdb/bin:/home/sohamc1909/.espressif/tools/xtensa-esp-elf/esp-13.2.0_20230928/xtensa-esp-elf/bin:/home/sohamc1909/.espressif/tools/riscv32-esp-elf/esp-13.2.0_20230928/riscv32-esp-elf/bin:/home/sohamc1909/.espressif/tools/esp32ulp-elf/2.35_20220830/esp32ulp-elf/bin:/home/sohamc1909/.espressif/tools/openocd-esp32/v0.12.0-esp32-20230921/openocd-esp32/bin:/home/sohamc1909/.espressif/tools/esp-rom-elfs/20230320",
"idf.customExtraVars": {
    "OPENOCD_SCRIPTS": "/home/sohamc1909/.espressif/tools/openocd-esp32/v0.12.0-esp32-20230921/openocd-esp32/share/openocd/scripts",
    "ESP_ROM_ELF_DIR": "/home/sohamc1909/.espressif/tools/esp-rom-elfs/20230320/"
},
"idf.espIdfPath": "/home/sohamc1909/esp/esp-idf",
"idf.pythonBinPath": "/home/sohamc1909/.espressif/python_env/idf5.2_py3.11_env/bin/python",
"idf.toolsPath": "/home/sohamc1909/.espressif",
```

## Input system configuration 
- Create a `config.json` file in the main project directory. An example is provided in the `examples/two-processors-two-tasks` folder. 
- Fill the `config.json` file with the configuration and tasks for your system. 

## Generate code 
- Run the python script using the command `python code_gen.py`. It takes the config.json file and generates C code for each processor in files called `processor#.c`, where # is the processor number. 
- Copy paste that code into the `.c` file in the `main` directory of the respective esp-idf project folder. 

## Building and flashing code 
- Make sure the ESP32 is hooked up to the computer. 
- Check the port of the esp32 by runnning the command `ls dev/tty*`
- Run the command `idf.py build` to build the projects 
- Run the command `idf.py -p <PORT_NAME> flash` to flash the code into the coressponding ESP32. 
