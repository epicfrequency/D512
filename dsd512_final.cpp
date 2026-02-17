#include <iostream>
#include <vector>
#include <cstdint>

/**
 * 5阶 CLANS 调制器
 * 针对 DSD512 (64x 升频) 优化的反馈系数
 */
struct SDM5 {
    double s[5] = {0,0,0,0,0};
    double q = 0;
    const double LIMIT = 100.0;

    // 核心调制：输入插值后的 PCM 样点，输出 1 个比特
    inline int modulate(double input) {
        // 增益调整 (0.5 = -6dB) 以确保稳定性
        double x = input * 0.5;

        s[0] += (x - q);
        s[1] += (s[0] - q * 0.5);
        s[2] += (s[1] - q * 0.25);
        s[3] += (s[2] - q * 0.125);
        s[4] += (s[3] - q * 0.0625);

        // 稳定性限位
        for (int i = 0; i < 5; ++i) {
            if (s[i] > LIMIT) s[i] = LIMIT;
            else if (s[i] < -LIMIT) s[i] = -LIMIT;
        }

        int bit = (s[4] >= 0) ? 1 : 0;
        q = bit ? 1.0 : -1.0;
        return bit;
    }
};

int main() {
    // 强制高性能二进制 I/O
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    SDM5 mod_l, mod_r;
    float current_f[2] = {0, 0};
    float next_f[2] = {0, 0};
    uint8_t out_l[8], out_r[8];

    // 预读第一帧
    if (!std::cin.read(reinterpret_cast<char*>(current_f), 8)) return 0;

    // 循环读取下一帧，以便进行线性插值
    while (std::cin.read(reinterpret_cast<char*>(next_f), 8)) {
        
        // 64倍升频：将一个 PCM 间隔分成 64 个细小的步骤
        for (int i = 0; i < 8; ++i) { // 每次生成 1 字节 (8 bits)
            uint8_t byte_l = 0, byte_r = 0;

            for (int bit = 7; bit >= 0; --bit) {
                // 计算插值系数 alpha (从 0.0 到 1.0)
                // 这里的 64.0 是因为我们从 current 平滑滑动到 next
                float alpha = static_cast<float>(i * 8 + (7 - bit)) / 64.0f;

                // 线性插值 PCM 样点
                double pcm_l = current_f[0] * (1.0f - alpha) + next_f[0] * alpha;
                double pcm_r = current_f[1] * (1.0f - alpha) + next_f[1] * alpha;

                // 调制并填充位 (MSB First)
                if (mod_l.modulate(pcm_l)) byte_l |= (1 << bit);
                if (mod_r.modulate(pcm_r)) byte_r |= (1 << bit);
            }
            out_l[i] = byte_l;
            out_r[i] = byte_r;
        }

        // 适配 DSD_U32_BE 格式：[L_4字节][R_4字节][L_4字节][R_4字节]
        std::cout.write(reinterpret_cast<char*>(&out_l[0]), 4);
        std::cout.write(reinterpret_cast<char*>(&out_r[0]), 4);
        std::cout.write(reinterpret_cast<char*>(&out_l[4]), 4);
        std::cout.write(reinterpret_cast<char*>(&out_r[4]), 4);

        // 更新当前帧
        current_f[0] = next_f[0];
        current_f[1] = next_f[1];
    }

    return 0;
}
#include <iostream>
#include <vector>
#include <cstdint>

/**
 * 5阶 CLANS 调制器
 * 针对 DSD512 (64x 升频) 优化的反馈系数
 */
struct SDM5 {
    double s[5] = {0,0,0,0,0};
    double q = 0;
    const double LIMIT = 100.0;

    // 核心调制：输入插值后的 PCM 样点，输出 1 个比特
    inline int modulate(double input) {
        // 增益调整 (0.5 = -6dB) 以确保稳定性
        double x = input * 0.5;

        s[0] += (x - q);
        s[1] += (s[0] - q * 0.5);
        s[2] += (s[1] - q * 0.25);
        s[3] += (s[2] - q * 0.125);
        s[4] += (s[3] - q * 0.0625);

        // 稳定性限位
        for (int i = 0; i < 5; ++i) {
            if (s[i] > LIMIT) s[i] = LIMIT;
            else if (s[i] < -LIMIT) s[i] = -LIMIT;
        }

        int bit = (s[4] >= 0) ? 1 : 0;
        q = bit ? 1.0 : -1.0;
        return bit;
    }
};

int main() {
    // 强制高性能二进制 I/O
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    SDM5 mod_l, mod_r;
    float current_f[2] = {0, 0};
    float next_f[2] = {0, 0};
    uint8_t out_l[8], out_r[8];

    // 预读第一帧
    if (!std::cin.read(reinterpret_cast<char*>(current_f), 8)) return 0;

    // 循环读取下一帧，以便进行线性插值
    while (std::cin.read(reinterpret_cast<char*>(next_f), 8)) {
        
        // 64倍升频：将一个 PCM 间隔分成 64 个细小的步骤
        for (int i = 0; i < 8; ++i) { // 每次生成 1 字节 (8 bits)
            uint8_t byte_l = 0, byte_r = 0;

            for (int bit = 7; bit >= 0; --bit) {
                // 计算插值系数 alpha (从 0.0 到 1.0)
                // 这里的 64.0 是因为我们从 current 平滑滑动到 next
                float alpha = static_cast<float>(i * 8 + (7 - bit)) / 64.0f;

                // 线性插值 PCM 样点
                double pcm_l = current_f[0] * (1.0f - alpha) + next_f[0] * alpha;
                double pcm_r = current_f[1] * (1.0f - alpha) + next_f[1] * alpha;

                // 调制并填充位 (MSB First)
                if (mod_l.modulate(pcm_l)) byte_l |= (1 << bit);
                if (mod_r.modulate(pcm_r)) byte_r |= (1 << bit);
            }
            out_l[i] = byte_l;
            out_r[i] = byte_r;
        }

        // 适配 DSD_U32_BE 格式：[L_4字节][R_4字节][L_4字节][R_4字节]
        std::cout.write(reinterpret_cast<char*>(&out_l[0]), 4);
        std::cout.write(reinterpret_cast<char*>(&out_r[0]), 4);
        std::cout.write(reinterpret_cast<char*>(&out_l[4]), 4);
        std::cout.write(reinterpret_cast<char*>(&out_r[4]), 4);

        // 更新当前帧
        current_f[0] = next_f[0];
        current_f[1] = next_f[1];
    }

    return 0;
}