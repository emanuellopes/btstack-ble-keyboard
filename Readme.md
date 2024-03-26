# raspberry-pi-pico

# 1. Pico SDK

- Download Pico SDK using git

```bash
git clone git@github.com:raspberrypi/pico-sdk.git --recurse-submodules
```

- Set the variable in .bashrc or .zshrc, it depends of if you are using zsh or bash
```bash
export PICO_SDK_PATH="$HOME/Documents/raspberry_pi_pico_w/pico-sdk"
```

For Mac OS install the following libraries to compile the program

```bash
brew install cmake
brew tap ArmMbed/homebrew-formulae
brew install arm-none-eabi-gcc
```

# 2. Configure the visual studio
- Install the cmake tools extension https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools
- Install C/C++ extension https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools
- Install C/C++ Extension Pack extension https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack

# 3. Note
Open the each project in a separated vscode window to make sure vscode is grabbing the correct settings.

# 4. Build project
1. Create make files
cmake -B build -S .
1.1 Debug mode
cmake -DCMAKE_BUILD_TYPE=Debug -B Build -S .

2. Inside build folder use make to build .uf2 file
make -j4

# 5. Load program using picotool
[PicoTool](https://github.com/raspberrypi/picotool)

load program and run the code
In main function we should put the code stdio_init_all();
picotool load -v -x program.uf2 -f

# Configure Debug

1. Flash new app
sudo openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000" -c "program KeyboardHIDBTClassic.elf verify reset exit

2. enable debug
sudo openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000"