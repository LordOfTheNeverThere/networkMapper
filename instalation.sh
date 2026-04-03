# Clone the repository
git clone https://github.com/LordOfTheNeverThere/networkMapper.git
cd networkMapper

# Create a build directory
mkdir build && cd build

# Configure the project (Release mode)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Compile
cmake --build .

#Intall in the system
cmake --install .
