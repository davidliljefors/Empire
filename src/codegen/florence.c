#include <SDL3/SDL.h>
#include <Empire/hash.inl>
#include <Empire/lz4.h>
#include <stdio.h>

#define MAX_ASSETS 4096
#define ASSETS_DIR "assets"

static uint64_t g_total_uncompressed_size = 0;

typedef struct {
    char* name;
    char* ext;
    char* path;
    uint64_t hash;
    uint64_t offset;
    uint64_t size;
} Asset;

static char* to_snake_case(const char* filename) {
    char* result = SDL_strdup(filename);
    for (char* p = result; *p; p++) {
        if (*p == '-' || *p == ' ') *p = '_';
        else if (*p >= 'A' && *p <= 'Z') *p += 32; // lowercase
    }
    return result;
}

static void upper_ext(const char* ext, char* buf)
{
    for (const char* p = ext; *p; p++, buf++) {
        *buf = SDL_toupper(*p);
    }
}

static uint64_t hash_file_contents(const char* path) {
    size_t file_size = 0;
    void* file_data = SDL_LoadFile(path, &file_size);
    if (!file_data) return 0;
    
    uint64_t hash = hash_murmur3((const uint8_t*)file_data, file_size);
    SDL_free(file_data);
    return hash;
}

static uint64_t compute_assets_checksum(Asset* assets, int count) {
	u64 seed = hash_str(__DATE__ __TIME__);
    uint64_t combined = seed;
    for (int i = 0; i < count; i++) {
        combined ^= assets[i].hash;
        combined = (combined << 13) | (combined >> 51);
    }
    return combined;
}

static uint64_t compute_output_checksum(const char* header_path, const char* source_path) {
    // Hash header file, excluding the output checksum line to avoid circular dependency
    size_t header_size = 0;
    char* header_content = (char*)SDL_LoadFile(header_path, &header_size);
    uint64_t header_hash = 0;
    
    if (header_content) {
        // Find and temporarily replace the output checksum line with placeholder
        char search[] = "#define GENERATED_OUTPUT_CHECKSUM";
        char* pos = SDL_strstr(header_content, search);
        char saved[128] = {0};
        
        if (pos) {
            char* line_end = SDL_strchr(pos, '\n');
            if (line_end) {
                size_t line_len = line_end - pos;
                if (line_len < sizeof(saved)) {
                    SDL_memcpy(saved, pos, line_len);
                    SDL_memcpy(pos, "#define GENERATED_OUTPUT_CHECKSUM 0x0000000000000000ULL", 57);
                }
            }
        }
        
        header_hash = hash_murmur3((const uint8_t*)header_content, header_size);
        SDL_free(header_content);
    }
    
    uint64_t source_hash = hash_file_contents(source_path);
    
    // Combine both hashes
    uint64_t combined = header_hash ^ source_hash;
    combined = (combined << 13) | (combined >> 51);
    return combined;
}

static void read_existing_checksums(const char* header_path, uint64_t* input_checksum, uint64_t* output_checksum) {
    *input_checksum = 0;
    *output_checksum = 0;
    
    size_t file_size = 0;
    char* content = (char*)SDL_LoadFile(header_path, &file_size);
    if (!content) return;
    
    char* line = content;
    while (line && *line) {
        char* next_line = SDL_strchr(line, '\n');
        if (next_line) {
            *next_line = '\0';
            next_line++;
        }
        
        if (SDL_strncmp(line, "#define", 7) == 0) {
            char define_name[128];
            unsigned long long value;
            if (SDL_sscanf(line, "#define %127s 0x%llx", define_name, &value) == 2) {
                if (SDL_strcmp(define_name, "GENERATED_ASSETS_CHECKSUM") == 0) {
                    *input_checksum = value;
                } else if (SDL_strcmp(define_name, "GENERATED_OUTPUT_CHECKSUM") == 0) {
                    *output_checksum = value;
                }
            }
        }
        
        line = next_line;
    }
    
    SDL_free(content);
}

static int needs_regeneration(uint64_t new_input_checksum, const char* header_path, const char* source_path) {
    uint64_t existing_input_checksum = 0;
    uint64_t existing_output_checksum = 0;
    read_existing_checksums(header_path, &existing_input_checksum, &existing_output_checksum);
    
    uint64_t current_output_checksum = compute_output_checksum(header_path, source_path);
    
    if (new_input_checksum == existing_input_checksum && 
        existing_input_checksum != 0 &&
        current_output_checksum == existing_output_checksum &&
        existing_output_checksum != 0) {
        printf("Assets unchanged and output unmodified, skipping generation.\n");
        return 0;
    }
    
    if (current_output_checksum != existing_output_checksum && existing_output_checksum != 0) {
        printf("Generated files were modified, regenerating...\n");
    }
    
    return 1;
}

static void update_output_checksum(const char* header_path, const char* source_path) {
    uint64_t final_output_checksum = compute_output_checksum(header_path, source_path);
    
    size_t header_size = 0;
    char* header_content = (char*)SDL_LoadFile(header_path, &header_size);
    if (!header_content) return;
    
    char search[] = "#define GENERATED_OUTPUT_CHECKSUM 0x0000000000000000ULL";
    char replace[128];
    SDL_snprintf(replace, sizeof(replace), "#define GENERATED_OUTPUT_CHECKSUM 0x%016llxULL", 
                (unsigned long long)final_output_checksum);
    
    char* pos = SDL_strstr(header_content, search);
    if (pos) {
        SDL_IOStream* f = SDL_IOFromFile(header_path, "w");
        if (f) {
            size_t before_len = pos - header_content;
            for (size_t i = 0; i < before_len; i++) {
                SDL_IOprintf(f, "%c", header_content[i]);
            }
            SDL_IOprintf(f, "%s", replace);
            const char* after = pos + SDL_strlen(search);
            SDL_IOprintf(f, "%s", after);
            
            SDL_CloseIO(f);
        }
    }
    SDL_free(header_content);
    
    printf("Output checksum: 0x%016llx\n", (unsigned long long)final_output_checksum);
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

static void write_header(Asset* assets, int count, uint64_t checksum) {
    SDL_CreateDirectory("include/Empire/generated");
    SDL_IOStream* f = SDL_IOFromFile("include/Empire/generated/assets_generated.h", "w");
    if (!f) {
        SDL_Log("Failed to open assets_generated.h for writing");
        return;
    }
    
    SDL_IOprintf(f, "#pragma once\n");
    SDL_IOprintf(f, "#include \"../assets.h\"\n\n");
    SDL_IOprintf(f, "// Auto-generated file. Do not edit.\n\n");
    SDL_IOprintf(f, "#define GENERATED_ASSETS_CHECKSUM 0x%016llxULL\n", (unsigned long long)checksum);
    SDL_IOprintf(f, "#define GENERATED_OUTPUT_CHECKSUM 0x0000000000000000ULL\n\n");
    
#ifdef FLORENCE_PACKAGE_ASSETS
    SDL_IOprintf(f, "#define FLORENCE_PACKAGE_ASSETS 1\n");
    SDL_IOprintf(f, "#define PACKAGED_UNCOMPRESSED_SIZE %lluULL\n\n", (unsigned long long)g_total_uncompressed_size);
#endif
    
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
        
        char uppercase_ext_buf[64]= {0};
        upper_ext(current_ext, uppercase_ext_buf);
        uint64_t ext_hash = hash_str(uppercase_ext_buf);
        SDL_IOprintf(f, "#define EMP_ASSET_TYPE_%s 0x%016llxULL\n\n", uppercase_ext_buf, ext_hash);
        SDL_IOprintf(f, "typedef struct emp_generated_%s_t\n{\n", current_ext);
        
        int asset_count = 0;
        for (int i = 0; i < count; i++) {
            if (SDL_strcmp(assets[i].ext, current_ext) == 0) {
                asset_count++;
            }
        }
        
        SDL_IOprintf(f, "    int count;\n");
        SDL_IOprintf(f, "    u64 type_hash;\n");
        
        for (int i = 0; i < count; i++) {
            if (SDL_strcmp(assets[i].ext, current_ext) == 0) {
                SDL_IOprintf(f, "    emp_asset_t %s;\n", assets[i].name);
            }
        }
        
        SDL_IOprintf(f, "} emp_generated_%s_t;\n\n", current_ext);
    }
    
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
        
        SDL_IOprintf(f, "    emp_generated_%s_t* %s;\n", current_ext, current_ext);
    }
    
#ifdef FLORENCE_PACKAGE_ASSETS
    SDL_IOprintf(f, "    u8* packaged_blob;\n");
#endif
    SDL_IOprintf(f, "} emp_generated_assets_o;\n\n");
    SDL_IOprintf(f, "emp_generated_assets_o* emp_generated_assets_create(const char* root);\n");
    
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
    SDL_IOprintf(f, "// Auto-generated file. Do not edit.\n\n");
    SDL_IOprintf(f, "#include <SDL3/SDL.h>\n");
#ifdef FLORENCE_PACKAGE_ASSETS
    SDL_IOprintf(f, "#include <Empire/lz4.h>\n");
#endif
    SDL_IOprintf(f, "\n");
    
    SDL_IOprintf(f, "emp_generated_assets_o* emp_generated_assets_create(const char* root) {\n");
    SDL_IOprintf(f, "    emp_generated_assets_o* assets = (emp_generated_assets_o*)SDL_malloc(sizeof(emp_generated_assets_o));\n");
    SDL_IOprintf(f, "    SDL_memset(assets, 0, sizeof(emp_generated_assets_o));\n\n");

#ifdef FLORENCE_PACKAGE_ASSETS
    SDL_IOprintf(f, "    size_t compressed_size = 0;\n");
    SDL_IOprintf(f, "    const char* pkg_path = emp_concat(root, \"assets.bin\");\n");
    SDL_IOprintf(f, "    void* compressed = SDL_LoadFile(pkg_path, &compressed_size);\n");
    SDL_IOprintf(f, "    SDL_free((void*)pkg_path);\n");
    SDL_IOprintf(f, "    assets->packaged_blob = (u8*)SDL_malloc(PACKAGED_UNCOMPRESSED_SIZE);\n");
    SDL_IOprintf(f, "    LZ4_decompress_safe((const char*)compressed, (char*)assets->packaged_blob, (int)compressed_size, (int)PACKAGED_UNCOMPRESSED_SIZE);\n");
    SDL_IOprintf(f, "    SDL_free(compressed);\n\n");
#endif
    
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
        
        int asset_count = 0;
        for (int i = 0; i < count; i++) {
            if (SDL_strcmp(assets[i].ext, current_ext) == 0) {
                asset_count++;
            }
        }
        
        char uppercase_ext_buf[64]= {0};
        upper_ext(current_ext, uppercase_ext_buf);
        uint64_t ext_hash = hash_str(uppercase_ext_buf);
        
        SDL_IOprintf(f, "    assets->%s = (emp_generated_%s_t*)SDL_malloc(sizeof(emp_generated_%s_t));\n",
            current_ext, current_ext, current_ext);
        SDL_IOprintf(f, "    SDL_memset(assets->%s, 0, sizeof(emp_generated_%s_t));\n", current_ext, current_ext);
        SDL_IOprintf(f, "    assets->%s->count = %d;\n", current_ext, asset_count);
        SDL_IOprintf(f, "    assets->%s->type_hash = 0x%016llxULL;\n", current_ext, (unsigned long long)ext_hash);
    }
    SDL_IOprintf(f, "\n");
    
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
        
        for (int i = 0; i < count; i++) {
            if (SDL_strcmp(assets[i].ext, current_ext) == 0) {
#ifdef FLORENCE_PACKAGE_ASSETS
                SDL_IOprintf(f, "    assets->%s->%s.path = \"%s\";\n", 
                    current_ext, assets[i].name, assets[i].path);
                SDL_IOprintf(f, "    assets->%s->%s.data.data = assets->packaged_blob + %lluULL;\n",
                    current_ext, assets[i].name, (unsigned long long)assets[i].offset);
                SDL_IOprintf(f, "    assets->%s->%s.data.size = %lluULL;\n",
                    current_ext, assets[i].name, (unsigned long long)assets[i].size);
#else
                SDL_IOprintf(f, "    assets->%s->%s.path = emp_concat(root, \"%s\");\n", 
                    current_ext, assets[i].name, assets[i].path);
                SDL_IOprintf(f, "    assets->%s->%s.data = emp_read_entire_file(assets->%s->%s.path);\n",
                    current_ext, assets[i].name, current_ext, assets[i].name);
#endif
                SDL_IOprintf(f, "    assets->%s->%s.hash = 0x%016llxULL;\n", 
                    current_ext, assets[i].name, (unsigned long long)assets[i].hash);
                SDL_IOprintf(f, "\n");
            }
        }
    }
    
    SDL_IOprintf(f, "    return assets;\n");
    SDL_IOprintf(f, "}\n");
    
    SDL_CloseIO(f);
}

#ifdef FLORENCE_PACKAGE_ASSETS
static void write_package(Asset* assets, int count) {
    uint64_t total_size = 0;
    for (int i = 0; i < count; i++) {
        size_t file_size = 0;
        void* data = SDL_LoadFile(assets[i].path, &file_size);
        if (data) {
            assets[i].offset = total_size;
            assets[i].size = file_size;
            total_size += file_size;
            SDL_free(data);
        }
    }
    
    g_total_uncompressed_size = total_size;
    
    uint8_t* blob = (uint8_t*)SDL_malloc(total_size);
    uint64_t write_offset = 0;
    
    for (int i = 0; i < count; i++) {
        size_t file_size = 0;
        void* data = SDL_LoadFile(assets[i].path, &file_size);
        if (data) {
            SDL_memcpy(blob + write_offset, data, file_size);
            write_offset += file_size;
            SDL_free(data);
        }
    }
    
    int max_compressed = LZ4_compressBound((int)total_size);
    char* compressed = (char*)SDL_malloc(max_compressed);
    int compressed_size = LZ4_compress_default((const char*)blob, compressed, (int)total_size, max_compressed);
    
    SDL_IOStream* f = SDL_IOFromFile("assets.bin", "wb");
    if (f) {
        SDL_WriteIO(f, compressed, compressed_size);
        SDL_CloseIO(f);
        printf("Packaged %d assets: %llu bytes -> %d bytes (%.1f%%)\n", 
            count, (unsigned long long)total_size, compressed_size, 
            100.0f * compressed_size / total_size);
    }
    
    SDL_free(blob);
    SDL_free(compressed);
}
#endif

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
    
    const char* header_path = "include/Empire/generated/assets_generated.h";
    const char* source_path = "src/generated/assets_generated.c";
    
    uint64_t new_input_checksum = compute_assets_checksum(assets, count);
    
    if (!needs_regeneration(new_input_checksum, header_path, source_path)) {
        for (int i = 0; i < count; i++) {
            SDL_free(assets[i].name);
            SDL_free(assets[i].ext);
            SDL_free(assets[i].path);
        }
        SDL_Quit();
        return 0;
    }
    
    printf("Generating assets (input checksum: 0x%016llx)...\n", (unsigned long long)new_input_checksum);
    
#ifdef FLORENCE_PACKAGE_ASSETS
    write_package(assets, count);
#endif
    write_header(assets, count, new_input_checksum);
    write_source(assets, count);
    update_output_checksum(header_path, source_path);
    
    for (int i = 0; i < count; i++) {
        SDL_free(assets[i].name);
        SDL_free(assets[i].ext);
        SDL_free(assets[i].path);
    }
    
    printf("Florence Codegen complete.\n");
    SDL_Quit();
    return 0;
}
