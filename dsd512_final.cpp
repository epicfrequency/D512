#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>

struct SDM5 {
    double s[5] = {0,0,0,0,0};
    double q = 0;
    const double LIMIT = 100.0; // 维持你原来的 100.0
    double gain_factor = 0.5;   // 将硬编码改为变量

    inline int modulate(double input) {
        // 使用动态增益
        double x = input * gain_factor;

        s[0] += (x - q);
        s[1] += (s[0] - q * 0.5);
        s[2] += (s[1] - q * 0.25);
        s[3] += (s[2] - q * 0.125);
        s[4] += (s[3] - q * 0.0625);

        // 维持你原来的限位逻辑
        for (int i = 0; i < 5; ++i) {
            if (s[i] > LIMIT) s[i] = LIMIT;
            else if (s[i] < -LIMIT) s[i] = -LIMIT;
        }

        int bit = (s[4] >= 0) ? 1 : 0;
        q = bit ? 1.0 : -1.0;
        return bit;
    }
};

int main(int argc, char* argv[]) {
    // 接收参数：如果脚本没传，就用你最放心的 0.5
    double target_gain = 0.5;
    if (argc > 1) {
        target_gain = std::atof(argv[1]);
    }

    // 维持你原来的高性能 I/O 设置
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    SDM5 mod_l, mod_r;
    mod_l.gain_factor = target_gain;
    mod_r.gain_factor = target_gain;

    float current_f[2] = {0, 0};
    float next_f[2] = {0, 0};
    uint8_t out_l[8], out_r[8];

    if (!std::cin.read(reinterpret_cast<char*>(current_f), 8)) return 0;

    // 打印状态到 stderr (不会混入音频流)
    std::cerr << "\n[Lumen Engine] Gain Set to: " << target_gain << std::endl;

    while (std::cin.read(reinterpret_cast<char*>(next_f), 8)) {
        for (int i = 0; i < 8; ++i) {
            uint8_t byte_l = 0, byte_r = 0;

            for (int bit = 7; bit >= 0; --bit) {
                // 完全维持你原来的插值逻辑
                float alpha = static_cast<float>(i * 8 + (7 - bit)) / 64.0f;
                double pcm_l = (double)current_f[0] * (1.0f - alpha) + (double)next_f[0] * alpha;
                double pcm_r = (double)current_f[1] * (1.0f - alpha) + (double)next_f[1] * alpha;

                if (mod_l.modulate(pcm_l)) byte_l |= (1 << bit);
                if (mod_r.modulate(pcm_r)) byte_r |= (1 << bit);
            }
            out_l[i] = byte_l;
            out_r[i] = byte_r;
        }

        // 维持你原本确定的 BE 封装顺序
        std::cout.write(reinterpret_cast<char*>(&out_l[0]), 4);
        std::cout.write(reinterpret_cast<char*>(&out_r[0]), 4);
        std::cout.write(reinterpret_cast<char*>(&out_l[4]), 4);
        std::cout.write(reinterpret_cast<char*>(&out_r[4]), 4);

        current_f[0] = next_f[0];
        current_f[1] = next_f[1];
    }

    return 0;
}