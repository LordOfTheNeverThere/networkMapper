# Clone the repository
#git clone https://github.com/LordOfTheNeverThere/networkMapper.git
#cd networkMapper

# Create a build directory
mkdir build && cd build

# Configure the project (Release mode)
cmake .. -DCMAKE_BUILD_TYPE=Release
#Or comment the above and uncomment below if you want to install the app to a folder
#cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$HOME/Some/Folder"

# Compile
cmake --build .

#Intall in the system
sudo cmake --install .
#Or comment the above and uncomment below if you want to install the app to a folder
#cmake --install .