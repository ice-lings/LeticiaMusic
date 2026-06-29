#!/usr/bin/env python3
"""LeticiaMusic 测试程序构建脚本

用法:
    python tests/build_tests.py --manual              # 构建 manual_test 并自动启动（默认 Windows Debug）
    python tests/build_tests.py --manual --wait       # 构建 + 启动，持续运行不退出
    python tests/build_tests.py --manual --mode handle_reuse  # 构建 + 启动，运行句柄复用测试
    python tests/build_tests.py -And -d --manual      # Android 构建 manual_test + 部署 + 日志捕获（5 秒后退出）
    python tests/build_tests.py -And -d --manual --wait  # Android 构建 + 部署 + 持续日志（Ctrl+C 退出）
    python tests/build_tests.py --core --run          # 构建所有核心测试并运行
    python tests/build_tests.py --all --run           # 构建全部测试并运行
    python tests/build_tests.py -Win -d               # 显式指定平台和构建类型
    python tests/build_tests.py --list                # 列出可构建的目标
"""

import argparse
import io
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import build as _build

MUMU_ADDRESS = "127.0.0.1:16416"

# ============================================================
# 颜色
# ============================================================
RESET = "\033[0m"
BOLD  = "\033[1m"
GREEN = "\033[32m"
YELLOW = "\033[33m"
RED   = "\033[31m"
CYAN  = "\033[36m"
GRAY  = "\033[90m"


def cprint(text, color=RESET, bold=False):
    prefix = BOLD if bold else ""
    suffix = RESET if color != RESET else ""
    sys.stdout.write(f"{prefix}{color}{text}{suffix}\n")
    sys.stdout.flush()


def get_build_dir(platform: str, build_type: str) -> str:
    display_type = build_type.capitalize()
    if platform == "win":
        suffix = f"Desktop_Qt_{_build.QT_VERSION.replace('.', '_')}_MinGW_64_bit-{display_type}"
    else:
        suffix = f"Qt_{_build.QT_VERSION.replace('.', '_')}_Clang_arm64_v8a-{display_type}"
    return os.path.join(_build._build.PROJECT_ROOT, "build", suffix).replace("\\", "/")


def get_build_ninja(build_dir: str) -> str:
    return os.path.join(build_dir, "build.ninja")


def do_cmake(platform: str, build_type: str, build_dir: str) -> bool:
    source_dir = _build._build.PROJECT_ROOT.replace("\\", "/")

    cmake_args = [
        _build.CMAKE,
        "-G", "Ninja",
        f"-DCMAKE_MAKE_PROGRAM={_build.NINJA}",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-S{source_dir}",
        f"-B{build_dir}",
        "-DBUILD_TESTS=ON",
    ]

    if platform == "win":
        cmake_args.append(f"-DCMAKE_PREFIX_PATH={_build.QT_WIN}")
        cmake_args.append(f"-DCMAKE_C_COMPILER={_build.MINGW_HOME}/bin/gcc.exe")
        cmake_args.append(f"-DCMAKE_CXX_COMPILER={_build.MINGW_HOME}/bin/g++.exe")
    else:
        cmake_args.append(f"-DCMAKE_TOOLCHAIN_FILE={_build.QT_TOOLCHAIN}")
        cmake_args.append(f"-DANDROID_NDK_ROOT={_build.ANDROID_NDK_ROOT}")
        cmake_args.append(f"-DANDROID_SDK_ROOT={_build.ANDROID_SDK_ROOT}")

    cprint(f"  → 源目录: {source_dir}", GRAY)
    cprint(f"  → 构建目录: {os.path.relpath(build_dir, _build._build.PROJECT_ROOT)}", GRAY)

    env = os.environ.copy()
    if platform == "android":
        env["JAVA_HOME"] = _build.JAVA_HOME
    else:
        env["PATH"] = f"{_build.MINGW_HOME}/bin;{_build.QT_WIN}/bin;{env.get('PATH', '')}"

    print("")
    result = subprocess.run(cmake_args, env=env)

    if result.returncode != 0:
        cprint(f"\n  CMake 配置失败 ({result.returncode})", RED)
        return False
    return True


def do_build(platform: str, build_dir: str, targets: list) -> bool:
    cmd = [_build.NINJA, "-C", build_dir] + targets

    env = os.environ.copy()
    if platform == "android":
        env["JAVA_HOME"] = _build.JAVA_HOME
    else:
        env["PATH"] = f"{_build.MINGW_HOME}/bin;{_build.QT_WIN}/bin;{env.get('PATH', '')}"

    print("")
    result = subprocess.run(cmd, env=env, stdin=subprocess.DEVNULL)

    if result.returncode != 0:
        cprint(f"\n  构建失败 ({result.returncode})", RED)
        return False
    return True


MANUAL_PACKAGE = "org.qtproject.example.ManualTest"
MANUAL_ACTIVITY = "org.qtproject.qt.android.bindings.QtActivity"
MANUAL_COMPONENT = f"{MANUAL_PACKAGE}/{MANUAL_ACTIVITY}"

WAIT_FLAG = "/sdcard/manual_test_wait"


def run_manual_windows(build_dir: str, wait: bool, mode: str = ""):
    """启动 Windows manual_test.exe 并实时捕获日志

    Args:
        build_dir: 构建目录路径
        wait: True=持续运行直到 Ctrl+C, False=5 秒超时自动退出
        mode: 测试模式（如 handle_reuse），透传给 manual_test.exe 的 --mode 参数
    """
    exe_path = os.path.join(build_dir, "manual_test.exe").replace("\\", "/")
    if not os.path.exists(exe_path):
        cprint(f"  EXE 不存在: {exe_path}", RED)
        return

    env = os.environ.copy()
    env["PATH"] = f"{_build.MINGW_HOME}/bin;{_build.QT_WIN}/bin;{build_dir};{env.get('PATH', '')}"

    cmd = [exe_path]
    if mode:
        cmd.append("--mode")
        cmd.append(mode)
    if wait:
        cmd.append("--wait")
        cprint("  → 模式: 持续运行 (--wait)，Ctrl+C 停止", GRAY)
    else:
        cprint("  → 模式: 5 秒超时自动退出", GRAY)

    cprint("  → 启动 manual_test.exe...", GRAY)
    print("")

    proc = None
    try:
        proc = subprocess.Popen(
            cmd,
            env=env,
            cwd=build_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            stdin=subprocess.DEVNULL,
        )

        if wait:
            while True:
                line = proc.stdout.readline()
                if not line:
                    if proc.poll() is not None:
                        break
                    continue
                sys.stdout.write(line)
                sys.stdout.flush()
            returncode = proc.wait()
            cprint(f"\n  程序退出 (exit code: {returncode})", YELLOW)
        else:
            deadline = time.time() + 5.0
            crashed = False
            while time.time() < deadline:
                line = proc.stdout.readline()
                if line:
                    sys.stdout.write(line)
                    sys.stdout.flush()
                if proc.poll() is not None:
                    returncode = proc.wait()
                    if returncode != 0:
                        crashed = True
                        cprint(f"\n  程序异常退出! (exit code: {returncode})", RED)
                    break

            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait()

            if not crashed:
                cprint("\n  5 秒已到，运行正常，脚本退出", GREEN)

    except KeyboardInterrupt:
        if proc and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
        cprint("\n  用户中断", YELLOW)


def do_deploy_manual_apk(build_dir: str, wait: bool):
    """部署 manual_test APK 到 Mumu 模拟器（安装 + 启动 + 日志捕获）

    Args:
        build_dir: 构建目录路径
        wait: True=持续运行直到 Ctrl+C, False=5 秒超时自动退出
    """
    apk_path = os.path.join(build_dir, "tests", "manual", "android-build", "manual_test.apk").replace("\\", "/")

    if not os.path.exists(apk_path):
        cprint(f"  APK 不存在: {apk_path}", RED)
        return

    cprint("  → 连接 Mumu 模拟器...", GRAY)
    subprocess.run([_build.ADB, "connect", MUMU_ADDRESS],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    cprint("  → 安装 APK...", GRAY)
    result = subprocess.run(
        [_build.ADB, "-s", MUMU_ADDRESS, "install", "-r", apk_path],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        errors="replace",
    )

    if "Success" in result.stdout:
        cprint("  APK 安装成功 [OK]", GREEN)
    else:
        cprint("  APK 安装可能失败", YELLOW)
        if result.stdout.strip():
            cprint(f"  {result.stdout.strip()}", GRAY)
        if result.stderr.strip():
            cprint(f"  {result.stderr.strip()}", RED)
        return

    if wait:
        subprocess.run(
            [_build.ADB, "-s", MUMU_ADDRESS, "shell", "touch", WAIT_FLAG],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )
        cprint("  → 模式: 持续运行 (--wait)，Ctrl+C 停止", GRAY)
    else:
        subprocess.run(
            [_build.ADB, "-s", MUMU_ADDRESS, "shell", "rm", "-f", WAIT_FLAG],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )
        cprint("  → 模式: 5 秒超时自动退出", GRAY)

    cprint("  → 启动 APP...", GRAY)
    subprocess.run(
        [_build.ADB, "-s", MUMU_ADDRESS, "shell", "logcat", "-c"],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    )
    subprocess.run(
        [_build.ADB, "-s", MUMU_ADDRESS, "shell",
         "am", "start", "-n", MANUAL_COMPONENT],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    )
    time.sleep(1.5)

    cprint("  → 捕获日志...", GRAY)
    print("")

    proc = None
    try:
        if wait:
            subprocess.run(
                [_build.ADB, "-s", MUMU_ADDRESS, "logcat", "-s", "Leticia", "Qt"]
            )
        else:
            proc = subprocess.Popen(
                [_build.ADB, "-s", MUMU_ADDRESS, "logcat", "-s", "Leticia", "Qt"],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                encoding="utf-8",
                errors="replace",
            )
            deadline = time.time() + 5.0
            crashed = False
            while time.time() < deadline:
                line = proc.stdout.readline()
                if line:
                    sys.stdout.write(line)
                    sys.stdout.flush()
                if proc.poll() is not None:
                    cprint("\n  logcat 异常退出", RED)
                    break

            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()

            subprocess.run(
                [_build.ADB, "-s", MUMU_ADDRESS, "shell", "am", "force-stop", MANUAL_PACKAGE],
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
            )

            if not crashed:
                cprint("\n  5 秒已到，运行正常，脚本退出", GREEN)

    except KeyboardInterrupt:
        if proc and proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
        cprint("\n  用户中断", YELLOW)


# ============================================================
# 测试目标定义
# ============================================================
CORE_TARGETS = [
    "test-musicitem",
    "test-logger",
    "test-viewmodelmanager",
    "test-ftp-sync",
    "test-sync-compare",
    "test-sync-runner",
]

MANUAL_TARGET = "manual_test"


def resolve_targets(args, platform: str) -> list:
    """根据参数解析需要构建的 ninja 目标列表"""
    targets = []
    if args.manual or args.all:
        if platform == "android":
            targets.append("manual_test_make_apk")
        else:
            targets.append(MANUAL_TARGET)
    if args.core or args.all:
        targets.extend(CORE_TARGETS)
    return targets


def main():
    # 控制台使用 UTF-8 编码
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

    # 参数大小写标准化
    for i in range(1, len(sys.argv)):
        if sys.argv[i].lower() in ("-win", "-and", "-d", "-r"):
            sys.argv[i] = sys.argv[i].lower()

    parser = argparse.ArgumentParser(description="LeticiaMusic 测试程序构建脚本")
    parser.add_argument("-win", dest="platform", action="store_const", const="win", help="Windows 平台")
    parser.add_argument("-and", dest="platform", action="store_const", const="android", help="Android 平台")
    parser.add_argument("-d", dest="build_type", action="store_const", const="debug", help="Debug 模式")
    parser.add_argument("-r", dest="build_type", action="store_const", const="release", help="Release 模式")
    parser.add_argument("--manual", action="store_true", help="构建 manual_test")
    parser.add_argument("--core", action="store_true", help="构建所有核心测试")
    parser.add_argument("--all", action="store_true", help="构建 manual_test + 所有核心测试")
    parser.add_argument("--run", action="store_true", help="构建后运行核心测试")
    parser.add_argument("--wait", action="store_true", help="持续运行直到手动 Ctrl+C（默认 5 秒超时自动退出）")
    parser.add_argument("--mode", type=str, default="", help="透传给 manual_test.exe 的 --mode 参数（如 handle_reuse）")
    parser.add_argument("--list", action="store_true", help="列出所有可构建的目标")
    args = parser.parse_args()

    # --list
    if args.list:
        cprint("可构建的测试目标:", BOLD)
        print(f"  manual_test (--manual)")
        print("")
        cprint("核心测试 (--core):", BOLD)
        for t in CORE_TARGETS:
            print(f"  {t}")
        return

    _build._resolve_config()

    # 默认值：Windows Debug
    if not args.platform:
        args.platform = "win"
    if not args.build_type:
        args.build_type = "debug"

    # 必须指定构建范围
    if not args.manual and not args.core and not args.all:
        cprint("错误: 必须指定构建范围 --manual / --core / --all", RED)
        parser.print_help()
        sys.exit(1)

    # 确定目标列表
    targets = resolve_targets(args, args.platform)
    display_platform = "Windows" if args.platform == "win" else "Android"
    display_type = args.build_type.capitalize()

    build_dir = get_build_dir(args.platform, args.build_type)

    # 打印头部
    print("")
    cprint("========================================", BOLD)
    cprint("  LeticiaMusic Test Build Script", BOLD)
    cprint(f"  平台: {display_platform:8s} | "
           f"类型: {display_type:7s} | "
           f"范围: {'manual_test + core' if args.all else ('manual_test' if args.manual else 'core')}", CYAN)
    cprint(f"  目标: {', '.join(targets)}", GRAY)
    cprint("========================================", BOLD)
    print("")

    start_time = time.time()
    total_steps = 2  # cmake + ninja（默认）

    step = 0

    try:
        # [CMake] 仅在 build.ninja 不存在时执行
        ninja_file = get_build_ninja(build_dir)
        if not os.path.exists(ninja_file):
            step += 1
            cprint(f"[{step}/{total_steps}] 配置 CMake（BUILD_TESTS=ON）...", BOLD + YELLOW)
            if not do_cmake(args.platform, args.build_type, build_dir):
                sys.exit(1)
            cprint("  CMake 配置完成 [OK]", GREEN)
            print("")
        else:
            cprint(f"  CMake 已配置，跳过（{os.path.relpath(build_dir, _build.PROJECT_ROOT)}）", GRAY)
            print("")

        # [Ninja]
        step += 1
        cprint(f"[{step}/{total_steps}] 构建测试目标...", BOLD + YELLOW)
        if not do_build(args.platform, build_dir, targets):
            sys.exit(1)
        cprint("  Ninja 构建完成 [OK]", GREEN)
        print("")

        elapsed = time.time() - start_time
        print("")
        cprint("========================================", BOLD)
        cprint(f"  构建成功 (耗时: {elapsed:.0f}s)", BOLD + GREEN)
        cprint(f"  构建目录: {os.path.relpath(build_dir, _build.PROJECT_ROOT)}", GRAY)
        cprint("========================================", BOLD)
        print("")

        # ============================================================
        # --manual 构建后自动启动产物
        # ============================================================
        if args.manual:
            if args.platform == "android":
                cprint("  → 部署到 Mumu 模拟器并启动...", GRAY)
                do_deploy_manual_apk(build_dir, wait=args.wait)
            else:
                cprint("  → 启动 manual_test...", GRAY)
                run_manual_windows(build_dir, wait=args.wait, mode=args.mode)

        # ============================================================
        # --run 选项（核心测试，或显式指定运行）
        # ============================================================
        if args.run:
            if not args.manual or args.core or args.all:
                run_args = ["python", os.path.join(_build.PROJECT_ROOT, "tests", "run_tests.py")]
                if args.all or args.core:
                    run_args.append("--all")
                elif args.manual:
                    run_args.append("manual")
                run_args.extend(["--build-dir", build_dir])

                cprint("  → 启动核心测试...", GRAY)
                print("")
                subprocess.run(run_args, env=os.environ.copy())

    except KeyboardInterrupt:
        cprint("\n\n构建已取消", YELLOW)
        sys.exit(1)
    except Exception as e:
        cprint(f"\n构建异常: {e}", RED)
        sys.exit(1)


if __name__ == "__main__":
    main()
