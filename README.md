# Voidchain

A blockchain project is under construction 🏗 🚧

## Locally Running

Install vcpkg first, $VCPKG_ROOT env var is essential for running this project:

```bash
#imediately after cloning the repo
cd voidchain # or maybe your home dir
git clone https://github.com/microsoft/vcpkg.git # about 100 MBs
./vcpkg/bootstrap-vcpkg.sh # bootstrap-vcpkg.bat for windows

# define env vars for vcpkg (important for cmakePresets)
export VCPKG_ROOT=$(pwd)/vcpkg
export PATH=$VCPKG_ROOT:$PATH
```

run the build script:

```bash
# The run the script
cd voidchain # the repo dir
chmod +x ./run.sh
./run.sh
```
