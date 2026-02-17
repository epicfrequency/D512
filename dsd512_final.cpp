

// #include <iostream>
// #include <vector>
// #include <cstdint>
// #include <cmath>
// #include <algorithm>
// #include <string>
// #include <iomanip>

// struct SDM5 {
//     double s[5] = {0,0,0,0,0};
//     double q = 0;
//     const double LIMIT = 100.0;
//     double gain_factor = 0.5;
//     double current_stress = 0;
//     bool had_clip = false; // 新增：标记是否发生过硬截断

//     inline int modulate(double input) {
//         double x = input * gain_factor;
//         s[0] += (x - q);
//         s[1] += (s[0] - q * 0.5);
//         s[2] += (s[1] - q * 0.25);
//         s[3] += (s[2] - q * 0.125);
//         s[4] += (s[3] - q * 0.0625);

//         double max_s = 0;
//         for (int i = 0; i < 5; ++i) {
//             double abs_s = std::abs(s[i]);
//             if (abs_s > max_s) max_s = abs_s;
//             if (abs_s >= LIMIT) {
//                 if (s[i] > LIMIT) s[i] = LIMIT;
//                 else if (s[i] < -LIMIT) s[i] = -LIMIT;
//                 had_clip = true; // 只要发生截断，立即记录
//             }
//         }
//         // 确保 UI 刷新前，current_stress 始终保留这期间出现过的最大值
//         if (max_s > current_stress) current_stress = max_s;

//         int bit = (s[4] >= 0) ? 1 : 0;
//         q = bit ? 1.0 : -1.0;
//         return bit;
//     }
// };

// std::string make_long_bar(int filled, int width, std::string color, std::string label, std::string suffix) {
//     std::string res = label + " " + color + "[";
//     for (int i = 0; i < width; ++i) {
//         if (i < filled) res += "#";
//         else res += "-";
//     }
//     return res + "] " + suffix + "\033[0m";
// }

// int main(int argc, char* argv[]) {
//     double target_gain = 0.5;
//     if (argc > 1) target_gain = std::atof(argv[1]);

//     std::ios_base::sync_with_stdio(false);
//     std::cin.tie(NULL);

//     SDM5 mod_l, mod_r;
//     mod_l.gain_factor = target_gain;
//     mod_r.gain_factor = target_gain;

//     float cur[2], nxt[2];
//     uint8_t out_l[8], out_r[8];
//     uint64_t total_frames = 0;
//     float peak_l = 0, peak_r = 0;

//     if (!std::cin.read(reinterpret_cast<char*>(cur), 8)) return 0;
//     std::cerr << "\033[2J\033[?25l"; 

//     while (std::cin.read(reinterpret_cast<char*>(nxt), 8)) {
//         peak_l = std::max(peak_l, std::abs(cur[0]));
//         peak_r = std::max(peak_r, std::abs(cur[1]));

//         for (int i = 0; i < 8; ++i) {
//             uint8_t bl = 0, br = 0;
//             for (int bit = 7; bit >= 0; --bit) {
//                 float alpha = static_cast<float>(i * 8 + (7 - bit)) / 64.0f;
//                 double pl = (double)cur[0] * (1.0f - alpha) + (double)nxt[0] * alpha;
//                 double pr = (double)cur[1] * (1.0f - alpha) + (double)nxt[1] * alpha;
//                 if (mod_l.modulate(pl)) bl |= (1 << bit);
//                 if (mod_r.modulate(pr)) br |= (1 << bit);
//             }
//             out_l[i] = bl; out_r[i] = br;
//         }

//         std::cout.write(reinterpret_cast<char*>(&out_l[0]), 4);
//         std::cout.write(reinterpret_cast<char*>(&out_r[0]), 4);
//         std::cout.write(reinterpret_cast<char*>(&out_l[4]), 4);
//         std::cout.write(reinterpret_cast<char*>(&out_r[4]), 4);

//         cur[0] = nxt[0]; cur[1] = nxt[1];
//         total_frames++;

//         // 稍微加快 UI 刷新率
//         if (total_frames % 8192 == 0) {
//             std::cerr << "\033[H"; 
//             std::cerr << "\033[1;36m--- LUMEN DSD512 | GAIN: " << target_gain << " | MONITOR ACTIVE ---\033[0m\n\n";
            
//             auto render_chan = [&](char label, float peak, SDM5& mod) {
//                 // PCM Level (Log Scale)
//                 float db = (peak < 1e-6) ? -60.0f : 20.0f * std::log10(peak);
//                 int p_filled = static_cast<int>((std::max(0.0f, db + 60.0f) / 60.0f) * 40);
//                 std::cerr << make_long_bar(p_filled, 40, "\033[1;32m", std::string(1, label) + " LEVEL ", std::to_string((int)db) + " dB") << "\n";
                
//                 // Stress Bar (Linear Scale 0-100)
//                 // 如果发生了 clip，强制显示红色报警文字
//                 std::string status = mod.had_clip ? "\033[1;41m CLIP! \033[0m" : "\033[1;30m SAFE  \033[0m";
//                 int s_filled = static_cast<int>((std::min(100.0, mod.current_stress) / 100.0) * 40);
//                 std::cerr << make_long_bar(s_filled, 40, "\033[1;31m", std::string(1, label) + " STRESS", status) << "\n\n";
                
//                 // 重置本周期数据
//                 mod.current_stress *= 0.1; // 留一点残影
//                 mod.had_clip = false;
//             };

//             render_chan('L', peak_l, mod_l);
//             render_chan('R', peak_r, mod_r);
//             peak_l *= 0.5f; peak_r *= 0.5f;
//         }
//     }
//     std::cerr << "\033[?25h";
//     return 0;
// }


#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <string>
#include <iomanip>

struct SDM5 {
    double s[5] = {0,0,0,0,0};
    double q = 0;
    const double LIMIT = 100.0;
    double gain_factor = 0.5;
    
    // 监控专用变量
    double max_s_this_period = 0;
    bool clipped_this_period = false;

    // 如果算法逃逸（数值大得离谱），强制重置积分器
    void reset() {
        for(int i=0; i<5; ++i) s[i] = 0;
        q = 0;
    }

    inline int modulate(double input) {
        double x = input * gain_factor;
        
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
                s[i] = (s[i] > 0) ? LIMIT : -LIMIT;
                clipped_this_period = true;
            }
        }
        
        // 自动救治：如果压力超过 LIMIT 的 10 倍，说明算法崩了，强制重置
        if (current_max > 1000.0) reset();

        if (current_max > max_s_this_period) max_s_this_period = current_max;

        int bit = (s[4] >= 0) ? 1 : 0;
        q = bit ? 1.0 : -1.0;
        return bit;
    }
};

// 修正后的长条函数：确保数值一定能显示
std::string get_bar_row(std::string label, float val, float max_v, std::string color, std::string suffix) {
    const int width = 40;
    int filled = static_cast<int>((std::min(val, max_v) / max_v) * width);
    if (filled < 0) filled = 0;
    
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
    
    // 初始化清屏
    std::cerr << "\033[2J\033[H\033[?25l"; 

    while (std::cin.read(reinterpret_cast<char*>(nxt), 8)) {
        if (std::abs(cur[0]) > peak_l) peak_l = std::abs(cur[0]);
        if (std::abs(cur[1]) > peak_r) peak_r = std::abs(cur[1]);

        for (int i = 0; i < 8; ++i) {
            uint8_t bl = 0, br = 0;
            for (int bit = 7; bit >= 0; --bit) {
                float alpha = static_cast<float>(i * 8 + (7 - bit)) / 64.0f;
                double pl = (double)cur[0] * (1.0f - alpha) + (double)nxt[0] * alpha;
                double pr = (double)cur[1] * (1.0f - alpha) + (double)nxt[1] * alpha;
                
                if (mod_l.modulate(pl)) bl |= (1 << bit);
                if (mod_r.modulate(pr)) br |= (1 << bit);
            }
            out_l[i] = bl; out_r[i] = br;
        }

        std::cout.write(reinterpret_cast<char*>(&out_l[0]), 4);
        std::cout.write(reinterpret_cast<char*>(&out_r[0]), 4);
        std::cout.write(reinterpret_cast<char*>(&out_l[4]), 4);
        std::cout.write(reinterpret_cast<char*>(&out_r[4]), 4);

        cur[0] = nxt[0]; cur[1] = nxt[1];
        total_frames++;

        // 每秒刷新约 10 次，不给 CPU 增加显示负担
        if (total_frames % 44100 == 0) {
            std::cerr << "\033[H"; // 回到顶部
            std::cerr << "\033[1;36mLUMEN DSD512 ENGINE | GAIN: " << std::fixed << std::setprecision(2) << target_gain << "\033[0m\n\n";

            auto render = [&](std::string ch, float peak, SDM5& m) {
                float db = (peak < 1e-6) ? -60.0f : 20.0f * std::log10(peak);
                std::string db_txt = std::to_string((int)db) + " dB";
                std::cerr << get_bar_row(ch + " LEVEL ", db + 60.0f, 60.0f, "\033[1;32m", db_txt);
                
                std::string s_txt = m.clipped_this_period ? "CLIP!!" : "STABLE";
                std::cerr << get_bar_row(ch + " STRESS", m.max_s_this_period, 100.0f, "\033[1;31m", s_txt);
                
                // 重置本周期监控
                m.max_s_this_period = 0;
                m.clipped_this_period = false;
            };

            render("L", peak_l, mod_l);
            std::cerr << "\n";
            render("R", peak_r, mod_r);
            peak_l = 0; peak_r = 0;
            std::cerr << std::flush;
        }
    }
    std::cerr << "\033[?25h";
    return 0;
}