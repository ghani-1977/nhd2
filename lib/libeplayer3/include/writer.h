#ifndef WRITER_H_
#define WRITER_H_

#include <stdio.h>

#ifndef __sh__
typedef enum {
	VIDEO_STREAMTYPE_MPEG2,
	VIDEO_STREAMTYPE_MPEG4_H264,
	VIDEO_STREAMTYPE_MPEG4_H263,
	VIDEO_STREAMTYPE_VC1,
	VIDEO_STREAMTYPE_MPEG4_Part2,
	VIDEO_STREAMTYPE_VC1_SM,
	VIDEO_STREAMTYPE_MPEG1,
	VIDEO_STREAMTYPE_DIVX311
}VIDEO_FORMAT;

typedef enum {
	AUDIO_STREAMTYPE_AC3 = 0,
	AUDIO_STREAMTYPE_MPEG,
	AUDIO_STREAMTYPE_DTS,
	AUDIO_STREAMTYPE_LPCMDVD = 6,
	AUDIO_STREAMTYPE_AAC = 8,
	AUDIO_STREAMTYPE_AACPLUS,
	AUDIO_STREAMTYPE_MP3
}AUDIO_FORMAT;
#endif

typedef enum { 
	eNone, 
	eAudio, 
	eVideo, 
	eGfx
} eWriterType_t;

typedef struct {
	int                    fd;
	unsigned char*         data;
	unsigned int           len;
	unsigned long long int Pts;
	unsigned char*         private_data;
	unsigned int           private_size;
	unsigned int           FrameRate;
	unsigned int           FrameScale;
	unsigned int           Width;
	unsigned int           Height;
	unsigned char          Version;
} WriterAVCallData_t;

typedef struct {
	unsigned char*         data;
	unsigned int           Width;
	unsigned int           Height;
	unsigned int           Stride;
	unsigned int           color;

	unsigned int           x;       /* dst x ->given by ass */
	unsigned int           y;       /* dst y ->given by ass */

	/* destination values if we use a shared framebuffer */
	int                    fd;
	unsigned int           Screen_Width;
	unsigned int           Screen_Height;
	unsigned char*         destination;
	unsigned int           destStride;
} WriterFBCallData_t;

typedef struct WriterCaps_s {
	char*          name;
	eWriterType_t  type;
	char*          textEncoding;
	/* fixme: revise if this is an enum! */
	int            dvbEncoding;
} WriterCaps_t;

typedef struct Writer_s {
	int           (* reset) ();
	int           (* writeData) (void*);
	int           (* writeReverseData) (void*);
	WriterCaps_t *caps;
} Writer_t;

// audio
extern Writer_t WriterAudioIPCM;
extern Writer_t WriterAudioMP3;
extern Writer_t WriterAudioMPEGL3;
extern Writer_t WriterAudioAC3;
extern Writer_t WriterAudioEAC3;
extern Writer_t WriterAudioAAC;
extern Writer_t WriterAudioDTS;
extern Writer_t WriterAudioWMA;
extern Writer_t WriterAudioFLAC;
extern Writer_t WriterAudioVORBIS;

// video
extern Writer_t WriterVideoMPEG2;
extern Writer_t WriterVideoMPEGH264;
extern Writer_t WriterVideoH264;
extern Writer_t WriterVideoWMV;
extern Writer_t WriterVideoDIVX;
extern Writer_t WriterVideoFOURCC;
extern Writer_t WriterVideoMSCOMP;
extern Writer_t WriterVideoH263;
extern Writer_t WriterVideoFLV;
extern Writer_t WriterVideoVC1;
extern Writer_t WriterFramebuffer;

static Writer_t * AvailableWriter[] = {
	&WriterAudioIPCM,
	&WriterAudioMP3,
	&WriterAudioMPEGL3,
	&WriterAudioAC3,
	&WriterAudioEAC3,
	&WriterAudioAAC,
	&WriterAudioDTS,
	&WriterAudioWMA,
	&WriterAudioFLAC,
	&WriterAudioVORBIS,

	&WriterVideoMPEG2,
	&WriterVideoMPEGH264,
	&WriterVideoH264,
	&WriterVideoDIVX,
	&WriterVideoFOURCC,
	&WriterVideoMSCOMP,
	&WriterVideoWMV,
	&WriterVideoH263,
	&WriterVideoFLV,
	&WriterVideoVC1,   
	&WriterFramebuffer,   
	NULL
};

Writer_t* getWriter(char* encoding);
Writer_t* getDefaultVideoWriter();
Writer_t* getDefaultAudioWriter();
Writer_t* getDefaultFramebufferWriter();

#endif
