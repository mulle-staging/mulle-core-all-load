#!/bin/bash
set -e

# Get repo URL and reference from arguments
REPO_URL="${1:-https://github.com/mulle-core/mulle-core-all-load.git}"
REPO_REF="${2:-}"

TEMP_DIR=$(mktemp -d)
cd "$TEMP_DIR"

echo "Creating test project in $TEMP_DIR"
echo "Using repository: $REPO_URL"
if [ -n "$REPO_REF" ]; then
    echo "Using reference: $REPO_REF"
fi

# Initialize git repo
git init
git config user.email "test@example.com"
git config user.name "Test User"
git config protocol.file.allow always

# Add mulle-core-all-load as actual submodule
git submodule add "$REPO_URL" mulle-core-all-load
cd mulle-core-all-load
if [ -n "$REPO_REF" ]; then
    git checkout "$REPO_REF"
fi
cd ..
git submodule update --init --recursive

# Create CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.15)
project(submodule-test)
add_subdirectory(mulle-core-all-load/mulle-core)
add_subdirectory(mulle-core-all-load)
add_executable(test main.c)
target_link_libraries(test PRIVATE mulle-core-all-load)
EOF

# Create main.c
cat > main.c << 'EOF'
#include <mulle-core-all-load/mulle-core-all-load.h>

int main() {
    // TODO: do something
    mulle_fprintf(stdout, "mulle-core-all-load submodule test: %s\n", "SUCCESS");


    return 0;
}
EOF

# Build and test
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/test

echo "Test completed successfully"
cd - > /dev/null
rm -rf "$TEMP_DIR"
