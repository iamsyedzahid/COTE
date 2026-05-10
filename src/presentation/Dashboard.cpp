#include "presentation/Dashboard.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

// Modern Premium Palette
static constexpr Color BG_DARK      = { 20, 20, 26, 255 };    // Deep Charcoal
static constexpr Color CARD_BG     = { 30, 30, 38, 200 };    // Translucent Gray
static constexpr Color CARD_BORDER = { 50, 50, 65, 255 };    // Subtle Border
static constexpr Color TEXT_MAIN   = { 220, 220, 235, 255 }; // Soft White
static constexpr Color TEXT_DIM    = { 120, 120, 150, 255 }; // Muted Blue-Gray

static constexpr Color ACCENT_CYAN  = { 0, 225, 255, 255 };  // Electric Cyan
static constexpr Color ACCENT_GOLD  = { 255, 200, 50, 255 };  // Muted Gold
static constexpr Color ACCENT_BERRY = { 255, 60, 120, 255 };  // Vivid Berry
static constexpr Color ACCENT_LIME  = { 150, 255, 50, 255 };  // Soft Lime

Dashboard::Dashboard(MetricsService& metrics, int width, int height)
    : metrics_(metrics)
    , width_(width)
    , height_(height)
    , throughput_history_(GRAPH_CAPACITY)
    , latency_history_(GRAPH_CAPACITY)
    , queue_history_(GRAPH_CAPACITY)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(width_, height_, "Cloud Task Orchestration Engine");
    SetTargetFPS(30);

    // Bulletproof Font Loading: Check if file exists and is valid size before loading
    bool fontLoaded = false;
    std::ifstream f("resources/font.ttf", std::ios::binary | std::ios::ate);
    if (f.is_open()) {
        long size = f.tellg();
        if (size > 10240) { // Must be at least 10KB to be a real font
            font_ = LoadFontEx("resources/font.ttf", 64, nullptr, 0);
            if (font_.texture.id != 0) {
                SetTextureFilter(font_.texture, TEXTURE_FILTER_BILINEAR);
                fontLoaded = true;
            }
        }
        f.close();
    }

    if (!fontLoaded) {
        std::cout << "[INFO] Using default system font." << std::endl;
        font_ = GetFontDefault();
    }
}

Dashboard::~Dashboard() {
    UnloadFont(font_);
    CloseWindow();
}

bool Dashboard::shouldClose() const {
    return WindowShouldClose();
}

void Dashboard::update() {
    auto snap = metrics_.snapshot();
    {
        std::lock_guard<std::mutex> lg(throughput_history_.mutex);
        throughput_history_.push(static_cast<float>(snap.throughput_per_second));
    }
    {
        std::lock_guard<std::mutex> lg(latency_history_.mutex);
        latency_history_.push(static_cast<float>(snap.average_latency_ms));
    }
    {
        std::lock_guard<std::mutex> lg(queue_history_.mutex);
        queue_history_.push(static_cast<float>(snap.queued));
    }
}

void Dashboard::render() {
    width_  = GetScreenWidth();
    height_ = GetScreenHeight();

    auto snap = metrics_.snapshot();

    BeginDrawing();
    ClearBackground(BG_DARK);

    drawHeader();
    drawTaskCountCards(snap);
    drawWorkerBar(snap);
    drawActivityStrip(snap);

    // Dynamic layout: graphs fill the middle, stats panel takes the rest
    float graphTop = 335.0f;
    float graphH   = std::max(100.0f, static_cast<float>(height_) - 28.0f - graphTop - 155.0f);
    float graphW   = (width_ - 60.0f) / 3.0f;

    {
        std::lock_guard<std::mutex> lg(throughput_history_.mutex);
        drawGraph(throughput_history_, "Throughput", "tasks/s",
                  { 20.0f, graphTop, graphW, graphH }, ACCENT_LIME);
    }
    {
        std::lock_guard<std::mutex> lg(latency_history_.mutex);
        drawGraph(latency_history_, "Avg Latency", "ms",
                  { 20.0f + graphW + 20.0f, graphTop, graphW, graphH }, ACCENT_GOLD);
    }
    {
        std::lock_guard<std::mutex> lg(queue_history_.mutex);
        drawGraph(queue_history_, "Queue Depth", "tasks",
                  { 20.0f + 2.0f*(graphW + 20.0f), graphTop, graphW, graphH }, ACCENT_CYAN);
    }

    float statsTop = graphTop + graphH + 10.0f;
    drawStatsPanel(snap, statsTop);
    drawFooter();

    if (snap.is_paused) {
        // Less opaque overlay (alpha 80 instead of 150)
        DrawRectangle(0, 0, width_, height_, { 0, 0, 0, 80 });
        
        // Move to top-right to keep the center and activity strip clear
        const char* msg = "SIMULATION PAUSED";
        const char* sub = "Press SPACE to resume";
        
        float margin = 40.0f;
        Vector2 sz1 = MeasureTextEx(font_, msg, 24, 2.0f);
        Vector2 sz2 = MeasureTextEx(font_, sub, 14, 1.0f);
        
        float x = width_ - std::max(sz1.x, sz2.x) - margin;
        float y = margin + 40.0f; // Below the header

        DrawRectangleRounded({ x - 15, y - 10, std::max(sz1.x, sz2.x) + 30, sz1.y + sz2.y + 25 }, 0.2f, 8, { 20, 20, 20, 200 });
        DrawRectangleRoundedLines({ x - 15, y - 10, std::max(sz1.x, sz2.x) + 30, sz1.y + sz2.y + 25 }, 0.2f, 8, 2.0f, ACCENT_GOLD);

        DrawTextEx(font_, msg, { x, y }, 24, 2.0f, ACCENT_GOLD);
        DrawTextEx(font_, sub, { x, y + 30 }, 14, 1.0f, TEXT_DIM);
    }

    EndDrawing();
}

void Dashboard::drawHeader() const {
    DrawRectangle(0, 0, width_, 52, CARD_BG);
    DrawLine(0, 52, width_, 52, CARD_BORDER);
    DrawTextEx(font_, "CLOUD TASK ORCHESTRATION ENGINE", { 20, 16 }, 22, 2.0f, TEXT_MAIN);
    
    // Fullscreen hint
    DrawTextEx(font_, "F11: FULLSCREEN", { (float)width_ - 150, 18 }, 14, 1.0f, TEXT_DIM);
}

void Dashboard::drawTaskCountCards(const MetricsSnapshot& snap) const {
    float totalSpacing = 100.0f; // total side and middle gaps
    float cardW = (width_ - totalSpacing) / 4.0f;
    float top   = 68.0f;
    float h     = 100.0f;

    drawCard(font_, { 20.0f,                   top, cardW, h }, "QUEUED",    snap.queued,     ACCENT_CYAN);
    drawCard(font_, { 20.0f + (cardW+20.0f),   top, cardW, h }, "RUNNING",   snap.running,    ACCENT_GOLD);
    drawCard(font_, { 20.0f + 2*(cardW+20.0f), top, cardW, h }, "COMPLETED", snap.completed,  ACCENT_LIME);
    drawCard(font_, { 20.0f + 3*(cardW+20.0f), top, cardW, h }, "TIMED OUT", snap.timed_out,  ACCENT_BERRY);
}

void Dashboard::drawCard(Font font, Rectangle bounds, const char* title, uint64_t value, Color accent) {
    // Glass Card Shadow/Border
    DrawRectangleRounded(bounds, 0.15f, 8, CARD_BG);
    DrawRectangleRoundedLines(bounds, 0.15f, 8, 2.0f, CARD_BORDER);
    
    // Accent Glow (Top line)
    DrawRectangle(static_cast<int>(bounds.x) + 15, static_cast<int>(bounds.y), static_cast<int>(bounds.width) - 30, 3, accent);

    // Title (Small, Dim, Uppercase)
    char upperTitle[64];
    size_t i = 0;
    for (; title[i] != '\0' && i < 63; ++i) upperTitle[i] = (char)toupper(title[i]);
    upperTitle[i] = '\0';
    
    DrawTextEx(font, upperTitle, { bounds.x + 20, bounds.y + 20 }, 14, 1.5f, TEXT_DIM);
    
    // Value (Large, Bright)
    std::string valStr = std::to_string(value);
    Vector2 valSize = MeasureTextEx(font, valStr.c_str(), 40, 1.0f);
    DrawTextEx(font, valStr.c_str(), { bounds.x + (bounds.width - valSize.x) / 2, bounds.y + 45 }, 40, 1.0f, TEXT_MAIN);
}

void Dashboard::drawWorkerBar(const MetricsSnapshot& snap) const {
    float top  = 180.0f;
    float barW = static_cast<float>(width_) - 40.0f;

    DrawRectangle(20, static_cast<int>(top), static_cast<int>(barW), 36, CARD_BG);
    DrawRectangleLinesEx({ 20.0f, top, barW, 36.0f }, 1.0f, CARD_BORDER);

    std::string workerStr = "WORKERS: " + std::to_string(snap.worker_count) + "  |  TOTAL JOBS: " + std::to_string(snap.total_submitted);
    DrawTextEx(font_, workerStr.c_str(), { 28, top + 10 }, 16, 1.0f, TEXT_DIM);

    int maxVisibleBlocks = (int)(barW / 14.0f) - 10;
    int blocks    = std::min(static_cast<int>(snap.worker_count), maxVisibleBlocks);
    int bx        = width_ - 20 - blocks * 14;
    for (int i = 0; i < blocks; ++i) {
        Color col = (static_cast<uint64_t>(i) < snap.running) ? ACCENT_GOLD : ACCENT_LIME;
        DrawRectangle(bx + i * 14, static_cast<int>(top) + 8, 10, 20, col);
    }
}

void Dashboard::drawActivityStrip(const MetricsSnapshot& snap) const {
    float top  = 245.0f;
    float h    = 54.0f;
    float barW = static_cast<float>(width_) - 40.0f;

    DrawTextEx(font_, "PRIORITY ACTIVITY", { 20, top - 22 }, 14, 2.0f, TEXT_DIM);
    DrawRectangle(20, static_cast<int>(top), static_cast<int>(barW), static_cast<int>(h), CARD_BG);
    DrawRectangleLinesEx({ 20.0f, top, barW, h }, 1.0f, CARD_BORDER);

    if (snap.recent_events.empty()) {
        DrawTextEx(font_, "Waiting for tasks...", { 30, top + 20 }, 13, 1.0f, TEXT_DIM);
        return;
    }

    // Deduplicate: only show the latest state for each unique task ID
    std::vector<TaskEvent> unique_tasks;
    for (auto it = snap.recent_events.rbegin(); it != snap.recent_events.rend(); ++it) {
        bool exists = false;
        for (const auto& ut : unique_tasks) {
            if (ut.id == it->id) {
                exists = true;
                break;
            }
        }
        if (!exists) unique_tasks.push_back(*it);
        if (unique_tasks.size() >= 10) break; 
    }
    // Reverse again so they appear in chronological order (oldest on left)
    std::reverse(unique_tasks.begin(), unique_tasks.end());

    float chipW  = 95.0f;
    float chipH  = 28.0f;
    float chipY  = top + h / 2.0f - chipH / 2.0f;
    float startX = 30.0f;

    for (size_t i = 0; i < unique_tasks.size(); ++i) {
        const auto& ev  = unique_tasks[i];
        float cx = startX + static_cast<float>(i) * (chipW + 6.0f);

        Rectangle chipRec = { cx, chipY, chipW, chipH };
        bool isHovered = CheckCollisionPointRec(GetMousePosition(), chipRec);
        
        if (isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            const_cast<Dashboard*>(this)->selected_task_id_ = ev.id;
        }

        Color stateCol = (ev.state == "QUEUED")  ? ACCENT_CYAN  :
                         (ev.state == "RUNNING") ? ACCENT_GOLD :
                         (ev.state == "DONE")    ? ACCENT_LIME : ACCENT_BERRY;

        bool isHighPrio = ev.priority >= 8;

        DrawRectangleRounded(chipRec, 0.2f, 8, (selected_task_id_ == ev.id) ? CARD_BORDER : BG_DARK);
        
        // Use a thicker, glowing border for high-priority tasks
        if (isHighPrio) {
            DrawRectangleRoundedLines(chipRec, 0.2f, 8, 3.0f, (selected_task_id_ == ev.id) ? WHITE : ACCENT_GOLD);
        } else {
            DrawRectangleRoundedLines(chipRec, 0.2f, 8, 1.5f, (isHovered || selected_task_id_ == ev.id) ? WHITE : stateCol);
        }
        
        std::string pstr = "P:" + std::to_string(ev.priority) + " #" + std::to_string(ev.id);
        
        // Highlight the "P:X" text for high priority tasks
        Color pCol = isHighPrio ? ACCENT_BERRY : TEXT_MAIN;
        DrawTextEx(font_, pstr.c_str(), { cx + 5, chipY + 4 }, 11, 1.0f, pCol);
        DrawTextEx(font_, ev.state.c_str(), { cx + 5, chipY + 16 }, 10, 1.0f, stateCol);

        if (selected_task_id_ == ev.id) {
            char durBuf[32];
            sprintf(durBuf, "DUR: %ds", ev.duration_ms / 1000);
            DrawTextEx(font_, durBuf, { cx + 55, chipY + 16 }, 10, 1.0f, ACCENT_CYAN);
        }
    }
}

void Dashboard::drawGraph(const GraphBuffer& buf,
                          const char*        label,
                          const char*        unit,
                          Rectangle          bounds,
                          Color              line_color) const {
    DrawRectangleRec(bounds, CARD_BG);
    DrawRectangleLinesEx(bounds, 1.0f, CARD_BORDER);

    const auto& data = buf.data();
    if (data.size() < 2) return;

    float peak = buf.max();
    if (peak < 1.0f) peak = 1.0f;

    float pad_left  = 10.0f;
    float pad_right = 10.0f;
    float pad_top   = 28.0f;
    float pad_bot   = 20.0f;

    float gx = bounds.x + pad_left;
    float gy = bounds.y + pad_top;
    float gw = bounds.width  - pad_left - pad_right;
    float gh = bounds.height - pad_top  - pad_bot;

    for (size_t i = 1; i < data.size(); ++i) {
        float x0 = gx + gw * (float)(i - 1) / (float)(GRAPH_CAPACITY - 1);
        float x1 = gx + gw * (float)(i)     / (float)(GRAPH_CAPACITY - 1);
        float y0 = gy + gh - gh * (data[i - 1] / peak);
        float y1 = gy + gh - gh * (data[i]     / peak);
        DrawLineEx({ x0, y0 }, { x1, y1 }, 1.8f, line_color);
    }

    DrawTextEx(font_, label, { bounds.x, bounds.y - 25 }, 16, 1.0f, TEXT_DIM);
    
    char valStr[32];
    sprintf(valStr, "%.1f %s", data.back(), unit);
    Vector2 valSize = MeasureTextEx(font_, valStr, 16, 1.0f);
    DrawTextEx(font_, valStr, { bounds.x + bounds.width - valSize.x, bounds.y - 25 }, 16, 1.0f, line_color);
}

void Dashboard::drawStatsPanel(const MetricsSnapshot& snap, float top) const {
    float panW = static_cast<float>(width_) - 40.0f;
    float panH = static_cast<float>(height_) - top - 36.0f;

    DrawRectangle(20, static_cast<int>(top), static_cast<int>(panW), static_cast<int>(panH), CARD_BG);
    DrawRectangleLinesEx({ 20.0f, top, panW, panH }, 1.0f, CARD_BORDER);
    DrawTextEx(font_, "PERFORMANCE STATISTICS", { 32, top + 10 }, 16, 2.0f, TEXT_DIM);

    auto drawStat = [&](float x, float y, const std::string& key, const std::string& val, Color vc) {
        DrawTextEx(font_, key.c_str(), { x, y }, 15, 1.0f, TEXT_DIM);
        DrawTextEx(font_, val.c_str(), { x + MeasureTextEx(font_, key.c_str(), 15, 1.0f).x + 5, y }, 15, 1.0f, vc);
    };

    char latencyBuf[32];
    sprintf(latencyBuf, "%.1f ms", snap.average_latency_ms);
    drawStat(40.0f, top + 40.0f, "Throughput:", std::to_string(snap.throughput_per_second) + " tasks/sec", ACCENT_LIME);
    drawStat(40.0f, top + 70.0f, "Avg Latency:", latencyBuf, ACCENT_GOLD);
    drawStat(40.0f, top + 100.0f, "Total Jobs:", std::to_string(snap.total_submitted), TEXT_MAIN);

    uint64_t success_rate = (snap.total_submitted > 0) ? (snap.completed * 100 / snap.total_submitted) : 100;
    char buf[64];
    sprintf(buf, "Success Rate: %d%%", (int)success_rate);
    
    // Position success rate on the right side of the stats panel
    float srX = width_ - MeasureTextEx(font_, buf, 15, 1.0f).x - 40.0f;
    DrawTextEx(font_, buf, { srX, top + 40.0f }, 15, 1.0f, success_rate > 90 ? ACCENT_LIME : ACCENT_BERRY);
}

void Dashboard::drawFooter() const {
    int fy = height_ - 28;
    DrawRectangle(0, fy, width_, 28, BG_DARK);
    DrawLine(0, fy, width_, fy, CARD_BORDER);
    DrawTextEx(font_, "SPACE: PAUSE  |  B: BURST (25)  |  T: SINGLE TASK  |  F11: FULLSCREEN  |  ESC: QUIT",
             { 20, (float)fy + 7 }, 12, 1.0f, TEXT_MAIN);
    
    char fpsBuf[16];
    sprintf(fpsBuf, "FPS: %d", GetFPS());
    DrawTextEx(font_, fpsBuf, { (float)width_ - 60, (float)fy + 7 }, 12, 1.0f, TEXT_DIM);
}
