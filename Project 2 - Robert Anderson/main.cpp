///*
//==================================================================================================================================================================
//|Copyright 2019 Oscar Lara	- scarlara@hotmail.com																												   |
//|																																								   |
//|See individual libraries separate legal notices																												   |
//|																																								   |
//|Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"),				   |
//|to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,				   |
//|and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :					   |
//|																																								   |
//|The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.									   |
//|																																								   |
//|THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,			   |
//|FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,	   |
//|WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.		   |
//==================================================================================================================================================================
//*/
//


#include <stdio.h>
#include <assert.h>
#include <vector>
#include <string>
#include <sstream>
#include <cstdio>

#include <glad/glad.h>
#include <GLFW\glfw3.h>

#include <fmod.hpp>
#include <fmod_errors.h>

#include <ft2build.h>
#include <fstream>
#include <iostream>
#include FT_FREETYPE_H

#define BUFFER_SIZE 255
#define FMOD_TIMEUNIT_MS 0x00000001


//Globals
unsigned int _vertex_shader, _fragment_shader, _program;
GLfloat _current_y_position = 0.0f;
GLfloat _y_offset = 40.0f;
char _text_buffer[512];
FMOD::System* _system = 0;
FMOD::Sound* _sound = 0;
FMOD::Channel* _channel = 0;
FMOD_RESULT _result = FMOD_OK;
int _release_counter = 0;
struct AudioItem;
std::vector<AudioItem> audioItems;
std::vector<AudioItem> audioItemsCompressed;
int currentItemIndex = 0;

//Functions
bool init_gl();
bool init_text();
bool init_shaders();
void render_text(const char* text);
void errorCheck(FMOD_RESULT);
void releaseFMOD();
bool initFMOD();

//GLFW
int _window_width = 640;
int _window_height = 480;
GLFWwindow* _main_window = NULL;

FT_Library _ft;
FT_Face _face;
unsigned int _text_program;
unsigned int _uniform_tex;
unsigned int _attribute_coord;
unsigned int _dp_vbo;
unsigned int _uniform_color;

//=============================================================
struct point {
	GLfloat x;
	GLfloat y;
	GLfloat s;
	GLfloat t;
};


//=============================================================

//Audio Item
struct AudioItem
{

private:
	FMOD_RESULT result;
	std::string path;
	char name[BUFFER_SIZE];
	float volume;
	float pitch;
	float pan;
	//float speed;
	unsigned int position;
	unsigned int length;
	FMOD_SOUND_TYPE type;
	FMOD_SOUND_FORMAT format;
	bool is_paused;
	bool is_playing;

public:
	FMOD::Sound* sound;
	FMOD::Channel* channel;
	//constructor
	AudioItem() {
		result = FMOD_OK;
		path = "";
		type = FMOD_SOUND_TYPE_UNKNOWN;
		format = FMOD_SOUND_FORMAT_NONE;
		position = 0.0f;
		length = 0.0f;
		name[BUFFER_SIZE - 1] = { '\0' };
		volume = 0.5f;
		pitch = 0.0f;//check documentation for pitch allowed values
		pan = 0.0f; //check documentation for pan allowed values
		//speed = 1.0f;
		is_paused = true;
		is_playing = false;
		sound = 0;
		channel = 0;
	}

	/*
	2 modes to create sounds
	1. sample loads all audio file into memory
	2. stream sound loads on demand you pay the price of a bit CPU overhead
	*/
	bool create_and_play_sound(bool is_streamed_sound) {
		if (_system) {
			result = _system->createSound(path.c_str(), (is_streamed_sound) ? FMOD_CREATESTREAM : FMOD_DEFAULT, 0, &sound);
			errorCheck(result);
			result = _system->playSound(sound, 0, is_paused, &channel);
			errorCheck(result);
			return true;
		}

		return false;
	}

	std::string get_name() {
		if (sound) {
			result = sound->getName(name, BUFFER_SIZE);
			errorCheck(result);
		}
		return name;
	}

	float getPitch() {
		if (channel) {
			result = channel->getPitch(&audioItems.at(currentItemIndex).pitch);
			errorCheck(result);
		}
		return pitch;
	}

	void downPitch(float newPitch) {
		if (channel) {
			pitch -= newPitch;
			result = channel->setPitch(pitch);
			errorCheck(result);
		}
	}

	void upPitch(float newPitch) {
		if (channel) {
			pitch += newPitch;
			result = channel->setPitch(pitch);
			errorCheck(result);
		}
	}

	//float getSpeed() {
	//	if (sound) {
	//		result = sound->getMusicSpeed(&audioItems.at(currentItemIndex).speed);
	//		errorCheck(result);
	//	}
	//	return speed;
	//}

	float getPan() {
		return pan;
	}

	void downPan(float newPan) {
		if (channel) {
			pan -= newPan;
			result = channel->setPan(newPan);
			errorCheck(result);
		}
	}

	void upPan(float newPan) {
		if (channel) {
			pan += newPan;
			result = channel->setPan(newPan);
			errorCheck(result);
		}
	}

	float get_volume() {
		if (channel) {
			result = channel->getVolume(&volume);
			errorCheck(result);
		}

		return volume;
	}

	void turnUpVolume(float volumeToAdd) {
		if (channel) {
			volume = volume + volumeToAdd;
			result = channel->setVolume(volume);
			errorCheck(result);
		}
	}

	void turnDownVolume(float volumeToSubtract) {
		if (channel) {
			volume = volume - volumeToSubtract;
			result = channel->setVolume(volume);
			errorCheck(result);
		}
	}

	float getPosition() {
		if (channel)
		{
			result = channel->getPosition(&position, FMOD_TIMEUNIT_MS);
			errorCheck(result);
		}
		return position;
	}	

	float getLength() {
		if (sound)
		{
			sound->getLength(&length, FMOD_TIMEUNIT_MS);
			errorCheck(result);
		}
		return length;
	}

	void set_path(std::string new_path) {
		path = new_path;
	}

	bool get_is_paused() {
		if (channel) {
			result = channel->getPaused(&is_paused);
			errorCheck(result);
		}
		return is_paused;
	}
	void set_is_paused(bool new_is_paused) {
		if (channel) {
			is_paused = new_is_paused;
			result = channel->setPaused(is_paused);
			errorCheck(result);
		}
	}

	std::string getType() {
		if (sound) {
			result = sound->getFormat(&audioItems.at(currentItemIndex).type, 0, 0, 0);
			errorCheck(result);
		}
		std::string value = "";
		if (type == FMOD_SOUND_TYPE_WAV)
		{
			value = "FMOD_SOUND_TYPE_WAV";
		}
		else if (type == FMOD_SOUND_TYPE_MPEG)
		{
			value = "FMOD_SOUND_TYPE_MPEG";
		}
		else if (type == FMOD_SOUND_TYPE_OGGVORBIS)
		{
			value = "FMOD_SOUND_TYPE_OGGVORBIS";
		}
		else if (type == FMOD_SOUND_TYPE_AIFF)
		{
			value = "FMOD_SOUND_TYPE_AIFF";
		}
		else if (type == FMOD_SOUND_TYPE_ASF)
		{
			value = "FMOD_SOUND_TYPE_ASF";
		}
		else if (type == FMOD_SOUND_TYPE_DLS)
		{
			value = "FMOD_SOUND_TYPE_DLS";
		}
		else if (type == FMOD_SOUND_TYPE_FLAC)
		{
			value = "FMOD_SOUND_TYPE_FLAC";
		}
		else if (type == FMOD_SOUND_TYPE_FSB)
		{
			value = "FMOD_SOUND_TYPE_FSB";
		}
		else if (type == FMOD_SOUND_TYPE_IT)
		{
			value = "FMOD_SOUND_TYPE_IT";
		}
		else if (type == FMOD_SOUND_TYPE_MIDI)
		{
			value = "FMOD_SOUND_TYPE_MIDI";
		}
		else if (type == FMOD_SOUND_TYPE_MOD)
		{
			value = "FMOD_SOUND_TYPE_MOD";
		}
		else if (type == FMOD_SOUND_TYPE_PLAYLIST)
		{
			value = "FMOD_SOUND_TYPE_PLAYLIST";
		}
		else if (type == FMOD_SOUND_TYPE_RAW)
		{
			value = "FMOD_SOUND_TYPE_RAW";
		}
		else if (type == FMOD_SOUND_TYPE_S3M)
		{
			value = "FMOD_SOUND_TYPE_S3M";
		}
		else if (type == FMOD_SOUND_TYPE_USER)
		{
			value = "FMOD_SOUND_TYPE_USER";
		}
		else if (type == FMOD_SOUND_TYPE_XM)
		{
			value = "FMOD_SOUND_TYPE_XM";
		}
		else if (type == FMOD_SOUND_TYPE_XMA)
		{
			value = "FMOD_SOUND_TYPE_XMA";
		}
		else if (type == FMOD_SOUND_TYPE_AUDIOQUEUE)
		{
			value = "FMOD_SOUND_TYPE_AUDIOQUEUE";
		}
		else if (type == FMOD_SOUND_TYPE_AT9)
		{
			value = "FMOD_SOUND_TYPE_AT9";
		}
		else if (type == FMOD_SOUND_TYPE_VORBIS)
		{
			value = "FMOD_SOUND_TYPE_VORBIS";
		}
		else if (type == FMOD_SOUND_TYPE_MEDIA_FOUNDATION)
		{
			value = "FMOD_SOUND_TYPE_MEDIA_FOUNDATION";
		}
		else if (type == FMOD_SOUND_TYPE_MEDIACODEC)
		{
			value = "FMOD_SOUND_TYPE_MEDIACODEC";
		}
		else if (type == FMOD_SOUND_TYPE_FADPCM)
		{
			value = "FMOD_SOUND_TYPE_FADPCM";
		}
		else if (type == FMOD_SOUND_TYPE_MAX)
		{
			value = "FMOD_SOUND_TYPE_MAX";
		}
		return value;
	}

	std::string getFormat() {
		if (sound) {
			result = sound->getFormat(0, &audioItems.at(currentItemIndex).format, 0, 0);
			errorCheck(result);
		}
		std::string value = "";
		if (format == FMOD_SOUND_FORMAT_PCM16)
		{
			value = "FMOD_SOUND_FORMAT_PCM16";
		}
		else if (format == FMOD_SOUND_FORMAT_PCM8)
		{
			value = "FMOD_SOUND_FORMAT_PCM8";
		}
		else if (format == FMOD_SOUND_FORMAT_PCM24)
		{
			value = "FMOD_SOUND_FORMAT_PCM24";
		}
		else if (format == FMOD_SOUND_FORMAT_PCM32)
		{
			value = "FMOD_SOUND_FORMAT_PCM32";
		}
		else if (format == FMOD_SOUND_FORMAT_PCMFLOAT)
		{
			value = "FMOD_SOUND_FORMAT_PCMFLOAT";
		}
		else if (format == FMOD_SOUND_FORMAT_BITSTREAM)
		{
			value = "FMOD_SOUND_FORMAT_BITSTREAM";
		}
		else if (format == FMOD_SOUND_FORMAT_MAX)
		{
			value = "FMOD_SOUND_FORMAT_MAX";
		}
		return value;
	}

	//create get info method
	std::string get_info() {
		std::ostringstream ss;

		//get name
		ss << "Name: " << this->get_name() << "    ";
		ss << "Volume: " << this->get_volume() << "    ";
		ss << "Is paused: " << ((this->get_is_paused()) ? "TRUE" : "FALSE");

		std::string result = ss.str();
		return result;
	}
};

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

bool shiftKeyPressed(int mods)
{
	if (mods == GLFW_MOD_SHIFT)
	{
		return true;
	}
	return false;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		if (audioItems.at(currentItemIndex).get_is_paused() == true)
		{
			audioItems.at(currentItemIndex).set_is_paused(false);
		}
		else
		{
			audioItems.at(currentItemIndex).set_is_paused(true);
		}
		//bool is_paused = false;
		//_result = _channel->getPaused(&is_paused);
		//errorCheck(_result);
		////we want to flip pause status on the channel
		//_result = _channel->setPaused(!is_paused);
		//errorCheck(_result);
	}
	if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
	{
		audioItems.at(currentItemIndex).set_is_paused(true);
		if (currentItemIndex >= audioItems.size() - 1)
		{
			currentItemIndex = 0;
		}
		else
		{
			currentItemIndex++;
		}
		audioItems.at(currentItemIndex).set_is_paused(false);
	}
	if (shiftKeyPressed(mods))
	{
		if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
		{
			audioItems.at(currentItemIndex).set_is_paused(true);
			if (currentItemIndex <= 0)
			{
				currentItemIndex = audioItems.size() - 1;
			}
			else
			{
				currentItemIndex--;
			}
			audioItems.at(currentItemIndex).set_is_paused(false);
		}
	}
	if (key == GLFW_KEY_UP && action == GLFW_PRESS)
	{
		audioItems.at(currentItemIndex).turnUpVolume(0.1f);
	}
	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
	{
		audioItems.at(currentItemIndex).turnDownVolume(0.1f);
	}
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
	{
		audioItems.at(currentItemIndex).downPan(0.1f);
	}
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
	{
		audioItems.at(currentItemIndex).upPan(0.1f);
	}
	if (key == GLFW_KEY_W && action == GLFW_PRESS)
	{
		audioItems.at(currentItemIndex).upPitch(0.1f);
	}
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
	{
		audioItems.at(currentItemIndex).downPitch(0.1f);
	}
}

void errorCheck(FMOD_RESULT result) {

	if (result != FMOD_OK) {
		releaseFMOD();
		system("pause");
		exit(1);
	}
}

bool initFMOD() {

	_result = FMOD::System_Create(&_system);
	errorCheck(_result);

	_result = _system->init(32, FMOD_INIT_NORMAL, 0);
	errorCheck(_result);

	return true;
}

void releaseFMOD() {

	//_release_counter++;
	//if (_release_counter >= 3) {
	//	_sound = 0;
	//	/*_sound1 = 0;
	//	_sound2 = 0;
	//	_sound3 = 0;
	//	_sound4 = 0;
	//	_sound5 = 0;
	//	_sound6 = 0;
	//	_sound7 = 0;
	//	_sound8 = 0;
	//	_sound9 = 0;
	//	_sound10 = 0;
	//	_sound11 = 0;
	//	_sound12 = 0;
	//	_sound13 = 0;*/
	//	_system = 0;
	//	exit(1);
	//}

	//if (_sound) {
	//	_result = _sound->release();
	//	errorCheck(_result);
	//}
	for (int i = 0; i < audioItems.size(); ++i)
	{
		if (audioItems.at(i).sound)
		{
			_result = audioItems.at(i).sound->release();
			errorCheck(_result);
		}
	}
	if (_system) {
		_result = _system->close();
		errorCheck(_result);
		_result = _system->release();
		errorCheck(_result);
	}
}

int main() {

	bool validInput = false;
	std::ifstream soundLibrary;
	while (!validInput)
	{
		std::string userInput;
		std::cout << "Please choose to use either compressed (c) or uncompressed (u) files to run the program: ";
		std::cin >> userInput;

		if (userInput == "u" || userInput == "U")
		{
			soundLibrary.open("soundLibrary.txt");
			validInput = true;
		}
		else if (userInput == "c" || userInput == "C")
		{
			soundLibrary.open("soundLibraryCompressed.txt");
			validInput = true;
		}
		else
		{
			std::cout << "\nInvalid choice. Please enter a 'u' or a 'c'.\n";
		}
	}
	if (!soundLibrary)
	{
		std::cout << "Unable to open text file. Might be in the wrong location?" << std::endl;
		system("pause");
	}

	fprintf(stdout, "Init opengl ...\n");
	assert(init_gl());

	fprintf(stdout, "Init text...\n");
	assert(init_text());

	fprintf(stdout, "Init shaders...\n");
	assert(init_shaders());


	//=======================================================================================
	fprintf(stdout, "Ready ...!\n");

	//Init FMOD here not inside the loop
	initFMOD();

	//Create and load audio items
	std::string line;
	AudioItem item;
	while (std::getline(soundLibrary, line))
	{
		if (line.length() > 0)
		{
			item.set_path(line.c_str());
			item.create_and_play_sound(true);
			audioItems.push_back(item);
		}
	}

	//_result = _system->createSound("../common/assets/audio/people_talking.wav", FMOD_DEFAULT, 0, &_sound1);
	//errorCheck(_result);
	//_result = _system->createSound("../common/assets/audio/door_close.wav", FMOD_DEFAULT, 0, &_sound2);
	//errorCheck(_result);
	//_result = _system->createSound("../common/assets/audio/footsteps.wav", FMOD_DEFAULT, 0, &_sound3);
	//errorCheck(_result);
	//_result = _system->createSound("../common/assets/audio/car_door.wav", FMOD_DEFAULT, 0, &_sound4);
	//errorCheck(_result);
	//_result = _system->createSound("../common/assets/audio/car_driving.wav", FMOD_DEFAULT, 0, &_sound5);
	//errorCheck(_result);
	//_result = _system->createSound("../common/assets/audio/police_siren.wav", FMOD_DEFAULT, 0, &_sound6);
	//errorCheck(_result);
	//_result = _system->createSound("../common/assets/audio/frustrated.wav", FMOD_DEFAULT, 0, &_sound7);
	//errorCheck(_result);
	//_result = _system->createSound("../common/assets/audio/footsteps_gravel.wav", FMOD_DEFAULT, 0, &_sound8);
	//errorCheck(_result);
	//_result = _system->createSound("../common/assets/audio/knock_on_door.wav", FMOD_DEFAULT, 0, &_sound9);
	//errorCheck(_result);
	//_result = _system->createSound("../common/assets/audio/car_window_down.wav", FMOD_DEFAULT, 0, &_sound10);
	//errorCheck(_result);
	//_result = _system->createSound("../common/assets/audio/driver_talking.ogg", FMOD_DEFAULT, 0, &_sound11);
	//errorCheck(_result);
	//_result = _system->createSound("../common/assets/audio/car_window_up.wav", FMOD_DEFAULT, 0, &_sound12);
	//errorCheck(_result);
	//_result = _system->createSound("../common/assets/audio/car_start.wav", FMOD_DEFAULT, 0, &_sound13);
	//errorCheck(_result);


	//_result = _system->playSound(_sound1, 0, true, &_channel);
	//errorCheck(_result);
	//_result = _system->playSound(_sound2, 0, true, &_channel);
	//errorCheck(_result);
	//_result = _system->playSound(_sound3, 0, true, &_channel);
	//errorCheck(_result);
	//_result = _system->playSound(_sound4, 0, true, &_channel);
	//errorCheck(_result);
	//_result = _system->playSound(_sound5, 0, true, &_channel);
	//errorCheck(_result);
	//_result = _system->playSound(_sound6, 0, true, &_channel);
	//errorCheck(_result);
	//_result = _system->playSound(_sound7, 0, true, &_channel);
	//errorCheck(_result);
	//_result = _system->playSound(_sound8, 0, true, &_channel);
	//errorCheck(_result);
	//_result = _system->playSound(_sound9, 0, true, &_channel);
	//errorCheck(_result);
	//_result = _system->playSound(_sound10, 0, true, &_channel);
	//errorCheck(_result);
	//_result = _system->playSound(_sound11, 0, true, &_channel);
	//errorCheck(_result);
	//_result = _system->playSound(_sound12, 0, true, &_channel);
	//errorCheck(_result);
	//_result = _system->playSound(_sound13, 0, true, &_channel);
	//errorCheck(_result);
	

	//Main loop
	while (!glfwWindowShouldClose(_main_window)) {		

		//Reset text y position
		_current_y_position = 30.0f;

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(_program);	

		bool is_paused = false;
		unsigned int pos;

		FMOD::Sound* current_sound = 0;


		if (_channel) {
			//get is paused
			_result = _channel->getPaused(&is_paused);
			errorCheck(_result);
			_result = _channel->getPosition(&pos, FMOD_TIMEUNIT_MS);
			errorCheck(_result);
			_result = _channel->getCurrentSound(&current_sound);
			errorCheck(_result);
		}

		const int buffer_size = 255;
		char name[buffer_size] = { '\0' };
		unsigned int len;

		if (_sound) {
			_result = _sound->getName(name, buffer_size);
			errorCheck(_result);
			_result = _sound->getLength(&len, FMOD_TIMEUNIT_MS);
			errorCheck(_result);
		}

		render_text("=====================================================");
		render_text("Media Fundamentals play sound...");
		render_text("=====================================================");
		render_text("Press ESC to Exit!");
		render_text("Press SPACE to pause and play.");
		render_text("Press TAB to go to the next audio clip.");
		render_text("Press SHIFT+TAB to go to the previous audio clip.");
		render_text("Press UP or DOWN to control the volume.");
		render_text("Press LEFT and RIGHT to control the pan.");
		render_text("=====================================================");
		//print numbers, strings using buffer
		std::string fileInfo = audioItems.at(currentItemIndex).get_info();
		render_text(fileInfo.c_str());
		sprintf(_text_buffer, "Current volume: %f", audioItems.at(currentItemIndex).get_volume());
		render_text(_text_buffer);
		std::string fileType = audioItems.at(currentItemIndex).getType();
		render_text("File type: ");
		render_text(fileType.c_str());
		std::string fileFormat = audioItems.at(currentItemIndex).getFormat();
		render_text("File format: ");
		render_text(fileFormat.c_str());
		sprintf(_text_buffer, "Current pan: %f", audioItems.at(currentItemIndex).getPan());
		render_text(_text_buffer);
		sprintf(_text_buffer, "Current position: %f", audioItems.at(currentItemIndex).getPosition());
		render_text(_text_buffer);
		sprintf(_text_buffer, "File length: %f", audioItems.at(currentItemIndex).getLength());
		render_text(_text_buffer);
		//sprintf(_text_buffer, "Current speed: %f", audioItems.at(currentItemIndex).getSpeed());
		//render_text(_text_buffer);
		sprintf(_text_buffer, "Current pitch: %f", audioItems.at(currentItemIndex).getPitch());
		render_text(_text_buffer);
		render_text("=====================================================");
		


		glfwSwapBuffers(_main_window);
		glfwPollEvents();

		glfwGetWindowSize(_main_window, &_window_width, &_window_height);

	}

	glfwDestroyWindow(_main_window);
	glfwTerminate();

	releaseFMOD();

	return 0;
}



bool init_gl() {

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		fprintf(stderr, "Unable to init glfw\n");
		return false;
	}
		
	//Full screen
	//_main_window = glfwCreateWindow(1920, 1080, "Media Fundamentals... play sound", glfwGetPrimaryMonitor(), NULL);
	_main_window = glfwCreateWindow(1920, 1080, "Media Fundamentals... play sound", NULL, NULL);

	if (!_main_window)
	{
		glfwTerminate();
		fprintf(stderr, "Unable to create main window glfw\n");
		return false;
	}
	
	glfwGetWindowSize(_main_window, &_window_width, &_window_height);

	glfwSetKeyCallback(_main_window, key_callback);
	glfwMakeContextCurrent(_main_window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		fprintf(stderr, "Unable to init glad\n");
		return false;
	}
	glfwSwapInterval(1);
	glfwWindowHint(GLFW_SAMPLES, 4);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	//glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_TRUE);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);

	return true;
}

bool init_text() {

	if (FT_Init_FreeType(&_ft)) {
		fprintf(stderr, "Unable to init text...\n");
		return false;
	}
	if (FT_New_Face(_ft, "../common/assets/fonts/arial.ttf", 0, &_face)) {
		fprintf(stderr, "Unable to init text...\n");
		return false;
	}

	//Font size
	FT_Set_Pixel_Sizes(_face, 0, 36);

	if (FT_Load_Char(_face, 'X', FT_LOAD_RENDER))
	{
		fprintf(stderr, "unable to load character\n");
		return false;
	}

	return true;
}

bool init_shaders() {
	//=======================================================================================
//Shaders
	int params = -1;

	glGetProgramiv(_program, GL_LINK_STATUS, &params);
	if (!params) {
		fprintf(stderr, "Unable to link shader program");
		return 1;
	}
	//--	
	FILE* f = fopen("../common/src/shaders/simple_text.vert", "rb");
	long filesize = ftell(f);
	fseek(f, 0, SEEK_END);
	filesize = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* simple_text_vert = (char*)malloc(filesize + 1);
	fread(simple_text_vert, filesize, 1, f);
	fclose(f);
	simple_text_vert[filesize] = 0;
	//--
	f = fopen("../common/src/shaders/simple_text.frag", "rb");
	fseek(f, 0, SEEK_END);
	filesize = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* simple_text_frag = (char*)malloc(filesize + 1);
	fread(simple_text_frag, filesize, 1, f);
	fclose(f);
	simple_text_frag[filesize] = 0;

	//text vertex shader
	unsigned int simple_text_vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(simple_text_vs, 1, &simple_text_vert, NULL);
	glCompileShader(simple_text_vs);
	glGetShaderiv(simple_text_vs, GL_COMPILE_STATUS, &params);
	if (!params) {
		fprintf(stderr, "Unable to compile simple text vertex shader");
		return 1;
	}
	unsigned int simple_text_fg = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(simple_text_fg, 1, &simple_text_frag, NULL);
	glCompileShader(simple_text_fg);
	glGetShaderiv(simple_text_fg, GL_COMPILE_STATUS, &params);
	if (!params) {
		fprintf(stderr, "Unable to compile simple text fragment shader");
		return 1;
	}

	_text_program = glCreateProgram();
	glAttachShader(_text_program, simple_text_vs);
	glAttachShader(_text_program, simple_text_fg);
	glLinkProgram(_text_program);

	glGetProgramiv(_text_program, GL_LINK_STATUS, &params);
	if (!params) {
		fprintf(stderr, "Unable to link simple text shader program");
		return 1;
	}

	_uniform_tex = glGetUniformLocation(_text_program, "tex");
	_attribute_coord = glGetAttribLocation(_text_program, "coord");
	_uniform_color = glGetUniformLocation(_text_program, "color");
	glGenBuffers(1, &_dp_vbo);

	glUseProgram(_program);
	return true;
}

void render_text(const char *text) {
	//render_text(const char *text, float x, float y, float sx, float sy)
	float sx = 2.0f / _window_width;
	float sy = 2.0f / _window_height;
	
	//GLfloat _current_y_position = 30.0f;
	GLfloat xoffset = 8 * sx;
	float x = -1 + xoffset;
	float y = 1 - _current_y_position * sy;


	glUseProgram(_text_program);
	GLfloat black[4] = { 0, 0, 0, 1 };
	glUniform4fv(_uniform_color, 1, black);
	const char *p;
	FT_GlyphSlot g = _face->glyph;

	GLuint tex;
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1i(_uniform_tex, 0);

	/* We require 1 byte alignment when uploading texture data */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	/* Clamping to edges is important to prevent artifacts when scaling */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	/* Linear filtering usually looks best for text */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	/* Set up the VBO for our vertex data */
	glEnableVertexAttribArray(_attribute_coord);
	glBindBuffer(GL_ARRAY_BUFFER, _dp_vbo);
	glVertexAttribPointer(_attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);


	/* Loop through all characters */
	for (p = text; *p; p++) {
		/* Try to load and render the character */
		if (FT_Load_Char(_face, *p, FT_LOAD_RENDER))
			continue;


		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			g->bitmap.width,
			g->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			g->bitmap.buffer
		);

		/* Calculate the vertex and texture coordinates */
		float x2 = x + g->bitmap_left * sx;
		float y2 = -y - g->bitmap_top * sy;
		float w = g->bitmap.width * sx;
		float h = g->bitmap.rows * sy;

		point box[4] = {
			{ x2, -y2, 0, 0 },
		{ x2 + w, -y2, 1, 0 },
		{ x2, -y2 - h, 0, 1 },
		{ x2 + w, -y2 - h, 1, 1 },
		};

		
		/* Draw the character on the screen */
		glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		/* Advance the cursor to the start of the next character */
		x += (g->advance.x >> 6) * sx;
		y += (g->advance.y >> 6) * sy;
	}
	glDisableVertexAttribArray(_attribute_coord);
	glDeleteTextures(1, &tex);
	glEnableVertexAttribArray(0);

	_current_y_position = (_current_y_position > 800.0f) ? 30.0f : (_current_y_position + _y_offset);
}

