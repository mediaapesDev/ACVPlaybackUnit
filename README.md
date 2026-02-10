# ACV Playback Unit

## Dependencies
#### Compiler
```
sudo apt update
sudo apt install clang
```
or
```
sudo apt update
sudo apt install g++
```

#### Jack Audio
```
sudo apt update
sudo apt install libasound2-dev libjack-jackd2-dev \
    ladspa-sdk \
    libcurl4-openssl-dev  \
    libfreetype-dev libfontconfig1-dev \
    libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
    libwebkit2gtk-4.1-dev \
    libglu1-mesa-dev mesa-common-dev
```

#### FFTW3
```
sudo apt update
sudo apt install libfftw3-dev
``` 
    
#### JUCE
...included as submodule.



## Compilation
```
git clone --recurse-submodules https://github.com/mediaapesDev/ACVPlaybackUnit.git
```

#### in .../path/to/ACVPlaybackUnit
```
mkdir build
cd build
cmake..
cmake --build . --config Release
```
