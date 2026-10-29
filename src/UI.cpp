#include "UI.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <cmath>
#include <cstdio>

UI::UI(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;  // do not write imgui.ini into the repository
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 8.0f;
    s.FrameRounding = 4.0f;
    s.GrabRounding = 4.0f;
    s.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.08f, 0.08f, 0.88f);
    s.Colors[ImGuiCol_Header] = ImVec4(0.55f, 0.33f, 0.20f, 0.55f);
    s.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.65f, 0.40f, 0.24f, 0.75f);
    s.Colors[ImGuiCol_SliderGrab] = ImVec4(0.95f, 0.65f, 0.35f, 1.0f);
    s.Colors[ImGuiCol_CheckMark] = ImVec4(0.98f, 0.72f, 0.40f, 1.0f);
    s.Colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.18f, 0.15f, 0.7f);
    s.Colors[ImGuiCol_Button] = ImVec4(0.45f, 0.28f, 0.18f, 0.7f);
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

UI::~UI() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

bool UI::wantsMouse() const { return ImGui::GetIO().WantCaptureMouse; }
bool UI::wantsKeyboard() const { return ImGui::GetIO().WantCaptureKeyboard; }

static void helpWindow(AppState& st) {
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(ds.x * 0.5f, ds.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::Begin("Controls", &st.showHelp, ImGuiWindowFlags_AlwaysAutoResize);
    const char* rows[][2] = {
        {"Mouse drag / scroll", "orbit camera / zoom"},
        {"W A S D, Q E", "orbit (Orbit mode) / move (Fly mode)"},
        {"Shift", "faster camera"},
        {"C", "switch Orbit <-> Fly camera"},
        {"1 2 3 4", "camera presets"},
        {"[  ]", "time of day - / +"},
        {"P", "play / pause time-lapse"},
        {"L", "desk lamp on/off"},
        {"K", "floor lamp on/off"},
        {"F", "fairy lights on/off"},
        {"N", "automatic lights at dusk"},
        {"M", "record player play/stop"},
        {"Arrow keys", "aim the desk lamp"},
        {"G / B / V", "shadows / dust / light shafts"},
        {"T", "warm-cold colour grading"},
        {"X", "wireframe"},
        {"F12", "save screenshot (screenshots/)"},
        {"Tab", "hide / show panel"},
        {"H", "this help"},
        {"Esc", "release mouse / quit"},
    };
    if (ImGui::BeginTable("keys", 2, ImGuiTableFlags_RowBg)) {
        for (auto& r : rows) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.45f, 1.0f), "%s", r[0]);
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(r[1]);
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void UI::draw(AppState& st, Camera& cam, const UIStats& stats) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // FPS overlay (always visible, top-right)
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 10.0f, 10.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.55f);
    ImGui::Begin("##fps", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                       ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav);
    int hh = int(st.hour), mm = int(std::fmod(st.hour, 1.0f) * 60.0f);
    ImGui::Text("%.0f FPS  (%.2f ms)", stats.fps, stats.frameMs);
    ImGui::Text("%dx%d   %02d:%02d", stats.fbWidth, stats.fbHeight, hh, mm);
    if (!st.showPanel) ImGui::TextDisabled("Tab: panel   H: help");
    ImGui::End();

    if (st.showPanel) {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver);
        ImGui::Begin("Cozy Room Diorama", &st.showPanel);

        if (ImGui::CollapsingHeader("Time of day", ImGuiTreeNodeFlags_DefaultOpen)) {
            char label[16];
            std::snprintf(label, sizeof(label), "%02d:%02d", hh, mm);
            ImGui::SliderFloat("Time", &st.hour, 0.0f, 23.99f, label);
            ImGui::Checkbox("Time-lapse (P)", &st.playing);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(110);
            ImGui::SliderFloat("h/s", &st.timeSpeed, 0.1f, 4.0f, "%.1f");
            ImGui::Checkbox("Warm / cold colour grading (T)", &st.warmColdGrading);
        }
        if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Desk lamp (L)", &st.deskLamp);
            ImGui::SameLine(150);
            ImGui::Checkbox("Floor lamp (K)", &st.floorLamp);
            ImGui::Checkbox("Fairy lights (F)", &st.fairyLights);
            ImGui::SameLine(150);
            ImGui::Checkbox("Auto at dusk (N)", &st.autoLights);
            ImGui::SliderFloat("Lamp yaw", &st.lampYaw, -80.0f, 80.0f, "%.0f deg");
            ImGui::SliderFloat("Lamp head", &st.lampHeadPitch, 60.0f, 170.0f, "%.0f deg");
        }
        if (ImGui::CollapsingHeader("Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Shadows (G)", &st.shadows);
            ImGui::SameLine(150);
            ImGui::Checkbox("Light shafts (V)", &st.lightShafts);
            ImGui::Checkbox("Dust (B)", &st.dust);
            ImGui::SameLine(150);
            ImGui::Checkbox("Bloom", &st.bloom);
            ImGui::SliderFloat("Dust amount", &st.dustAmount, 0.0f, 3.0f, "%.1f");
            ImGui::SliderFloat("Exposure", &st.exposure, 0.3f, 3.0f, "%.2f");
            ImGui::Checkbox("Record player (M)", &st.recordPlaying);
            ImGui::SameLine(150);
            ImGui::Checkbox("Wireframe (X)", &st.wireframe);
        }
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button(cam.mode() == Camera::Mode::Orbit ? "Mode: Orbit (C)" : "Mode: Fly (C)")) cam.toggleMode();
            const char* names[] = {"Overview", "Window", "Bed", "Records"};
            for (int i = 0; i < 4; ++i) {
                if (i) ImGui::SameLine();
                if (ImGui::Button(names[i])) cam.goToPreset(i);
            }
        }
        if (ImGui::CollapsingHeader("Stats")) {
            ImGui::Text("Objects: %d   Particles: %d", stats.objects, stats.particles);
            ImGui::Text("Point lights: %d   Draw calls: %d", stats.pointLights, stats.drawCalls);
        }
        if (ImGui::Button("Help (H)")) st.showHelp = !st.showHelp;
        ImGui::SameLine();
        if (ImGui::Button("Screenshot (F12)")) st.screenshotRequested = true;
        ImGui::End();
    }
    if (st.showHelp) helpWindow(st);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
