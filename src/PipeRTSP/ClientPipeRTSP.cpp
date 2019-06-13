#include "nvenc_rtsp_common.h"
#include "nvenc_rtsp/ClientPipeRTSP.h"

/* **********************************************************************************
#                                                                                   #
# Copyright (c) 2019,                                                               #
# Research group MITI                                                               #
# Technical University of Munich                                                    #
#                                                                                   #
# All rights reserved.                                                              #
# Kevin Yu - kevin.yu@tum.de                                                        #
#                                                                                   #
# Redistribution and use in source and binary forms, with or without                #
# modification, are restricted to the following conditions:                         #
#                                                                                   #
#  * The software is permitted to be used internally only by the research group     #
#    MITI and CAMPAR and any associated/collaborating groups and/or individuals.    #
#  * The software is provided for your internal use only and you may                #
#    not sell, rent, lease or sublicense the software to any other entity           #
#    without specific prior written permission.                                     #
#    You acknowledge that the software in source form remains a confidential        #
#    trade secret of the research group MITI and therefore you agree not to         #
#    attempt to reverse-engineer, decompile, disassemble, or otherwise develop      #
#    source code for the software or knowingly allow others to do so.               #
#  * Redistributions of source code must retain the above copyright notice,         #
#    this list of conditions and the following disclaimer.                          #
#  * Redistributions in binary form must reproduce the above copyright notice,      #
#    this list of conditions and the following disclaimer in the documentation      #
#    and/or other materials provided with the distribution.                         #
#  * Neither the name of the research group MITI nor the names of its               #
#    contributors may be used to endorse or promote products derived from this      #
#    software without specific prior written permission.                            #
#                                                                                   #
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND   #
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED     #
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE            #
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR   #
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    #
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;      #
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND       #
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT        #
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS     #
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                      #
#                                                                                   #
*************************************************************************************/

using namespace std;
using namespace nvenc_rtsp;

#define RTP_OFFSET (12)
#define FU_OFFSET (14)

ClientPipeRTSP::ClientPipeRTSP(std::string _rtspAddress, int _width, int _height, int _bytesPerPixel, PurposeID _purpose, NvPipe_Format _decFormat, NvPipe_Codec _codec, RecvCallFn _recv_cb)
    : Decoder(_width, _height, _bytesPerPixel, _purpose, _decFormat, _codec, _recv_cb),
      m_rtspAddress(_rtspAddress)
{
    m_player = std::make_shared<RK::RtspPlayer>(
        [=](uint8_t *buffer, ssize_t bufferLength) {
            ssize_t myLength;

            uint8_t frameCounter = buffer[3];
            if (frameCounter != m_currentFrameCounter + 1)
                m_pkgCorrupted = true;

            uint32_t timestamp = (buffer[4] << 24) |(buffer[5] << 16) |(buffer[6] << 8) | buffer[7];
            
            int type = cvtBuffer(buffer, bufferLength, &m_frameBuffer.data()[m_currentOffset], &myLength);

            m_currentFrameCounter = frameCounter;

            m_currentOffset += myLength;

            // find new NAL package.
            if (!(bufferLength > 35 && bufferLength < 40 && type == 0))
                return;

            if (m_pkgCorrupted)
            {
                m_currentOffset = 0;
                m_pkgCorrupted = false;
                return;
            }

            m_timer.reset();

            uint64_t size = NvPipe_Decode(m_decoder, m_frameBuffer.data(), m_currentOffset, m_gpuDevice, m_width, m_height);
            double decodeMs = m_timer.getElapsedMilliseconds();

            m_currentOffset = 0;
            m_pkgCorrupted = false;

            if (size == 0)
                return;

            //Retrieve from GPU
            m_timer.reset();
            cv::Mat outMat;
            switch (m_purpose)
            {
                case Video:
                {
                    outMat = cv::Mat(cv::Size(m_width, m_height), CV_8UC4);
                    cudaMemcpy(outMat.data, m_gpuDevice, m_dataSize, cudaMemcpyDeviceToHost);
                    break;
                }
                case Depth:
                {
                    outMat = cv::Mat(cv::Size(m_width, m_height), CV_16UC1);
                    cudaMemcpy(outMat.data, m_gpuDevice, m_dataSize, cudaMemcpyDeviceToHost);
                    break;
                }
            }

            double downloadMs = m_timer.getElapsedMilliseconds();

#ifdef DISPPIPETIME
            std::cout << size << " " << std::setw(11) << decodeMs << std::setw(11) << downloadMs << std::endl;
#endif
            if (m_recv_cb != NULL)
                m_recv_cb(outMat, timestamp);
        }, PurposeString(m_purpose));

    m_player->Play(m_rtspAddress.c_str());

    //provide some time for the player to connect
    usleep(500000);
}

ClientPipeRTSP::ClientPipeRTSP(std::string _rtspAddress, int _width, int _height, int _bytesPerPixel, PurposeID _purpose, NvPipe_Format _decFormat, RecvCallFn _recv_cb)
: ClientPipeRTSP(_rtspAddress, _width, _height, _bytesPerPixel, _purpose, _decFormat, CODEC, _recv_cb)
{    
}

int ClientPipeRTSP::cvtBuffer(uint8_t *buf, ssize_t bufsize, uint8_t *outBuf, ssize_t *outLength)
{
    uint8_t header[] = {0, 0, 0, 1};
    struct RK::Nalu nalu = *(struct RK::Nalu *)(buf + RTP_OFFSET);
    if (nalu.type >= 0 && nalu.type < 24)
    { //one nalu
        *outLength = bufsize - RTP_OFFSET;
        memcpy(outBuf, buf + RTP_OFFSET, bufsize - RTP_OFFSET);
    }
    else if (nalu.type == 28)
    { //fu-a slice
        struct RK::FU fu;
        uint8_t in = buf[RTP_OFFSET + 1];
        fu.S = in >> 7;
        fu.E = (in >> 6) & 0x01;
        fu.R = (in >> 5) & 0x01;
        fu.type = in & 0x1f;
        if (fu.S == 1)
        {
            uint8_t naluType = nalu.forbidden_zero_bit << 7 | nalu.nal_ref_idc << 5 | fu.type;
            *outLength = 4 + 1 + bufsize - FU_OFFSET;
            memcpy(outBuf, header, 4);
            memcpy(&outBuf[4], &naluType, 1);
            memcpy(&outBuf[5], buf + FU_OFFSET, bufsize - FU_OFFSET);
        }
        else
        {
            *outLength = bufsize - FU_OFFSET;
            memcpy(outBuf, buf + FU_OFFSET, bufsize - FU_OFFSET);
        }
    }
    return nalu.type;
}

void ClientPipeRTSP::cleanUp()
{
    m_player->Stop();
    Decoder::cleanUp();
}
