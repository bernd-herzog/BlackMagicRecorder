//============================================================================
// Name        : Stream.cpp
// Author      : Bernd Herzog
// Version     :
// Copyright   : All rights reserved
// Description : Hello World in C++, Ansi-style
//============================================================================

//#include <iostream>
//using namespace std;

#include <DeckLinkAPI.h>
//#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <DeckLinkAPI.h>
#include <libavutil/pixfmt.h>

extern "C"
{
#include <x264.h>
#include <libswscale/swscale.h>
}

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <time.h>

#include "DeckLinkCaptureDelegate.h"
#include "SafeQueue.h"

#define in_width 1920
#define in_height 1080

#define out_width 1280
#define out_height 720

//#define out_width in_width
//#define out_height in_height

struct SwsContext* convertCtx;
x264_t* encoder;

void frameLoop(int sock, FILE *outfile, FILE *rawOutfile, SafeQueue *sq);

int main(int argc, char *argv[])
{
	bool saveRaw = false;
	bool RawOnly = false;
	char *sOnly;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--help") == 0)
		{
			std::cout << "--raw Save RAW stream\n";
		}
		else if (strcmp(argv[i], "--raw") == 0)
		{
			saveRaw = true;
		}
		else if (strcmp(argv[i], "--rawonly") == 0)
		{
			RawOnly = true;
			sOnly = argv[++i];
		}
	}

	int sock = 0;
	if (!RawOnly)
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	time_t timeStamp;
	char outFileName[255];
	struct tm * timeInfo;

	time(&timeStamp);

	timeInfo = localtime(&timeStamp);

	FILE *outfile;
	if (RawOnly)
	{
		outfile = fopen(sOnly, "w");
		if (outfile == NULL)
		{
			perror("open");
			exit(0);

		}
	}
	else
	{
		strftime(outFileName, 255, "/mnt/riesending/home/wow/%Y-%m-%d_%H-%M-%S.h264", timeInfo);
		outfile = fopen(outFileName, "w");
		if (outfile == NULL)
		{
			perror("open");
			exit(0);

		}
	}

	FILE *rawOutfile = NULL;
	if (saveRaw)
	{
		strftime(outFileName, 255, "/mnt/riesending/home/wow/%Y-%m-%d_%H-%M-%S.raw", timeInfo);
		rawOutfile = fopen(outFileName, "w");
		if (rawOutfile == NULL)
		{
			perror("open");
			exit(0);

		}
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(2537);
	addr.sin_addr.s_addr = inet_addr("88.198.49.124");
	//addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (!RawOnly)
	{
		if (connect(sock, (sockaddr *) &addr, sizeof(addr)) != 0)
		{
			perror("connect to 88.198.49.124:2537 failed");
			return 0;
		}
	}

	IDeckLink *deckLink;
	HRESULT result;
	IDeckLinkIterator *deckLinkIterator = CreateDeckLinkIteratorInstance();

	SafeQueue *sq = new SafeQueue(24, in_width * in_height * 2);
	REFIID Iid =
	{ 0x6D, 0x40, 0xEF, 0x78, 0x28, 0xB9, 0x4E, 0x21, 0x99, 0x0D, 0x95, 0xBB, 0x77, 0x50, 0xA0, 0x4F };

	result = deckLinkIterator->Next(&deckLink);
	if (result != S_OK)
	{
		fprintf(stderr, "No DeckLink PCI cards found.\n");
		goto bail;
	}

	IDeckLinkInput *deckLinkInput;
	if (deckLink->QueryInterface(Iid, (void**) &deckLinkInput) != S_OK)
		goto bail;

	DeckLinkCaptureDelegate *dlcdelegate;
	dlcdelegate = new DeckLinkCaptureDelegate(sq);

	deckLinkInput->SetCallback(dlcdelegate);

	result = deckLinkInput->EnableVideoInput(bmdModeHD1080p24, bmdFormat8BitYUV, 0);
	if (result != S_OK)
	{
		fprintf(stderr, "Failed to enable video input. Is another application using the card?\n");
		goto bail;
	}

	/*
	 result = deckLinkInput->EnableAudioInput(bmdAudioSampleRate48kHz, g_audioSampleDepth, g_audioChannels);
	 if(result != S_OK)
	 {
	 goto bail;
	 }
	 */

	convertCtx = sws_getContext(in_width, in_height, PIX_FMT_UYVY422, out_width, out_height, PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);

	sws_init_context(convertCtx, NULL, NULL);

	x264_param_t param;
	x264_param_default_preset(&param, "veryfast", "zerolatency");
	// x264_param_default_preset(&param, "faster", "zerolatency");
	param.i_threads = 4;
	param.i_width = out_width;
	param.i_height = out_height;
	param.i_fps_num = 24;
	param.i_fps_den = 1;
//	param.i_frame_reference = 7;
	// Intra refres:
	param.i_keyint_max = 12;
	param.b_intra_refresh = 1;

	//Rate control:
	param.rc.i_rc_method = X264_RC_ABR;
	//	param.rc.f_rf_constant = 25;
	//	param.rc.f_rf_constant_max = 35;
	param.rc.i_bitrate = 2800;
	//param.rc.i_vbv_max_bitrate = 4000;
	//For streaming:
	param.b_repeat_headers = 1;
	param.b_annexb = 1;
	param.b_aud = 1;


	x264_param_apply_profile(&param, "baseline");

	encoder = x264_encoder_open(&param);

	result = deckLinkInput->StartStreams();
	if (result != S_OK)
	{
		goto bail;
	}

	frameLoop(sock, outfile, rawOutfile, sq);

	bail:

	if (deckLinkInput != NULL)
	{
		deckLinkInput->Release();
		deckLinkInput = NULL;
	}

	if (deckLink != NULL)
	{
		deckLink->Release();
		deckLink = NULL;
	}

	if (deckLinkIterator != NULL)
		deckLinkIterator->Release();

	return 0;
}

void frameLoop(int sock, FILE *outfile, FILE *rawOutfile, SafeQueue *sq)
{
	while (true)
	{
		uint8_t * buf = (uint8_t *) sq->Dequeue();
		if (rawOutfile != NULL)
		{
			write(rawOutfile->_fileno, buf, in_width * in_height * 2);
		}

		int srcstride = in_width * 2; //RGB stride is just 3*width
		x264_picture_t pic_out, pic_in;

		x264_picture_alloc(&pic_in, X264_CSP_I420, out_width, out_height);

		sws_scale(convertCtx, &buf, &srcstride, 0, in_height, pic_in.img.plane, pic_in.img.i_stride);

		x264_nal_t* nals;
		int i_nals;

		int frame_size = x264_encoder_encode(encoder, &nals, &i_nals, &pic_in, &pic_out);

		x264_picture_clean(&pic_in);

		uint8_t *plData = nals->p_payload;

		if (sock > 0)
		{
			uint8_t *p = plData;
			int max_len = 50000;
			do
			{
				int send_len = (frame_size - (p - plData)) > max_len ? max_len : (frame_size - (p - plData));
				int len = send(sock, p, send_len, 0);
				if (len != send_len)
				{
					perror("send");
					exit(0);
				}
				p += len;
			} while (p < plData + frame_size);
		}
		write(outfile->_fileno, plData, frame_size);
	}
}
