#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <math.h>
#include <random>
#include "sharedmem.h"

#define N_IN_BUFFERS 2
#define N_OUT_BUFFERS 4

using namespace std;

#define ASSERT_EQ(x, y) do { if ((x) == (y)) ; else { fprintf(stderr, "0x%x != 0x%x\n", \
    (unsigned) (x), (unsigned) (y)); assert((x) == (y)); } } while (0)
// default values


static SLuint32 channels = 1;       // -c#

static SLuint32 outBufferCnt=0;
static SLuint32 inBufferCnt=0;
static SLuint32 bufSizeInBytes = 0; // calculated
// Storage area for the buffer queues
static short **outBuffers;
static short **inBuffers;
static short *pPlaying;
static short *pProcessing;
static short *pCapturing;
static short *sineSignal;

SLEngineItf engineEngine;
SLObjectItf engineObject;
SLObjectItf outputmixObject;
SLDataSource audiosrc;
    SLDataSink audiosnk;
    SLDataFormat_PCM pcm;
    SLDataLocator_OutputMix locator_outputmix;
    SLDataLocator_BufferQueue locator_bufferqueue_tx;
SLObjectItf playerObject = NULL;
    SLObjectItf recorderObject = NULL;
    SLInterfaceID ids_tx[2] = {SL_IID_BUFFERQUEUE,SL_IID_ANDROIDCONFIGURATION};
    SLboolean flags_tx[1] = {SL_BOOLEAN_TRUE};
 SLDataLocator_IODevice locator_iodevice;
 SLAndroidConfigurationItf recorderConfig,playerConfig;
    SLDataLocator_AndroidSimpleBufferQueue locator_bufferqueue_rx;
SLInterfaceID ids_rx[2] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE,SL_IID_ANDROIDCONFIGURATION};
    SLboolean flags_rx[1] = {SL_BOOLEAN_TRUE};

static SLAndroidSimpleBufferQueueItf recorderBufferQueue;
static SLBufferQueueItf playerBufferQueue;


void (*sdCallBack)(short *)=NULL;
void getSineSignal(short *x,double freq,double fs,int nSamples);
void addNoise(short *x,double noisePower,int nSamples);


// Called after audio recorder fills a buffer with data
static void recorderCallback(SLAndroidSimpleBufferQueueItf caller, void *context)
{
    SLresult result;

    if(readInputSource()==0){

    pProcessing=inBuffers[inBufferCnt];
    inBufferCnt=(inBufferCnt+1)%N_IN_BUFFERS;
  }else{
      getSineSignal(sineSignal,readSineFrequency(),readSamplingRate(),bufSizeInBytes/sizeof(short));
      pProcessing=sineSignal;

  }

  if(readNoiseFlag()==1.0)
     addNoise(pProcessing,readNoisePower(),bufSizeInBytes/sizeof(short));


    writeInputSignal(pProcessing,bufSizeInBytes/sizeof(short));






    if(sdCallBack)
	sdCallBack(pProcessing);





}

// Enable audio
void enableAudio(int bufSizeInFrames,int sampleRate)
{
    SLresult result;

    // compute buffer size
    bufSizeInBytes = channels * bufSizeInFrames * sizeof(short);

    outBuffers=(short**)malloc(N_OUT_BUFFERS*sizeof(short *));
    for(int i=0;i<N_OUT_BUFFERS;i++)
	outBuffers[i]=(short*)malloc(bufSizeInBytes);

   inBuffers=(short**)malloc(N_IN_BUFFERS*sizeof(short *));
    for(int i=0;i<N_IN_BUFFERS;i++)
	inBuffers[i]=(short*)malloc(bufSizeInBytes);

  sineSignal=(short*)malloc(bufSizeInBytes);


writeSamplingRate((double)sampleRate);

    // create engine

    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    ASSERT_EQ(SL_RESULT_SUCCESS, result);
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    ASSERT_EQ(SL_RESULT_SUCCESS, result);

    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    ASSERT_EQ(SL_RESULT_SUCCESS, result);
    // create output mix
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputmixObject, 0, NULL, NULL);
    ASSERT_EQ(SL_RESULT_SUCCESS, result);
    result = (*outputmixObject)->Realize(outputmixObject, SL_BOOLEAN_FALSE);
    ASSERT_EQ(SL_RESULT_SUCCESS, result);
    // create an audio player with buffer queue source and output mix sink
    locator_bufferqueue_tx.locatorType = SL_DATALOCATOR_BUFFERQUEUE;
    locator_bufferqueue_tx.numBuffers = N_OUT_BUFFERS;
    locator_outputmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    locator_outputmix.outputMix = outputmixObject;
    pcm.formatType = SL_DATAFORMAT_PCM;
    pcm.numChannels = channels;
    pcm.samplesPerSec = sampleRate * 1000;
    pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    pcm.containerSize = 16;
    pcm.channelMask = channels == 1 ? SL_SPEAKER_FRONT_CENTER :
        (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT);
    pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
    audiosrc.pLocator = &locator_bufferqueue_tx;
    audiosrc.pFormat = &pcm;
    audiosnk.pLocator = &locator_outputmix;
    audiosnk.pFormat = NULL;

    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &playerObject, &audiosrc, &audiosnk,
        2, ids_tx, flags_tx);
    if (SL_RESULT_CONTENT_UNSUPPORTED == result) {
        fprintf(stderr, "Could not create audio player (result %lx), check sample rate\n", result);
        return;
    }


    result = (*playerObject)->GetInterface(playerObject, SL_IID_ANDROIDCONFIGURATION,
                                &playerConfig);
    ASSERT_EQ(SL_RESULT_SUCCESS, result);

    SLuint32 presetValue = SL_ANDROID_STREAM_MEDIA;
    (*playerConfig)->SetConfiguration(playerConfig, SL_ANDROID_KEY_STREAM_TYPE,
                           &presetValue, sizeof(SLuint32));




    result = (*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE);
    ASSERT_EQ(SL_RESULT_SUCCESS, result);
    SLPlayItf playerPlay;
    result = (*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playerPlay);
    ASSERT_EQ(SL_RESULT_SUCCESS, result);
    result = (*playerObject)->GetInterface(playerObject, SL_IID_BUFFERQUEUE, &playerBufferQueue);
    ASSERT_EQ(SL_RESULT_SUCCESS, result);


    result = (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_PLAYING);
    ASSERT_EQ(SL_RESULT_SUCCESS, result);
    // Create an audio recorder with microphone device source and buffer queue sink.
    // The buffer queue as sink is an Android-specific extension.

    locator_iodevice.locatorType = SL_DATALOCATOR_IODEVICE;
    locator_iodevice.deviceType = SL_IODEVICE_AUDIOINPUT;
    locator_iodevice.deviceID = SL_DEFAULTDEVICEID_AUDIOINPUT;
    locator_iodevice.device = NULL;
    audiosrc.pLocator = &locator_iodevice;
    audiosrc.pFormat = &pcm;
    locator_bufferqueue_rx.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    locator_bufferqueue_rx.numBuffers = N_IN_BUFFERS;
    audiosnk.pLocator = &locator_bufferqueue_rx;
    audiosnk.pFormat = &pcm;


    result = (*engineEngine)->CreateAudioRecorder(engineEngine, &recorderObject, &audiosrc,
        &audiosnk, 2, ids_rx, flags_rx);
    if (SL_RESULT_SUCCESS != result) {
        fprintf(stderr, "Could not create audio recorder (result %lx), "
                "check sample rate and channel count\n", result);
        return;
    }

    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDCONFIGURATION,
                                &recorderConfig);
    ASSERT_EQ(SL_RESULT_SUCCESS, result);

     presetValue = SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION;
    (*recorderConfig)->SetConfiguration(recorderConfig, SL_ANDROID_KEY_RECORDING_PRESET,
                           &presetValue, sizeof(SLuint32));


    result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE);
    ASSERT_EQ(SL_RESULT_SUCCESS, result);
    SLRecordItf recorderRecord;
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecord);
    ASSERT_EQ(SL_RESULT_SUCCESS, result);
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
        &recorderBufferQueue);
    ASSERT_EQ(SL_RESULT_SUCCESS, result);





    result = (*recorderBufferQueue)->RegisterCallback(recorderBufferQueue, recorderCallback, NULL);
    ASSERT_EQ(SL_RESULT_SUCCESS, result);
    // Kick off the recorder
    result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_RECORDING);
    ASSERT_EQ(SL_RESULT_SUCCESS, result);




}



void captureBlock(   void (*callback)(short *) ){
SLresult result;
	sdCallBack=callback;

pCapturing=inBuffers[inBufferCnt];

	//Enqueue
        result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue,
            pCapturing, bufSizeInBytes);
        ASSERT_EQ(SL_RESULT_SUCCESS, result);

}


void playBlock( short* pBuffer){

SLresult result;
 pPlaying=outBuffers[outBufferCnt];

memcpy(pPlaying,pBuffer, bufSizeInBytes);
writeOutputSignal(pBuffer,bufSizeInBytes/sizeof(short));

	// Enqueue the just-filled buffer for the player
    if(readInputSource()==0){
    result = (*playerBufferQueue)->Enqueue(playerBufferQueue, pPlaying, bufSizeInBytes);
    if (SL_RESULT_SUCCESS == result) {



outBufferCnt=(outBufferCnt+1)%N_OUT_BUFFERS;
    } else {
        // Here if record has a filled buffer to play, but play queue is full.
        assert(SL_RESULT_BUFFER_INSUFFICIENT == result);
        //write(1, "?", 1);

    }
  }


}

void getSineSignal(short *x,double freq,double fs,int nSamples){
  static double angle=0;

  double Om=2*M_PI*freq/fs;

  for(int n=0;n<nSamples;n++){
    x[n]=16384*sin(angle);
    angle=fmod(angle+Om,2*M_PI);
  }



}

void addNoise(short *x,double noisePower,int nSamples){
  // construct a trivial random generator engine from a time-based seed:
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator (seed);


  double stdDev= sqrt(pow(10.0,(noisePower/10.0)));
   std::normal_distribution<double> distribution (0.0,stdDev);

  for(int n=0;n<nSamples;n++)
   x[n]=(x[n]+32767.0*distribution(generator));
}




void println(string text){
  cout<<text<<endl;
}
void println(int number){
    cout<<number<<endl;
}
void println(short number){
cout<<number<<endl;
}
void println(double number){
  cout<<number<<endl;
}
void println(float number){
    cout<<number<<endl;
}
void println(const char *format, ...){
  va_list args;
  va_start (args, format);
  vprintf (format, args);
  va_end (args);

  cout<<endl;
}
