# networkMapper

A high-performance network mapping and diagnostic tool built in C++17. This project utilizes the `socks` library (https://github.com/LordOfTheNeverThere/socks) for low-level packet manipulation and provides detailed insights into network topology. With a LAN and WAN mapping capabilities using ARP and PING, respectively. As well as tracerouting capabilities using an incremental TTL field on the IP Header.

## 🗺️️ Prerequisites

Before building the project, ensure you have the following installed:

* **CMake** (v3.28 or higher)
* **C++ Compiler** (supporting C++17)
* **Git**

---

## ⚒️ Installation

Follow these steps to clone, build, and install **networkMapper**.

### 1. Clone the Repository
```bash
git clone https://github.com/LordOfTheNeverThere/networkMapper.git
cd networkMapper
```

### 2. Build

```bash
# Create a build directory
mkdir build && cd build

# Option A: Standard Configuration (Prepares for system-wide installation)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Option B: Local Configuration (Uncomment to install to a specific folder)
# cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$HOME/MyApps/networkMapper"

# Install for Option A:
sudo cmake --install .

# Install for Option B:
#cmake --install .
```

## Using it
*TODO*


## 📄 License

This project is licensed under the MIT License.
