#!/bin/sh

## script for creating the release binaries on
rm -rf build

rm -rf VST
rm -rf VST3
rm -rf LV2
rm -rf AAX
rm -rf Standalone

cmake -B build -DCMAKE_BUILD_TYPE=Release -DIEM_BUILD_VST3=ON -DIEM_BUILD_VST2=ON -DVST2SDKPATH=src/VST_SDK/VST2_SDK/ -DIEM_BUILD_LV2=ON -DIEM_BUILD_STANDALONE=ON -DIEM_BUILD_AAX=ON -DCMAKE_OSX_DEPLOYMENT_TARGET=10.11 -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release -- -j 8

# cd ..
mkdir VST
cp -r build/*/*_artefacts/Release/VST/*.vst VST/
mkdir VST3
cp -r build/*/*_artefacts/Release/VST3/*.vst3 VST3/
mkdir LV2
cp -r build/*/*_artefacts/Release/LV2/*.lv2 LV2/
mkdir AAX
cp -r build/*/*_artefacts/Release/AAX/*.aaxplugin AAX/
mkdir Standalone
cp -r build/*/*_artefacts/Release/Standalone/*.app Standalone/
