// things i learned:
// - memory space 0 - 7fff doesnt even exist, it goes straight into the cartridge! from there mapped to
//      individual memory chips. similarly for RAM at a000 - bfff

// remember to include opengl32.lib in the project, even in 64bit (lol)!!!
#define GLEW_STATIC
#define _CRT_SECURE_NO_WARNINGS

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_memory_editor.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <fstream> // for shader compilation
#include <sstream>

#include "disassembler.h"
#include "Emulator.h"

#define SCREEN_HEIGHT 800
#define SCREEN_WIDTH 1200

#define WIDTH 160//256 //160
#define HEIGHT 144//256 //144

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

static ImVec4 cyan = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
static ImVec4 magenta = ImVec4(1.0f, 0.502f, 0.957f, 1.0f);
static ImVec4 yellow = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
static ImVec4 red = ImVec4(1.0f, 0.149f, 0.447f, 1.0f);
static ImVec4 green = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
static ImVec4 white = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
static ImVec4 gray = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
static ImVec4 dark_gray = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);

class Emulator_Wrapper // this class is necessary so that key_callback has access to m_Emulator 
{
public:
    Emulator_Wrapper();
    ~Emulator_Wrapper();
    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
private:
    Emulator* emu;
    unsigned int VBO, VAO, EBO;
    GLFWwindow* window;
    GLuint texture[NUM_TEXTURES];
    bool emulator_on = false;
    bool emulator_stepping = false;
};







ImFont* AddDefaultFont(float pixel_size) // Chad https://github.com/ocornut/imgui/issues/1018
{
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig config;
    config.SizePixels = pixel_size;
    config.OversampleH = config.OversampleV = 2;
    config.PixelSnapH = true;
    ImFont* font = io.Fonts->AddFontDefault(&config);
    return font;
}





// open-GL
#pragma region
class Shader
{
public:
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath)
    {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            fprintf(stderr, "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ\n");
            //std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
    // activate the shader
    // ------------------------------------------------------------------------
    void use()
    {
        glUseProgram(ID);
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                fprintf(stderr, "ERROR::SHADER_COMPILATION_ERROR of type: ");
                //std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                fprintf(stderr, "ERROR::PROGRAM_LINKING_ERROR of type: ");
                //std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};
// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
#pragma endregion


void Emulator_Wrapper::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        int key_code = -1;
        switch (key)
        {
        case GLFW_KEY_A: key_code = 4; break;
        case GLFW_KEY_S: key_code = 5; break;
        case GLFW_KEY_Z: key_code = 7; break;
        case GLFW_KEY_X: key_code = 6; break;
        case GLFW_KEY_RIGHT: key_code = 0; break;
        case GLFW_KEY_LEFT: key_code = 1; break;
        case GLFW_KEY_UP: key_code = 2; break;
        case GLFW_KEY_DOWN: key_code = 3; break;

        case GLFW_KEY_V: emu->run_step(); break;
        case GLFW_KEY_B: emulator_stepping = true; break;
        case GLFW_KEY_P: emulator_on = !emulator_on; break;
        case GLFW_KEY_ESCAPE: exit(69); break;
        }
        if (key_code != -1) emu->key_pressed(key_code);
    }
    else if (action == GLFW_RELEASE)
    {
        int key_code = -1;
        switch (key)
        {
        case GLFW_KEY_A: key_code = 4; break;
        case GLFW_KEY_S: key_code = 5; break;
        case GLFW_KEY_Z: key_code = 7; break;
        case GLFW_KEY_X: key_code = 6; break;
        case GLFW_KEY_RIGHT: key_code = 0; break;
        case GLFW_KEY_LEFT: key_code = 1; break;
        case GLFW_KEY_UP: key_code = 2; break;
        case GLFW_KEY_DOWN: key_code = 3; break;

        case GLFW_KEY_B: emulator_stepping = false; break;
        }
        if (key_code != -1) emu->key_released(key_code);
    }
}


Emulator_Wrapper::Emulator_Wrapper() : VBO(0), VAO(0), EBO(0), window(0), emu(0), texture()
{
#pragma region opengl stuff 
    // glfw & glew
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Soy", NULL, NULL);
    if (window == NULL) return;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    // https://stackoverflow.com/questions/7676971/pointing-to-a-function-that-is-a-class-member-glfw-setkeycallback
    glfwSetWindowUserPointer(window, this);
    auto func = [](GLFWwindow* w, int a, int b, int c, int d)
    {static_cast<Emulator_Wrapper*>(glfwGetWindowUserPointer(w))->key_callback(w, a, b, c, d); };
    glfwSetKeyCallback(window, func);
    /*
    if (glewInit() != GLEW_OK) return;
    // build and compile our shader program
    Shader ourShader = Shader("C:\\Users\\pwnag\\source\\repos\\chip8\\4.1.texture.vs", "C:\\Users\\pwnag\\source\\repos\\chip8\\4.1.texture.fs");
    ourShader.use();
    // set up vertex data (and buffer(s)) and configure vertex attributes
    float vertices[] = {
        // positions          // colors           // texture coords
         1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
         1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
        -1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left
    };
    unsigned int indices[] = { 0, 1, 3, 1, 2, 3 };
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    */
#pragma endregion
    // load and create a texture 
    // -------------------------
    glGenTextures(NUM_TEXTURES, texture);
    for (int i = 0; i < NUM_TEXTURES; i++)
    {
        glBindTexture(GL_TEXTURE_2D, texture[i]); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    //emu = new Emulator("C:\\Users\\pwnag\\Desktop\\retro\\my_GBC\\10-bit ops.gb");
    emu = new Emulator("C:\\Users\\pwnag\\Desktop\\retro\\my_GBC\\GBTICTAC.GB");
    //emu = new Emulator("C:\\Users\\pwnag\\Desktop\\retro\\my_GBC\\Tetris (World) (Rev 1).gb");
    //emu = new Emulator("C:\\Users\\pwnag\\Desktop\\retro\\my_GBC\\opus5.gb");
    //Pokemon Blue.gb
    //Legend of Zelda, The - Link's Awakening.gb


    // start loop
    double fps = 60;
    double interval = 1000 / fps;
    double time = glfwGetTime() * 1000;
    bool show_demo_window = true;
    static MemoryEditor mem_edit;


    ImFont* fontC = AddDefaultFont(16);


    for (;;)
    {
        if (emulator_on) emu->run_frame();
        if (emulator_stepping) emu->run_step();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::PushFont(fontC);

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        //if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

        // main window 
        {
            glBindTexture(GL_TEXTURE_2D, texture[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, emu->gpu_screen_data);
            ImGui::Begin("GameWindow"); // https://gamedev.stackexchange.com/questions/140693/how-can-i-render-an-opengl-scene-into-an-imgui-window
            {
                ImVec2 size = ImVec2(160 * 2, 144 * 2);
                ImGui::Image((ImTextureID)texture[0], size);
            }
            ImGui::End();
        }

        // memory editor 
        {
            ImGui::Begin("Memory Editor");
            /*
            ImGui::TextColored(cyan, "  BANKS: "); ImGui::SameLine();
            ImGui::TextColored(magenta, "ROM1"); ImGui::SameLine();
            ImGui::Text("$%02X", 1); ImGui::SameLine();
            ImGui::TextColored(magenta, "  RAM"); ImGui::SameLine();
            ImGui::Text("$%02X", 0); ImGui::SameLine();
            ImGui::TextColored(magenta, "  WRAM1"); ImGui::SameLine();
            ImGui::Text("$%02X", 1); ImGui::SameLine();
            ImGui::TextColored(magenta, "  VRAM"); ImGui::SameLine();
            ImGui::Text("$%02X", 0);
            */
            if (ImGui::BeginTabBar("##memory_tabs", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem("ROM0"))
                {
                    mem_edit.DrawContents(emu->mem_cart, 0x4000, 0);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("ROM1"))
                {
                    mem_edit.DrawContents(&emu->mem_cart[0x4000], 0x4000, 0x4000);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("VRAM"))
                {
                    mem_edit.DrawContents(emu->mem_vram, 0x2000, 0x8000);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("ERAM"))
                {
                    mem_edit.DrawContents(emu->mem_eram, 0x2000, 0xA000);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("WRAM"))
                {
                    mem_edit.DrawContents(emu->mem_wram, 0x2000, 0xC000);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("OAM"))
                {
                    mem_edit.DrawContents(emu->mem_oam, 0x00A0, 0xFE00);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("HIRAM"))
                {
                    mem_edit.DrawContents(emu->mem_hram, 0x007F, 0xFF80);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::End();
        }

        // vram viewer
        {
            ImGui::Begin("VRAM Viewer");
            if (ImGui::BeginTabBar("##memory_tabs", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem("chill"))
                {
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("background"))
                {
                    emu->gpu_generate_background();
                    glBindTexture(GL_TEXTURE_2D, texture[1]);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, emu->gpu_background);

                    ImVec2 size = ImVec2(256 * 2, 256 * 2);
                    ImGui::Image((ImTextureID)texture[1], size);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("tileset"))
                {
                    emu->gpu_generate_tileset_view();
                    glBindTexture(GL_TEXTURE_2D, texture[2]);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 192, 0, GL_RGBA, GL_UNSIGNED_BYTE, emu->gpu_tileset_view);

                    ImVec2 size = ImVec2(128 * 2, 192 * 2);
                    ImGui::Image((ImTextureID)texture[2], size);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::End();
        }

        // processor 
        {
            ImGui::Begin("Processor");
            ImGui::Separator();
            ImGui::TextColored(magenta, "   Z"); ImGui::SameLine(); // flags 
            ImGui::Text("= %d", (bool)(emu->flags.cc.z)); ImGui::SameLine();
            ImGui::TextColored(magenta, "  N"); ImGui::SameLine();
            ImGui::Text("= %d", (bool)(emu->flags.cc.n));
            ImGui::TextColored(magenta, "   H"); ImGui::SameLine();
            ImGui::Text("= %d", (bool)(emu->flags.cc.h)); ImGui::SameLine();
            ImGui::TextColored(magenta, "  C"); ImGui::SameLine();
            ImGui::Text("= %d", (bool)(emu->flags.cc.c));

            ImGui::Columns(2, "registers"); // regs 
            ImGui::Separator();
            ImGui::TextColored(cyan, " A"); ImGui::SameLine();
            ImGui::Text("= $%02X", emu->regs[0]);
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(emu->regs[0]));
            ImGui::NextColumn();
            ImGui::TextColored(cyan, " F"); ImGui::SameLine();
            ImGui::Text("= $%02X", emu->flags.psw);
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(emu->flags.psw));
            ImGui::NextColumn();
            ImGui::Separator();
            ImGui::TextColored(cyan, " B"); ImGui::SameLine();
            ImGui::Text("= $%02X", emu->regs[1]);
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(emu->regs[1]));
            ImGui::NextColumn();
            ImGui::TextColored(cyan, " C"); ImGui::SameLine();
            ImGui::Text("= $%02X", emu->regs[2]);
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(emu->regs[2]));
            ImGui::NextColumn();
            ImGui::Separator();
            ImGui::TextColored(cyan, " D"); ImGui::SameLine();
            ImGui::Text("= $%02X", emu->regs[3]);
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(emu->regs[3]));
            ImGui::NextColumn();
            ImGui::TextColored(cyan, " E"); ImGui::SameLine();
            ImGui::Text("= $%02X", emu->regs[4]);
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(emu->regs[4]));
            ImGui::NextColumn();
            ImGui::Separator();
            ImGui::TextColored(cyan, " H"); ImGui::SameLine();
            ImGui::Text("= $%02X", emu->regs[5]);
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(emu->regs[5]));
            ImGui::NextColumn();
            ImGui::TextColored(cyan, " L"); ImGui::SameLine();
            ImGui::Text("= $%02X", emu->regs[6]);
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(emu->regs[6]));
            ImGui::NextColumn();
            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::TextColored(yellow, "    SP"); ImGui::SameLine();
            ImGui::Text("= $%04X", emu->sp);
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY((emu->sp >> 8)), BYTE_TO_BINARY(((uint8_t)emu->sp)));
            ImGui::Separator();
            ImGui::TextColored(yellow, "    PC"); ImGui::SameLine();
            ImGui::Text("= $%04X", emu->pc);
            ImGui::Text(BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(emu->pc >> 8), BYTE_TO_BINARY(((uint8_t)emu->pc)));


            ImGui::Columns(2);
            ImGui::Separator();
            ImGui::TextColored(magenta, " IME"); ImGui::SameLine();
            ImGui::Text("= %d", emu->system_master_interrupt_en);
            ImGui::NextColumn();
            ImGui::TextColored(magenta, "HALT"); ImGui::SameLine();
            ImGui::Text("= %d", emu->system_is_halted);

            ImGui::Columns(1);

            ImGui::TextColored(yellow, "INTERRUPTS:");

            ImGui::TextColored(cyan, " $FFFF"); ImGui::SameLine();
            ImGui::TextColored(magenta, "IE  "); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FFFF_interrupt_en, BYTE_TO_BINARY(emu->reg_FFFF_interrupt_en));

            ImGui::TextColored(cyan, " $FF0F"); ImGui::SameLine();
            ImGui::TextColored(magenta, "IF  "); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF0F_interrupt_flag, BYTE_TO_BINARY(emu->reg_FF0F_interrupt_flag));
            /*
            ImGui::TextColored(cyan, " VBLNK  "); ImGui::SameLine();
            get_bit(emu->reg_FF0F_interrupt_flag, 0) && get_bit(emu->reg_FFFF_interrupt_en, 0) ? ImGui::TextColored(green, "ON  ") : ImGui::TextColored(gray, "OFF "); ImGui::SameLine();
            ImGui::TextColored(magenta, "IF:"); ImGui::SameLine();
            ImGui::Text("%d", get_bit(emu->reg_FF0F_interrupt_flag, 0)); ImGui::SameLine();
            ImGui::TextColored(magenta, "  IE:"); ImGui::SameLine();
            ImGui::Text("%d", get_bit(emu->reg_FFFF_interrupt_en, 0));

            ImGui::TextColored(cyan, " STAT   "); ImGui::SameLine();
            get_bit(emu->reg_FF0F_interrupt_flag, 1) && get_bit(emu->reg_FFFF_interrupt_en, 1) ? ImGui::TextColored(green, "ON  ") : ImGui::TextColored(gray, "OFF "); ImGui::SameLine();
            ImGui::TextColored(magenta, "IF:"); ImGui::SameLine();
            ImGui::Text("%d", get_bit(emu->reg_FF0F_interrupt_flag, 1)); ImGui::SameLine();
            ImGui::TextColored(magenta, "  IE:"); ImGui::SameLine();
            ImGui::Text("%d", get_bit(emu->reg_FFFF_interrupt_en, 1));

            ImGui::TextColored(cyan, " TIMER  "); ImGui::SameLine();
            get_bit(emu->reg_FF0F_interrupt_flag, 2) && get_bit(emu->reg_FFFF_interrupt_en, 2) ? ImGui::TextColored(green, "ON  ") : ImGui::TextColored(gray, "OFF "); ImGui::SameLine();
            ImGui::TextColored(magenta, "IF:"); ImGui::SameLine();
            ImGui::Text("%d", get_bit(emu->reg_FF0F_interrupt_flag, 2)); ImGui::SameLine();
            ImGui::TextColored(magenta, "  IE:"); ImGui::SameLine();
            ImGui::Text("%d", get_bit(emu->reg_FFFF_interrupt_en, 2));

            ImGui::TextColored(cyan, " SERIAL "); ImGui::SameLine();
            get_bit(emu->reg_FF0F_interrupt_flag, 3) && get_bit(emu->reg_FFFF_interrupt_en, 3) ? ImGui::TextColored(green, "ON  ") : ImGui::TextColored(gray, "OFF "); ImGui::SameLine();
            ImGui::TextColored(magenta, "IF:"); ImGui::SameLine();
            ImGui::Text("%d", get_bit(emu->reg_FF0F_interrupt_flag, 3)); ImGui::SameLine();
            ImGui::TextColored(magenta, "  IE:"); ImGui::SameLine();
            ImGui::Text("%d", get_bit(emu->reg_FFFF_interrupt_en, 3));

            ImGui::TextColored(cyan, " JOYPAD "); ImGui::SameLine();
            get_bit(emu->reg_FF0F_interrupt_flag, 4) && get_bit(emu->reg_FFFF_interrupt_en, 4) ? ImGui::TextColored(green, "ON  ") : ImGui::TextColored(gray, "OFF "); ImGui::SameLine();
            ImGui::TextColored(magenta, "IF:"); ImGui::SameLine();
            ImGui::Text("%d", get_bit(emu->reg_FF0F_interrupt_flag, 4)); ImGui::SameLine();
            ImGui::TextColored(magenta, "  IE:"); ImGui::SameLine();
            ImGui::Text("%d", get_bit(emu->reg_FFFF_interrupt_en, 4));
            */

            ImGui::TextColored(yellow, "LCD:");

            ImGui::TextColored(cyan, " $FF40"); ImGui::SameLine();
            ImGui::TextColored(magenta, "LCDC"); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF40_LCD_ctrl, BYTE_TO_BINARY(emu->reg_FF40_LCD_ctrl));

            ImGui::TextColored(cyan, " $FF41"); ImGui::SameLine();
            ImGui::TextColored(magenta, "STAT"); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF41_LCD_stat, BYTE_TO_BINARY(emu->reg_FF41_LCD_stat));

            ImGui::TextColored(cyan, " $FF42"); ImGui::SameLine();
            ImGui::TextColored(magenta, "SCY "); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF42_scroll_Y, BYTE_TO_BINARY(emu->reg_FF42_scroll_Y));

            ImGui::TextColored(cyan, " $FF43"); ImGui::SameLine();
            ImGui::TextColored(magenta, "SCX "); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF43_scroll_X, BYTE_TO_BINARY(emu->reg_FF43_scroll_X));

            ImGui::TextColored(cyan, " $FF44"); ImGui::SameLine();
            ImGui::TextColored(magenta, "LY  "); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF44_lineY, BYTE_TO_BINARY(emu->reg_FF44_lineY));

            ImGui::TextColored(cyan, " $FF45"); ImGui::SameLine();
            ImGui::TextColored(magenta, "LYC "); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF45_lineY_compare, BYTE_TO_BINARY(emu->reg_FF45_lineY_compare));

            ImGui::TextColored(cyan, " $FF46"); ImGui::SameLine();
            ImGui::TextColored(magenta, "DMA "); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF46_dma, BYTE_TO_BINARY(emu->reg_FF46_dma));

            ImGui::TextColored(cyan, " $FF47"); ImGui::SameLine();
            ImGui::TextColored(magenta, "BGP "); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF47_bg_palette, BYTE_TO_BINARY(emu->reg_FF47_bg_palette));

            ImGui::TextColored(cyan, " $FF48"); ImGui::SameLine();
            ImGui::TextColored(magenta, "OBP0"); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF48_obj_palette_0, BYTE_TO_BINARY(emu->reg_FF48_obj_palette_0));

            ImGui::TextColored(cyan, " $FF49"); ImGui::SameLine();
            ImGui::TextColored(magenta, "OBP1"); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF49_obj_palette_1, BYTE_TO_BINARY(emu->reg_FF49_obj_palette_1));

            ImGui::TextColored(cyan, " $FF4A"); ImGui::SameLine();
            ImGui::TextColored(magenta, "WY  "); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF4A_window_Y, BYTE_TO_BINARY(emu->reg_FF4A_window_Y));

            ImGui::TextColored(cyan, " $FF4B"); ImGui::SameLine();
            ImGui::TextColored(magenta, "WX  "); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF4B_window_X, BYTE_TO_BINARY(emu->reg_FF4B_window_X));
            /*
            ImGui::TextColored(yellow, "TIMER:");

            ImGui::TextColored(cyan, " $FF04"); ImGui::SameLine();
            ImGui::TextColored(magenta, "DIV "); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF04_div, BYTE_TO_BINARY(emu->reg_FF04_div));

            ImGui::TextColored(cyan, " $FF05"); ImGui::SameLine();
            ImGui::TextColored(magenta, "TIMA"); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF05_tima, BYTE_TO_BINARY(emu->reg_FF05_tima));

            ImGui::TextColored(cyan, " $FF06"); ImGui::SameLine();
            ImGui::TextColored(magenta, "TMA "); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF06_tma, BYTE_TO_BINARY(emu->reg_FF06_tma));

            ImGui::TextColored(cyan, " $FF07"); ImGui::SameLine();
            ImGui::TextColored(magenta, "TAC "); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF07_tac, BYTE_TO_BINARY(emu->reg_FF07_tac));
            */

            ImGui::TextColored(yellow, "INPUT:");

            ImGui::TextColored(cyan, " $FF00"); ImGui::SameLine();
            ImGui::TextColored(magenta, "JOYP"); ImGui::SameLine();
            ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF00_joypad, BYTE_TO_BINARY(emu->reg_FF00_joypad));

            ImGui::End();
        }

        // Reuben disassembler 
        {
            ImGui::Begin("Reuben Disassembler");

            u16 dis_pc = emu->pc;

            dis_line r = disassemble(emu->mem_cart, dis_pc);
            ImGui::TextColored(cyan, "%04x", r._address); ImGui::SameLine();
            switch (r._size)
            {
            case 1: ImGui::TextColored(gray, "%02x      ", r._bytes[0]); ImGui::SameLine(); break;
            case 2: ImGui::TextColored(gray, "%02x %02x   ", r._bytes[0], r._bytes[1]); ImGui::SameLine(); break;
            case 3: ImGui::TextColored(gray, "%02x %02x %02x", r._bytes[0], r._bytes[1], r._bytes[2]); ImGui::SameLine(); break;
            }
            ImGui::TextColored(red, "->"); ImGui::SameLine();
            ImGui::TextColored(yellow, "%s\n", r._text);
            dis_pc += r._size;

            const int start = dis_pc;
            for (int i = start; i < start + 30; i++)
            {
                dis_line r = disassemble(emu->mem_cart, dis_pc);
                ImGui::TextColored(cyan, "%04x", r._address); ImGui::SameLine();
                switch (r._size)
                {
                case 1: ImGui::TextColored(gray, "%02x      ", r._bytes[0]); ImGui::SameLine(); break;
                case 2: ImGui::TextColored(gray, "%02x %02x   ", r._bytes[0], r._bytes[1]); ImGui::SameLine(); break;
                case 3: ImGui::TextColored(gray, "%02x %02x %02x", r._bytes[0], r._bytes[1], r._bytes[2]); ImGui::SameLine(); break;
                }
                ImGui::TextColored(yellow, "   %s\n", r._text);
                dis_pc += r._size;
            }
            ImGui::End();
        }

        /*
        // DRAW TO SCREEN
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, dada);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        */

        ImGui::PopFont();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        // INPUT 
        glfwPollEvents();
    }
}

Emulator_Wrapper::~Emulator_Wrapper()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glfwTerminate();
}


int main(int argc, char* argv[])
{
    Emulator_Wrapper* gb = new Emulator_Wrapper();
    return 0;
}
