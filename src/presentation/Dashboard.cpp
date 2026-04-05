#include "presentation/Dashboard.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

static const Color BG_DARK      = { 13,  17,  23,  255 };
static const Color BG_CARD      = { 22,  27,  34,  255 };
static const Color BG_PANEL     = { 30,  35,  42,  255 };
static const Color ACCENT_BLUE  = { 56,  139, 253, 255 };
static const Color ACCENT_GREEN = { 63,  185, 80,  255 };
static const Color ACCENT_RED   = { 248, 81,  73,  255 };
static const Color ACCENT_AMBER = { 210, 153, 34,  255 };
static const Color ACCENT_VIOLET= { 188, 140, 255, 255 };
static const Color TEXT_PRIMARY = { 230, 237, 243, 255 };
static const Color TEXT_DIM     = { 125, 133, 144, 255 };
static const Color BORDER_COLOR = { 48,  54,  61,  255 };

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
}

Dashboard::~Dashboard() {
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
    float graphTop = 295.0f;
    float graphH   = std::max(100.0f, static_cast<float>(height_) - 28.0f - graphTop - 145.0f);
    float graphW   = (width_ - 60.0f) / 3.0f;

    {
        std::lock_guard<std::mutex> lg(throughput_history_.mutex);
        drawGraph(throughput_history_, "Throughput", "tasks/s",
                  { 20.0f, graphTop, graphW, graphH }, ACCENT_GREEN);
    }
    {
        std::lock_guard<std::mutex> lg(latency_history_.mutex);
        drawGraph(latency_history_, "Avg Latency", "ms",
                  { 20.0f + graphW + 20.0f, graphTop, graphW, graphH }, ACCENT_AMBER);
    }
    {
        std::lock_guard<std::mutex> lg(queue_history_.mutex);
        drawGraph(queue_history_, "Queue Depth", "tasks",
                  { 20.0f + 2.0f*(graphW + 20.0f), graphTop, graphW, graphH }, ACCENT_BLUE);
    }

    float statsTop = graphTop + graphH + 10.0f;
    drawStatsPanel(snap, statsTop);
    drawFooter();

    EndDrawing();
}

void Dashboard::drawHeader() const {
    DrawRectangle(0, 0, width_, 52, BG_PANEL);
    DrawLine(0, 52, width_, 52, BORDER_COLOR);
    DrawText("Cloud Task Orchestration Engine", 20, 16, 22, TEXT_PRIMARY);
}

void Dashboard::drawTaskCountCards(const MetricsSnapshot& snap) const {
    float cardW = (width_ - 100.0f) / 4.0f;
    float top   = 68.0f;
    float h     = 100.0f;

    drawCard({ 20.0f,                   top, cardW, h }, "QUEUED",    snap.queued,     ACCENT_BLUE);
    drawCard({ 20.0f + (cardW+20.0f),   top, cardW, h }, "RUNNING",   snap.running,    ACCENT_AMBER);
    drawCard({ 20.0f + 2*(cardW+20.0f), top, cardW, h }, "COMPLETED", snap.completed,  ACCENT_GREEN);
    drawCard({ 20.0f + 3*(cardW+20.0f), top, cardW, h }, "TIMED OUT", snap.timed_out,  ACCENT_RED);
}

void Dashboard::drawCard(Rectangle b, const char* title, uint64_t value, Color accent) {
    DrawRectangleRec(b, BG_CARD);
    DrawRectangleLinesEx(b, 1.0f, BORDER_COLOR);
    DrawRectangle(static_cast<int>(b.x), static_cast<int>(b.y),
                  4, static_cast<int>(b.height), accent);

    DrawText(title, static_cast<int>(b.x) + 14, static_cast<int>(b.y) + 14, 12, TEXT_DIM);

    std::string val = std::to_string(value);
    int fw = MeasureText(val.c_str(), 36);
    DrawText(val.c_str(),
             static_cast<int>(b.x + b.width / 2.0f - fw / 2.0f),
             static_cast<int>(b.y + b.height / 2.0f - 2.0f),
             36, TEXT_PRIMARY);
}

void Dashboard::drawWorkerBar(const MetricsSnapshot& snap) const {
    float top  = 188.0f;
    float barW = static_cast<float>(width_) - 40.0f;

    DrawRectangle(20, static_cast<int>(top), static_cast<int>(barW), 36, BG_CARD);
    DrawRectangleLinesEx({ 20.0f, top, barW, 36.0f }, 1.0f, BORDER_COLOR);

    std::ostringstream label;
    label << "Workers: " << snap.worker_count
          << "   Total Submitted: " << snap.total_submitted;
    DrawText(label.str().c_str(), 28, static_cast<int>(top) + 11, 14, TEXT_PRIMARY);

    int blocks    = std::min(snap.worker_count, 48);
    int bx        = width_ - 20 - blocks * 14;
    for (int i = 0; i < blocks; ++i) {
        Color col = (i < snap.running) ? ACCENT_AMBER : ACCENT_GREEN;
        DrawRectangle(bx + i * 14, static_cast<int>(top) + 8, 10, 20, col);
    }
}

void Dashboard::drawActivityStrip(const MetricsSnapshot& snap) const {
    float top  = 232.0f;
    float h    = 54.0f;
    float barW = static_cast<float>(width_) - 40.0f;

    DrawRectangle(20, static_cast<int>(top), static_cast<int>(barW), static_cast<int>(h), BG_CARD);
    DrawRectangleLinesEx({ 20.0f, top, barW, h }, 1.0f, BORDER_COLOR);
    DrawText("PRIORITY ACTIVITY", 30, static_cast<int>(top) + 6, 11, TEXT_DIM);

    if (snap.recent_events.empty()) {
        DrawText("Waiting for tasks...", 30, static_cast<int>(top) + 24, 13, TEXT_DIM);
        return;
    }

    // Draw chips from left to right: newest events at right
    float chipW  = 95.0f;
    float chipH  = 28.0f;
    float chipY  = top + h / 2.0f - chipH / 2.0f + 3.0f;
    float startX = 30.0f;

    for (size_t i = 0; i < snap.recent_events.size(); ++i) {
        const auto& ev  = snap.recent_events[i];
        float cx = startX + static_cast<float>(i) * (chipW + 6.0f);

        // State color for chip border/left bar
        Color stateCol = (ev.state == "QUEUED")  ? ACCENT_BLUE  :
                         (ev.state == "RUNNING") ? ACCENT_AMBER :
                         (ev.state == "DONE")    ? ACCENT_GREEN : ACCENT_RED;

        // Priority badge color: 8-10 red, 5-7 amber, 1-4 blue
        Color prioCol  = (ev.priority >= 8) ? ACCENT_RED   :
                         (ev.priority >= 5) ? ACCENT_AMBER : ACCENT_BLUE;

        DrawRectangle(static_cast<int>(cx), static_cast<int>(chipY),
                      static_cast<int>(chipW), static_cast<int>(chipH), BG_PANEL);
        DrawRectangleLinesEx({ cx, chipY, chipW, chipH }, 1.0f, stateCol);
        // Left priority bar
        DrawRectangle(static_cast<int>(cx), static_cast<int>(chipY),
                      3, static_cast<int>(chipH), prioCol);

        // "P:9" badge
        std::string pstr = "P:" + std::to_string(ev.priority);
        DrawText(pstr.c_str(), static_cast<int>(cx) + 7, static_cast<int>(chipY) + 4, 11, prioCol);

        // Task ID (truncated)
        std::string idstr = "#" + std::to_string(ev.id);
        DrawText(idstr.c_str(), static_cast<int>(cx) + 29, static_cast<int>(chipY) + 4, 11, TEXT_PRIMARY);

        // State label
        DrawText(ev.state.c_str(), static_cast<int>(cx) + 7, static_cast<int>(chipY) + 16, 10, stateCol);
    }
}

void Dashboard::drawGraph(const GraphBuffer& buf,
                          const char*        label,
                          const char*        unit,
                          Rectangle          bounds,
                          Color              line_color) const {
    DrawRectangleRec(bounds, BG_CARD);
    DrawRectangleLinesEx(bounds, 1.0f, BORDER_COLOR);

    std::string header = std::string(label) + " (" + unit + ")";
    DrawText(header.c_str(), static_cast<int>(bounds.x) + 10, static_cast<int>(bounds.y) + 8, 13, TEXT_DIM);

    const auto& data = buf.data();
    if (data.size() < 2) {
        EndDrawing();
        BeginDrawing();
        return;
    }

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

    float last = data.back();
    std::ostringstream val;
    val << std::fixed << std::setprecision(1) << last;
    DrawText(val.str().c_str(),
             static_cast<int>(bounds.x + bounds.width - 50),
             static_cast<int>(bounds.y + 8),
             14, line_color);

    DrawText("0", static_cast<int>(gx), static_cast<int>(gy + gh - 8), 10, TEXT_DIM);
    std::string top_label = std::to_string(static_cast<int>(peak));
    DrawText(top_label.c_str(), static_cast<int>(gx), static_cast<int>(gy), 10, TEXT_DIM);
}

void Dashboard::drawStatsPanel(const MetricsSnapshot& snap, float top) const {
    float panW = static_cast<float>(width_) - 40.0f;
    float panH = static_cast<float>(height_) - top - 36.0f;

    DrawRectangle(20, static_cast<int>(top), static_cast<int>(panW), static_cast<int>(panH), BG_CARD);
    DrawRectangleLinesEx({ 20.0f, top, panW, panH }, 1.0f, BORDER_COLOR);
    DrawText("Performance Statistics", 32, static_cast<int>(top) + 10, 14, TEXT_DIM);

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    float col1x = 40.0f;
    float col2x = panW / 2.0f + 40.0f;
    float row1y = top + 34.0f;
    float row2y = top + 64.0f;
    float row3y = top + 94.0f;

    auto drawStat = [&](float x, float y, const std::string& key, const std::string& val, Color vc) {
        std::string full = key + val;
        DrawText(key.c_str(), static_cast<int>(x), static_cast<int>(y), 15, TEXT_DIM);
        DrawText(val.c_str(), static_cast<int>(x) + MeasureText(key.c_str(), 15),
                 static_cast<int>(y), 15, vc);
    };

    oss.str(""); oss << snap.throughput_per_second << " tasks/sec";
    drawStat(col1x, row1y, "Throughput:    ", oss.str(), ACCENT_GREEN);

    oss.str(""); oss << snap.average_latency_ms << " ms";
    drawStat(col1x, row2y, "Avg Latency:   ", oss.str(), ACCENT_AMBER);

    oss.str(""); oss << snap.total_submitted;
    drawStat(col1x, row3y, "Total Jobs:    ", oss.str(), TEXT_PRIMARY);

    oss.str(""); oss << snap.worker_count << " threads active";
    drawStat(col2x, row1y, "Worker Pool:   ", oss.str(), ACCENT_VIOLET);

    oss.str(""); oss << snap.timed_out;
    drawStat(col2x, row2y, "Timed Out:     ", oss.str(), ACCENT_RED);

    uint64_t success_rate = (snap.total_submitted > 0)
        ? (snap.completed * 100 / snap.total_submitted)
        : 100;
    oss.str(""); oss << success_rate << "%";
    drawStat(col2x, row3y, "Success Rate:  ", oss.str(),
             success_rate > 90 ? ACCENT_GREEN : ACCENT_RED);
}

void Dashboard::drawFooter() const {
    int fy = height_ - 28;
    DrawRectangle(0, fy, width_, 28, BG_PANEL);
    DrawLine(0, fy, width_, fy, BORDER_COLOR);
    DrawText("ESC to quit   |   Priority Scheduling  |  Dynamic Scaling  |  Timeout Enforcement  |  Live Metrics",
             20, fy + 7, 12, TEXT_DIM);
    int fps = GetFPS();
    std::string fpsLabel = "FPS: " + std::to_string(fps);
    DrawText(fpsLabel.c_str(), width_ - MeasureText(fpsLabel.c_str(), 12) - 20, fy + 7, 12, TEXT_DIM);
}
