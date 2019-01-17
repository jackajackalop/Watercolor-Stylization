#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>

//A simple sound system for games.

namespace Sound {

struct PlayingSample;

enum LoopOrOnce {
	Once,
	Loop
};

// 'Sample' objects are mono (one-channel) audio 
struct Sample {
	//load from a ".wav" file:
	// will warn and downmix to mono if file is stereo
	// will warn and perform not-very-good interpolation if file is not Sound::AudioRate
	Sample(std::string const &filename);

	//start playing an instance of this sample at a given initial position and volume:
	// the returned 'PlayingSample' handle can be used to change position, fade volume, or cancel playback.
	std::shared_ptr< PlayingSample > play(
		glm::vec3 const &position,
		float volume = 1.0f,
		LoopOrOnce loop_or_once = Once
	) const;

	std::vector< float > data;
};

//Ramp<> is a template to help with managing values that should be smoothly
// interpolated to a target over a certain amount of time:
template< typename T >
struct Ramp {
	template< typename... Args >
	Ramp(Args&&... args) : value(std::forward< Args >(args)...), target(value) { }
	void set(T const &value_, float ramp_) {
		if (ramp_ <= 0.0f) {
			value = target = value_;
			ramp = 0.0f;
		} else {
			target = value_;
			ramp = ramp_;
		}
	}
	T value;
	T target;
	float ramp = 0.0f;
};

struct PlayingSample {
	//change the position or volume of a playing sample;
	// value will change over 'ramp' seconds to avoid creating audible artifacts:
	void set_position(glm::vec3 const &new_position, float ramp = 1.0f / 60.0f);
	void set_volume(float new_volume, float ramp = 1.0f / 60.0f);
	void stop(float ramp = 1.0f / 60.0f);

	//internals:
	std::vector< float > const &data; //reference to sample data being played
	uint32_t i = 0; //next data value to read
	bool loop = false; //should playback loop after data runs out?
	bool stopped = false; //was playback stopped (either by running out of sample, or by stop())?

	Ramp< glm::vec3 > position = Ramp< glm::vec3 >(0.0f);
	Ramp< float > volume = Ramp< float >(1.0f);

	PlayingSample(Sample const *sample_, glm::vec3 const &position_, float volume_, bool loop_)
		: data(sample_->data), loop(loop_), position(position_), volume(volume_) { }
};

struct Listener {
	void set_position(glm::vec3 const &new_position, float ramp = 1.0f / 60.0f);
	void set_right(glm::vec3 const &new_right, float ramp = 1.0f / 60.0f);

	//internals:
	Ramp< glm::vec3 > position = Ramp< glm::vec3 >(0.0f); //listener's location
	Ramp< glm::vec3 > right = Ramp< glm::vec3 >(1.0f, 0.0f, 0.0f); //unit vector pointing to listener's right
};
extern struct Listener listener;



constexpr const uint32_t AudioRate = 48000; //sample rate, in Hz, for audio output
constexpr const uint32_t MixSamples = 1024; //samples to mix at once; SDL requires a power of two; smaller values mean more reactive sound, but require more frequent audio callback invocation

void init(); //should call Sound::init() from main.cpp before using any member functions

//the audio callback doesn't run between Sound::lock() and Sound::unlock()
// the set_*/stop/play/... functions already use these helpers, so you shouldn't need
// to call them unless your code is modifying values directly
void lock();
void unlock();

void stop_all_samples(); //sort of a 'panic button' to stop all playing samples

void set_volume(float new_volume, float ramp = 1.0f / 60.0f);
extern Ramp< float > volume;

}; //namespace Sound
