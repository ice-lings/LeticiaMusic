# LeticiaMusic

跨平台音乐播放器 (Windows / Android)，基于 Qt 6.7 + C++20。

## 功能

- 本地音乐扫描与索引
- 元数据编辑（封面、标签）
- 播放列表管理
- 收藏管理
- NAS FTP 同步（多设备同步音乐库）

## 构建

### 前置条件

| 工具 | 版本 | 说明 |
|------|------|------|
| Qt | 6.7+ | Core / Quick / Gui / Multimedia / Sql / Network |
| CMake | 3.16+ | |
| Ninja | | 构建系统 |
| MinGW | 11.2+ | Windows 编译 |
| Android NDK | 26+ | Android 编译 |
| JDK | 17+ | Android 编译 |
| OpenSSL | 3.x | Android 编译需要预编译库 |

### 环境变量

| 变量 | 说明 | 必需 |
|------|------|:--:|
| `LM_QT_ROOT` | Qt 安装根目录 | 是 |
| `LM_MINGW_HOME` | MinGW 工具链根目录 | Win |
| `ANDROID_NDK_ROOT` | Android NDK 根目录 | Android |
| `ANDROID_SDK_ROOT` | Android SDK 根目录 | Android |
| `ANDROID_OPENSSL_DIR` | Android OpenSSL 预编译库目录 | Android |
| `LM_CMAKE` | CMake 可执行文件路径 | 否 |
| `LM_NINJA` | Ninja 可执行文件路径 | 否 |
| `LM_KEYSTORE_PASS` | Android 签名密钥密码（仅 Release） | 否 |

> 标记为"否"的变量留空则从 PATH 自动搜索，CMake 侧同样支持环境变量回退。

### 克隆

```bash
git clone --recursive https://github.com/ice-lings/LeticiaMusic.git
cd LeticiaMusic
```

### 编译

```bash
# Windows Debug
python build.py -Win -d --no-deploy

# Android Debug
python build.py -And -d --no-deploy

# Windows Release（编译 + windeployqt + 运行）
python build.py -Win -r

# Android Release（编译 + 签名 + 安装至设备）
python build.py -And -r
```

### 运行测试

```bash
# 运行全部单元测试
python tests/run_tests.py --all

# 运行指定模块
python tests/run_tests.py sync-compare
```

## 项目结构

```
├── build.py                     # 统一构建脚本
├── CMakeLists.txt               # 根构建配置
├── main.cpp                     # 入口
├── android/                     # Android 平台配置
├── assets/svgs/                 # SVG 图标
├── res.qrc / winqmlres.qrc      # Qt 资源文件
├── src/
│   ├── core/                    # C++ 核心逻辑
│   │   ├── application/         # 应用入口、播放控制、配置
│   │   ├── database/            # SQLite 数据层
│   │   ├── design/              # 设计系统 (色彩/字体/间距)
│   │   ├── filesystem/          # 文件扫描、元数据管理
│   │   ├── playlist/            # 播放列表、收藏
│   │   ├── services/            # 服务层 (播放/扫描/初始化)
│   │   ├── sync/                # NAS FTP 同步
│   │   ├── utils/               # 工具 (日志/拼音/去重)
│   │   └── viewmodels/          # 数据模型
│   ├── qml/
│   │   ├── android/             # Android 端 UI
│   │   ├── common/              # 公共组件
│   │   └── win/                 # Windows 端 UI
│   └── thirdparty/              # Git 子模块 (curl, taglib)
└── tests/
    └── core/                    # 单元测试
```

## 第三方依赖

| 库 | 用途 | 许可 |
|------|------|------|
| [TagLib](https://github.com/taglib/taglib) | 音频元数据读写 | LGPL 2.1 |
| [libcurl](https://github.com/curl/curl) | FTP 网络传输 | curl License |
| [utf8cpp](https://github.com/nemtrif/utfcpp) | UTF 编码转换 (taglib 依赖) | BSL-1.0 |
| [Qt](https://www.qt.io/) | UI 框架 | LGPL v3 |

以上通过 Git 子模块管理，执行 `git submodule update --init --recursive` 即可拉取。
