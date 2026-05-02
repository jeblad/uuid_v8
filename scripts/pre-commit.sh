#!/bin/bash
set -e

# Get the project root directory
REPO_ROOT=$(git rev-parse --show-toplevel)
cd "$REPO_ROOT"

echo "Running pre-commit version sync and tests..."

# 1. Ensure build directory exists and is configured
cmake -B build -S .

# 2. Build the test executable
cmake --build build --target test_uuid_v8

# 3. Run tests to ensure the versioning and code are valid
ctest --test-dir build --output-on-failure

# 4. Stage the updated header only if all checks passed
git add include/uuid_v8/uuid_v8.hpp

echo "Pre-commit checks passed!"
exit 0
