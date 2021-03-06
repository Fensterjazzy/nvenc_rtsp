#pragma once

#include "nvenc_rtsp_common.h"
#include "Decoder.h"
#include "RtspPlayer.h"
#include <thread>
#include <functional>
#include <queue>
#include <tuple>
#include <mutex>
#include <condition_variable>

#include "nvenc_rtsp_config.h"

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

namespace nvenc_rtsp
{
	class NVENCRTSP_EXPORT ClientPipeRTSP : public Decoder
	{
	public:
		ClientPipeRTSP(std::string rtspAddress, NvPipe_Format decFormat, NvPipe_Codec codec, RecvCallFn recv_cb = NULL);
		ClientPipeRTSP(std::string rtspAddress, NvPipe_Format decFormat, RecvCallFn recv_cb = NULL);

		virtual void cleanUp() override;

	private:
		int cvtBuffer(uint8_t *buf, ssize_t bufsize, uint8_t *outBuf, ssize_t *outLength);

		const short m_maxStoredFrames = 2;

		uint8_t m_currentFrameCounter = 0;
		bool m_pkgCorrupted = false;
		int m_currentOffset = 0;
		uint32_t m_currentTimestamp = 0;

		RK::RtspPlayer::Ptr m_player;

		bool m_runProcess = true;
		std::unique_ptr<std::thread> m_decodeThread;
		std::queue<std::tuple<uint8_t*, size_t, uint32_t>> m_decodeQueue;
		std::mutex m_decodeMutex;
		std::condition_variable m_decodeCV;

		std::unique_ptr<std::thread> m_processThread;
		std::queue<std::tuple<cv::Mat, uint32_t>> m_processQueue;
		std::mutex m_processMutex;
		std::condition_variable m_processCV;

		std::string m_rtspAddress;
	};
}
