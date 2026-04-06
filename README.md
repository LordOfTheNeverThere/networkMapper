# networkMapper

A multi-threaded network mapping and diagnostic tool built in C++17. This project utilizes the `socks` library (https://github.com/LordOfTheNeverThere/socks) for low-level packet manipulation and provides detailed insights into network topology. With a LAN and WAN mapping capabilities using ARP and PING, respectively. As well as tracerouting capabilities using an incremental TTL field on the IP Header.

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

# Build:
cmake --build .

# Install for Option A:
sudo cmake --install .

# Install for Option B:
#cmake --install .
```

## ✅ Using it
### Help Section:
```bash
Usage: /.../bin/networkMapper { -m | -t | -h } [ARGUMENTS...]

Options:
  -m, --map <IP> <NET>              Map an IPv4 address to a network address.
  -t, --trace [-c, --count COUNT] <IP>... 
                                    Perform a traceroute. COUNT sets max hops.
  -h, --help                        Display this help and exit.
```
### Examples:

* ```networkMapper -m 127.0.0.0 255.255.255.0```
* ```networkMapper -t 8.8.8.8 1.1.1.1 127.0.0.1```
* ```networkMapper -t -c 3 8.8.8.8 1.1.1.1 127.0.0.1```

### Valgrind reports

```
==44215== Memcheck, a memory error detector
==44215== Copyright (C) 2002-2022, and GNU GPL'd, by Julian Seward et al.
==44215== Using Valgrind-3.22.0 and LibVEX; rerun with -h for copyright info
==44215== Command: ./networkMapper -m 127.0.0.0 255.255.255.0
==44215== Parent PID: 44214
==44215== 
==44215== 
==44215== HEAP SUMMARY:
==44215==     in use at exit: 0 bytes in 0 blocks
==44215==   total heap usage: 1,644 allocs, 1,644 frees, 25,885,450 bytes allocated
==44215== 
==44215== All heap blocks were freed -- no leaks are possible
==44215== 
==44215== For lists of detected and suppressed errors, rerun with: -s
==44215== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

```
==44308== Memcheck, a memory error detector
==44308== Copyright (C) 2002-2022, and GNU GPL'd, by Julian Seward et al.
==44308== Using Valgrind-3.22.0 and LibVEX; rerun with -h for copyright info
==44308== Command: ./networkMapper -t 8.8.8.8 1.1.1.1 127.0.0.1
==44308== Parent PID: 44307
==44308== 
==44308== 
==44308== HEAP SUMMARY:
==44308==     in use at exit: 0 bytes in 0 blocks
==44308==   total heap usage: 403 allocs, 403 frees, 4,583,073 bytes allocated
==44308== 
==44308== All heap blocks were freed -- no leaks are possible
==44308== 
==44308== For lists of detected and suppressed errors, rerun with: -s
==44308== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

```
==44326== Memcheck, a memory error detector
==44326== Copyright (C) 2002-2022, and GNU GPL'd, by Julian Seward et al.
==44326== Using Valgrind-3.22.0 and LibVEX; rerun with -h for copyright info
==44326== Command: ./networkMapper -t -c 3 8.8.8.8 1.1.1.1 127.0.0.1
==44326== Parent PID: 44325
==44326== 
==44326== 
==44326== HEAP SUMMARY:
==44326==     in use at exit: 0 bytes in 0 blocks
==44326==   total heap usage: 244 allocs, 244 frees, 1,488,916 bytes allocated
==44326== 
==44326== All heap blocks were freed -- no leaks are possible
==44326== 
==44326== For lists of detected and suppressed errors, rerun with: -s
==44326== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

## 📄 License

This project is licensed under the MIT License.
