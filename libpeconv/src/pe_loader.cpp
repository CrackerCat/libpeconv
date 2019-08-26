#include "peconv/pe_loader.h"

#include "peconv/relocate.h"
#include "peconv/imports_loader.h"
#include "peconv/buffer_util.h"
#include "peconv/function_resolver.h"
#include "peconv/exports_lookup.h"

#include <iostream>

using namespace peconv;

BYTE* peconv::load_pe_module(BYTE* dllRawData, size_t r_size, OUT size_t &v_size, bool executable, bool relocate)
{
    // by default, allow to load the PE at any base:
    ULONGLONG desired_base = NULL;
    // if relocating is required, but the PE has no relocation table...
    if (relocate && !has_relocations(dllRawData)) {
        // ...enforce loading the PE image at its default base (so that it will need no relocations)
        desired_base = get_image_base(dllRawData);
    }
    // load a virtual image of the PE file at the desired_base address (random if desired_base is NULL):
    BYTE *mappedDLL = pe_raw_to_virtual(dllRawData, r_size, v_size, executable, desired_base);
    if (mappedDLL) {
        //if the image was loaded at its default base, relocate_module will return always true (because relocating is already done)
        if (relocate && !relocate_module(mappedDLL, v_size, (ULONGLONG)mappedDLL)) {
            // relocating was required, but it failed - thus, the full PE image is useless
            printf("Could not relocate the module!");
            free_pe_buffer(mappedDLL, v_size);
            mappedDLL = NULL;
        }
    } else {
        printf("Could not allocate memory at the desired base!\n");
    }
    return mappedDLL;
}

BYTE* peconv::load_pe_module(const char *filename, OUT size_t &v_size, bool executable, bool relocate)
{
    size_t r_size = 0;
    BYTE *dllRawData = load_file(filename, r_size);
    if (!dllRawData) {
        std::cerr << "Cannot load the file: " << filename << std::endl;
        return NULL;
    }
    BYTE* mappedDLL = load_pe_module(dllRawData, r_size, v_size, executable, relocate);
    free_pe_buffer(dllRawData);
    return mappedDLL;
}

BYTE* peconv::load_pe_executable(BYTE* dllRawData, size_t r_size, OUT size_t &v_size, t_function_resolver* import_resolver)
{
    BYTE* loaded_pe = load_pe_module(dllRawData, r_size, v_size, true, true);
    if (!loaded_pe) {
        printf("Loading failed!\n");
        return NULL;
    }
#if _DEBUG
    printf("Loaded at: %p\n", loaded_pe);
#endif
    if (!load_imports(loaded_pe, import_resolver)) {
        printf("[-] Loading imports failed!");
        free_pe_buffer(loaded_pe, v_size);
        return NULL;
    }
    return loaded_pe;
}


BYTE* peconv::load_pe_executable(const char *my_path, OUT size_t &v_size, t_function_resolver* import_resolver)
{
#if _DEBUG
    printf("Module: %s\n", my_path);
#endif
    BYTE* loaded_pe = load_pe_module(my_path, v_size, true, true);
    if (!loaded_pe) {
         printf("Loading failed!\n");
        return NULL;
    }
#if _DEBUG
    printf("Loaded at: %p\n", loaded_pe);
#endif
    if (!load_imports(loaded_pe, import_resolver)) {
        printf("[-] Loading imports failed!");
        free_pe_buffer(loaded_pe, v_size);
        return NULL;
    }
    return loaded_pe;
}
