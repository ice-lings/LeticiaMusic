#!/usr/bin/env python3
"""LeticiaMusic 构建脚本
用法:
    python build.py -Win -d                  # Windows Debug，编译+运行（5秒后自动关闭）
    python build.py -Win -r                  # Windows Release，编译+运行（5秒后自动关闭）
    python build.py -And -d                  # Android Debug，编译APK+安装至USB设备
    python build.py -And -r                  # Android Release，编译APK+签名+安装至USB设备
    python build.py -Win -d --no-deploy      # 仅编译，不部署
    python build.py -Win -d --clean          # 清理构建目录后编译+运行
    python build.py -And -d --clean --no-deploy  # 清理+仅编译APK
    python build.py -Win -d --wait           # 编译+运行，持续等待直到程序退出
    python build.py -And -d --wait           # 编译+安装+启动APP+捕获logcat
"""

import argparse
import glob
import os
import shutil
import subprocess
import sys
import tempfile
import time

# ============================================================
# 配置区（按需填写或设置环境变量）
#   优先级：硬编码值 → 环境变量 → 报错中断
#   填写为空则自动从环境变量获取
# ============================================================
PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))

QT_VERSION = "6.7.2"

QT_ROOT = ""                # Qt 安装根目录 → 环境变量: LM_QT_ROOT
NINJA = ""                  # Ninja 可执行文件路径 → 环境变量: LM_NINJA（留空则搜索 PATH）
CMAKE = ""                  # CMake 可执行文件路径 → 环境变量: LM_CMAKE（留空则搜索 PATH）
MINGW_HOME = ""             # MinGW 工具链根目录 → 环境变量: LM_MINGW_HOME
JAVA_HOME = ""              # JDK 根目录 → 环境变量: JAVA_HOME（留空则从 java 位置推导）
ANDROID_NDK_ROOT = ""       # Android NDK 根目录 → 环境变量: ANDROID_NDK_ROOT
ANDROID_SDK_ROOT = ""       # Android SDK 根目录 → 环境变量: ANDROID_SDK_ROOT
ADB = ""                    # ADB 可执行文件路径 → 环境变量: LM_ADB（留空则搜索 PATH）
APKSIGNER = ""              # apksigner 路径 → 环境变量: LM_APKSIGNER（留空则自动搜索 SDK）
RELEASE_STORE_PASS = ""     # 签名密钥密码 → 环境变量: LM_KEYSTORE_PASS

ANDROID_PKG_DEBUG = "org.qtproject.example.LeticiaMusic.debug"
ANDROID_PKG_RELEASE = "org.qtproject.example.LeticiaMusic"
ANDROID_ACTIVITY = "org.qtproject.qt.android.bindings.QtActivity"

# ============================================================
# 颜色
# ============================================================
COLOR_RESET = "\033[0m"
COLOR_BOLD = "\033[1m"
COLOR_GREEN = "\033[32m"
COLOR_YELLOW = "\033[33m"
COLOR_RED = "\033[31m"
COLOR_CYAN = "\033[36m"
COLOR_GRAY = "\033[90m"


def cprint(text, color=COLOR_RESET, bold=False):
    prefix = COLOR_BOLD if bold else ""
    suffix = COLOR_RESET if color != COLOR_RESET else ""
    sys.stdout.write(f"{prefix}{color}{text}{suffix}\n")
    sys.stdout.flush()


# ============================================================
# 配置解析（三级回退：硬编码值 → 环境变量 → 报错）
# ============================================================
def _resolve_config():
    """在 main() 开始时调用，解析所有配置变量并生成派生路径"""
    global QT_ROOT, NINJA, CMAKE, MINGW_HOME, JAVA_HOME
    global ANDROID_NDK_ROOT, ANDROID_SDK_ROOT, ADB, APKSIGNER, RELEASE_STORE_PASS
    global QT_WIN, QT_ANDROID, QT_TOOLCHAIN, WINDEPLOYQT, KEYSTORE, RELEASE_KEYSTORE

    def _r(name, val, env):
        """通用解析：硬编码 → 环境变量 → 报错"""
        if val:
            return val
        v = os.environ.get(env, "")
        if v:
            return v
        sys.exit(f"错误：未设置 {name}。请填写 build.py 配置区变量或设置环境变量 {env}")

    def _r_tool(name, val, env):
        """工具路径解析：硬编码 → 环境变量 → PATH 搜索 → 报错"""
        if val:
            return val
        v = os.environ.get(env, "")
        if v:
            return v
        tool = env.replace("LM_", "").lower()
        found = shutil.which(tool)
        if found:
            return found
        sys.exit(f"错误：未找到 {name}。请填写 build.py 配置区变量、设置环境变量 {env}，或确保 {tool} 在 PATH 中")

    QT_ROOT = _r("QT_ROOT", QT_ROOT, "LM_QT_ROOT")
    NINJA = _r_tool("NINJA", NINJA, "LM_NINJA")
    CMAKE = _r_tool("CMAKE", CMAKE, "LM_CMAKE")
    MINGW_HOME = _r("MINGW_HOME", MINGW_HOME, "LM_MINGW_HOME")

    if not JAVA_HOME:
        JAVA_HOME = os.environ.get("JAVA_HOME", "")
    if not JAVA_HOME:
        java_bin = shutil.which("java")
        if java_bin:
            JAVA_HOME = os.path.dirname(os.path.dirname(java_bin))

    ANDROID_NDK_ROOT = _r("ANDROID_NDK_ROOT", ANDROID_NDK_ROOT, "ANDROID_NDK_ROOT")
    ANDROID_SDK_ROOT = _r("ANDROID_SDK_ROOT", ANDROID_SDK_ROOT, "ANDROID_SDK_ROOT")
    ADB = _r_tool("ADB", ADB, "LM_ADB")

    if not APKSIGNER:
        APKSIGNER = os.environ.get("LM_APKSIGNER", "")
    if not APKSIGNER and ANDROID_SDK_ROOT:
        bt_dir = os.path.join(ANDROID_SDK_ROOT, "build-tools")
        if os.path.isdir(bt_dir):
            versions = sorted(
                [d for d in os.listdir(bt_dir) if os.path.isdir(os.path.join(bt_dir, d))],
                reverse=True
            )
            for ver in versions:
                candidate = os.path.join(bt_dir, ver, "apksigner.bat")
                if os.path.exists(candidate):
                    APKSIGNER = candidate
                    break

    RELEASE_STORE_PASS = RELEASE_STORE_PASS or os.environ.get("LM_KEYSTORE_PASS", "")

    QT_WIN = f"{QT_ROOT}/mingw_64"
    QT_ANDROID = f"{QT_ROOT}/android_arm64_v8a"
    QT_TOOLCHAIN = f"{QT_ANDROID}/lib/cmake/Qt6/qt.toolchain.cmake"
    WINDEPLOYQT = f"{QT_WIN}/bin/windeployqt.exe"
    KEYSTORE = os.path.join(os.path.expanduser("~"), ".android", "debug.keystore")
    RELEASE_KEYSTORE = os.path.join(os.path.expanduser("~"), ".android", "release.keystore")


# ============================================================
# 构建目录映射
# ============================================================
def get_build_dir(platform: str, build_type: str) -> str:
    display_type = build_type.capitalize()
    if platform == "win":
        suffix = f"Desktop_Qt_{QT_VERSION.replace('.', '_')}_MinGW_64_bit-{display_type}"
    else:
        suffix = f"Qt_{QT_VERSION.replace('.', '_')}_Clang_arm64_v8a-{display_type}"
    return os.path.join(PROJECT_ROOT, "build", suffix).replace("\\", "/")


def get_output_path(platform: str, build_dir: str) -> str:
    if platform == "win":
        return os.path.join(build_dir, "LeticiaMusic.exe")
    else:
        is_debug = "Debug" in os.path.basename(build_dir)
        variant = "debug" if is_debug else "release"
        apk_name = "android-build-debug.apk" if is_debug else "android-build-release-unsigned.apk"
        return os.path.join(build_dir, "android-build", "build", "outputs", "apk",
                            variant, apk_name)


# ============================================================
# 步骤 1: 清理
# ============================================================
def do_clean(build_dir: str):
    if os.path.exists(build_dir):
        # 备份用户数据文件（数据库/config/音乐/封面等运行时产物）
        preserved = _preserve_user_data(build_dir)

        cprint(f"  删除构建目录: {build_dir}", COLOR_YELLOW)
        shutil.rmtree(build_dir, ignore_errors=True)

        # 重建目录并恢复数据文件
        if preserved:
            os.makedirs(build_dir, exist_ok=True)
            _restore_user_data(build_dir, preserved)
            cprint(f"  已保留用户数据: {len(preserved)} 项", COLOR_GREEN)
        else:
            cprint("  清理完成 [OK]", COLOR_GREEN)
    else:
        cprint("  构建目录不存在，无需清理 [OK]", COLOR_GRAY)


def _preserve_user_data(build_dir: str) -> dict:
    """清理前备份用户数据文件，返回 {相对路径: (temp_path, is_dir)}"""
    preserved = {}
    # 需要保留的文件模式
    keep_patterns = ["*.db", "config.json"]
    # 需要保留的目录
    keep_dirs = ["Music", "cache", "covers", "logs", "export"]

    tmp_root = os.path.join(tempfile.gettempdir(), "leticia_backup")
    for item in keep_dirs:
        src = os.path.join(build_dir, item)
        if os.path.isdir(src):
            dst = os.path.join(tmp_root, item)
            try:
                shutil.copytree(src, dst)
                preserved[item] = (dst, True)
            except Exception:
                pass

    for pattern in keep_patterns:
        for src in glob.glob(os.path.join(build_dir, pattern)):
            basename = os.path.basename(src)
            dst = os.path.join(tmp_root, basename)
            try:
                shutil.copy2(src, dst)
                preserved[basename] = (dst, False)
            except Exception:
                pass

    return preserved


def _restore_user_data(build_dir: str, preserved: dict):
    """将备份的用户数据文件恢复到构建目录"""
    for rel_path, (src, is_dir) in preserved.items():
        dst = os.path.join(build_dir, rel_path)
        try:
            if is_dir:
                if os.path.exists(dst):
                    continue
                shutil.copytree(src, dst)
            else:
                shutil.copy2(src, dst)
        except Exception:
            pass
    # 清理临时备份
    if preserved:
        tmp_root = os.path.dirname(list(preserved.values())[0][0])
        try:
            shutil.rmtree(tmp_root, ignore_errors=True)
        except Exception:
            pass


# ============================================================
# 步骤 2: CMake 配置
# ============================================================
def do_cmake(platform: str, build_type: str, build_dir: str) -> bool:
    source_dir = PROJECT_ROOT.replace("\\", "/")

    cmake_args = [
        CMAKE,
        "-G", "Ninja",
        f"-DCMAKE_MAKE_PROGRAM={NINJA}",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-S{source_dir}",
        f"-B{build_dir}",
    ]

    if platform == "win":
        cmake_args.append(f"-DCMAKE_PREFIX_PATH={QT_WIN}")
        cmake_args.append(f"-DCMAKE_C_COMPILER={MINGW_HOME}/bin/gcc.exe")
        cmake_args.append(f"-DCMAKE_CXX_COMPILER={MINGW_HOME}/bin/g++.exe")
    else:
        cmake_args.append(f"-DCMAKE_TOOLCHAIN_FILE={QT_TOOLCHAIN}")
        cmake_args.append(f"-DANDROID_NDK_ROOT={ANDROID_NDK_ROOT}")
        cmake_args.append(f"-DANDROID_SDK_ROOT={ANDROID_SDK_ROOT}")

    # Debug 模式默认编译测试，Release 不编译
    tests_value = "ON" if build_type == "debug" else "OFF"
    cmake_args.append(f"-DBUILD_TESTS={tests_value}")

    cprint(f"  → 源目录: {source_dir}", COLOR_GRAY)
    cprint(f"  → 构建目录: {os.path.relpath(build_dir, PROJECT_ROOT)}", COLOR_GRAY)

    env = os.environ.copy()
    if platform == "android":
        env["JAVA_HOME"] = JAVA_HOME
        cprint(f"  → JAVA_HOME: {JAVA_HOME}", COLOR_GRAY)
    else:
        env["PATH"] = f"{MINGW_HOME}/bin;{QT_WIN}/bin;{env.get('PATH', '')}"

    print("")
    result = subprocess.run(cmake_args, env=env)

    if result.returncode != 0:
        cprint(f"\n  CMake 配置失败 ({result.returncode})", COLOR_RED)
        return False
    return True


# ============================================================
# 步骤 3: Ninja 构建
# ============================================================
def do_build(platform: str, build_dir: str, target: str = None) -> bool:
    if target:
        build_target = [target]
    else:
        build_target = ["apk"] if platform == "android" else []
    cmd = [NINJA, "-C", build_dir] + build_target

    env = os.environ.copy()
    if platform == "android":
        env["JAVA_HOME"] = JAVA_HOME
    else:
        env["PATH"] = f"{MINGW_HOME}/bin;{QT_WIN}/bin;{env.get('PATH', '')}"

    print("")
    result = subprocess.run(cmd, env=env)

    if result.returncode != 0:
        cprint(f"\n  构建失败 ({result.returncode})", COLOR_RED)
        return False
    return True


# ============================================================
# 步骤 4: 部署依赖 DLL（Windows 专属）
# ============================================================
def do_windeployqt(output_path: str) -> bool:
    if not os.path.exists(output_path):
        cprint(f"  → 产物不存在: {output_path}", COLOR_RED)
        return False

    qml_dir = os.path.join(PROJECT_ROOT, "src", "qml").replace("\\", "/")
    cmd = [
        WINDEPLOYQT,
        output_path,
        "--qmldir", qml_dir,
        "--no-translations",
        "--no-system-d3d-compiler",
    ]

    cprint(f"  → QML目录: {qml_dir}", COLOR_GRAY)

    env = os.environ.copy()
    env["PATH"] = f"{MINGW_HOME}/bin;{QT_WIN}/bin;{env.get('PATH', '')}"

    result = subprocess.run(cmd, env=env)
    if result.returncode != 0:
        cprint(f"\n  windeployqt 失败 ({result.returncode})", COLOR_RED)
        return False

    # windeployqt 可能漏掉 MinGW 运行时 DLL，手动补上
    build_dir = os.path.dirname(output_path)
    mgw_bin = f"{MINGW_HOME}/bin"
    runtime_dlls = [
        "libgcc_s_seh-1.dll",
        "libstdc++-6.dll",
        "libwinpthread-1.dll",
    ]
    for dll in runtime_dlls:
        src = os.path.join(mgw_bin, dll)
        dst = os.path.join(build_dir, dll)
        if os.path.exists(src):
            shutil.copy2(src, dst)
            cprint(f"  → {dll}", COLOR_GRAY)

    # 复制 taglib DLL 到 exe 同目录
    taglib_dll = os.path.join(build_dir, "src", "thirdparty", "taglib", "taglib", "libtag.dll")
    if os.path.exists(taglib_dll):
        shutil.copy2(taglib_dll, os.path.join(build_dir, "libtag.dll"))
        cprint(f"  → libtag.dll", COLOR_GRAY)
    return True


# ============================================================
# 步骤 5: 部署
# ============================================================
def _get_android_devices() -> list:
    """搜索可用的 adb 设备，返回 [(serial, state, is_usb)]"""
    result = subprocess.run(
        [ADB, "devices", "-l"],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True, encoding="utf-8", errors="replace"
    )
    devices = []
    for line in result.stdout.strip().split("\n")[1:]:
        line = line.strip()
        if not line or "offline" in line:
            continue
        parts = line.split()
        serial = parts[0]
        is_usb = ("usb:" in line or "model:" in line) and "127.0.0.1" not in serial
        devices.append((serial, parts[1] if len(parts) > 1 else "unknown", is_usb))
    return devices


def _sign_apk(unsigned_path: str, signed_path: str) -> bool:
    """对 Release APK 执行签名"""
    if not os.path.exists(unsigned_path):
        cprint(f"  → unsigned APK 不存在: {unsigned_path}", COLOR_RED)
        return False

    if os.path.exists(RELEASE_KEYSTORE):
        keystore = RELEASE_KEYSTORE
        alias = "leticiamusic"
        storepass = RELEASE_STORE_PASS
        keypass = RELEASE_STORE_PASS
        cprint(f"  → 使用 release keystore", COLOR_GRAY)
    else:
        keystore = KEYSTORE
        alias = "androiddebugkey"
        storepass = "android"
        keypass = "android"
        cprint(f"  → 使用 debug keystore", COLOR_GRAY)

    result = subprocess.run(
        [APKSIGNER, "sign",
         "--ks", keystore,
         "--ks-key-alias", alias,
         "--ks-pass", f"pass:{storepass}",
         "--key-pass", f"pass:{keypass}",
         "--out", signed_path,
         unsigned_path],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True, encoding="utf-8", errors="replace"
    )
    return result.returncode == 0


def do_deploy_win(output_path: str, wait: bool = False):
    if not os.path.exists(output_path):
        cprint(f"  → 产物不存在: {output_path}", COLOR_RED)
        return

    cprint(f"  → 停止旧进程...", COLOR_GRAY)
    subprocess.run(
        ["taskkill", "/F", "/IM", "LeticiaMusic.exe"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    cprint(f"  → 启动: {output_path}", COLOR_GRAY)
    proc = subprocess.Popen(
        [output_path],
        cwd=os.path.dirname(output_path),
    )
    cprint("  LeticiaMusic.exe 已启动 [OK]", COLOR_GREEN)

    if wait:
        cprint("  → 等待程序退出（Ctrl+C 可强制终止）...", COLOR_GRAY)
        print("")
        try:
            proc.wait()
        except KeyboardInterrupt:
            proc.terminate()
            cprint("\n  程序已终止", COLOR_YELLOW)
    else:
        cprint("  → 5秒后自动关闭...", COLOR_GRAY)
        print("")
        time.sleep(5)
        subprocess.run(
            ["taskkill", "/F", "/IM", "LeticiaMusic.exe"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        cprint("  程序已自动关闭 [OK]", COLOR_GREEN)


def do_deploy_android(output_path: str, wait: bool = False, build_type: str = "debug"):
    if not os.path.exists(output_path):
        cprint(f"  → APK 不存在: {output_path}", COLOR_RED)
        return

    is_debug = build_type == "debug"
    pkg_name = ANDROID_PKG_DEBUG if is_debug else ANDROID_PKG_RELEASE

    apk_path = output_path
    if not is_debug:
        signed_path = output_path.replace("-unsigned.apk", ".apk")
        cprint(f"  → 签名 Release APK...", COLOR_GRAY)
        if not _sign_apk(output_path, signed_path):
            cprint("  APK 签名失败", COLOR_RED)
            return
        apk_path = signed_path
        cprint("  APK 签名完成 [OK]", COLOR_GREEN)

    devices = _get_android_devices()
    if not devices:
        cprint("  → 未找到 adb 设备", COLOR_RED)
        return

    targets = [d for d in devices if d[2]]  # 优先 USB
    if not targets:
        targets = devices  # 无 USB 则用网络设备

    cprint(f"  → 发现 {len(targets)} 台设备，将全部部署", COLOR_GRAY)

    for i, (serial, state, is_usb) in enumerate(targets):
        device_type = "USB" if is_usb else "网络"
        cprint(f"  [{i+1}/{len(targets)}] {serial} ({device_type})", COLOR_GRAY)

        cprint(f"    → 安装 APK...", COLOR_GRAY)
        result = subprocess.run(
            [ADB, "-s", serial, "install", "-r", apk_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True, encoding="utf-8", errors="replace",
        )

        if "Success" in result.stdout:
            cprint("    APK 安装成功 [OK]", COLOR_GREEN)
        else:
            cprint("    APK 安装可能失败", COLOR_YELLOW)
            if result.stdout.strip():
                cprint(f"    {result.stdout.strip()}", COLOR_GRAY)
            if result.stderr.strip():
                cprint(f"    {result.stderr.strip()}", COLOR_RED)

        cprint(f"    → 启动 APP...", COLOR_GRAY)
        subprocess.run(
            [ADB, "-s", serial, "shell",
             "am", "start", "-n", f"{pkg_name}/{ANDROID_ACTIVITY}"],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )

    if not wait:
        return

    cprint("  → 捕获日志 (Ctrl+C 停止)...", COLOR_GRAY)
    print("")
    try:
        subprocess.run(
            [ADB, "-s", targets[0][0], "logcat", "-s", "Leticia", "LeticiaScan"]
        )
    except KeyboardInterrupt:
        cprint("\n  日志捕获已停止", COLOR_YELLOW)


# ============================================================
# 主流程
# ============================================================
def main():
    _resolve_config()

    # 参数大小写标准化
    for i in range(1, len(sys.argv)):
        if sys.argv[i].lower() in ("-win", "-and", "-d", "-r"):
            sys.argv[i] = sys.argv[i].lower()

    parser = argparse.ArgumentParser(description="LeticiaMusic 构建脚本")
    parser.add_argument("-win", dest="platform", action="store_const", const="win", help="编译 Windows 平台")
    parser.add_argument("-and", dest="platform", action="store_const", const="android", help="编译 Android 平台")
    parser.add_argument("-d", dest="build_type", action="store_const", const="debug", help="Debug 模式")
    parser.add_argument("-r", dest="build_type", action="store_const", const="release", help="Release 模式")
    parser.add_argument("--no-deploy", action="store_true", help="仅编译，不部署/运行")
    parser.add_argument("--clean", action="store_true", help="先清理构建目录再编译")
    parser.add_argument("--wait", action="store_true", help="持续等待直到程序退出（Windows）/ 安装后启动APP并捕获logcat（Android）")
    parser.add_argument("--launch", action="store_true", help="[已废弃] 同 --wait，请直接使用 --wait")
    args = parser.parse_args()

    if not args.platform:
        cprint("错误: 必须指定平台 -Win 或 -And", COLOR_RED)
        parser.print_help()
        sys.exit(1)
    if not args.build_type:
        cprint("错误: 必须指定构建类型 -d 或 -r", COLOR_RED)
        parser.print_help()
        sys.exit(1)

    # --launch 和 --wait 等效，合并处理
    do_wait = args.wait or args.launch

    build_dir = get_build_dir(args.platform, args.build_type)
    output_path = get_output_path(args.platform, build_dir)

    display_platform = "Windows" if args.platform == "win" else "Android"
    display_type = args.build_type.capitalize()

    # 打印头部
    print("")
    cprint("========================================", COLOR_BOLD)
    cprint("  LeticiaMusic Build Script", COLOR_BOLD)
    cprint(f"  平台: {display_platform:8s} | "
           f"类型: {display_type:7s} | "
           f"清理: {'是' if args.clean else '否':3s} | "
           f"部署: {'否' if args.no_deploy else '是':3s} | "
           f"等待: {'是' if do_wait else '否':3s}", COLOR_CYAN)
    cprint("========================================", COLOR_BOLD)
    print("")

    start_time = time.time()
    total_steps = 2  # cmake + ninja
    if args.clean:
        total_steps += 1
    if args.platform == "win":
        total_steps += 1  # windeployqt
    if not args.no_deploy:
        total_steps += 1
    step = 0

    try:
        # [清理]
        if args.clean:
            step += 1
            cprint(f"[{step}/{total_steps}] 清理构建目录...", COLOR_BOLD + COLOR_YELLOW)
            do_clean(build_dir)
            print("")

        # [CMake]
        step += 1
        cprint(f"[{step}/{total_steps}] 配置 CMake...", COLOR_BOLD + COLOR_YELLOW)
        if not do_cmake(args.platform, args.build_type, build_dir):
            sys.exit(1)
        cprint("  CMake 配置完成 [OK]", COLOR_GREEN)
        print("")

        # [Ninja]
        step += 1
        cprint(f"[{step}/{total_steps}] 构建中...", COLOR_BOLD + COLOR_YELLOW)
        if not do_build(args.platform, build_dir):
            sys.exit(1)
        cprint("  Ninja 构建完成 [OK]", COLOR_GREEN)
        print("")

        # [windeployqt]
        if args.platform == "win":
            step += 1
            cprint(f"[{step}/{total_steps}] 部署Qt依赖...", COLOR_BOLD + COLOR_YELLOW)
            if not do_windeployqt(output_path):
                sys.exit(1)
            cprint("  windeployqt 完成 [OK]", COLOR_GREEN)
            print("")

        # [部署]
        if not args.no_deploy:
            step += 1
            cprint(f"[{step}/{total_steps}] 运行...", COLOR_BOLD + COLOR_YELLOW)
            if args.platform == "win":
                do_deploy_win(output_path, wait=do_wait)
            else:
                do_deploy_android(output_path, wait=do_wait, build_type=args.build_type)

        elapsed = time.time() - start_time
        print("")
        cprint("========================================", COLOR_BOLD)
        cprint(f"  构建成功 (耗时: {elapsed:.0f}s)", COLOR_BOLD + COLOR_GREEN)
        cprint(f"  产物: {output_path}", COLOR_GRAY)
        cprint("========================================", COLOR_BOLD)
        print("")

    except KeyboardInterrupt:
        cprint("\n\n构建已取消", COLOR_YELLOW)
        sys.exit(1)
    except Exception as e:
        cprint(f"\n构建异常: {e}", COLOR_RED)
        sys.exit(1)


if __name__ == "__main__":
    main()
