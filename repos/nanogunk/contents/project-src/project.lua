local ffi = require("ffi")

local libs = ffi_luajit_libs or {
   OSX     = { x86 = "bin/OSX/libproject.dylib", x64 = "bin/OSX/libproject.dylib" },
   Windows = { x86 = "bin/Windows/x86/project.dll", x64 = "bin/Windows/x64/project.dll" },
   Linux   = { x86 = "SDL", x64 = "bin/Linux/x64/project.so", arm = "bin/Linux/arm/project.so" },
   BSD     = { x86 = "bin/luajit32.so",  x64 = "bin/luajit64.so" },
   POSIX   = { x86 = "bin/luajit32.so",  x64 = "bin/luajit64.so" },
   Other   = { x86 = "bin/luajit32.so",  x64 = "bin/luajit64.so" },
}

local project  = ffi.load( ffi_SDL_lib or ffi_sdl_lib or libs[ ffi.os ][ ffi.arch ]  or "project" )

ffi.cdef[[

void c_test();

// ---- sound

void sound_init();

void sound_release(void *s);
void sound_start(void *s);

void *square_make();
void square_set(void *s, double volume, double pitch, double decay);

struct sample {
	double *data;
	int len;
};
struct sampleset {
	struct sample **samples; // Array of sample pointers
	int len;
};
struct sample *load_sample(const char *ogg);
struct sample *crop(struct sample *s, int start, int len);
struct sampleset *make_sampleset(struct sample *samples[], int len);
void destroy_sample(struct sample *s); // Destroys the sample data also. BE CAREFUL!!!

void *playsample_make();
void playsample_set(void *_s, struct sample *play, double volume, int repeatp);
void playsample_mute(void *_s);

enum {
	playset_play,
	playset_random,
	playset_skip
};
void *playset_make();
void playset_set(void *_s, struct sampleset *play, double volume, int mode, int moded1, int moded2, double modef1, double modef2);

void *stutter_make();
void stutter_set(void *_s, struct sample *play, double volume, int every, int jump);

void *worms_make();

// ---- image

SDL_Surface *load_image(const char *filename);
void set_color(SDL_Surface *surface, int x, int y, uint32_t color);
uint32_t get_color(SDL_Surface *surface, int x, int y);
void set_color_x(SDL_Surface *surface, int channel, int x, int y, uint32_t color);
uint32_t get_color_x(SDL_Surface *surface, int channel, int x, int y);
uint32_t get_component(uint32_t col, int i);
uint32_t lesser(uint32_t c1, uint32_t c2);
uint32_t greater(uint32_t c1, uint32_t c2);

/// ---- ahahahahah my functions

void level_init(SDL_Surface *surface, int worms, int worm_length, int worm_start_dist, uint32_t color1, uint32_t color2, uint32_t color3);
void level_init2(SDL_Surface *surface, int worms, int worm_length, int worm_start_dist, uint32_t color1, uint32_t color2);
void also_do_something_neat_idk(SDL_Surface *surface, uint32_t color);
void do_a_thing(SDL_Surface *surface);
void do_another_thing(SDL_Surface *surface);
void do_a_weird_thing(SDL_Surface *surface);
void textify(SDL_Surface *surface);
void do_a_locational_thing(SDL_Surface *surface, int x, int y);
void circle_thing(SDL_Surface *surface, int X, int Y);
void flipline(SDL_Surface *surface, int X, int Y);
void set_brush_size(int n);
void set_brush_type(int n);
void down_thing(SDL_Surface *surface);
void find_edges(SDL_Surface *surface, int channel, int level, uint32_t color);
void bandit(SDL_Surface *surface);
void magic(SDL_Surface *surface);
double compute_similarity(SDL_Surface *surface1, SDL_Surface *surface2);
int increase_similarity(SDL_Surface *surface1, SDL_Surface *surface2, int x, int y);

void preserve_mouse(SDL_Surface *surface, int x, int y);
void restore_mouse(SDL_Surface *surface, int x, int y);

// ---- stb (images, oggs). should this even be lua-visible?

typedef unsigned char stbi_uc;
stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
stbi_uc *stbi_load            (char const *filename,     int *x, int *y, int *comp, int req_comp);

	// Image cruft
const char *stbi_failure_reason  (void); 
void     stbi_image_free      (void *retval_from_stbi_load);
int      stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp);
int      stbi_info            (char const *filename,     int *x, int *y, int *comp);

	// Save image.
void save_image(SDL_Surface *surface, const char *name);
int stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);

	// Load ogg.
int stb_vorbis_decode_memory(unsigned char *mem, int len, int *channels, short **output);
int stb_vorbis_decode_filename(const char *filename, int *channels, short **output);

]]

return project

