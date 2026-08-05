# Patch 黑名单

来源: termux-packages 的 `nodejs-lts`（24.x，共 32 个）与 `nodejs`（26.x，共 33 个）包。
本仓库按大版本存放补丁集（`patches/24/`、`patches/26/`），每套只采用 12 个。

以下 patch **禁止**放入 `patches/<大版本>/termux/` 参与构建。按原因分三类：

## 1. termux 路径硬编码（`@TERMUX_PREFIX@`）— 黑名单

这些 patch 把临时目录 / shell 路径改成 termux 专属前缀。
本仓库产物跑在**普通 Android** 上，没有 termux 环境，应用后行为反而错误。

| patch | 改动 | 为什么不用 |
| --- | --- | --- |
| `deps-uv-src-unix-core.c.patch` | tmpdir 回退 `/data/local/tmp` → `@TERMUX_PREFIX@/tmp` | 普通 Android 的 `/data/local/tmp` 是正确的 |
| `deps-v8-src-flags-flag-definitions.h.patch` | v8 的 `/tmp` 路径 → `@TERMUX_PREFIX@/tmp` | 同上 |
| `lib-os.js.patch` | `os.tmpdir()` 回退 → `@TERMUX_PREFIX@/tmp` | 同上 |
| `lib-child_process.js.patch` | 默认 shell `/system/bin/sh` → `@TERMUX_PREFIX@/bin/sh` | 普通 Android 的 `/system/bin/sh` 才是对的 |

## 2. `__TERMUX__` 守卫的 termux 专属代码 — 黑名单

这些是 termux 为绕行 Android 系统库问题写的专用实现，整个文件包在
`#ifdef __TERMUX__` 里，对我们的构建无意义甚至有害。

| patch | 改动 | 为什么不用 |
| --- | --- | --- |
| `src-getaddrinfo.patch` | 新增 `src/getaddrinfo.c`（dlopen 系统 libc 的 DNS 绕行） | 整个文件被 `#ifdef __TERMUX__` 守卫；普通 Android 用系统 getaddrinfo 即可 |
| `node.gyp.patch` | 把 `src/getaddrinfo.c` 加进 node_main 源列表 | **依赖上面的文件**；单独应用会让编译找不到文件而失败（26.x 版还带 `-latomic` 改动，我们用 node.gyp 的 sed 覆盖，无需此 patch） |
| `src-debug_utils.cc.patch` | 日志输出改为非 termux 时走 logcat | `__TERMUX__` 守卫；普通 Android 走 logcat 是正确行为 |

## 3. 测试用例 patch — 黑名单

只修 `test/` 下的测试，我们只编译 Release 不跑测试，无需也不应应用：

```
test-fs-readfile-pipe-large.js.patch
test-parallel-test-child-process-exec-env.js.patch
test-parallel-test-child-process-spawnsync-shell.js.patch
test-parallel-test-child-process-uid-gid.js.patch
test-parallel-test-cluster-bind-privileged-port.js.patch
test-parallel-test-cluster-shared-handle-bind-privileged-port.js.patch
test-parallel-test-dgram-bind-fd.js.patch
test-parallel-test-dgram-membership.js.patch
test-parallel-test-dgram-socket-buffer-size.js.patch
test-parallel-test-fs-readfile-pipe.js.patch
test-parallel-test-fs-readfilesync-pipe-large.js.patch
test-parallel-test-process-constants-noatime.js.patch
test-sequential-test-fs-watch.js.patch
```

## 对照：已包含的白名单（`patches/<大版本>/termux/`）

| patch | 作用 |
| --- | --- |
| `avoid-ficlone-ioctl.patch` | Android 不支持 FICLONE ioctl |
| `common.gypi.patch` | 移除失效的 `android_ndk_path/cpufeatures` 引用（NDK r26+ 已删该库） |
| `deps-uv-src-unix-process.c.patch` | 子进程 spawn 移除 setuid/setgroups（Android 下不可用） |
| `deps-uv-uv.gyp.patch` | libuv 的 `__USE_GNU` 等 Android 编译修正 |
| `deps-uvwasi-src-uvwasi.c.patch` | uvwasi 上游已接受的修复 |
| `deps-v8-src-trap-handler-trap-handler.h.patch` | v8 trap-handler 在 Android 上不支持 |
| `fix_multiple_definitions.patch` | libuv `uv_get_free_memory` 重复定义 |
| `src-node_internals.h.patch` | 启用 POSIX credentials API（getuid 等） |
| `src-node_report.cc.patch` | `uv_get_cpu_info` 失败时 node_report 崩溃修复 |
| `tools-gyp-pylib-gyp-generator-ninja.py.patch` | ninja 交叉编译 host/target 中间目录冲突 |
| `tools-v8_gypfiles-toolchain.gypi.patch` | v8 日志输出 stdout；移除 arm64 分支保护参数 |
| `tools-v8_gypfiles-v8.gyp.patch` | v8 在 Android 链接 `-latomic`；补 platform-linux.h |

> 不在 termux 体系内、由本仓库自行处理的项（`patches/<大版本>/stubs/cpu-features.h`、
> node.gyp 的 `-mbranch-protection` sed、`-latomic` 保险、trap-handler sed 兜底）
> 不受本黑名单影响，见 README「构建要点」。

## 版本对照

- `patches/24/`：12 个 patch，对 node 24.19.0 实测全部可应用（来源 nodejs-lts 24.x）
- `patches/26/`：12 个 patch，对 node 26.6.0 实测全部可应用（来源 nodejs 26.x；
  唯一差异：`deps-v8-src-trap-handler-trap-handler.h.patch` 换成 26 版，对应 v8 新的
  USE_SIMULATOR 结构，旧版打不上）
- 26.x 新增的 `deps-v8-src-flags-flag-definitions.h.patch`（v8 `/tmp` 路径 → `@TERMUX_PREFIX@/tmp`）
  属第 1 类（路径硬编码），继续黑名单

## 维护规则

termux 升级 nodejs-lts 后，新 patch 按以下规则分类：

1. 名字与白名单相同 → 直接更新对应文件
2. 与黑名单同名（或同类：路径替换 / `__TERMUX__` 守卫 / test 开头）→ 跳过
3. 全新名字 → 先人工看内容：目标是修复 Android 编译/运行问题 → 进白名单；
   是 termux 环境适配 → 进黑名单；不确定 → 手动触发单架构构建看日志验证
