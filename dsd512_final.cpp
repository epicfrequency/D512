#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <chrono>
#include <algorithm>

struct SDM5 {
    double s[5] = {0,0,0,0,0};
    double q = 0;
    const double SAFE_ZONE = 80.0;
    uint64_t clip_count = 0;

    inline int modulate(double input) {
        double x = input * 0.42;
        s[0] += (x - q);
        s[1] += (s[0] - q * 0.50);
        s[2] += (s[1] - q * 0.26); 
        s[3] += (s[2] - q * 0.12);
        s[4] += (s[3] - q * 0.05);

        if (std::abs(s[4]) > SAFE_ZONE) {
            clip_count++; 
            for(int i=0; i<5; ++i) s[i] *= 0.90; 
        }

        int bit = (s[4] >= 0) ? 1 : 0;
        q = bit ? 1.0 : -1.0;
        return bit;
    }
};

// 渲染 VU Meter 字符串
std::string get_vu_bar(float peak, bool is_clipped) {
    int width = 20;
    int pos = static_cast<int>(std::min(1.0f, std::abs(peak)) * width);
    std::string bar = "[";
    for (int i = 0; i < width; ++i) {
        if (i < pos) bar += "=";
        else bar += " ";
    }
    bar += "]";
    if (is_clipped || std::abs(peak) > 0.98f) return "\033[1;31m" + bar + "\033[0m"; // 变红
    return "\033[1;32m" + bar + "\033[0m"; // 绿色
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    SDM5 mod_l, mod_r;
    float cur[2], nxt[2];
    uint8_t out_l[8], out_r[8];

    auto last_time = std::chrono::steady_clock::now();
    uint64_t total_frames = 0;
    float peak_l = 0, peak_r = 0;

    std::cerr << "\n\033[1;36m[Lumen Console] Engine Active. Rendering DSD512...\033[0m" << std::endl;

    while (true) {
        if (!std::cin.read(reinterpret_cast<char*>(cur), 8)) {
            std::cin.clear();
            continue; 
        }

        while (std::cin.read(reinterpret_cast<char*>(nxt), 8)) {
            // 追踪峰值 (用于 VU Meter)
            peak_l = std::max(peak_l, std::abs(cur[0]));
            peak_r = std::max(peak_r, std::abs(cur[1]));

            for (int i = 0; i < 8; ++i) {
                uint8_t b_l = 0, b_r = 0;
                for (int bit = 7; bit >= 0; --bit) {
                    double alpha = static_cast<double>(i * 8 + (7 - bit)) / 64.0;
                    double pcm_l = cur[0] * (1.0 - alpha) + nxt[0] * alpha;
                    double pcm_r = cur[1] * (1.0 - alpha) + nxt[1] * alpha;

                    if (mod_l.modulate(pcm_l)) b_l |= (1 << bit);
                    if (mod_r.modulate(pcm_r)) b_r |= (1 << bit);
                }
                out_l[i] = b_l; out_r[i] = b_r;
            }

            std::cout.write(reinterpret_cast<char*>(&out_l[0]), 4);
            std::cout.write(reinterpret_cast<char*>(&out_r[0]), 4);
            std::cout.write(reinterpret_cast<char*>(&out_l[4]), 4);
            std::cout.write(reinterpret_cast<char*>(&out_r[4]), 4);

            cur[0] = nxt[0]; cur[1] = nxt[1];
            total_frames++;

            // 每 1/15 秒更新一次 VU Meter (约 25600 帧 at 384k)
            if (total_frames % 25600 == 0) {
                uint64_t clips = mod_l.clip_count + mod_r.clip_count;
                std::cerr << "\rL: " << get_vu_bar(peak_l, mod_l.clip_count > 0) 
                          << " R: " << get_vu_bar(peak_r, mod_r.clip_count > 0)
                          << " | Clips: " << (clips > 0 ? "\033[1;31m" : "\033[1;37m") << clips << "\033[0m"
                          << " | DSD512 " << std::flush;
                
                // 缓慢释放峰值，模拟模拟表针衰减
                peak_l *= 0.9f; peak_r *= 0.9f;
                // 仅在每秒重置一次 Clipping 状态标志，让红色闪烁可见
                if (total_frames % 384000 == 0) {
                   // mod_l.clip_count = 0; mod_r.clip_count = 0; // 如果想累计则不重置
                }
            }
        }
        std::cin.clear();
    }
    return 0;
}