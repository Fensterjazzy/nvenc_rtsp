#pragma once

#include <NvPipe.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <unistd.h>
#include <cuda_runtime_api.h>
#include <opencv2/opencv.hpp>

/* **********************************************************************************
#  																					#
# Copyright (c) 2019,															    #
# Research group MITI		                                                        #
# Technical University of Munich													#
# 																					#
# All rights reserved.																#
# Kevin Yu - kevin.yu@tum.de													    #
# 																					#
# Redistribution and use in source and binary forms, with or without				#
# modification, are restricted to the following conditions:							#
# 																					#
#  * The software is permitted to be used internally only by the research group     #
#    MITI and CAMPAR and any associated/collaborating groups and/or individuals.	#
#  * The software is provided for your internal use only and you may				#
#    not sell, rent, lease or sublicense the software to any other entity			#
#    without specific prior written permission.										#
#    You acknowledge that the software in source form remains a confidential		#
#    trade secret of the research group MITI and therefore you agree not to 		#
#    attempt to reverse-engineer, decompile, disassemble, or otherwise develop 		#
#    source code for the software or knowingly allow others to do so.				#
#  * Redistributions of source code must retain the above copyright notice,			#
#    this list of conditions and the following disclaimer.							#
#  * Redistributions in binary form must reproduce the above copyright notice,		#
#    this list of conditions and the following disclaimer in the documentation		#
#    and/or other materials provided with the distribution.							#
#  * Neither the name of the research group MITI nor the names of its       		#
#    contributors may be used to endorse or promote products derived from this 		#
#    software without specific prior written permission.							#
# 																					#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND	#
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED		#
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE			#
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR	#
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES	#
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;		#
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND		#
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT		#
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS		#
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.						#
# 																					#
*************************************************************************************/


// If defined, time will be displayed
//#define DISPPIPETIME


namespace nvenc_rtsp
{

#define CWIDTH 1920
#define CHEIGHT 1440
#define CBYTESPERPIXEL 4
//#define CNVFORMAT NVPIPE_RGBA32
#define CNVFORMAT NVPIPE_BGRA32

#define DWIDTH 640
#define DHEIGHT 480
#define DBYTESPERPIXEL 2
#define DNVFORMAT NVPIPE_UINT16

#define CODEC NVPIPE_H264

    enum PurposeID
    {
        Video = 0,
        Depth
    };

    inline std::string PurposeString(PurposeID id)
    {
        switch (id)
        {
        case Video:
            return "Video";
        case Depth:
            return "Depth";
        default:
            return "Unknown";
        }
    }

    class Timer
    {
    public:
        Timer()
        {
            this->time = std::chrono::high_resolution_clock::now();
        }

        void reset()
        {
            this->time  = std::chrono::high_resolution_clock::now();
        }

        double getElapsedSeconds() const
        {
            return 1.0e-6 * std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - this->time).count();
        }

        double getElapsedMilliseconds() const
        {
            return 1.0e-3 * std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - this->time).count();
        }

    private:
        std::chrono::high_resolution_clock::time_point time;
    };
}

