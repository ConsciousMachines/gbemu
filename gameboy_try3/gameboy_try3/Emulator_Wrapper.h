#pragma once

#define GLEW_STATIC
#define _CRT_SECURE_NO_WARNINGS

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_memory_editor.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include "Emulator.h"

#define SCREEN_HEIGHT 800
#define SCREEN_WIDTH 1200
#define WIDTH 160
#define HEIGHT 144
#define NUM_TEXTURES 3

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY_PATTERN_SPACED "%c%c%c%c %c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 


class Emulator_Wrapper
{
public:
    Emulator_Wrapper();
    ~Emulator_Wrapper();
    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
private:
    void render_imgui_main_window();
    void render_imgui_memory_editor(MemoryEditor mem_edit);
    void render_imgui_vram_viewer();
    void render_imgui_processor();
    void render_imgui_disassembler();
    ImFont* AddDefaultFont(float pixel_size);
    Emulator* emu;
    unsigned int VBO, VAO, EBO;
    GLFWwindow* window;
    GLuint texture[NUM_TEXTURES];
    bool emulator_on = true;
    bool emulator_stepping = false;
};