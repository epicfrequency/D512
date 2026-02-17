#!/bin/bash

# ==========================================================
# LUMEN Hi-Fi 启动脚本 - DSD512 实时控制台
# ==========================================================

# 1. 配置参数解析
# 默认增益 0.25 (约 -12dB)，可从命令行传入覆盖：例如 ./run_hifi.sh 0.35
USER_GAIN=${1:-0.25}

PIPE_PATH="/tmp/mpd_dsd_pipe"  # 与 mpd.conf 对应的命名管道路径
ENGINE_BIN="./dsd512_final"    # 编译后的二进制文件名
SOURCE_CODE="dsd512_final.cpp" # 源代码文件名

# 2. 自动检查并编译
# 使用 -nt (newer than) 判断源码是否被修改过
if [ ! -f "$ENGINE_BIN" ] || [ "$SOURCE_CODE" -nt "$ENGINE_BIN" ]; then
    echo -e "\033[1;33m[BUILD] 检测到源码更新或二进制文件缺失，正在编译...\033[0m"
    # -O3: 最高级优化
    # -march=native: 针对树莓派5核心(Cortex-A76)进行硬件加速
    # -ffast-math: 牺牲极小精度换取大幅浮点运算速度提升
    g++ -O3 -march=native -ffast-math -funroll-loops "$SOURCE_CODE" -o "$ENGINE_BIN"
fi

# 3. 管道环境预处理
[ -p "$PIPE_PATH" ] || mkfifo "$PIPE_PATH"

# 4. 强制杀死旧的残留进程，释放音频硬件资源
killall -9 aplay "$ENGINE_BIN" 2>/dev/null

echo -e "\033[1;34m================================================\033[0m"
echo -e "\033[1;34m  LUMEN Hi-Fi Console - Independent Clip Alarm  \033[0m"
echo -e "\033[1;34m  架构: 384kHz PCM -> DSD512 Realtime           \033[0m"
echo -e "\033[1;34m================================================\033[0m"

# 5. 启动核心链路
# cat 从管道读取数据 -> taskset 锁定核心 2&3 运行引擎 -> aplay 播放
# -B 100000: 设置 100ms 缓冲区
# 2>/dev/null: 屏蔽 aplay 的标准错误输出，确保 VU 表在终端不闪烁
cat "$PIPE_PATH" | \
taskset -c 2,3 chrt -f 99 "$ENGINE_BIN" "$USER_GAIN" | \
aplay -D hw:0,0 -c 2 -f DSD_U32_BE -r 768000 -q -B 100000 2>/dev/null