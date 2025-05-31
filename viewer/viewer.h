#ifndef __VIEWER_H__
#define __VIEWER_H__

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <iostream>
#include <chrono>
#include "shader.h"
#include "dataloader.h"
#include "viewport.h"
#include "renderer.h"
    
class LiteViewer{

private:
    std::string title;
    Viewport viewport;

    static LiteViewer* viewer;

    GLFWwindow* window;
    ImGuiWindowFlags window_flags = 0;

    Eigen::Vector4f clearColor = Eigen::Vector4f(0.20f, 0.20f, 0.20f, 0.00f);
    
    bool any_window_active = false;

public:
    LiteViewer(std::string title, int width, int height):
        title(title), viewport(width, height){
        viewer = this;
    }

public:

    bool init(){
        
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW!" << std::endl;
            return false;
        }

        glfwWindowHint(GLFW_SAMPLES, 8);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
        glfwWindowHint(GLFW_DEPTH_BITS, 24);
    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

        window = glfwCreateWindow(
            viewer->viewport.windowSize.x(), 
            viewer->viewport.windowSize.y(), 
            viewer->title.c_str(), NULL, NULL);

        if (window == NULL){
            std::cerr << "Failed to create GLFW window!" << std::endl;
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            // LOG_ERROR("GLAD init failed");
            std::cerr << "GLAD init failed" << std::endl;
            glfwTerminate();
            return -1;
        }

        glfwSwapInterval(1); // Enable vsync

        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetCursorPosCallback(window, cursorPosCallback);
        glfwSetScrollCallback(window, scrollCallback);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetDropCallback(window, dropCallback);

        glEnable(GL_LINE_SMOOTH);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);  
        glEnable(GL_PROGRAM_POINT_SIZE);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigWindowsMoveFromTitleBarOnly = true;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        const char* glsl_version = "#version 400";
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        // Set Fonts
        std::string font_path = "./viewer/assets/JetBrainsMono-Regular.ttf";
        io.Fonts->AddFontFromFileTTF(font_path.c_str(), 14.0f);

        // Set Windows option
        window_flags |= ImGuiWindowFlags_NoScrollbar;
        window_flags |= ImGuiWindowFlags_NoResize;
        // window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.WindowPadding = ImVec2(0.0f, 6.0f);
        style.WindowRounding = 6.0f;
        style.WindowBorderSize = 0.0f;

        return true;
    }

    void updateWindowSize(){
        glfwGetWindowSize(viewer->window, &viewer->viewport.windowSize.x(), &viewer->viewport.windowSize.y());
        glfwGetFramebufferSize(viewer->window, &viewer->viewport.frameBufferSize.x(), &viewer->viewport.frameBufferSize.y());
        glViewport(0, 0, viewer->viewport.frameBufferSize.x(), viewer->viewport.frameBufferSize.y());
    }


    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        if(viewer->any_window_active)
            return;

        if((button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) && action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            viewer->viewport.camera.initScreenPos(Eigen::Vector2f(xpos, ypos));
        }
    }

    static void cursorPosCallback(GLFWwindow* window, double x, double y) {
        if(viewer->any_window_active)
            return;

        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            viewer->viewport.camera.translate(Eigen::Vector2f(x, y));
        } else if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            viewer->viewport.camera.rotate(Eigen::Vector2f(x, y));
        }
    }

    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        if(viewer->any_window_active)
            return;

        const float delta = static_cast<float>(yoffset);
        if(std::abs(delta) < 1.0e-2f) return;

        viewer->viewport.camera.zoom(delta);
    }

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
        if(viewer->any_window_active)
            return;

        auto &viewport = viewer->viewport;

        // TODO: not implemented yet (pxk)
    }

    static void dropCallback(GLFWwindow* window, int count, const char** paths) {
        if(viewer->any_window_active)
            return;

        if (count > 0) {
            std::string path(paths[0]);
            if (std::filesystem::exists(path)) {
                std::cout << "File dropped: " << path << std::endl;

            } else {
                std::cerr << "File does not exist: " << path << std::endl;
            }
        }
    }

    std::string getTimestamp(){
        auto now        = std::chrono::system_clock::now();
        auto now_ms     = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        auto sectime    = std::chrono::duration_cast<std::chrono::seconds>(now_ms);

        std::time_t timet = sectime.count();
        struct tm curtime;
        localtime_r(&timet, &curtime);

        std::stringstream ss;
        ss << std::put_time(&curtime, "%Y-%m-%d-%H-%M-%S");
        std::string buffer = ss.str();
        return std::string(buffer);
    }

    void configuration(RenderConfig& config) {

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        any_window_active = ImGui::IsAnyItemActive() ;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.3f));
        ImGui::Begin("Configuration", nullptr,  window_flags); 
        ImGui::SetWindowSize(ImVec2(300, 0));

        static const char* mode_items[] = {
            "                 COLOR(SH-0)", 
            "                 COLOR(SH-1)", 
            "                 COLOR(SH-2)", 
            "                 COLOR(SH-3)", 
            "                 DEPTH", 
            "                 GAUSS BALL", 
            "                 SURFEL"
        };
        static int current_item = config.render_mode;
        ImGui::SetNextItemWidth(300.0f);
        if (ImGui::Combo("##render_mode", &current_item, mode_items, 7)) {
            config.render_mode = static_cast<RenderConfig::RenderMode>(current_item);
        }

        ImGui::SetNextItemWidth(200.0f);
        ImGui::SliderFloat("##scale_slider", &config.scale_modifier, 0.01f, 3.0f, "Scale=%.2f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(85.0f);
        if (ImGui::Button("Reset##scale", ImVec2(85.0f, 0.0f))) {
            config.scale_modifier = 1.0f;
        }
        ImGui::SetNextItemWidth(200.0f);
        ImGui::SliderFloat("##fov_slider", &config.fov, 10.0f, 120.0f, "FoV=%.1f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(85.0f);
        if (ImGui::Button("Reset##fov", ImVec2(85.0f, 0.0f))) {
            config.fov = 60.0f;
        }


        ImGui::Checkbox("Vertical Synch.", &config.vsync);
        ImGui::Checkbox("Depth Sort", &config.depth_sort);

        ImGui::ColorEdit4("Background", (float*)&clearColor);

        ImGui::Separator();
        ImGui::Text("Primitive Count: %zu", config.num_primitives);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        ImGui::End();
        ImGui::PopStyleColor();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // action
        {
            glfwSwapInterval(config.vsync ? 1 : 0);

            viewer->viewport.setFoV(config.fov);
        }
    }

    void draw(const GaussianData& data) {

        if(!init()){
            std::cerr << "Failed to init LiteViz" << std::endl;
            return;
        }

        std::shared_ptr<Shader> splatShader = std::make_shared<Shader>(
            "./viewer/shaders/draw_splat.vert", "./viewer/shaders/draw_splat.frag", false);

        Renderer renderer(data, splatShader.get());

        while (!glfwWindowShouldClose(window)){

            glClearBufferfv(GL_COLOR, 0, clearColor.data());

            updateWindowSize();

            renderer.render(viewer->viewport);

            configuration(renderer.config());

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

};

LiteViewer* LiteViewer::viewer = nullptr;


#endif // __VIEWER_H__



