#include "GMAudioTranscode.h"
#include "ffmpeg_audio_transcode.h"


GMMediaTranscoder::GMMediaTranscoder()
: state_callback_(NULL)
, except_callback_(NULL)
{

}

GMMediaTranscoder::~GMMediaTranscoder()
{
	
}

void GMMediaTranscoder::SetupStateCallbackFunc(_GMAudioTranscodeStateCallback func)
{
	state_callback_ = func;
}

void GMMediaTranscoder::SetupExceptionCallbackFunc(_GMAudioTranscodeExceptionCallback func)
{
	except_callback_ = func;
}

int GMMediaTranscoder::Transcode(std::string source_audio, std::string target_audio)
{
	int errCode = 0;

	audio_transcoder_ = new FFmpegAudioTranscode();
	
	// 打开输入文件
	errCode = audio_transcoder_->open_input_file(source_audio.c_str());
    if (errCode != 0)
	{
		if(except_callback_) except_callback_(errCode, "Open input file failed...");
		delete audio_transcoder_;
        return errCode;
	}

    // 打开输出文件
	errCode = audio_transcoder_->open_output_file(target_audio.c_str());
    if (errCode != 0)
	{
		if(except_callback_) except_callback_(errCode, "Open output file failed...");
		delete audio_transcoder_;
		return errCode;
	}

    /** Initialize the resampler to be able to convert audio sample formats. */
	// 
	errCode = audio_transcoder_->init_resampler();
    if (errCode != 0)
	{
		if(except_callback_) except_callback_(errCode, "Init resampler failed...");
		delete audio_transcoder_;
		return errCode;
	}

    /** Initialize the FIFO buffer to store audio samples to be encoded. */
	errCode = audio_transcoder_->init_fifo();
    if (errCode != 0)
	{
		if(except_callback_) except_callback_(errCode, "Initialize the FIFO buffer failed...");
		delete audio_transcoder_;
		return errCode;
	}

    /** Write the header of the output file container. */
	errCode = audio_transcoder_->write_output_file_header();
    if (errCode != 0)
	{
		if(except_callback_) except_callback_(errCode, "Write the header failed...");
		delete audio_transcoder_;
		return errCode;
	}

    /**
     * Loop as long as we have input samples to read or output samples
     * to write; abort as soon as we have neither.
     */
    while (1) {
        /** Use the encoder's desired frame size for processing. */
        const int output_frame_size = audio_transcoder_->output_codec_context->frame_size;
        int finished                = 0;

        /**
         * Make sure that there is one frame worth of samples in the FIFO
         * buffer so that the encoder can do its work.
         * Since the decoder's and the encoder's frame size may differ, we
         * need to FIFO buffer to store as many frames worth of input samples
         * that they make up at least one frame worth of output samples.
         */
        while (av_audio_fifo_size(audio_transcoder_->fifo) < output_frame_size) {
            /**
             * Decode one frame worth of audio samples, convert it to the
             * output sample format and put it into the FIFO buffer.
             */
			errCode = audio_transcoder_->read_decode_convert_and_store(&finished);
            if (errCode != 0)
			{
                if(except_callback_) except_callback_(errCode, "Read decode convert and store failed...");
				break;
			}

            /**
             * If we are at the end of the input file, we continue
             * encoding the remaining audio samples to the output file.
             */
            if (finished)
                break;
        }

        /**
         * If we have enough samples for the encoder, we encode them.
         * At the end of the file, we pass the remaining samples to
         * the encoder.
         */
        while (av_audio_fifo_size(audio_transcoder_->fifo) >= output_frame_size || (finished && av_audio_fifo_size(audio_transcoder_->fifo) > 0))
            /**
             * Take one frame worth of audio samples from the FIFO buffer,
             * encode it and write it to the output file.
             */
			errCode = audio_transcoder_->load_encode_and_write();
            if (errCode != 0)
			{
				if(except_callback_) except_callback_(errCode, "Load encode write failed...");
				break;
			}

        /**
         * If we are at the end of the input file and have encoded
         * all remaining samples, we can exit this loop and finish.
         */
        if (finished) {
            int data_written;
            /** Flush the encoder as it may have delayed frames. */
            do {
				errCode = audio_transcoder_->encode_audio_frame(NULL, &data_written);
                if (errCode != 0)
                    if(except_callback_) except_callback_(errCode, "Flush the encoder failed...");
            } while (data_written);
            break;
        }
    }

    /** Write the trailer of the output file container. */
	errCode = audio_transcoder_->write_output_file_trailer();
    if (errCode != 0)
        if(except_callback_) except_callback_(errCode, "Write the trailer failed...");

	delete audio_transcoder_;
    return errCode;
}