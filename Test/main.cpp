#include "TrackBuffer.h"
#include "AudioReadWrite.h"
#include "SamplerDirect.h"
#include "SamplerScratch.h"
#include "SampleToTrackBuffer.h"


#include <cmath>
#define PI 3.141592653589792

int main()
{
	TrackBuffer* buf_in = ReadAudioFromFile("scratch10.mp3");

	SamplerScratch* sampler = new SamplerScratch(buf_in);
	sampler->set_start_pos(0.1f);
	sampler->add_control_point(0.3f, 0.5f);
	sampler->add_control_point(0.8f, 0.5f);
	sampler->add_control_point(1.1f, 0.1f);
	sampler->add_control_point(1.6f, 0.1f);
	sampler->add_control_point(1.9f, 0.5f);
	sampler->add_control_point(2.4f, 0.5f);
	sampler->add_control_point(2.7f, 0.1f);
	sampler->add_control_point(3.2f, 0.1f);
	sampler->add_control_point(3.5f, 0.5f);
	sampler->add_control_point(4.0f, 0.5f);
	sampler->add_control_point(4.3f, 0.1f);

	printf("%f\n", sampler->get_duration());

	std::vector<float> v;
	sampler->uniform_time_samples(0.05, v);
	FILE* fp = fopen("dump.txt", "w");
	for (size_t i = 0; i < v.size(); i++)
	{
		fprintf(fp, "%f\n", v[i]);
	}
	fclose(fp);
	
	TrackBuffer buf48000(48000);
	SampleToTrackBuffer(*sampler, buf48000);
	WriteAudioToFile(&buf48000, "test.mp3");


	delete sampler;
	delete buf_in;

	return 0;
}

