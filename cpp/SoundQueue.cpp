//
// Created by cjf12 on 2019-10-22.
//

#include "include/Log.h"
#include "include/SoundQueue.h"

SoundQueue::SoundQueue() :
        mPlayerObj(NULL),
        mPlayer(NULL),
        mPlayerQueue() {
}

status SoundQueue::initialize(SLEngineItf pEngine, SLObjectItf pOutputMixObj) {
    Log::info("Starting sound player.");
    SLresult result;

    //Set-up sound audio source.
    SLDataLocator_AndroidSimpleBufferQueue dataLocatorIn;
    dataLocatorIn.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;

    //At most one buffer in the queue.
    dataLocatorIn.numBuffers = 1;

    SLDataFormat_PCM dataFormat;
    dataFormat.formatType = SL_DATAFORMAT_PCM; //The format type, which must always be SL_DATAFORMAT_PCM for this structure.
    dataFormat.numChannels = 1; //Numbers of audio channels present in the data. Multi-channel audio is always interleaved in the data buffer.
    dataFormat.samplesPerSec = SL_SAMPLINGRATE_44_1; //The audio sample rate of the data in milliHertz
    dataFormat.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16; //Number of actual data bits in a sample. If bitsPerSample is equal to 8 then the data’s representation is
    dataFormat.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16; //The container size for PCM data in bits, for example 24 bit data in a 32 bit container. Data is left-justified within the container. For best performance, it is recommended that the container size be the size of the native data types.
    dataFormat.channelMask = SL_SPEAKER_FRONT_CENTER; //Channel mask indicating mapping of audio channels to speaker location.
    // The channelMask member specifies which channels are present in the
    // multichannel stream. The least significant bit corresponds to the front
    // left speaker (SL_SPEAKER_FRONT_LEFT), the next least significant bit
    // corresponds to the front right speaker (SL_SPEAKER_FRONT_RIGHT),
    // and so on. The number of bits set in channelMask should be
    //the same as the number of channels specified in numChannels with the
    //caveat that a default setting of zero indicates stereo format
    dataFormat.endianness = SL_BYTEORDER_LITTLEENDIAN; //Endianness of the audio data

    SLDataSource dataSource;
    dataSource.pLocator = &dataLocatorIn;
    dataSource.pFormat = &dataFormat;

    SLDataLocator_OutputMix dataLocatorOut;
    dataLocatorOut.locatorType = SL_DATALOCATOR_OUTPUTMIX; //Locator type, which must be SL_DATALOCATOR_OUTPUTMIX for this structure
    dataLocatorOut.outputMix = pOutputMixObj; //The OutputMix object as retrieved from the engine.

    SLDataSink dataSink;
    dataSink.pLocator = &dataLocatorOut;
    dataSink.pFormat = NULL;

    const SLuint32 soundPlayerIIDCount = 2;
    const SLInterfaceID soundPlayerIIDs[] = {SL_IID_PLAY, SL_IID_BUFFERQUEUE};
    const SLboolean soundPlayerRegs[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    result = (*pEngine)->CreateAudioPlayer(pEngine, &mPlayerObj, &dataSource, &dataSink,
                                           soundPlayerIIDCount, soundPlayerIIDs, soundPlayerRegs); //Create a player object, require the player interface, and buffer queue interface.
    if (result != SL_RESULT_SUCCESS) goto ERROR;
    result = (*mPlayerObj)->Realize(mPlayerObj, SL_BOOLEAN_FALSE); //allocate resources for the object
    if (result != SL_RESULT_SUCCESS) goto ERROR;
    result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_PLAY, &mPlayer); //get the player interface
    if (result != SL_RESULT_SUCCESS) goto ERROR;
    result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_BUFFERQUEUE, &mPlayerQueue); //get the buffer queue interface
    if (result != SL_RESULT_SUCCESS) goto ERROR;
    result = (*mPlayer)->SetPlayState(mPlayer, SL_PLAYSTATE_PLAYING); // Requests a transition of the player into the given play state.
    if (result != SL_RESULT_SUCCESS) goto ERROR;
    return STATUS_OK;

    ERROR:
    Log::error("Error while starting Sound queue");
    return STATUS_KO;
}

void SoundQueue::finalize() {
    Log::info("Stopping SoundQueue");
    if (mPlayerObj != NULL) {
        (*mPlayerObj)->Destroy(mPlayerObj);
        mPlayerObj = NULL;
        mPlayer = NULL;
        mPlayerQueue = NULL;
    }

}

void SoundQueue::playSound(Sound *pSound) {
    SLresult result;
    SLuint32 playerState;
    (*mPlayerObj)->GetState(mPlayerObj, &playerState);
    if (playerState == SL_OBJECT_STATE_REALIZED) {
        int16_t *buffer = (int16_t *) pSound->getBuffer();
        off_t length = pSound->getLength();

        //Removes any sound from the queue.
        result = (*mPlayerQueue)->Clear(mPlayerQueue);
        if (result != SL_RESULT_SUCCESS) goto ERROR;
        //Plays the new sound.
        result = (*mPlayerQueue)->Enqueue(mPlayerQueue, buffer, length); //enqueue a buffer to be played.
        if (result != SL_RESULT_SUCCESS) goto ERROR;
    }
    return;

    ERROR:
    Log::error("Error trying to play sound");
}
