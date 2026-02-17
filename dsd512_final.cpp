#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <string>

/**
 * SDM5: 5阶 Sigma-Delta 调制器结构体
 * 采用 CLANS (Closed-Loop Analysis of Noise Shapers) 架构系数
 */
struct SDM5 {
    double s[5] = {0,0,0,0,0}; // 5个积分器状态变量
    double q = 0;              // 反馈量 (Quantizer output)
    const double SAFE_ZONE = 40.0; // 积分器能量阈值，超过此值说明算法即将崩溃
    uint64_t clip_count = 0;   // 累计削波次数
    double gain_factor = 0.25; // 核心增益系数（由外部输入控制）
    int clip_hold_timer = 0;   // 界面报警灯持续时间计时器

    /**
     * 调制核心函数：将双精度 PCM 样本转换为 1-bit DSD 比特
     * @param input: 输入的 PCM 样本值 (-1.0 到 1.0)
     * @return: 返回 1 (高电平) 或 0 (低电平)
     */
    inline int modulate(double input) {
        // 1. 应用输入增益
        double x = input * gain_factor;

        // 2. 5阶积分器链 (高阶反馈回路)
        // 系数经优化以确保 DSD512 下的信噪比与稳定性平衡
        s[0] += (x - q);
        s[1] += (s[0] - q * 0.50);
        s[2] += (s[1] - q * 0.25); 
        s[3] += (s[2] - q * 0.12);
        s[4] += (s[3] - q * 0.05);

        // 3. 稳定性监控 (Anti-Explosion Logic)
        // 如果最后一级积分器输出绝对值超过 SAFE_ZONE，则认为算法溢出
        if (std::abs(s[4]) > SAFE_ZONE) {
            clip_count++; 
            clip_hold_timer = 15; // 激活报警灯，持续约1秒
            // 强力能量回收：将所有积分器能量减半，强制回归线性区
            for(int i=0; i<5; ++i) s[i] *= 0.50; 
        }

        // 4. 量化器 (1-bit Quantizer)
        int bit = (s[4] >= 0) ? 1 : 0;
        q = bit ? 1.0 : -1.0; // 反馈值为 +1 或 -1
        return bit;
    }
};

/**
 * 渲染电平表字符串
 */
std::string get_level_bar(float peak) {
    int width = 25;
    int pos = static_cast<int>(std::min(1.0f, peak) * width);
    std::string bar = "\033[1;32m["; // 绿色开始
    for (int i = 0; i < width; ++i) {
        if (i < pos) bar += "=";
        else bar += " ";
    }
    return bar + "]\033[0m"; // 颜色重置
}

int main(int argc, char* argv[]) {
    // 参数解析：从命令行获取增益值
    double target_gain = 0.25;
    if (argc > 1) {
        try {
            target_gain = std::stod(argv[1]);
        } catch (...) {
            std::cerr << "Gain parameter error, using 0.25" << std::endl;
        }
    }

    // 提升 I/O 效率：关闭 C++ 与 C 标准流同步
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    SDM5 mod_l, mod_r;
    mod_l.gain_factor = target_gain;
    mod_r.gain_factor = target_gain;

    float cur[2], nxt[2]; // 用于线性插值的“当前帧”和“下一帧”
    uint8_t out_l[8], out_r[8]; // 缓存生成的 DSD 字节
    uint64_t total_frames = 0;
    float peak_l = 0, peak_r = 0;

    std::cerr << "\n\033[1;36m[Lumen Master] DSD512 Engine Active...\033[0m" << std::endl;

    while (true) {
        // 从 stdin 读取 MPD 传来的 32bit Float PCM (2声道，共8字节)
        if (!std::cin.read(reinterpret_cast<char*>(cur), 8)) {
            std::cin.clear();
            continue; // 等待流重新开启
        }

        // 循环读取下一帧，进行 64x 升频插值
        while (std::cin.read(reinterpret_cast<char*>(nxt), 8)) {
            // 实时追踪峰值用于电平表
            peak_l = std::max(peak_l, std::abs(cur[0]));
            peak_r = std::max(peak_r, std::abs(cur[1]));

            // 64倍升频逻辑：384kHz -> 24.576MHz (DSD512)
            for (int i = 0; i < 8; ++i) {
                uint8_t b_l = 0, b_r = 0;
                for (int bit = 7; bit >= 0; --bit) {
                    // 线性插值计算 (Alpha 从 0 到 1)
                    double alpha = static_cast<double>(i * 8 + (7 - bit)) / 64.0;
                    double pcm_l = static_cast<double>(cur[0]) * (1.0 - alpha) + static_cast<double>(nxt[0]) * alpha;
                    double pcm_r = static_cast<double>(cur[1]) * (1.0 - alpha) + static_cast<double>(nxt[1]) * alpha;
                    
                    // 调用调制器
                    if (mod_l.modulate(pcm_l)) b_l |= (1 << bit);
                    if (mod_r.modulate(pcm_r)) b_r |= (1 << bit);
                }
                out_l[i] = b_l; out_r[i] = b_r;
            }

            // 输出符合 ALSA DSD_U32_BE 格式的数据：
            // 每帧 32bit 数据中包含 32 个 DSD 采样，左右声道交织
            std::cout.write(reinterpret_cast<char*>(&out_l[0]), 4);
            std::cout.write(reinterpret_cast<char*>(&out_r[0]), 4);
            std::cout.write(reinterpret_cast<char*>(&out_l[4]), 4);
            std::cout.write(reinterpret_cast<char*>(&out_r[4]), 4);

            cur[0] = nxt[0]; cur[1] = nxt[1];
            total_frames++;

            // 界面刷新逻辑：每 25600 帧更新一次控制台
            if (total_frames % 25600 == 0) {
                // CLIP 报警灯逻辑：红色代表触发了 SAFE_ZONE 保护
                std::string clip_l = (mod_l.clip_hold_timer > 0) ? "\033[1;41m CLIP \033[0m" : "\033[1;30m CLIP \033[0m";
                std::string clip_r = (mod_r.clip_hold_timer > 0) ? "\033[1;41m CLIP \033[0m" : "\033[1;30m CLIP \033[0m";

                std::cerr << "\rL: " << get_level_bar(peak_l) << clip_l 
                          << " | R: " << get_level_bar(peak_r) << clip_r
                          << " | Gain: " << target_gain << std::flush;
                
                // 状态衰减：表针掉落效果
                peak_l *= 0.88f; peak_r *= 0.88f;
                if (mod_l.clip_hold_timer > 0) mod_l.clip_hold_timer--;
                if (mod_r.clip_hold_timer > 0) mod_r.clip_hold_timer--;
            }
        }
        // 当 MPD 停止播放时执行重置
        std::cin.clear();
        mod_l.clip_count = 0; mod_r.clip_count = 0;
        for(int i=0; i<5; i++) { mod_l.s[i] = mod_r.s[i] = 0; }
    }
    return 0;
}