

#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <string>
#include <iomanip>
#include <sstream>

struct SDM5 {
    double s[5] = {0,0,0,0,0};
    double q = 0;
    const double LIMIT = 100.0;
    double gain_factor = 0.5;
    
    // DC Blocker
    double last_x = 0, last_y = 0;
    const double R = 0.99999; 

    // 监控
    double max_s_this_period = 0;
    bool clipped_this_period = false;

    inline int modulate(double input) {
        double y = input - last_x + R * last_y;
        last_x = input; last_y = y;

        double x = y * gain_factor;
        
        s[0] += (x - q);
        s[1] += (s[0] - q * 0.5);
        s[2] += (s[1] - q * 0.25);
        s[3] += (s[2] - q * 0.125);
        s[4] += (s[3] - q * 0.0625);

        double current_max = 0;
        for (int i = 0; i < 5; ++i) {
            double a = std::abs(s[i]);
            if (a > current_max) current_max = a;
            if (a >= LIMIT) {
                // 仅在计算时限位，但监控变量 current_max 记录真实溢出值
                if (s[i] > LIMIT) s[i] = LIMIT;
                else if (s[i] < -LIMIT) s[i] = -LIMIT;
                clipped_this_period = true;
            }
        }
        
        if (current_max > max_s_this_period) max_s_this_period = current_max;
        
        // 自动重置保护（防止彻底锁死）
        if (current_max > 2000.0) {
            for(int i=0; i<5; ++i) s[i] = 0;
        }

        int bit = (s[4] >= 0) ? 1 : 0;
        q = (bit) ? 1.0 : -1.0;
        return bit;
    }
};

// 增强版 UI 渲染：支持溢出数值显示
std::string get_bar_row(std::string label, float val, float max_v, std::string color, std::string suffix) {
    const int width = 40;
    // 进度条依然基于 LIMIT(100) 渲染，但后缀显示真实数值
    int filled = static_cast<int>((std::min(val, max_v) / max_v) * width);
    
    std::string bar = label + " " + color + "[";
    for (int i = 0; i < width; ++i) bar += (i < filled) ? "#" : "-";
    bar += "] " + suffix + "\033[0m\n";
    return bar;
}

int main(int argc, char* argv[]) {
    double target_gain = 0.5;
    if (argc > 1) target_gain = std::atof(argv[1]);

    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    SDM5 mod_l, mod_r;
    mod_l.gain_factor = target_gain;
    mod_r.gain_factor = target_gain;

    float cur[2], nxt[2];
    uint8_t out_l[8], out_r[8];
    uint64_t total_frames = 0;
    float peak_l = 0, peak_r = 0;

    if (!std::cin.read(reinterpret_cast<char*>(cur), 8)) return 0;
    std::cerr << "\033[2J\033[H\033[?25l"; 

    while (std::cin.read(reinterpret_cast<char*>(nxt), 8)) {
        if (std::abs(cur[0]) > peak_l) peak_l = std::abs(cur[0]);
        if (std::abs(cur[1]) > peak_r) peak_r = std::abs(cur[1]);

        for (int i = 0; i < 8; ++i) {
            uint8_t bl = 0, br = 0;
            for (int bit = 7; bit >= 0; --bit) {
                float alpha = (i * 8 + (7 - bit)) / 64.0f;
                double pl = cur[0] * (1.0f - alpha) + nxt[0] * alpha;
                double pr = cur[1] * (1.0f - alpha) + nxt[1] * alpha;
                if (mod_l.modulate(pl)) bl |= (1 << bit);
                if (mod_r.modulate(pr)) br |= (1 << bit);
            }
            out_l[i] = bl; out_r[i] = br;
        }
        std::cout.write(reinterpret_cast<char*>(out_l), 8);
        std::cout.write(reinterpret_cast<char*>(out_r), 8);

        cur[0] = nxt[0]; cur[1] = nxt[1];
        total_frames++;

        if (total_frames % 32768 == 0) {
            std::cerr << "\033[H"; 
            std::cerr << "\033[1;36mLUMEN DSD512 | GAIN: " << std::fixed << std::setprecision(2) << target_gain << "\033[0m\n\n";

            auto render_chan = [&](std::string ch, float p, SDM5& m) {
                // 音量条
                float db = (p < 1e-6) ? -60.0f : 20.0f * std::log10(p);
                std::string db_txt = std::to_string((int)db) + " dB  ";
                std::cerr << get_bar_row(ch + " LEVEL ", db + 60.0f, 60.0f, "\033[1;32m", db_txt);
                
                // 压力条：显示真实数值
                std::stringstream ss;
                ss << std::fixed << std::setprecision(1) << m.max_s_this_period;
                std::string s_val = ss.str();
                
                std::string status = m.clipped_this_period ? "\033[1;31mCLIP " + s_val + "\033[0m" : "\033[1;30mSAFE " + s_val + "\033[0m";
                std::cerr << get_bar_row(ch + " STRESS", m.max_s_this_period, 100.0f, "\033[1;31m", status);
                
                m.max_s_this_period = 0;
                m.clipped_this_period = false;
            };

            render_chan("L", peak_l, mod_l);
            std::cerr << "\n";
            render_chan("R", peak_r, mod_r);
            peak_l = 0; peak_r = 0;
            std::cerr << std::flush;
        }
    }
    std::cerr << "\033[?25h";
    return 0;
}