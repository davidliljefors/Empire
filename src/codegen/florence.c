#include <SDL3/SDL.h>
#include <Empire/hash.inl>
#include <stdio.h>

#define MAX_ASSETS 4096
#define ASSETS_DIR "assets"

typedef struct {
    char* name;        // filename without extension
    char* ext;         // extension (e.g., "obj", "png")
    char* path;        // relative path from project root
    uint64_t hash;
} Asset;

static char* to_snake_case(const char* filename) {
    char* result = SDL_strdup(filename);
    for (char* p = result; *p; p++) {
        if (*p == '-' || *p == ' ') *p = '_';
        else if (*p >= 'A' && *p <= 'Z') *p += 32; // lowercase
    }
    return result;
}

static int scan_directory(const char* dir, Asset* assets, int* count) {
    char** files = SDL_GlobDirectory(dir, NULL, SDL_GLOB_CASEINSENSITIVE, NULL);
    if (!files) return 0;
    
    for (char** file = files; *file && *count < MAX_ASSETS; file++) {
        const char* filename = *file;
        
        // Skip directories and hidden files
        if (filename[0] == '.') continue;
        
        // Find extension
        const char* dot = SDL_strrchr(filename, '.');
        if (!dot) continue;
        
        Asset* asset = &assets[*count];
        
        // Build full path
        size_t path_len = SDL_strlen(dir) + SDL_strlen(filename) + 2;
        asset->path = (char*)SDL_malloc(path_len);
        SDL_snprintf(asset->path, path_len, "%s/%s", dir, filename);
        
        // Get name without extension
        size_t name_len = dot - filename;
        char* temp_name = (char*)SDL_malloc(name_len + 1);
        SDL_memcpy(temp_name, filename, name_len);
        temp_name[name_len] = '\0';
        asset->name = to_snake_case(temp_name);
        SDL_free(temp_name);
        
        // Get extension
        asset->ext = SDL_strdup(dot + 1);
        
        // Compute hash
        size_t file_size = 0;
        void* file_data = SDL_LoadFile(asset->path, &file_size);
        if (file_data) {
            asset->hash = hash_murmur3((const uint8_t*)file_data, file_size);
            SDL_free(file_data);
        } else {
            asset->hash = 0;
        }
        
        (*count)++;
    }
    
    return 1;
}

static void write_header(Asset* assets, int count) {
    SDL_CreateDirectory("include/Empire/generated");
    SDL_IOStream* f = SDL_IOFromFile("include/Empire/generated/assets_generated.h", "w");
    if (!f) {
        SDL_Log("Failed to open assets_generated.h for writing");
        return;
    }
    
    SDL_IOprintf(f, "#pragma once\n");
    SDL_IOprintf(f, "#include \"../assets.h\"\n\n");
    SDL_IOprintf(f, "// Auto-generated file. Do not edit.\n\n");
    
    // Group by extension
    for (int ext_pass = 0; ext_pass < count; ext_pass++) {
        const char* current_ext = assets[ext_pass].ext;
        int already_written = 0;
        
        // Check if we already wrote this extension
        for (int i = 0; i < ext_pass; i++) {
            if (SDL_strcmp(assets[i].ext, current_ext) == 0) {
                already_written = 1;
                break;
            }
        }
        if (already_written) continue;
        
        // Write struct for this extension
        SDL_IOprintf(f, "typedef struct emp_generated_%s_t\n{\n", current_ext);
        
        for (int i = 0; i < count; i++) {
            if (SDL_strcmp(assets[i].ext, current_ext) == 0) {
                SDL_IOprintf(f, "    emp_asset_t %s;\n", assets[i].name);
            }
        }
        
        SDL_IOprintf(f, "} emp_generated_%s_t;\n\n", current_ext);
    }
    
    // Write main struct
    SDL_IOprintf(f, "typedef struct emp_generated_assets_o\n{\n");
    
    for (int ext_pass = 0; ext_pass < count; ext_pass++) {
        const char* current_ext = assets[ext_pass].ext;
        int already_written = 0;
        
        for (int i = 0; i < ext_pass; i++) {
            if (SDL_strcmp(assets[i].ext, current_ext) == 0) {
                already_written = 1;
                break;
            }
        }
        if (already_written) continue;
        
        SDL_IOprintf(f, "    emp_generated_%s_t %s;\n", current_ext, current_ext);
    }
    
    SDL_IOprintf(f, "} emp_generated_assets_o;\n\n");
    SDL_IOprintf(f, "emp_generated_assets_o* emp_generated_assets_create(void);\n");
    
    SDL_CloseIO(f);
}

static void write_source(Asset* assets, int count) {
    SDL_CreateDirectory("src/generated");
    SDL_IOStream* f = SDL_IOFromFile("src/generated/assets_generated.c", "w");
    if (!f) {
        SDL_Log("Failed to open assets_generated.c for writing");
        return;
    }
    
    SDL_IOprintf(f, "#include <Empire/generated/assets_generated.h>\n");
    SDL_IOprintf(f, "#include <Empire/util.h>\n");
    SDL_IOprintf(f, "#include <SDL3/SDL.h>\n\n");
    SDL_IOprintf(f, "// Auto-generated file. Do not edit.\n\n");
    
    SDL_IOprintf(f, "emp_generated_assets_o* emp_generated_assets_create(void) {\n");
    SDL_IOprintf(f, "    emp_generated_assets_o* assets = (emp_generated_assets_o*)SDL_malloc(sizeof(emp_generated_assets_o));\n\n");
    
    // Group by extension
    for (int ext_pass = 0; ext_pass < count; ext_pass++) {
        const char* current_ext = assets[ext_pass].ext;
        int already_written = 0;
        
        for (int i = 0; i < ext_pass; i++) {
            if (SDL_strcmp(assets[i].ext, current_ext) == 0) {
                already_written = 1;
                break;
            }
        }
        if (already_written) continue;
        
        SDL_IOprintf(f, "    // %s Assets\n", current_ext);
        
        for (int i = 0; i < count; i++) {
            if (SDL_strcmp(assets[i].ext, current_ext) == 0) {
                SDL_IOprintf(f, "    assets->%s.%s.path = \"%s\";\n", 
                    current_ext, assets[i].name, assets[i].path);
                SDL_IOprintf(f, "    assets->%s.%s.data = emp_read_entire_file(assets->%s.%s.path);\n",
                    current_ext, assets[i].name, current_ext, assets[i].name);
                SDL_IOprintf(f, "    assets->%s.%s.hash = 0x%016llxULL;\n", 
                    current_ext, assets[i].name, (unsigned long long)assets[i].hash);
                SDL_IOprintf(f, "\n");
            }
        }
    }
    
    SDL_IOprintf(f, "    return assets;\n");
    SDL_IOprintf(f, "}\n");
    
    SDL_CloseIO(f);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    SDL_Init(0);
    
    printf("Florence Codegen running...\n");
    
    Asset assets[MAX_ASSETS];
    int count = 0;
    
    if (!scan_directory(ASSETS_DIR, assets, &count)) {
        SDL_Log("Failed to scan assets directory");
        SDL_Quit();
        return 1;
    }
    
    printf("Found %d assets\n", count);
    
    write_header(assets, count);
    write_source(assets, count);
    
    // Cleanup
    for (int i = 0; i < count; i++) {
        SDL_free(assets[i].name);
        SDL_free(assets[i].ext);
        SDL_free(assets[i].path);
    }
    
    printf("Florence Codegen complete.\n");
    SDL_Quit();
    return 0;
}
