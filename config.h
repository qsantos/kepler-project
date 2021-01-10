#ifndef CONFIG_h
#define CONFIG_h

struct SystemConfig {
    char* default_focus;
    char* display_name;
    char* root;
    double spaceship_altitude;
    double star_temperature;
    char* system_data;
    char* textures_directory;
};

struct Config {
    struct SystemConfig system;
};

struct Config load_config(const char* filename, const char* system);
void delete_config(struct Config* config);

#endif
