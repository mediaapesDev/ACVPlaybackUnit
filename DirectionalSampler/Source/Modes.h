//This is where all the mode data is stored.

#pragma once

namespace Modes
{
//Gain, Pitch, Lpf, Float
constexpr float mod[72] = {1.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 1.0f, 0.0f,
                            1.0f, 0.0f, 1.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f,
    
                            1.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 1.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 1.0f, 0.0f,
                            1.0f, 1.0f, 0.0f, 0.0f,
                            1.0f, 0.0f, 1.0f, 0.0f,
                            0.0f, 1.0f, 1.0f, 0.0f,
                            1.0f, 1.0f, 1.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f,
    
                            1.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 1.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 1.0f, 0.0f,
                            1.0f, 1.0f, 0.0f, 0.0f,
                            1.0f, 0.0f, 1.0f, 0.0f,
                            0.0f, 1.0f, 1.0f, 0.0f,
                            1.0f, 1.0f, 1.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f};

}
