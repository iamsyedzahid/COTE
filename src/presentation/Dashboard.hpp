#pragma once
#include "infrastructure/MetricsService.hpp"
#include "presentation/GraphBuffer.hpp"
#include "raylib.h"
#include <memory>
#include <string>

class Dashboard {
public:
    Dashboard(MetricsService& metrics, int width, int height);
    ~Dashboard();

    bool shouldClose() const;
    void update();
    void render();

private:
    void drawHeader() const;
    void drawTaskCountCards(const MetricsSnapshot& snap) const;
    void drawWorkerBar(const MetricsSnapshot& snap) const;
    void drawActivityStrip(const MetricsSnapshot& snap) const;
    void drawGraph(const GraphBuffer& buf,
                   const char*        label,
                   const char*        unit,
                   Rectangle          bounds,
                   Color              line_color) const;
    void drawStatsPanel(const MetricsSnapshot& snap, float top) const;
    void drawFooter() const;

    static void drawCard(Rectangle bounds,
                         const char* title,
                         uint64_t    value,
                         Color       accent);

    MetricsService& metrics_;
    int             width_;
    int             height_;

    GraphBuffer throughput_history_;
    GraphBuffer latency_history_;
    GraphBuffer queue_history_;

    static constexpr size_t GRAPH_CAPACITY = 120;
};
