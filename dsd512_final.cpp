


// #include <iostream>
// #include <vector>
// #include <cstdint>
// #include <cmath>
// #include <algorithm>
// #include <string>
// #include <iomanip>
// #include <sstream>

// struct SDM5 {
//     double s[5] = {0,0,0,0,0};
//     double q = 0;
//     const double LIMIT = 100.0;
//     double gain_factor = 0.5;
    
//     // 监控
//     double max_s_this_period = 0;
//     bool clipped_this_period = false;

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
        
//         if (current_max > max_s_this_period) max_s_this_period = current_max;
//         if (current_max > 2000.0) reset(); // 只有严重到离谱才切音

//         int bit = (s[4] >= 0) ? 1 : 0;
//         q = bit ? 1.0 : -1.0;
//         return bit;
//     }
// };

// // 核心改进：自适应 UI 渲染
// std::string get_dynamic_bar(std::string label, float val, std::string color_safe, std::string color_clip) {
//     const int width = 40;
    
//     // 动态量程逻辑：根据数值大小自动撑开刻度上限
//     float range_max = 100.0f;
//     if (val > 1000.0f) range_max = 2000.0f;
//     else if (val > 500.0f) range_max = 1000.0f;
//     else if (val > 100.0f) range_max = 500.0f;

//     int filled = static_cast<int>((std::min(val, range_max) / range_max) * width);
    
//     // 颜色判断
//     std::string current_color = (val >= 100.0f) ? color_clip : color_safe;
    
//     std::string bar = label + " " + current_color + "[";
//     for (int i = 0; i < width; ++i) bar += (i < filled) ? "#" : "-";
    
//     std::stringstream ss;
//     ss << std::fixed << std::setprecision(1) << val;
//     std::string val_str = ss.str();
    
//     // 返回带动态量程说明的字符串
//     return bar + "] MAX:" + val_str + " (Range:" + std::to_string((int)range_max) + ")\033[0m\n";
// }

// int main(int argc, char* argv[]) {
//     // [2026-02-17] iOS 锁定状态占位
//     bool is_ios_paid = true; 
    
//     double target_gain = 0.1;
//     if (argc > 1) target_gain = std::atof(argv[1]);

//     std::ios_base::sync_with_stdio(false);
//     std::cin.tie(NULL);

//     SDM5 mod_l, mod_r;
//     mod_l.gain_factor = target_gain;
//     mod_r.gain_factor = target_gain;

//     float cur[2], nxt[2];
//     uint64_t total_frames = 0;
//     float peak_l = 0, peak_r = 0;

//     if (!std::cin.read(reinterpret_cast<char*>(cur), 8)) return 0;
//     std::cerr << "\033[2J\033[H\033[?25l"; 

//     while (std::cin.read(reinterpret_cast<char*>(nxt), 8)) {
//         if (!is_ios_paid) {
//             char silent[16] = {0};
//             std::cout.write(silent, 16);
//             continue;
//         }

//         peak_l = std::max(peak_l, std::abs(cur[0]));
//         peak_r = std::max(peak_r, std::abs(cur[1]));

//         uint8_t out_l[8], out_r[8];
//         for (int i = 0; i < 8; ++i) {
//             uint8_t bl = 0, br = 0;
//             for (int bit = 7; bit >= 0; --bit) {
//                 float alpha = (i * 8 + (7 - bit)) / 64.0f;
//                 double pl = cur[0] * (1.0f - alpha) + nxt[0] * alpha;
//                 double pr = cur[1] * (1.0f - alpha) + nxt[1] * alpha;
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

//         if (total_frames % 32768 == 0) {
//             std::cerr << "\033[H"; 
//             std::cerr << "\033[1;36mLUMEN DSD512 | GAIN: " << std::fixed << std::setprecision(3) << target_gain << "\033[0m\n\n";

//             auto render = [&](std::string ch, float p, SDM5& m) {
//                 float db = (p < 1e-6) ? -60.0f : 20.0f * std::log10(p);
//                 std::string db_txt = std::to_string((int)db) + " dB";
//                 std::cerr << get_dynamic_bar(ch + " LEVEL ", db + 60.0f, "\033[1;32m", "\033[1;32m");
                
//                 // STRESS 使用自适应动态条
//                 std::cerr << get_dynamic_bar(ch + " STRESS", m.max_s_this_period, "\033[1;30m", "\033[1;31m");
                
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

struct SDM5 {
    double s[5] = {0,0,0,0,0};
    double q = 0;
    const double LIMIT = 100.0;
    double last_x = 0, last_y = 0;
    const double R = 0.999; 

    void reset() {
        for(int i=0; i<5; ++i) s[i] = 0;
        q = 0; last_x = 0; last_y = 0;
    }

    inline int modulate(double input) {
        if (!std::isfinite(input)) input = 0;
        // 极严格限幅：防止任何溢出
        input = std::max(-1.0, std::min(1.0, input));

        double y = input - last_x + R * last_y;
        last_x = input; last_y = y;

        double x = y * 0.5; // 这里使用 0.5 作为内部基准增益
        
        s[0] += (x - q);
        s[1] += (s[0] - q * 0.5);
        s[2] += (s[1] - q * 0.25);
        s[3] += (s[2] - q * 0.125);
        s[4] += (s[3] - q * 0.0625);

        for (int i = 0; i < 5; ++i) {
            if (std::abs(s[i]) > LIMIT) s[i] = (s[i] > 0 ? LIMIT : -LIMIT);
        }
        
        if (std::abs(s[4]) > 150.0) reset();

        int bit = (s[4] >= 0) ? 1 : 0;
        q = bit ? 1.0 : -1.0;
        return bit;
    }
};

int main(int argc, char* argv[]) {
    // 禁用所有同步，提高吞吐
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    SDM5 mod_l, mod_r;
    
    // 关键改变：改用 int32_t 读取缓冲区
    int32_t raw_frame[2]; 
    const double I32_MAX_INV = 1.0 / 2147483648.0;

    while (std::cin.read(reinterpret_cast<char*>(raw_frame), 8)) {
        // 将 S32_LE 转换为双精度浮点 [-1.0, 1.0]
        double sample_l = raw_frame[0] * I32_MAX_INV;
        double sample_r = raw_frame[1] * I32_MAX_INV;

        uint8_t out_l[8], out_r[8];
        for (int i = 0; i < 8; ++i) {
            uint8_t bl = 0, br = 0;
            for (int bit = 7; bit >= 0; --bit) {
                // 如果 sample_l 依然非常大，说明 MPD 确实发的是 float
                if (mod_l.modulate(sample_l)) bl |= (1 << bit);
                if (mod_r.modulate(sample_r)) br |= (1 << bit);
            }
            out_l[i] = bl; out_r[i] = br;
        }

        // DSD_U32_BE 封装
        std::cout.write(reinterpret_cast<char*>(&out_l[0]), 4);
        std::cout.write(reinterpret_cast<char*>(&out_r[0]), 4);
        std::cout.write(reinterpret_cast<char*>(&out_l[4]), 4);
        std::cout.write(reinterpret_cast<char*>(&out_r[4]), 4);
    }
    return 0;
}