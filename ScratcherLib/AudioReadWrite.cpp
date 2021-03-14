#include "TrackBuffer.h"
#include "AudioReadWrite.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

inline bool exists_test(const char* name)
{
	if (FILE *file = fopen(name, "r"))
	{
		fclose(file);
		return true;
	}
	else
	{
		return false;
	}
}


TrackBuffer* ReadAudioFromFile(const char* fileName)
{
	if (!exists_test(fileName))
	{
		printf("Failed loading %s\n", fileName);
		return nullptr;
	}

	AVFormatContext* p_fmt_ctx = nullptr;
	avformat_open_input(&p_fmt_ctx, fileName, nullptr, nullptr);
	avformat_find_stream_info(p_fmt_ctx, nullptr);

	int a_idx = -1;

	for (unsigned i = 0; i < p_fmt_ctx->nb_streams; i++)
	{
		if (p_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			a_idx = i;
		}
	}

	AVCodecContext* p_codec_ctx_audio;
	AVFrame *p_frm_raw_audio;
	AVFrame *p_frm_f32_audio;	
	SwrContext *swr_ctx;
	int sample_rate;

	// audio
	if (a_idx >= 0)
	{
		AVCodecParameters* p_codec_par = p_fmt_ctx->streams[a_idx]->codecpar;
		AVCodec* p_codec = avcodec_find_decoder(p_codec_par->codec_id);
		p_codec_ctx_audio = avcodec_alloc_context3(p_codec);
		avcodec_parameters_to_context(p_codec_ctx_audio, p_codec_par);
		avcodec_open2(p_codec_ctx_audio, p_codec, nullptr);

		sample_rate = p_codec_ctx_audio->sample_rate;

		p_frm_raw_audio = av_frame_alloc();
		p_frm_f32_audio = av_frame_alloc();

		int64_t layout_in = av_get_default_channel_layout(p_codec_ctx_audio->channels);
		swr_ctx = swr_alloc_set_opts(nullptr, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_FLT, sample_rate, layout_in, p_codec_ctx_audio->sample_fmt, sample_rate, 0, nullptr);
		swr_init(swr_ctx);		
	}
	else
	{
		printf("%s is not an audio file!\n", fileName);
		return nullptr;
	}

	TrackBuffer* track = new TrackBuffer(sample_rate, 2);

	NoteBuffer buf;
	buf.m_sampleRate = (float)sample_rate;
	buf.m_channelNum = 2;

	AVPacket packet;
	while (av_read_frame(p_fmt_ctx, &packet) == 0)
	{
		if (packet.stream_index == a_idx)
		{
			avcodec_send_packet(p_codec_ctx_audio, &packet);
			while (avcodec_receive_frame(p_codec_ctx_audio, p_frm_raw_audio) == 0)
			{
				int in_length = p_frm_raw_audio->nb_samples;
				if (buf.m_data == nullptr || buf.m_sampleNum != in_length)
				{
					buf.m_sampleNum = in_length;
					buf.m_cursorDelta = (float)in_length;
					buf.Allocate();
					av_samples_fill_arrays(p_frm_f32_audio->data, p_frm_f32_audio->linesize, (const uint8_t*)buf.m_data, 2, in_length, AV_SAMPLE_FMT_FLT, 0);
				}				
				swr_convert(swr_ctx, p_frm_f32_audio->data, in_length, (const uint8_t **)p_frm_raw_audio->data, in_length);
				track->WriteBlend(buf);				
			}
		}
		av_packet_unref(&packet);
	}

	swr_free(&swr_ctx);
	av_frame_free(&p_frm_f32_audio);
	av_frame_free(&p_frm_raw_audio);
	avcodec_free_context(&p_codec_ctx_audio);
	avformat_close_input(&p_fmt_ctx);

	return track;
}

static const AVCodecID audio_codec_id = AV_CODEC_ID_MP3;
static const AVSampleFormat sample_fmt = AV_SAMPLE_FMT_FLTP;


inline AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples)
{
	AVFrame *frame = av_frame_alloc();
	frame->format = sample_fmt;
	frame->channel_layout = channel_layout;
	frame->sample_rate = sample_rate;
	frame->nb_samples = nb_samples;
	av_frame_get_buffer(frame, 0);
	return frame;
}

void WriteAudioToFile(TrackBuffer* track, const char* fileName, int bit_rate)
{
	unsigned num_samples = track->NumberOfSamples();
	unsigned chn = track->NumberOfChannels();
	unsigned sample_rate = track->Rate();

	AVOutputFormat *output_format = av_guess_format("mp3", nullptr, fileName);

	AVFormatContext *p_fmt_ctx;
	avformat_alloc_output_context2(&p_fmt_ctx, output_format, nullptr, fileName);
	output_format = p_fmt_ctx->oformat;

	AVCodec *audio_codec = avcodec_find_encoder(audio_codec_id);
	AVStream* stream = avformat_new_stream(p_fmt_ctx, nullptr);
	stream->id = p_fmt_ctx->nb_streams - 1;
	AVCodecContext *p_codec_ctx_audio = avcodec_alloc_context3(audio_codec);
	p_codec_ctx_audio->sample_fmt = sample_fmt;
	p_codec_ctx_audio->bit_rate = bit_rate;
	p_codec_ctx_audio->sample_rate = sample_rate;
	p_codec_ctx_audio->channel_layout = AV_CH_LAYOUT_STEREO;
	p_codec_ctx_audio->channels = av_get_channel_layout_nb_channels(p_codec_ctx_audio->channel_layout);
	stream->time_base = { 1, (int)sample_rate };
	if (p_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		p_codec_ctx_audio->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	avcodec_open2(p_codec_ctx_audio, audio_codec, nullptr);
	int frame_size = p_codec_ctx_audio->frame_size;

	AVFrame *frame = alloc_audio_frame(p_codec_ctx_audio->sample_fmt, p_codec_ctx_audio->channel_layout, p_codec_ctx_audio->sample_rate, frame_size);
	AVFrame *tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_FLT, p_codec_ctx_audio->channel_layout, p_codec_ctx_audio->sample_rate, frame_size);

	avcodec_parameters_from_context(stream->codecpar, p_codec_ctx_audio);
	SwrContext *swr_ctx = swr_alloc();
	av_opt_set_int(swr_ctx, "in_channel_count", p_codec_ctx_audio->channels, 0);
	av_opt_set_int(swr_ctx, "in_sample_rate", p_codec_ctx_audio->sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
	av_opt_set_int(swr_ctx, "out_channel_count", p_codec_ctx_audio->channels, 0);
	av_opt_set_int(swr_ctx, "out_sample_rate", p_codec_ctx_audio->sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", p_codec_ctx_audio->sample_fmt, 0);

	swr_init(swr_ctx);

	av_dump_format(p_fmt_ctx, 0, fileName, 1);
	if (!(output_format->flags & AVFMT_NOFILE))
		avio_open(&p_fmt_ctx->pb, fileName, AVIO_FLAG_WRITE);
	avformat_write_header(p_fmt_ctx, nullptr);

	float *buffer = (float*)tmp_frame->data[0];

	unsigned pos = 0;
	int samples_count = 0;
	while (num_samples > 0)
	{
		unsigned writeCount = frame_size;
		if (writeCount > num_samples) writeCount = num_samples;
		track->GetSamples(pos, writeCount, buffer);

		int dst_nb_samples = (int)av_rescale_rnd(swr_get_delay(swr_ctx, sample_rate) + frame_size, sample_rate, sample_rate, AV_ROUND_UP);
		av_frame_make_writable(frame);
		swr_convert(swr_ctx, frame->data, dst_nb_samples, (const uint8_t **)tmp_frame->data, tmp_frame->nb_samples);
		frame->pts = av_rescale_q(samples_count, { 1, (int)sample_rate }, p_codec_ctx_audio->time_base);
		samples_count += dst_nb_samples;

		int ret;
		ret = avcodec_send_frame(p_codec_ctx_audio, frame);
		while (ret >= 0)
		{
			AVPacket packet = { 0 };
			ret = avcodec_receive_packet(p_codec_ctx_audio, &packet);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			av_packet_rescale_ts(&packet, p_codec_ctx_audio->time_base, stream->time_base);
			packet.stream_index = stream->index;
			av_interleaved_write_frame(p_fmt_ctx, &packet);
			av_packet_unref(&packet);
		}


		num_samples -= writeCount;
		pos += writeCount;
	}

	av_write_trailer(p_fmt_ctx);

	avcodec_free_context(&p_codec_ctx_audio);
	av_frame_free(&frame);
	av_frame_free(&tmp_frame);	
	swr_free(&swr_ctx);

	if (!(output_format->flags & AVFMT_NOFILE))
		avio_closep(&p_fmt_ctx->pb);
	avformat_free_context(p_fmt_ctx);
}


void DumpAudioToRawFile(TrackBuffer* track, const char* fileName)
{
	unsigned num_samples = track->NumberOfSamples();
	unsigned chn = track->NumberOfChannels();
	unsigned buffer_size = 1152;

	float *buffer = new float[buffer_size*chn];
	unsigned pos = 0;

	FILE* fp = fopen(fileName, "wb");
	while (num_samples > 0)
	{
		unsigned writeCount = buffer_size;
		if (writeCount > num_samples) writeCount = num_samples;
		track->GetSamples(pos, writeCount, buffer);
		fwrite(buffer, sizeof(float), writeCount*chn, fp);
		num_samples -= writeCount;
		pos += writeCount;
	}
	fclose(fp);

	delete[] buffer;

}