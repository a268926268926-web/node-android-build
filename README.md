# node-android-build

用 GitHub Actions 编译**最新 LTS** 的 Node.js for Android，产出自包含二进制
（`--partly-static`，不依赖 termux 环境，可在普通 Android 上直接运行）。

方法参考 **termux-packages 的 [`nodejs-lts` 包](https://github.com/termux/termux-packages/tree/master/packages/nodejs-lts)**
（活跃维护，当前 LTS v24.x）与 node 官方的 `android_configure.py`。
不再依赖已停更的 [zongou/build](https://github.com/zongou/build)（停留在 node 21.x）。

## 为什么需要自己编译

- Node.js 官方 dist 不提供 Android 产物（`nodejs.org/dist` 只有桌面/嵌入式平台）
- termux 的 node 包依赖共享库（openssl/icu 等），无法在普通 Android 上运行
- 需要的是 `--partly-static` 的独立二进制（如 Sub-Store 模块直接使用 `bin/node`）

## 产物

每次构建产出 `node-android-<arch>-<version>.tar.xz`，**每个版本一个独立 release tag**
（`node-android-arm64-24.19.0` / `node-android-arm-24.19.0` / `node-android-x64-24.19.0` / …），
**历史版本全部保留**，可随时回退下载：

```
nodejs-v24.19.0-android-arm64/
└── bin/
    └── node        # 自包含 node 二进制
```

## 使用

### GitHub Actions（推荐）

- 手动触发：Actions → Build Node.js for Android → Run workflow，可选
  - `node_version`：`lts`（默认，最新 LTS）/ 主版本如 `24` / 完整版本如 `24.19.0`
  - `target_arch`：`all`（默认，**全部 4 架构**）/ `arm64` / `arm` / `x64` / `ia32`
- 定时：每周一自动检查并构建最新 LTS（全部架构）
- 已发布过的版本自动跳过（不重复构建）
- **历史版本全部保留**：每个版本一个独立 release tag

### 本地构建

```sh
# 需要: NDK (r25+, 推荐 r26+)、curl、tar、xz、python3、ninja
export ANDROID_NDK_HOME=/path/to/android-ndk-r27
./build.sh                          # 最新 LTS + arm64
NODE_VERSION=24 TARGET_ARCH=arm ./build.sh
```

## 环境变量

| 变量 | 说明 | 默认 |
| --- | --- | --- |
| `NODE_VERSION` | `lts` / 主版本 / 完整版本 | `lts` |
| `TARGET_ARCH` | `arm64` / `arm` / `x64` / `ia32` | `arm64` |
| `ANDROID_SDK_VERSION` | Android API 级别 | `24` |
| `ANDROID_NDK_HOME` | NDK 路径（未设置时自动找 Actions 预装） | - |
| `OUT_DIR` | 输出目录 | `./out` |
| `NODE_BUILD_BACKEND` | `ninja` / `make` | `ninja` |
| `NODE_STRIP` | `1` = 用 llvm-strip 精简二进制 | `0` |

## 构建要点（对比 termux 的差异）

termux 编译 node 用的是共享库（`--shared-openssl --shared-icu ...`），依赖 termux 环境；
本仓库改为**全部内置**（bundled openssl/zlib/ICU/uv，`--partly-static`），产物独立可跑。

`patches/termux/` 直接来自 `nodejs-lts` 包（对 node 24.19.0 实测全部可应用）：

| Patch | 作用 |
| --- | --- |
| `deps-v8-src-trap-handler-trap-handler.h.patch` | v8 trap-handler 在 Android 上不支持（交叉编译会走到模拟器分支） |
| `tools-gyp-pylib-gyp-generator-ninja.py.patch` | ninja 交叉编译 host/target 中间目录冲突 |
| `tools-v8_gypfiles-v8.gyp.patch` | v8 在 Android 上链接 `-latomic`；补 platform-linux.h 源文件 |
| `tools-v8_gypfiles-toolchain.gypi.patch` | v8 日志输出到 stdout；移除 arm64 分支保护参数（旧 NDK 兼容） |
| `common.gypi.patch` | 移除已失效的 `android_ndk_path/cpufeatures` 引用（NDK r26+ 已删该库） |
| `fix_multiple_definitions.patch` | libuv `uv_get_free_memory` 重复定义 |
| `deps-uv-uv.gyp.patch` | libuv `__USE_GNU` 等 Android 编译修正 |
| `deps-uv-src-unix-process.c.patch` | 子进程 spawn 移除 setuid/setgroups（Android 下不可用） |
| `deps-uvwasi-src-uvwasi.c.patch` | uvwasi 上游已接受的修复 |
| `avoid-ficlone-ioctl.patch` | Android 不支持 FICLONE ioctl |
| `src-node_internals.h.patch` | 启用 POSIX credentials API（getuid 等） |
| `src-node_report.cc.patch` | `uv_get_cpu_info` 失败时 node_report 崩溃修复 |

**未采用**的 termux 专属项见 [patches/BLACKLIST.md](patches/BLACKLIST.md)
（黑名单共 20 个：termux 路径硬编码 / `__TERMUX__` 守卫 / 测试用例三类）。

**额外处理**（termux 用共享库所以碰不到的问题）：

| 问题 | 处理 |
| --- | --- |
| 内置 zlib 的 `cpu_features.c` 需要 `<cpu-features.h>`，NDK r26+ 已移除该库 | `patches/stubs/cpu-features.h` 提供最小 stub（禁用 CRC32/PMULL SIMD，安全） |
| node.gyp 自身也带 `-mbranch-protection=standard` | sed 移除 |
| API < 28 时 `aligned_alloc` 不存在（cctest 用） | CFLAGS 加 `-Daligned_alloc=memalign` |
| v8 的 host 工具链要求较新 LLVM（Ubuntu 自带太旧） | 从 `deps/v8/DEPS` 解析 Google 钉定的 LLVM 构建，失败回退 gcc |

> node 大版本升级时 v8/构建系统可能变化，个别补丁可能失效。
> build.sh 对所有补丁「尽力应用，失败告警 + sed 兜底」，不会因补丁失败中断，
> 但请留意构建日志中的 `[!]` 警告，必要时对照 termux 更新 `patches/termux/`。

## 维护指南

这个仓库是「构建脚本 + 补丁集」的轻量仓库，日常基本无人值守：

- **每周例行**：不用管。`schedule` 会在每周一自动构建最新 LTS（4 架构），
  新版本自动发 release，旧版本自动跳过。偶尔看一眼 Actions 是否全绿即可。
- **主动构建**：Actions → Run workflow，选版本（`lts` / 主版本 / 完整版本）和架构（`all` 或单个）。
- **Node 出新 LTS 大版本时**（如 25/26 转 LTS）：这是唯一需要动手的时候。
  1. 手动触发一次构建（先选单架构如 `arm64`，省时间）
  2. 看构建日志里的 `[!]` 警告——通常会有几个 termux 补丁失效
  3. 去 [termux-packages/packages/nodejs-lts](https://github.com/termux/termux-packages/tree/master/packages/nodejs-lts)
     对照更新 `patches/termux/`（termux 跟进新版本比我们快，直接抄它的 patch）
  4. 确认 4 架构都绿后打 commit（可顺带在 README 更新验证过的版本号）
- **更新流程**：提交推到 main 后无需手动触发，下周的 schedule 会自动构建最新 LTS；
  想立即验证就手动触发一次。
- **产物安全**：二进制全在 GitHub Releases，历史版本不删；仓库本身只存脚本和补丁，很轻。

## 许可

MIT（Node.js 本体为 MIT；补丁来自 termux-packages（GPL-3.0 仓库）的 nodejs-lts 包）。
