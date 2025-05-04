# Deribit Trading Application

A C++ based trading application for the Deribit cryptocurrency derivatives exchange.

## Prerequisites

Before installing the trading application, ensure you have the following dependencies installed on your system:

```bash
sudo apt update && sudo apt install build-essential perl pkg-config libssl-dev cmake git unzip curl libwebsocketpp-dev zip
```

## Installation

1. Clone the repository:
```bash
cd ~
git clone https://github.com/iprime2/deribit_trading.git
cd deribit_trading
```

2. Install vcpkg and required dependencies:
```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg install nlohmann-json cpr Crow uwebsockets boost-system boost-thread boost-asio boost-beast openssl
./vcpkg integrate install
```

3. Set up environment variables:
```bash
export VCPKG_ROOT=~/deribit_trading/vcpkg
export CMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

4. Build the project:
```bash
cd ..
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE
make
```

## Running the Bot

After successful compilation, you can run the trading bot with:

```bash
./DeribitTrader
```

## Dependencies

The project uses the following main dependencies:
- nlohmann-json: For JSON parsing and manipulation
- cpr: C++ Requests library for HTTP requests
- Crow: C++ microframework for web
- uwebsockets: WebSocket client/server library
- Boost libraries (system, thread, asio, beast)
- OpenSSL: For secure communications

## Project Structure

- `src/`: Contains the source code
- `include/`: Header files
- `build/`: Build directory (created during compilation)
- `vcpkg/`: Package manager for C++ dependencies

