#include <soloud.h>
#include <soloud_wavstream.h>
#include <cstdio>
#include <cstdlib>
#include <cstddef>

#define BGM_OGG_FILE "app0:/res/bgm.ogg"

extern "C"
{
#include "io.h"
#include "log.h"
#include "config.h"


	static SoLoud::Soloud soloud;
	static SoLoud::WavStream bgm;
	
	void init_sound() {
		if(!is_music_enabled()) return;
		
		soloud.init();
		soloud.setGlobalVolume(1.0);
		
		// read bgm
		PRINT_STR("Reading %s ...", BGM_OGG_FILE);
		size_t bgm_size = get_file_size(BGM_OGG_FILE);
		unsigned char* bgm_data = (unsigned char*)malloc(bgm_size);
		read_file(BGM_OGG_FILE, bgm_data, bgm_size);
		PRINT_STR(" Done\n", BGM_OGG_FILE);
		
		
		// load bgm
		PRINT_STR("Loading %s ...", BGM_OGG_FILE);
		bgm.loadMem(bgm_data, bgm_size, 0, 1);
		bgm.setLooping(1);
		soloud.play(bgm);
		PRINT_STR(" Done\n", BGM_OGG_FILE);
		
	}
	
	void term_sound() {
		soloud.deinit();
	}
	
}