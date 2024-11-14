cmake_minimum_required(VERSION 3.30)
project(Encryption)

set(CMAKE_CXX_STANDARD 20)

# Find OpenSSL package (make sure it's installed)
find_package(OpenSSL REQUIRED)

# Add the executable
add_executable(Encryption main.c encryption.c)

# Include directories for OpenSSL and your header files
target_include_directories(Encryption PRIVATE ${OPENSSL_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})

# Link OpenSSL libraries (ssl and crypto)
target_link_libraries(Encryption PRIVATE OpenSSL::SSL OpenSSL::Crypto)
