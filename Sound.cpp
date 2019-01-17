#include "Sound.hpp"

#include <SDL.h>

#include <algorithm>
#include <iostream>
#include <list>
#include <string>

namespace Sound {

Ramp< float > volume = Ramp< float >(1.0f);
struct Listener listener;

namespace {
//local functions + data:

//helpers for advancing ramps:
constexpr const float RampStep = float(MixSamples) / float(AudioRate);
void step_position_ramp(Ramp< glm::vec3 > &ramp) {
	if (ramp.ramp < RampStep) {
		ramp.value = ramp.target;
		ramp.ramp = 0.0f;
	} else {
		ramp.value = glm::mix(ramp.value, ramp.target, RampStep / ramp.ramp);
		ramp.ramp -= RampStep;
	}
}
void step_value_ramp(Ramp< float > &ramp) {
	if (ramp.ramp < RampStep) {
		ramp.value = ramp.target;
		ramp.ramp = 0.0f;
	} else {
		ramp.value = glm::mix(ramp.value, ramp.target, RampStep / ramp.ramp);
		ramp.ramp -= RampStep;
	}
}
void step_direction_ramp(Ramp< glm::vec3 > &ramp) {
	if (ramp.ramp < RampStep) {
		ramp.value = ramp.target;
		ramp.ramp = 0.0f;
	} else {
		//find normal to the plane containing value and target:
		glm::vec3 norm = glm::cross(ramp.value, ramp.target);
		if (norm == glm::vec3(0.0f)) {
			if (ramp.target.x <= ramp.target.y && ramp.target.x <= ramp.target.z) {
				norm = glm::vec3(1.0f, 0.0f, 0.0f);
			} else if (ramp.target.y <= ramp.target.z) {
				norm = glm::vec3(0.0f, 1.0f, 0.0f);
			} else {
				norm = glm::vec3(0.0f, 0.0f, 1.0f);
			}
			norm -= ramp.target * glm::dot(ramp.target, norm);
		}
		norm = glm::normalize(norm);
		//find perpendicular to target in this plane:
		glm::vec3 perp = glm::cross(norm, ramp.target);

		//find angle from target to value:
		float angle = std::acos(glm::clamp(glm::dot(ramp.value, ramp.target), -1.0f, 1.0f));

		//figure out new target value by moving angle toward target:
		angle *= (ramp.ramp - RampStep) / ramp.ramp;

		ramp.value = ramp.target * std::cos(angle) + perp * std::sin(angle);
		ramp.ramp -= RampStep;
	}
}

//helper to compute panning values given a listener and source:
void compute_pan_from_listener_and_position(
	glm::vec3 const &listener_position,
	glm::vec3 const &listener_right,
	glm::vec3 const &source_position,
	float *volume_left, float *volume_right
	) {
	glm::vec3 to = source_position - listener_position;
	float distance = glm::length(to);
	//start by panning based on direction.
	//note that for a LR fade to sound uniform, sound power (squared magnitude) should remain constant.
	if (distance == 0.0f) {
		*volume_left = *volume_right = std::sqrt(2.0f);
	} else {
		//amt ranges from -1 (most left) to 1 (most right):
		float amt = glm::dot(listener_right, to) / distance;
		//turn into an angle from 0.0f (most left) to pi/2 (most right):
		float ang = 0.5f * 3.1415926f * (0.5f * (amt + 1.0f));
		*volume_right = std::sin(ang);
		*volume_left = std::cos(ang);

		//squared distance attenuation is realistic if there are no walls,
		// but I'm going to use linear because it's sounds better to me.
		// (feel free to change it, of course)
		if (distance > 1.0f) {
			*volume_left /= distance;
			*volume_right /= distance;
		}
	}
}

//list of all currently playing samples:
std::list< std::shared_ptr< PlayingSample > > playing_samples;

void mix_audio(void *, Uint8 *stream, int len) {
	assert(stream); //should always have some audio buffer

	struct LR {
		float l;
		float r;
	};
	static_assert(sizeof(LR) == 8, "Sample is packed");
	assert(len == MixSamples * sizeof(LR)); //should always have the expected number of samples

	LR *buffer = reinterpret_cast< LR * >(stream);

	//zero the output buffer:
	for (uint32_t s = 0; s < MixSamples; ++s) {
		buffer[s].l = 0.0f;
		buffer[s].r = 0.0f;
	}
	
	//Figure out global info (listener position, volume) at start and end of mix period:
	glm::vec3 start_position = listener.position.value;
	glm::vec3 start_right = listener.right.value;
	float start_volume = volume.value;

	step_position_ramp(listener.position);
	step_direction_ramp(listener.right);
	step_value_ramp(volume);

	glm::vec3 end_position = listener.position.value;
	glm::vec3 end_right = listener.right.value;
	float end_volume = volume.value;

	//now add audio for each playing sample:
	for (auto si = playing_samples.begin(); si != playing_samples.end(); /* later */) {
		PlayingSample &source = **si; //iterator over shared pointers

		//Figure out sample panning/volume at start and end of the mix period:
		LR start_pan;
		compute_pan_from_listener_and_position(start_position, start_right, source.position.value, &start_pan.l, &start_pan.r);
		start_pan.l *= start_volume * source.volume.value;
		start_pan.r *= start_volume * source.volume.value;

		step_position_ramp(source.position);
		step_value_ramp(source.volume);

		LR end_pan;
		compute_pan_from_listener_and_position(end_position, end_right, source.position.value, &end_pan.l, &end_pan.r);
		end_pan.l *= end_volume * source.volume.value;
		end_pan.r *= end_volume * source.volume.value;

		LR pan = start_pan;
		LR pan_step;
		pan_step.l = (end_pan.l - start_pan.l) / MixSamples;
		pan_step.r = (end_pan.r - start_pan.r) / MixSamples;

		assert(source.i < source.data.size());

		for (uint32_t i = 0; i < MixSamples; ++i) {
			//mix one sample based on current pan values:
			buffer[i].l += pan.l * source.data[source.i];
			buffer[i].r += pan.r * source.data[source.i];

			//update position in sample:
			source.i += 1;
			if (source.i == source.data.size()) {
				if (source.loop) source.i = 0;
				else break;
			}

			//update pan values:
			pan.l += pan_step.l;
			pan.r += pan_step.r;
		}

		if (source.i >= source.data.size() //non-looping sample has finished
		 || (source.stopped && source.volume.ramp == 0.0f) //sample has finished stopping
		 ) {
		 	source.stopped = true;
			auto old = si;
			++si;
			playing_samples.erase(old);
		} else {
			++si;
		}
	}

	//DEBUG: report output power:
	float max_power = 0.0f;
	for (uint32_t s = 0; s < MixSamples; ++s) {
		max_power = std::max(max_power, (buffer[s].l * buffer[s].l + buffer[s].r * buffer[s].r));
	}
	//std::cout << "Max Power: " << std::sqrt(max_power) << std::endl; //DEBUG

};

SDL_AudioDeviceID device = 0;

} //end anon namespace

//------------------

Sample::Sample(std::string const &filename) {
	SDL_AudioSpec audio_spec;
	Uint8 *audio_buf = nullptr;
	Uint32 audio_len = 0;

	SDL_AudioSpec *have = SDL_LoadWAV(filename.c_str(), &audio_spec, &audio_buf, &audio_len);
	if (!have) {
		throw std::runtime_error("Failed to load WAV file '" + filename + "'; SDL says \"" + std::string(SDL_GetError()) + "\"");
	}

	//based on the SDL_AudioCVT example in the docs: https://wiki.libsdl.org/SDL_AudioCVT
	SDL_AudioCVT cvt;
	SDL_BuildAudioCVT(&cvt, have->format, have->channels, have->freq, AUDIO_F32SYS, 1, AudioRate);
	if (cvt.needed) {
		std::cout << "WAV file '" + filename + "' didn't load as " + std::to_string(AudioRate) + " Hz, float32, mono; converting." << std::endl;
		cvt.len = audio_len;
		cvt.buf = (Uint8 *)SDL_malloc(cvt.len * cvt.len_mult);
		SDL_memcpy(cvt.buf, audio_buf, audio_len);
		SDL_ConvertAudio(&cvt);
		data.assign(reinterpret_cast< float * >(cvt.buf), reinterpret_cast< float * >(cvt.buf + cvt.len_cvt));
		SDL_free(cvt.buf);
	} else {
		data.assign(reinterpret_cast< float * >(audio_buf), reinterpret_cast< float * >(audio_buf + audio_len));
	}
	SDL_FreeWAV(audio_buf);

	float min = 0.0f;
	float max = 0.0f;
	for (auto d : data) {
		min = std::min(min, d);
		max = std::max(max, d);
	}
	std::cout << "Range: " << min << ", " << max << std::endl;
}

std::shared_ptr< PlayingSample > Sample::play(glm::vec3 const &position, float volume, LoopOrOnce loop_or_once) const {
	lock();
	playing_samples.emplace_back(std::make_shared< PlayingSample >(this, position, volume, loop_or_once == Loop));
	unlock();
	return playing_samples.back();
}


//------------------

void PlayingSample::set_position(glm::vec3 const &new_position, float ramp) {
	lock();
	position.set(new_position, ramp);
	unlock();
}

void PlayingSample::set_volume(float new_volume, float ramp) {
	lock();
	volume.set(new_volume, ramp);
	unlock();
}

void PlayingSample::stop(float ramp) {
	lock();
	if (!stopped) {
		stopped = true;
		volume.target = 0.0f;
		volume.ramp = ramp;
	} else {
		volume.ramp = std::min(volume.ramp, ramp);
	}
	unlock();
}

//------------------

void Listener::set_position(glm::vec3 const &new_position, float ramp) {
	lock();
	position.set(new_position, ramp);
	unlock();
}

void Listener::set_right(glm::vec3 const &new_right, float ramp) {
	lock();
	//some extra code to make sure right is always a unit vector:
	if (new_right == glm::vec3(0.0f)) {
		right.set(glm::vec3(1.0f, 0.0f, 0.0f), ramp);
	} else {
		right.set(glm::normalize(new_right), ramp);
	}
	unlock();
}

//------------------

void init() {
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
		std::cerr << "Failed to initialize SDL audio subsytem:\n" << SDL_GetError() << std::endl;
		return;
	}

	//Based on the example on https://wiki.libsdl.org/SDL_OpenAudioDevice
	SDL_AudioSpec want, have;
	SDL_zero(want);
	want.freq = AudioRate;
	want.format = AUDIO_F32SYS;
	want.channels = 2;
	want.samples = MixSamples;
	want.callback = mix_audio;

	device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
	if (device == 0) {
		std::cerr << "Failed to open audio device:\n" << SDL_GetError() << std::endl;
	} else {
		//start audio playback:
		SDL_PauseAudioDevice(device, 0);
		std::cout << "Audio output initialized." << std::endl;
	}
}

void lock() {
	if (device) SDL_LockAudioDevice(device);
}

void unlock() {
	if (device) SDL_UnlockAudioDevice(device);
}

void stop_all_samples() {
	lock();
	for (auto &s : playing_samples) {
		s->stop();
	}
	unlock();
}

void set_volume(float new_volume, float ramp) {
	lock();
	volume.set(new_volume, ramp);
	unlock();
}

} //namespace Sound
