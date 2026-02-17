


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
    
//     // 监控专用变量
//     double max_s_this_period = 0;
//     bool clipped_this_period = false;

//     // 如果算法逃逸（数值大得离谱），强制重置积分器
//     void reset() {
//         for(int i=0; i<5; ++i) s[i] = 0;
//         q = 0;
//     }

//     inline int modulate(double input) {
//         double x = input * gain_factor;
        
//         s[0] += (x - q);
//         s[1] += (s[0] - q * 0.5);
//         s[2] += (s[1] - q * 0.25);
//         s[3] += (s[2] - q * 0.125);
//         s[4] += (s[3] - q * 0.0625);

//         double current_max = 0;
//         for (int i = 0; i < 5; ++i) {
//             double a = std::abs(s[i]);
//             if (a > current_max) current_max = a;
//             if (a >= LIMIT) {
//                 s[i] = (s[i] > 0) ? LIMIT : -LIMIT;
//                 clipped_this_period = true;
//             }
//         }
        
//         // 自动救治：如果压力超过 LIMIT 的 10 倍，说明算法崩了，强制重置
//         if (current_max > 1000.0) reset();

//         if (current_max > max_s_this_period) max_s_this_period = current_max;

//         int bit = (s[4] >= 0) ? 1 : 0;
//         q = bit ? 1.0 : -1.0;
//         return bit;
//     }
// };

// // 修正后的长条函数：确保数值一定能显示
// std::string get_bar_row(std::string label, float val, float max_v, std::string color, std::string suffix) {
//     const int width = 40;
//     int filled = static_cast<int>((std::min(val, max_v) / max_v) * width);
//     if (filled < 0) filled = 0;
    
//     std::string bar = label + " " + color + "[";
//     for (int i = 0; i < width; ++i) bar += (i < filled) ? "#" : "-";
//     bar += "] " + suffix + "\033[0m\n";
//     return bar;
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
    
//     // 初始化清屏
//     std::cerr << "\033[2J\033[H\033[?25l"; 

//     while (std::cin.read(reinterpret_cast<char*>(nxt), 8)) {
//         if (std::abs(cur[0]) > peak_l) peak_l = std::abs(cur[0]);
//         if (std::abs(cur[1]) > peak_r) peak_r = std::abs(cur[1]);

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

//         // 每秒刷新约 10 次，不给 CPU 增加显示负担
//         if (total_frames % 44100 == 0) {
//             std::cerr << "\033[H"; // 回到顶部
//             std::cerr << "\033[1;36mLUMEN DSD512 ENGINE | GAIN: " << std::fixed << std::setprecision(2) << target_gain << "\033[0m\n\n";

//             auto render = [&](std::string ch, float peak, SDM5& m) {
//                 float db = (peak < 1e-6) ? -60.0f : 20.0f * std::log10(peak);
//                 std::string db_txt = std::to_string((int)db) + " dB";
//                 std::cerr << get_bar_row(ch + " LEVEL ", db + 60.0f, 60.0f, "\033[1;32m", db_txt);
                
//                 std::string s_txt = m.clipped_this_period ? "CLIP!!" : "STABLE";
//                 std::cerr << get_bar_row(ch + " STRESS", m.max_s_this_period, 100.0f, "\033[1;31m", s_txt);
                
//                 // 重置本周期监控
//                 m.max_s_this_period = 0;
//                 m.clipped_this_period = false;
//             };

//             render("L", peak_l, mod_l);
//             std::cerr << "\n";
//             render("R", peak_r, mod_r);
//             peak_l = 0; peak_r = 0;
//             std::cerr << std::flush;
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
#include <sstream>

struct SDM5 {
    double s[5] = {0,0,0,0,0};
    double q = 0;
    const double LIMIT = 150.0; // 物理上限
    double max_s_this_period = 0;

    void reset() { for(int i=0; i<5; ++i) s[i]=0; q=0; }

    inline int modulate(double x) {
        // --- 稳定版系数：防止冲向 1085 ---
        s[0] += (x - q * 0.5);
        s[1] += (s[0] * 0.5 - q * 0.25);
        s[2] += (s[1] * 0.5 - q * 0.125);
        s[3] += (s[2] * 0.5 - q * 0.0625);
        s[4] += (s[3] * 0.5 - q * 0.0312);

        double cur_max = 0;
        for (int i = 0; i < 5; ++i) {
            double a = std::abs(s[i]);
            if (a > cur_max) cur_max = a;
            if (a >= LIMIT) s[i] = (s[i] > 0) ? LIMIT : -LIMIT;
        }
        if (cur_max > max_s_this_period) max_s_this_period = cur_max;
        if (cur_max > 400.0) reset(); 

        int bit = (s[4] >= 0) ? 1 : 0;
        q = bit ? 1.0 : -1.0;
        return bit;
    }
};

// 恢复你最喜欢的动态 Bar
std::string get_dynamic_bar(std::string label, float val, std::string color_safe) {
    const int width = 40;
    float range_max = 150.0f; // 针对新系数调整量程
    if (val > 150.0f) range_max = 500.0f;

    int filled = static_cast<int>((std::min(val, range_max) / range_max) * width);
    std::string color = (val > 140.0f) ? "\033[1;31m" : color_safe;
    
    std::string bar = label + " " + color + "[";
    for (int i = 0; i < width; ++i) bar += (i < filled) ? "#" : "-";
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << val;
    return bar + "] " + ss.str() + " (R:" + std::to_string((int)range_max) + ")\033[0m\n";
}

int main(int argc, char* argv[]) {
    bool is_ios_paid = true; 
    double target_gain = (argc > 1) ? std::atof(argv[1]) : 0.2;

    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    SDM5 mod_l, mod_r;
    float cur[2], nxt[2];
    uint64_t total_frames = 0;
    float peak_l = 0, peak_r = 0;

    if (!std::cin.read(reinterpret_cast<char*>(cur), 8)) return 0;
    std::cerr << "\033[2J\033[H\033[?25l"; 

    while (std::cin.read(reinterpret_cast<char*>(nxt), 8)) {
        if (!is_ios_paid) {
            uint32_t mute[4] = {0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA};
            std::cout.write(reinterpret_cast<char*>(mute), 16);
            continue;
        }

        peak_l = std::max(peak_l, std::abs(cur[0]));
        peak_r = std::max(peak_r, std::abs(cur[1]));

        uint8_t buffer[16];
        for (int chunk = 0; chunk < 2; ++chunk) {
            for (int ch = 0; ch < 2; ++ch) {
                for (int byte = 0; byte < 4; ++byte) {
                    uint8_t out_byte = 0;
                    for (int bit = 7; bit >= 0; --bit) {
                        float alpha = ((chunk * 64) + (byte * 8) + (7 - bit)) / 128.0f;
                        double pl = cur[0] * (1.0f - alpha) + nxt[0] * alpha;
                        double pr = cur[1] * (1.0f - alpha) + nxt[1] * alpha;
                        
                        if (ch == 0) {
                            if (mod_l.modulate(pl * target_gain)) out_byte |= (1 << bit);
                        } else {
                            if (mod_r.modulate(pr * target_gain)) out_byte |= (1 << bit);
                        }
                    }
                    buffer[chunk * 8 + ch * 4 + byte] = out_byte;
                }
            }
        }
        std::cout.write(reinterpret_cast<char*>(buffer), 16);

        cur[0] = nxt[0]; cur[1] = nxt[1];
        if (++total_frames % 16384 == 0) {
            std::cerr << "\033[H\033[1;36mLUMEN DSD512 | GAIN: " << target_gain << "\033[0m\n\n";
            auto render = [&](std::string ch, float p, SDM5& m) {
                float db = (p < 1e-6) ? -60.0f : 20.0f * std::log10(p);
                std::cerr << ch << " LEVEL  \033[1;32m" << std::fixed << std::setprecision(1) << db << " dB\033[0m\n";
                std::cerr << get_dynamic_bar(ch + " STRESS", m.max_s_this_period, "\033[1;30m");
                m.max_s_this_period = 0;
            };
            render("L", peak_l, mod_l); std::cerr << "\n";
            render("R", peak_r, mod_r);
            peak_l = 0; peak_r = 0;
            std::cerr << std::flush;
        }
    }
    return 0;
}