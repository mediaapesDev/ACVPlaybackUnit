# IEM Plug-in Suite

## Overview

The IEM Plug-in Suite is a free and Open-Source audio plug-in suite including Ambisonic plug-ins up to 7th order created by staff and students of the Institute of Electronic Music and Acoustics.

The suite provides plug-ins for a full Ambisonic production: encoders, reverbs, dynamics including limiter and multi-band compression, rotators, and decoders for both headphones and arbitrary loudspeaker layouts, and many more. The plug-ins are created with the [JUCE framework](https://juce.com) and can be compiled to any major plug-in format (VST, VST3, LV2, AU, AAX).

All the plug-ins can be built as standalones, e.g. for use with JACK or virtual soundcards.

For more information, installation guides and plug-in descriptions see:

- Website: <https://plugins.iem.at>
- Repository: <https://git.iem.at/audioplugins/IEMPluginSuite/>

## Compilation Guide

The IEM Plug-in Suite can be built using [CMake](https://cmake.org) (see commands below) and already comes with the JUCE dependency as a git submodule. The general system requirements are listed in the [JUCE Repository](https://github.com/juce-framework/JUCE/blob/8.0.7/README.md#minimum-system-requirements).

### Dependencies on Linux

Before compiling on Linux, some dependencies must be met. For Ubuntu (and most likely the majority of Debian-based systems), those are listed in the [JUCE docs](https://github.com/juce-framework/JUCE/blob/8.0.7/docs/Linux%20Dependencies.md). Alternatively, if [source code repositories](https://askubuntu.com/questions/158871/how-do-i-enable-the-source-code-repositories) are enabled, you can get them with

```sh
apt-get build-dep iem-plugin-suite
```

On RMP-based systems (e.g. Fedora), dependencies are installed with

```sh
dnf install alsa-lib-devel fftw-devel findutils freetype-devel gcc-c++  \
             libX11-devel libXcursor-devel curl-devel libXinerama-devel \
             libXrandr-devel libglvnd-devel make \
             pipewire-jack-audio-connection-kit-devel pkgconf-pkg-config \
             unzip util-linux which xorg-x11-server-Xvfb
```

Please note, that we are testing the plug-ins only on Debian and cannot guarantee operation on other distributions.

### FFTW Dependency on Windows and Linux

The **Binaural**Decoder plug-ins uses fast convolution for decoding the Ambisonic signals to binaural ear signals. Fast convolution relies on the Discrete Fourier Transform (DFT) to make the convolution more efficient. There are several DFT engines around, Apple comes with VDSP, and also JUCE comes with one, which should be considered as a fallback as it's not very optimized. For the remaining platforms Windows and Linux the [fftw3 library](http://fftw.org) is recommended. On Linux, you can simply install the `libfftw3-dev` package (for the static library) and you should be all set.

On Windows, you will need to download the source code from <http://fftw.org/download.html> (e.g. `fftw-3.3.10.tar.gz`) and unpack it into a `fftw` directory in the root of this repository, so that the FFTW's `CMakeLists.txt` is placed into `<directoryofthisreadme>/fftw/CMakeLists.txt`. That way, the IEM Plug-in Suite CMake project can find it!

### Select Plug-ins

Take a look at `CMakeLists.txt`, in the top you can select which plug-ins you want to build. Either leave it unchanged to build all of them, or comment non-wanted out.

### Formats

Per default, the plug-ins will be built as standalones only. Additionally, you can build VST2, VST3, LV2, and AAX versions of the plug-ins. Either change the `IEM_BUILD_VST2`, `IEM_BUILD_VST3`, `IEM_BUILD_LV2`, or `IEM_BUILD_AAX` options directly in the `CMakeLists.txt`, or use cmake command line flags to change them:

```sh
cmake .. -DIEM_BUILD_VST2=ON -DIEM_BUILD_VST3=ON -DIEM_BUILD_LV2=ON -DIEM_BUILD_STANDALONE=ON
```

However, there are some drawbacks with the LV2 versions (see below). For use in your DAW, we highly recommend the precompiled VST2's or VST3's.

#### VST2 versions

That's how it all began, however, time has passed and now you'll need to get hold of the VST2SDK to build the VST2 version of the suite. To let the build system know where to find the SDK, use the `VST2SDKPATH` variable:

```sh
cmake .. -DIEM_BUILD_VST2=ON -DVST2SDKPATH="pathtothesdk"
```

#### AAX versions

AAX is a proprietary format used by Avid for their ProTools DAW. The necessary SDK to build AAX plug-ins is supplied since JUCE 8. However, plug-ins in ProTools need to be verified and signed, for which you need a developer account with Avid. More information can be found [here](https://github.com/juce-framework/JUCE/blob/8.0.7/README.md#aax-plug-ins). However, you can install pre-compiled and signed AAX plug-ins ready to use with the installer from the [IEM Plug-in Suite website](https://plugins.iem.at/download).

#### Standalone versions

If you want to use the plug-ins outside a plug-in host, standalone versions can become quite handy! With them enabled, executables will be built which can be used with virtual soundcards of even JACK.

In case you don't want the plug-ins with JACK support, simply deactivate it: `-DIEM_STANDALONE_JACK_SUPPORT=OFF`. JACK is only supported on macOS and Linux.

#### Build them!

Okay, okay, enough with all those options, you came here to built, right?

Start up your shell and use the following:

```sh
mkdir build  # creates a build folder, recommended!
cd build     # enter it

# execute cmake and let it look for the project in the parent folder
# feel free to add those optiones from above
# for using an IDE use cmake' s -G flag like -G Xcode
cmake ..

# use "--config Release" for optimized release builds
cmake --build . --config Release # build the plug-ins / standalones
# alternatively, open the xcode project, msvc solution, or whatever floats your development boat
```

## Known issues

### LV2

At the moment, it's not possible with in LV2 format to get the currently available channel count. Therefore, the automatic Ambisonic order selection does not work, defaulting to 7th order regardless of the selected layout.

So in order to use the plug-ins correctly, you'll have to set the order manually in each plugin. This is missing feature in the LV2 standard and therefore out of our control. We already opened an [issue/feature request](https://gitlab.com/lv2/lv2/-/issues/63), but can't say if and when this will be supported.

There was a bug with LV2 plug-ins in REAPER on macOS, which causes the DAW to crash when the plug-in is removed. This issue is fixed with REAPER 6.74.

## Related repositories

- <https://git.iem.at/pd/vstplugin/releases>: to use the plug-ins in PD and SuperCollider
- <https://git.iem.at/ressi/iempluginosc>: to control the plug-ins using the OSC interface but without an OSC connection
