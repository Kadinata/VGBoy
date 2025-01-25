#!/bin/bash
TARGET=${1:-all}
echo "Running tests for $TARGET"

ceedling clean;
ceedling gcov:$TARGET;
# ceedling utils:gcov;
gcovr --html-nested ./build/artifacts/gcov/GcovCoverageResults.html