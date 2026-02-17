#!/bin/bash

# --- 配置区 ---
PIPE_PATH="/tmp/mpd_dsd_pipe"
ENGINE_BIN="./dsd512_final"
SOURCE_CODE="dsd512_final.cpp"
# DAC 采样率：DSD512 (48k系) = 24.576MHz -> 32位PCM封装下为 768k
SAMPLERATE=768000 

# --- 1. 自动化编译 (针对树莓派5硬件优化) ---
echo -e "\033[1;33m[BUILD] Compiling Engine for ARM Cortex-A76...\033[0m"
g++ -O3 -march=native -ffast-math -funroll-loops "$SOURCE_CODE" -o "$ENGINE_BIN"

if [ $? -ne 0 ]; then
    echo -e "\033[1;31m[ERROR] Compilation failed! Check your C++ code.\033[0m"
    exit 1
fi

# --- 2. 管道环境准备 ---
if [ ! -p "$PIPE_PATH" ]; then
    rm -f "$PIPE_PATH"
    mkfifo "$PIPE_PATH"
fi

# --- 3. 运行前清理 ---
# 杀死可能残留在后台的 aplay 或引擎进程，防止占用硬件
killall -9 aplay 2>/dev/null
killall -9 "$ENGINE_BIN" 2>/dev/null

echo -e "\033[1;34m------------------------------------------------\033[0m"
echo -e "\033[1;34m  LUMEN AUDIO CONSOLE v1.0 - HIGH-END DSD512    \033[0m"
echo -e "\033[1;34m  Input: 384kHz PCM | Output: 24.576MHz Native  \033[0m"
echo -e "\033[1;34m------------------------------------------------\033[0m"

# --- 4. 核心链路启动 ---
# 解释：
# - cat: 从管道读取 MPD 喂过来的 384k 流
# - taskset -c 2,3: 将计算量巨大的引擎锁定在 CPU 2和3号核心，避免被系统任务打断
# - chrt -f 99: 设置实时调度优先级(最高级)
# - aplay -q: 静默模式运行，让出终端给 VU Meter
# - -B 100000: 设置 100ms 硬件缓冲区，防止在高负载下出现 Underrun (咔嗒声)

cat "$PIPE_PATH" | \
taskset -c 2,3 chrt -f 99 "$ENGINE_BIN" | \
aplay -D hw:0,0 -c 2 -f DSD_U32_BE -r $SAMPLERATE -q -B 100000

# --- 5. 退出处理 ---
echo -e "\n\033[1;31m[STOP] Engine stopped.\033[0m"