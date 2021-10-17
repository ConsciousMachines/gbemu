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
#include "disassembler.h"
#include <string>

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

enum class EMULATOR_WRAPPER_STATE { ON, OFF, STEP_CODE };

class Emulator_Wrapper
{
public:
    Emulator_Wrapper();
    ~Emulator_Wrapper();
    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
private:
    EMULATOR_WRAPPER_STATE emu_wrap_state;
    void render_imgui_main_window();
    void render_imgui_memory_editor(MemoryEditor mem_edit);
    void render_imgui_vram_viewer();
    void render_imgui_processor();
    void render_imgui_disassembler();
    void render_imgui_debuggar();
    ImFont* AddDefaultFont(float pixel_size);
    Emulator* emu;
    unsigned int VBO, VAO, EBO;
    GLFWwindow* window;
    GLuint texture[NUM_TEXTURES];
};

int main(int argc, char* argv[])
{
    Emulator_Wrapper* gb = new Emulator_Wrapper();
    return 0;
}

static char brk_address_cpu[8] = "";
static ImVec4 cyan             = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
static ImVec4 magenta          = ImVec4(1.0f, 0.502f, 0.957f, 1.0f);
static ImVec4 yellow           = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
static ImVec4 red              = ImVec4(1.0f, 0.149f, 0.447f, 1.0f);
static ImVec4 green            = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
static ImVec4 white            = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
static ImVec4 gray             = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
static ImVec4 dark_gray        = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
static ImVec4 soy              = ImVec4(0.8f, 0.149f, 0.447f, 1.0f);
static MemoryEditor mem_edit;


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
        case GLFW_KEY_D: key_code = 4; break;
        case GLFW_KEY_S: key_code = 5; break;
        case GLFW_KEY_Z: key_code = 7; break;
        case GLFW_KEY_X: key_code = 6; break;
        case GLFW_KEY_RIGHT: key_code = 0; break;
        case GLFW_KEY_LEFT: key_code = 1; break;
        case GLFW_KEY_UP: key_code = 2; break;
        case GLFW_KEY_DOWN: key_code = 3; break;
        case GLFW_KEY_ESCAPE: exit(69); break;
            // - - - - - turn emulator off / on
        case GLFW_KEY_Q:
            if (emu_wrap_state == EMULATOR_WRAPPER_STATE::OFF) emu_wrap_state = EMULATOR_WRAPPER_STATE::ON;
            else if (emu_wrap_state == EMULATOR_WRAPPER_STATE::ON) emu_wrap_state = EMULATOR_WRAPPER_STATE::OFF;
            break;
            // - - - - - once it is off, we can single step certain things:
            // run one line of ASM
        case GLFW_KEY_L: emu->run_step(); break;
            // run ASM fast, one line per frame 
        case GLFW_KEY_K: emu_wrap_state = EMULATOR_WRAPPER_STATE::STEP_CODE; break;
            // run emu until next vblank 
        case GLFW_KEY_M: emu->run_to_vblank(); break;
            // run emu until next hblank
        case GLFW_KEY_N: emu->run_to_hblank(); break;
        }
        if (key_code != -1) emu->key_pressed(key_code);
    }
    else if (action == GLFW_RELEASE)
    {
        int key_code = -1;
        switch (key)
        {
        case GLFW_KEY_D: key_code = 4; break;
        case GLFW_KEY_S: key_code = 5; break;
        case GLFW_KEY_Z: key_code = 7; break;
        case GLFW_KEY_X: key_code = 6; break;
        case GLFW_KEY_RIGHT: key_code = 0; break;
        case GLFW_KEY_LEFT: key_code = 1; break;
        case GLFW_KEY_UP: key_code = 2; break;
        case GLFW_KEY_DOWN: key_code = 3; break;

        case GLFW_KEY_K: emu_wrap_state = EMULATOR_WRAPPER_STATE::OFF; break;
        }
        if (key_code != -1) emu->key_released(key_code);
    }
}


void Emulator_Wrapper::render_imgui_debuggar()
{
    ImGui::Begin("Debuggar");
    if (ImGui::Button("Q: Stop/Resume"))
    {
        if (emu_wrap_state == EMULATOR_WRAPPER_STATE::ON) emu_wrap_state = EMULATOR_WRAPPER_STATE::OFF;
        else if (emu_wrap_state == EMULATOR_WRAPPER_STATE::OFF) emu_wrap_state = EMULATOR_WRAPPER_STATE::ON;
    }
    ImGui::SameLine();
    if (ImGui::Button("L: Step ASM")) emu->run_step();
    ImGui::SameLine();
    if (ImGui::Button("N: Step HBLANK")) emu->run_to_hblank();
    ImGui::SameLine();
    if (ImGui::Button("M: Step VBLANK")) emu->run_to_vblank();

    if (ImGui::Button("Dump State A")) emu->dump_state('A');
    ImGui::SameLine();
    if (ImGui::Button("Dump State B")) emu->dump_state('B');
    ImGui::SameLine();
    if (ImGui::Button("Dump State C")) emu->dump_state('C');
    ImGui::SameLine();
    if (ImGui::Button("Dump State D")) emu->dump_state('D');
    ImGui::SameLine();
    ImGui::End();
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
    ImFont* fontC = AddDefaultFont(16);
#pragma endregion

    /*
    emu = new Emulator("C:\\Users\\pwnag\\Desktop\\retro\\my_GBC\\Tetris (World) (Rev 1).gb");
    emu = new Emulator("C:\\Users\\pwnag\\Desktop\\retro\\my_GBC\\cpu_instrs.gb");
    emu = new Emulator("C:\\Users\\pwnag\\Desktop\\retro\\my_GBC\\instr_timing.gb");
    emu = new Emulator("C:\\Users\\pwnag\\Desktop\\retro\\my_GBC\\mem_timing.gb");
    */
    emu = new Emulator("C:\\Users\\pwnag\\Desktop\\retro\\my_GBC\\Pokemon Blue.gb");
    
    /*
    for (int i = 0; i < 20000; i++)
    {
        //if (i > 300000)
        {
            fprintf(emu->fp, " %c %c %c %c %02x %02x %02x %02x %02x %02x %02x %04x %04x :::: %02x %02x :::: ",
                emu->flags.cc.c == 1 ? 'x' : '.', emu->flags.cc.h == 1 ? 'x' : '.', emu->flags.cc.n == 1 ? 'x' : '.', emu->flags.cc.z == 1 ? 'x' : '.',
                emu->regs[0], emu->regs[1], emu->regs[2], emu->regs[3], emu->regs[4], emu->regs[5], emu->regs[6], emu->sp, emu->pc, 
                emu->reg_FF0F_interrupt_flag, emu->reg_FFFF_interrupt_en);
            u16 dis_pc = emu->pc; // we need a second PC to step through a chunk of disassembly
            u8 bytes[90]; // a buffer used to store result of rb() since memory isnt contiguous 
            for (int i = 0; i < 90; i++) bytes[i] = emu->rb(dis_pc + i); // apparently instr can be read from memory outside cart :S
            // debug line for file 
            dis_line r = disassemble(bytes, 0);
            switch (r._size)
            {
            case 1: fprintf(emu->fp, "%04x : %02x       : %s\n", r._address + emu->pc, r._bytes[0], r._text); break;
            case 2: fprintf(emu->fp, "%04x : %02x %02x    : %s\n", r._address + emu->pc, r._bytes[0], r._bytes[1], r._text); break;
            case 3: fprintf(emu->fp, "%04x : %02x %02x %02x : %s\n", r._address + emu->pc, r._bytes[0], r._bytes[1], r._bytes[2], r._text); break;
            }
            // print state 
            //fprintf(emu->fp, "\t c h n z \tA\tB\tC\tD\tE\tH\tL\tSP\tPC\n\t %c %c %c %c \t%02x\t%02x\t%02x\t%02x\t%02x\t%02x\t%02x\t%04x\t%04x\t\n\n",
            //    emu->flags.cc.c == 1 ? 'x' : '.', emu->flags.cc.h == 1 ? 'x' : '.', emu->flags.cc.n == 1 ? 'x' : '.', emu->flags.cc.z == 1 ? 'x' : '.',
            //    emu->regs[0], emu->regs[1], emu->regs[2], emu->regs[3], emu->regs[4], emu->regs[5], emu->regs[6], emu->sp, emu->pc);
            
        }
        //emu->run_log();
        emu->run_step();

        //if (i == 17529) emu->reg_FF0F_interrupt_flag = 0xe1;
        //if (i == 52901) emu->reg_FF0F_interrupt_flag = 0xe9;
    }
    fclose(emu->fp);
    exit(420);
    */


    emu_wrap_state = EMULATOR_WRAPPER_STATE::ON;

    for (;;)
    {
        // reset output for new frame 
        emu->system_step_output = EMULATOR_OUTPUT::NOTHING;

        // what are we going to do this frame? 
        switch (emu_wrap_state)
        {
        case EMULATOR_WRAPPER_STATE::OFF: break;
        case EMULATOR_WRAPPER_STATE::ON: emu->run_to_vblank(); break;
        case EMULATOR_WRAPPER_STATE::STEP_CODE: emu->run_step(); break;
        }

        

        // what to do based on output of emulator's run this frame
        switch (emu->system_step_output)
        {
        case EMULATOR_OUTPUT::BREAKPOINT:
            // if we hit a breakpoint, reset the emu output (so it continues normally after) and pause the wrapper 
            emu_wrap_state = EMULATOR_WRAPPER_STATE::OFF;
            break;
        case EMULATOR_OUTPUT::VBLANK: break;
        case EMULATOR_OUTPUT::HBLANK: break;
        case EMULATOR_OUTPUT::NOTHING: break;
        }
        


        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::PushFont(fontC);
        render_imgui_main_window(); //if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
        render_imgui_processor();
        render_imgui_vram_viewer();
        render_imgui_memory_editor(mem_edit);
        render_imgui_disassembler();
        render_imgui_debuggar();
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
        glfwPollEvents();
    }
}

void Emulator_Wrapper::render_imgui_main_window()
{
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, emu->gpu_screen_data);
    ImGui::Begin("GameWindow"); // https://gamedev.stackexchange.com/questions/140693/how-can-i-render-an-opengl-scene-into-an-imgui-window
    ImVec2 size = ImVec2(160 * 2, 144 * 2);
    ImGui::Image((ImTextureID)texture[0], size);
    ImGui::End();
}

void Emulator_Wrapper::render_imgui_processor()
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


    ImGui::Separator();
    ImGui::TextColored(magenta, " IME"); ImGui::SameLine();
    ImGui::Text("= %d", emu->system_master_interrupt_en);

    ImGui::Columns(1);

    ImGui::TextColored(yellow, "INTERRUPTS:");

    ImGui::TextColored(cyan, " $FFFF"); ImGui::SameLine();
    ImGui::TextColored(magenta, "IE  "); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FFFF_interrupt_en, BYTE_TO_BINARY(emu->reg_FFFF_interrupt_en));

    ImGui::TextColored(cyan, " $FF0F"); ImGui::SameLine();
    ImGui::TextColored(magenta, "IF  "); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF0F_interrupt_flag, BYTE_TO_BINARY(emu->reg_FF0F_interrupt_flag));

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

    ImGui::TextColored(yellow, "INPUT:");

    ImGui::TextColored(cyan, " $FF00"); ImGui::SameLine();
    ImGui::TextColored(magenta, "JOYP"); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", emu->reg_FF00_joypad, BYTE_TO_BINARY(emu->reg_FF00_joypad));

    ImGui::End();
}

void Emulator_Wrapper::render_imgui_vram_viewer()
{
    ImGui::Begin("VRAM Viewer");
    if (ImGui::BeginTabBar("##memory_tabs", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("chill"))
        {
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("BG map"))
        {
            emu->debug_generate_background();
            glBindTexture(GL_TEXTURE_2D, texture[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, emu->gpu_background);

            ImVec2 size = ImVec2(256 * 2, 256 * 2);
            ImGui::Image((ImTextureID)texture[1], size);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("tileset"))
        {
            emu->debug_generate_tileset_view();
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

void Emulator_Wrapper::render_imgui_memory_editor(MemoryEditor mem_edit)
{
    ImGui::Begin("Memory Editor");
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

void Emulator_Wrapper::render_imgui_disassembler()
{
    ImGui::Begin("Reuben Disassembler");

    ImGui::Columns(2, "breakpoints_cpu");
    ImGui::SetColumnOffset(1, 85);
    ImGui::Separator();
    ImGui::PushItemWidth(70);
    if (ImGui::InputTextWithHint("##add_breakpoint_cpu", "XX:XXXX", brk_address_cpu, IM_ARRAYSIZE(brk_address_cpu), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
    {
        int input_len = strlen(brk_address_cpu);
        u16 target_address = 0;
        try
        {
            if (input_len == 4) target_address = std::stoul(brk_address_cpu, 0, 16);
            else return;
        }
        catch (const std::invalid_argument& ia) { return; }
        brk_address_cpu[0] = 0;
        emu->debug_mem_breakpoints_r.push_back(target_address);
    }
    ImGui::PopItemWidth();
    if (ImGui::Button("Add##add_cpu", ImVec2(70, 0)))
    {
        int input_len = strlen(brk_address_cpu);
        u16 target_address = 0;
        try
        {
            if (input_len == 4) target_address = std::stoul(brk_address_cpu, 0, 16);
            else return;
        }
        catch (const std::invalid_argument& ia) { return; }
        brk_address_cpu[0] = 0;
        emu->debug_mem_breakpoints_r.push_back(target_address);
    }
    ImGui::NextColumn();
    ImGui::BeginChild("breakpoints_cpu", ImVec2(0, 80), false);
    int remove = -1;
    for (long unsigned int b = 0; b < emu->debug_mem_breakpoints_r.size(); b++)
    {
        ImGui::PushID(b);
        if (ImGui::SmallButton("X"))
        {
            remove = b;
            ImGui::PopID();
            continue;
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::TextColored(red, "%04X", emu->debug_mem_breakpoints_r[b]);
    }
    if (remove >= 0) emu->debug_mem_breakpoints_r.erase(emu->debug_mem_breakpoints_r.begin() + remove);
    ImGui::EndChild();
    ImGui::Columns(1);
    ImGui::Separator();


    u16 dis_pc = emu->pc; // we need a second PC to step through a chunk of disassembly
    ImVec4 instr_clr; 
    u8 bytes[90]; // a buffer used to store result of rb() since memory isnt contiguous 
    for (int i = 0; i < 90; i++) bytes[i] = emu->rb(dis_pc + i, false); // apparently instr can be read from memory outside cart :S
    dis_pc = 0; // since now the PC is relative to the smol buffer 

    for (int i = 0; i < 30; i++)
    {
        dis_line r = disassemble(bytes, dis_pc);
        // print address and bytes in mem
        ImGui::TextColored(cyan, "%04x", r._address + emu->pc); ImGui::SameLine();
        switch (r._size)
        {
        case 1: ImGui::TextColored(gray, "%02x      ", r._bytes[0]); ImGui::SameLine(); break;
        case 2: ImGui::TextColored(gray, "%02x %02x   ", r._bytes[0], r._bytes[1]); ImGui::SameLine(); break;
        case 3: ImGui::TextColored(gray, "%02x %02x %02x", r._bytes[0], r._bytes[1], r._bytes[2]); ImGui::SameLine(); break;
        }
        // choose color based on category 
        switch (r._category)
        {
        case 0: instr_clr = cyan; break;
        case 1: instr_clr = magenta; break;
        case 2: instr_clr = yellow; break;
        case 3: instr_clr = red; break;
        case 4: instr_clr = green; break;
        case 5: instr_clr = white; break;
        case 6: instr_clr = gray; break;
        case 7: instr_clr = soy; break;
        }
        ImGui::TextColored(instr_clr, "%s\n", r._text);
        dis_pc += r._size;
    }
    ImGui::End();
}

ImFont* Emulator_Wrapper::AddDefaultFont(float pixel_size) // Chad https://github.com/ocornut/imgui/issues/1018
{
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig config;
    config.SizePixels = pixel_size;
    config.OversampleH = config.OversampleV = 2;
    config.PixelSnapH = true;
    ImFont* font = io.Fonts->AddFontDefault(&config);
    return font;
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
