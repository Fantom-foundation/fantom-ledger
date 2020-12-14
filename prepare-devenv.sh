# shellcheck disable=SC1091,SC2155

# SOURCE THIS FILE
# . prepare-devenv blue|s|x

if [ $# -ne 1 ]; then
    echo "Possible options: blue, s or x"
    return
elif [[ $1 == "-h" ]]; then
    echo "Possible options: blue, s or x"
    return
elif [[ $1 != "blue" ]] && [[ $1 != "s" ]] && [[ $1 != "x" ]]; then
    echo "Possible options: blue, s or x"
    return
fi

if [[ $(dpkg-query -s python3-venv 2>&1) == *'is not installed'* ]]; then
    printf "\nPackage python3-venv is missing.\nOn Debian-like distros, run:\n\napt install python3-venv\n\n"
    return
fi

if [[ $(cat /etc/udev/rules.d/20-hw1.rules) == *'ATTRS{idVendor}=="2c97", ATTRS{idProduct}=="0004"'* ]]; then
    printf "\nMissing udev rules. Please refer to https://support.ledger.com/hc/en-us/articles/115005165269-Fix-connection-issues\n\n"
    return
fi

if [ ! -d dev-env ]; then
    # toolchain elements
    GCC_ARCHIVE="gcc-arm-none-eabi-5_3-2016q1-20160330-linux.tar.bz2"
    CLANG7_ARCHIVE="clang+llvm-7.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz"
    LEDGER_BLUE_SDK_ARCHIVE="blue-r21.1.tar.gz"
    LEDGER_NANOS_SDK_ARCHIVE="nanos-1612.tar.gz"

    # setup directories
    mkdir dev-env
    mkdir dev-env/SDK
    mkdir dev-env/CC
    mkdir dev-env/CC/others
    mkdir dev-env/CC/nanox

    # where to keep the toolchain
    if [ ! -d toolchain ]; then
       mkdir toolchain
    fi

    # prepare GCC
    echo "Preparing GCC ..."
    if [ ! -f "toolchain/${GCC_ARCHIVE}" ]; then
        wget "https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q1-update/+download/${GCC_ARCHIVE}" -O "toolchain/${GCC_ARCHIVE}"
    fi
    tar xf "toolchain/${GCC_ARCHIVE}" -C ./
    cp -r gcc-arm-none-eabi-5_3-2016q1 dev-env/CC/nanox/gcc-arm-none-eabi-5_3-2016q1
    mv gcc-arm-none-eabi-5_3-2016q1 dev-env/CC/others/gcc-arm-none-eabi-5_3-2016q1

    # clang 7.0
    echo "Preparing LLVM v7 ..."
    if [ ! -f "toolchain/${CLANG7_ARCHIVE}" ]; then
        wget "http://releases.llvm.org/7.0.0/${CLANG7_ARCHIVE}" -O "toolchain/${CLANG7_ARCHIVE}"
    fi
    tar xf "toolchain/${CLANG7_ARCHIVE}" -C ./
    cp -r "clang+llvm-7.0.0-x86_64-linux-gnu-ubuntu-16.04" dev-env/CC/others/clang-arm-fropi
    mv "clang+llvm-7.0.0-x86_64-linux-gnu-ubuntu-16.04" dev-env/CC/nanox/clang-arm-fropi

    # Blue SDK
    echo "Preparing Blue SDK ..."
    if [ ! -f "toolchain/${LEDGER_BLUE_SDK_ARCHIVE}" ]; then
        wget "https://github.com/LedgerHQ/blue-secure-sdk/archive/${LEDGER_BLUE_SDK_ARCHIVE}" -O "toolchain/${LEDGER_BLUE_SDK_ARCHIVE}"
    fi
    tar xf "toolchain/${LEDGER_BLUE_SDK_ARCHIVE}" -C ./
    mv blue-secure-sdk* dev-env/SDK/blue-secure-sdk

    # NanoS SDK
    echo "Preparing Nano-S SDK ..."
    if [ ! -f "toolchain/${LEDGER_NANOS_SDK_ARCHIVE}" ]; then
        wget "https://github.com/LedgerHQ/nanos-secure-sdk/archive/${LEDGER_NANOS_SDK_ARCHIVE}" -O "toolchain/${LEDGER_NANOS_SDK_ARCHIVE}"
    fi
    tar xf "toolchain/${LEDGER_NANOS_SDK_ARCHIVE}" -C ./
    mv nanos-secure-sdk* dev-env/SDK/nanos-secure-sdk

    # Python environment
    echo "Preparing Python environment ..."
    python3 -m venv dev-env/ledger_py3
    source dev-env/ledger_py3/bin/activate
    pip install wheel
    pip install ledgerblue
fi

source dev-env/ledger_py3/bin/activate

if [[ $1 == "blue" ]]; then
    export BOLOS_SDK=$(pwd)/dev-env/SDK/blue-secure-sdk
    export BOLOS_ENV=$(pwd)/dev-env/CC/others
elif [[ $1 == "s" ]]; then
    export BOLOS_SDK=$(pwd)/dev-env/SDK/nanos-secure-sdk
    export BOLOS_ENV=$(pwd)/dev-env/CC/others
elif [[ $1 == "x" ]]; then
    export BOLOS_SDK=$(pwd)/dev-env/SDK/nanox-secure-sdk
    export BOLOS_ENV=$(pwd)/dev-env/CC/nanox
fi
