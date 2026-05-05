
#include "gui.h"

void run_gui(){
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_Window* window = SDL_CreateWindow(
        "Backend start", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1024, 768, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    glewExperimental = GL_TRUE;
    glewInit();

    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Включить Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Включить Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Включить Docking

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running = false;
            }
            
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_None);


IQData iq_raw, iq_fill, iq_sync, iq_freq;
{
    lock_guard<mutex> lock(shared.mtx);
    if (!shared.rx_samples_raw.empty())      iq_raw  = extractIQ(shared.rx_samples_raw);
    if (!shared.rx_samples_fil.empty())      iq_fill = extractIQ(shared.rx_samples_fil);
    if (!shared.rx_samples_sync_time.empty()) iq_sync = extractIQ(shared.rx_samples_sync_time);
    if (!shared.rx_samples_freq.empty())     iq_freq = extractIQ(shared.rx_samples_freq);
}
ImGui::Begin("Pipeline Visualizer");

if (ImGui::BeginTabBar("MainTabBar")) {

    // --- ВКЛАДКА 1: Временные графики (Верх) ---
    if (ImGui::BeginTabItem("Raw Data")) {
        
       
        if (ImPlot::BeginPlot("Raw Time Domain", ImVec2(-1, 250))) {
            if (!iq_raw.real.empty()) {
                ImPlot::PlotLine("I", iq_raw.count.data(), iq_raw.real.data(), iq_raw.real.size());
                ImPlot::PlotLine("Q", iq_raw.count.data(), iq_raw.imag.data(), iq_raw.imag.size());
            }
            ImPlot::EndPlot();
        }

        ImGui::Separator();

        if (ImPlot::BeginPlot("Raw Scatter", ImVec2(-1, 400))) {
            if (!iq_raw.real.empty())
                ImPlot::PlotScatter("##raw", iq_raw.real.data(), iq_raw.imag.data(), iq_raw.real.size());
            ImPlot::EndPlot();
        }
        
        ImGui::EndTabItem();
    }

    // --- ВКЛАДКА 2: Scatter графики (Низ) ---
    if (ImGui::BeginTabItem("Filtered Data")) {
        
       
        if (ImPlot::BeginPlot("Raw Time Domain", ImVec2(-1, 250))) {
            if (!iq_fill.real.empty()) {
                ImPlot::PlotLine("I", iq_fill.count.data(), iq_fill.real.data(), iq_fill.real.size());
                ImPlot::PlotLine("Q", iq_fill.count.data(), iq_fill.imag.data(), iq_fill.imag.size());
            }
            ImPlot::EndPlot();
        }

        ImGui::Separator();

        if (ImPlot::BeginPlot("Raw Scatter", ImVec2(-1, 400))) {
            if (!iq_fill.real.empty())
                ImPlot::PlotScatter("##raw", iq_fill.real.data(), iq_fill.imag.data(), iq_fill.real.size());
            ImPlot::EndPlot();
        }
        
        ImGui::EndTabItem();
    }
    // --- ВКЛАДКА 3: 
    if (ImGui::BeginTabItem("sync time Data")) {
        
       
        if (ImPlot::BeginPlot("Raw Time Domain", ImVec2(-1, 250))) {
            if (!iq_fill.real.empty()) {
                ImPlot::PlotLine("I", iq_sync.count.data(), iq_sync.real.data(), iq_sync.real.size());
                ImPlot::PlotLine("Q", iq_sync.count.data(), iq_sync.imag.data(), iq_sync.imag.size());
            }
            ImPlot::EndPlot();
        }

        ImGui::Separator();

        if (ImPlot::BeginPlot("Raw Scatter", ImVec2(-1, 400))) {
            if (!iq_sync.real.empty())
                ImPlot::PlotScatter("##raw", iq_sync.real.data(), iq_sync.imag.data(), iq_sync.real.size());
            ImPlot::EndPlot();
        }
        
        ImGui::EndTabItem();
    }

    
    // --- ВКЛАДКА 4: 
    if (ImGui::BeginTabItem("Freq sync Data")) {
        
       
        if (ImPlot::BeginPlot("Raw Time Domain", ImVec2(-1, 250))) {
            if (!iq_freq.real.empty()) {
                ImPlot::PlotLine("I", iq_freq.count.data(), iq_freq.real.data(), iq_freq.real.size());
                ImPlot::PlotLine("Q", iq_freq.count.data(), iq_freq.imag.data(), iq_freq.imag.size());
            }
            ImPlot::EndPlot();
        }

        ImGui::Separator();

        if (ImPlot::BeginPlot("Raw Scatter", ImVec2(-1, 400))) {
            if (!iq_freq.real.empty())
                ImPlot::PlotScatter("##raw", iq_freq.real.data(), iq_freq.imag.data(), iq_sync.real.size());
            ImPlot::EndPlot();
        }
        
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
}
ImGui::End();
        
        ImGui::Render();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
        
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    shared.program_running = false;
}

