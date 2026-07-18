#pragma once

struct Vector2 {
    float x;
    float y;
};

struct Rectangle {
    float x;
    float y;
    float width;
    float height;
};

struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

struct Texture2D {
    unsigned int id;
    int width;
    int height;
    int mipmaps;
    int format;
};
