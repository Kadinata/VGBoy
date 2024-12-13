#!/bin/bash
ceedling clean;
ceedling gcov:all;
# ceedling utils:gcov;
gcovr --html-nested ./build/artifacts/gcov/GcovCoverageResults.html