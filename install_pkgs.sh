#!/bin/sh

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "Detected Linux"
    #sudo apt-get update
    sudo apt-get install -y libmagic-dev libssl-dev zlib1g-dev cmake clang
elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Detected MacOS"
    #/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    brew install libmagic openssl@3 zlib cmake clang
else
    echo "Unknown OS"
    exit 1
fi
