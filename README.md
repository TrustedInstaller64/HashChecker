# HashChecker

跨平台命令行工具：递归计算两个目录中所有文件的哈希值，并行比较，找出复制错误。

## License

本项目使用 **[HashChecker Source Available License](LICENSE)**。允许查看、学习、个人修改，**禁止商业使用和公开再分发**。完整条款见 [LICENSE](LICENSE)。

## 构建

**前置条件**: 编译器（GCC 9+ / Clang 10+ / MSVC 2019+）+ CMake 3.16+。无其他依赖，测试框架 doctest 随项目自带。

### macOS

```bash
brew install cmake   # 如未安装
./build.sh           # 一键编译 + 可选测试
```

### Linux

```bash
sudo apt install cmake build-essential   # Debian/Ubuntu
sudo dnf install cmake gcc-c++           # Fedora
./build.sh                                # 一键编译 + 可选测试
```

### Windows

```powershell
# PowerShell / CMD（需安装 Visual Studio 2022+ 和 Ninja）
build.bat           # 一键编译 + 可选测试
```

`build.bat` 自动检测 VS 安装路径和 Ninja，无需手动配置环境变量。

> 手动构建：`cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`

## 用法

```
hashchecker --target <dir> --reference <dir> [options]
```

| 参数 | 简写 | 说明 | 默认 |
|------|------|------|------|
| `--target` | `-t` | 目标/副本目录（必选） | — |
| `--reference` | `-r` | 参考/原始目录（必选） | — |
| `--algorithm` | `-a` | md5 / sha1 / sha256 | md5 |
| `--threads` | `-j` | 总线程数，两边均分 | 共 80% CPU，按文件数分配 |
| `--target-threads` | `-T` | 目标盘专用线程数 | 用0.5倍 `-j` 的值 |
| `--ref-threads` | `-R` | 参考盘专用线程数 | 用0.5倍 `-j` 的值 |
| `--output` | `-o` | 报告保存到文件 | stdout |
| `--verbose` | `-v` | 逐文件进度 | off |
| `--quiet` | `-q` | 最简输出 | off |
| `--json` | | JSON 格式 | off |
| `--save-logs` | | 保存逐文件哈希日志 | off |
| `--help` | `-h` | 帮助 | — |
| `--version` | | 版本号 | — |

**退出码**: `0` = 相同 / `1` = 有差异 / `2` = 错误

## 示例

```bash
# 快速校验
hashchecker -t ./copy -r ./original -q

# 双盘性能不均时分线程
hashchecker -t Y:\data -r Z:\data -T 4 -R 32 -v

# JSON 用于脚本
hashchecker -t ./a -r ./b --json -o report.json

# MD5 更快
hashchecker -t ./a -r ./b -a md5 -v
```

## 构建选项

| 选项 | 默认 | 说明 |
|------|------|------|
| `CMAKE_BUILD_TYPE` | — | Debug / Release |
| `BUILD_TESTS` | ON | 编译单元测试 |
| `ENABLE_ASAN` | OFF | AddressSanitizer |
| `ENABLE_TSAN` | OFF | ThreadSanitizer |
| `ENABLE_UBSAN` | OFF | UB Sanitizer |

## 项目结构

```
src/
├── main.cpp              CLI 入口
├── app.h/cpp             编排器
├── hash/                 哈希算法 + 并行计算
├── scan/                 目录扫描 (Win32 API / std::filesystem)
├── compare/              有序合并比较器
├── report/               text / quiet / JSON
└── infra/                线程池 / Result<T,E>
tests/
├── third_party/doctest.h
├── test_*.cpp            7 个单元测试
└── scripts/
    ├── test_e2e.sh        macOS/Linux 端到端
    └── test_e2e.cmd       Windows 端到端
```
