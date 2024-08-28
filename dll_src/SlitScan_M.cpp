#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <algorithm>
#include <vector>
#include <iostream>
#include <windows.h>
#include "lua.hpp"

struct PixelBGRA {
    uint8_t b, g, r, a;
};

struct ImageData {
    int width;
    int height;
    HANDLE map;
    bool baton;

    ImageData(int w, int h) : width(w), height(h), baton(true) {
        auto bytes = w * h * sizeof(PixelBGRA);
        map = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READWRITE, 0, bytes, NULL);
        // std::cout << (map == INVALID_HANDLE_VALUE) << std::endl;
    }

    ~ImageData() noexcept {
        if (baton && map)
            CloseHandle(map);
    }

    ImageData(const ImageData&) = delete;

    ImageData(ImageData&& r) noexcept {
        width = r.width;
        height= r.height;
        map = r.map;
        baton = true;
        r.baton = false;
    }

    constexpr bool is_valid() const { return map; }

    constexpr int get_bytes() const { return width * height * sizeof(PixelBGRA); }

    constexpr bool operator==(const ImageData& r) const {
        return this->map == r.map;
    }
};

struct MappedPixelData {
    PixelBGRA* pixels;
    MappedPixelData(const ImageData& data) {
        pixels = static_cast<PixelBGRA*>(MapViewOfFile(data.map, FILE_MAP_ALL_ACCESS, 0, 0, data.get_bytes()));
    }
    ~MappedPixelData() {
        UnmapViewOfFile(pixels);
    }
    PixelBGRA& operator[](int i) const{
        return pixels[i];
    }
};

std::vector<ImageData> handles;

inline void utl_putpixeldata(lua_State *L, void* pix) {
    lua_getfield(L, -1, "putpixeldata");
    lua_pushlightuserdata(L, pix);
    lua_call(L, 1, 0);
}

inline void* utl_getpixeldata(lua_State *L) {
    lua_getfield(L, -1, "getpixeldata");
    lua_call(L, 0, 3);
    void* ptr = lua_touserdata(L, -3);
    lua_pop(L, 3);
    return ptr;
}

static int off = 0;

// push and swap
int slitscan(lua_State *L) {
    lua_getglobal(L, "obj");
    
    if (handles.size() == 0)
        return 0;
    
    int split = handles.size();

    lua_getfield(L, -1, "getpixel");
    lua_call(L, 0, 2);
    int w = lua_tointeger(L, -2);
    int h = lua_tointeger(L, -1);
    lua_pop(L, 2);

    if (w != handles[0].width || h != handles[0].height)
        return 0;

    PixelBGRA* pixel = reinterpret_cast<PixelBGRA*>(utl_getpixeldata(L));
    {
        int j = off % split;
        MappedPixelData mp(handles[j]);
        memcpy(mp.pixels, pixel, handles[j].get_bytes());
    }

    for (int i = 1; i < split; i++)
    {
        int j = (i + off) % split;
        double sh = static_cast<double>(h) / split;
        int k = sh * i;
        int l = std::min(static_cast<int>(sh * (i + 1)), h) - k;
        MappedPixelData mp(handles[j]);
        memcpy(&pixel[k * w], &mp.pixels[k * w], w * sizeof(PixelBGRA) * l);
    }

    utl_putpixeldata(L, pixel);

    off = (off - 1 + split) % split;

    return 0;
}

int alloc(lua_State *L) {
    lua_getglobal(L, "obj");
    int split = lua_tointeger(L, 1);

    lua_getfield(L, -1, "getpixel");
    lua_call(L, 0, 2);
    int w = lua_tointeger(L, -2);
    int h = lua_tointeger(L, -1);
    lua_pop(L, 2);

    handles.clear();
    for (int i = 0; i < split; i++)
    {
        ImageData id(w, h);
        MappedPixelData mp(id);
        memset(mp.pixels, 0, id.get_bytes());
        handles.push_back(std::move(id));
    }
    off = 0;
    return 0;
}

static luaL_Reg functions[] = {
    { "SlitScan", slitscan },
    { "Alloc", alloc },
    { nullptr, nullptr }
};

EXTERN_C __declspec(dllexport) int luaopen_SlitScan_M(lua_State* L) {
    luaL_register(L, "SlitScan_M", functions);
    return 1;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved ) {
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_DETACH:
            handles.clear();
            break;
    }
    return TRUE;
}

