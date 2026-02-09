#!/bin/bash

# Exit on any error
set -e

# URL to download
VERSION=3.3.10
URL="https://www.fftw.org/fftw-$VERSION.tar.gz"
TAR_FILE="fftw-$VERSION.tar.gz"
TARGET_DIR="fftw"

# Download the file
echo "Downloading $URL..."
curl -LO "$URL"

mkdir -p "$TARGET_DIR"

# Extract the tar.gz file
echo "Extracting $TAR_FILE..."
tar -xzf "$TAR_FILE" --strip-components=1 --directory "$TARGET_DIR"
rm "$TAR_FILE"
