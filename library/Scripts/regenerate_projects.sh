#!/bin/bash

# Do not OpenSource.  The OpenSource packager will run this to generate
# Xcode projects and Makefiles.

SCRIPTS_DIR=$(dirname $0)
cd $SCRIPTS_DIR/.. || exit 1

../../../../googlemac/gyp_config/gyp_runner.py --osx -IDashToHls-Dev.gypi DashToHlsTools.gyp
../../../../googlemac/gyp_config/gyp_runner.py --ios -IDashToHls-Dev.gypi DashToHlsTools.gyp
