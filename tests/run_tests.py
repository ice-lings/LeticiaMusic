#!/usr/bin/env python3
"""LeticiaMusic 测试运行脚本
用法:
    python tests/run_tests.py --list          # 列出所有可用的测试模块
    python tests/run_tests.py sync            # 仅运行同步模块测试
    python tests/run_tests.py musicitem       # 仅运行 MusicItem 测试
    python tests/run_tests.py --all           # 运行全部测试
"""

import argparse
import os
import shutil
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import build as _build

PROJECT_ROOT = _build.PROJECT_ROOT

# ============================================================
# 模块名 → 测试可执行文件名（不含 .exe）
# ============================================================
TEST_MODULES = {
    "musicitem":     "test-musicitem",
    "logger":        "test-logger",
    "viewmodel":     "test-viewmodelmanager",
    "sync":          "test-ftp-sync",
    "sync-compare":  "test-sync-compare",
    "sync-runner":   "test-sync-runner",
}

# ============================================================
# 颜色
# ============================================================
RESET = "\033[0m"
BOLD  = "\033[1m"
GREEN = "\033[32m"
RED   = "\033[31m"
CYAN  = "\033[36m"
GRAY  = "\033[90m"


def cprint(text, color=RESET, bold=False):
    prefix = BOLD if bold else ""
    suffix = RESET if color != RESET else ""
    sys.stdout.write(f"{prefix}{color}{text}{suffix}\n")
    sys.stdout.flush()


def find_build_dirs():
    """查找所有 Debug 构建目录，按修改时间降序排列"""
    build_root = os.path.join(PROJECT_ROOT, "build")
    if not os.path.isdir(build_root):
        return []
    dirs = []
    for name in os.listdir(build_root):
        full = os.path.join(build_root, name)
        if os.path.isdir(full) and "Debug" in name:
            test_dir = os.path.join(full, "tests", "core")
            if os.path.isdir(test_dir):
                dirs.append((os.path.getmtime(full), full))
    dirs.sort(reverse=True)
    return [d[1] for d in dirs]


def find_test_exe(build_dir, module_name):
    """在构建目录中查找测试可执行文件"""
    exe_name = f"{module_name}.exe"
    # 测试 exe 在构建目录根下（CMake 默认）
    test_path = os.path.join(build_dir, exe_name)
    if os.path.exists(test_path):
        return test_path
    # 兼容旧布局（tests/core/ 子目录）
    test_path = os.path.join(build_dir, "tests", "core", exe_name)
    if os.path.exists(test_path):
        return test_path
    return None


def ensure_platform_plugin(build_dir):
    """确保 offscreen 平台插件在构建目录中可用（部分测试需要 QGuiApplication）"""
    plugins_dst = os.path.join(build_dir, "platforms")
    offscreen_dst = os.path.join(plugins_dst, "qoffscreen.dll")
    if os.path.exists(offscreen_dst):
        return True

    offscreen_src = f"{_build.QT_WIN}/plugins/platforms/qoffscreen.dll"
    if not os.path.exists(offscreen_src):
        return False

    os.makedirs(plugins_dst, exist_ok=True)
    shutil.copy2(offscreen_src, offscreen_dst)
    return True


def run_test(test_exe, build_dir):
    """运行单个测试，返回是否全部通过"""
    env = os.environ.copy()
    env["PATH"] = f"{_build.MINGW_HOME}/bin;{_build.QT_WIN}/bin;{build_dir};{env.get('PATH', '')}"
    env["QT_QPA_PLATFORM"] = "offscreen"
    env["QT_QPA_PLATFORM_PLUGIN_PATH"] = f"{_build.QT_WIN}/plugins"

    t0 = time.time()
    result = subprocess.run(
        [test_exe],
        env=env,
        cwd=build_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    elapsed = time.time() - t0

    passed = result.returncode == 0
    return passed, elapsed, result.stdout, result.stderr


def main():
    parser = argparse.ArgumentParser(description="LeticiaMusic 测试运行脚本")
    parser.add_argument("module", nargs="?", help=f"要运行的测试模块: {', '.join(TEST_MODULES.keys())}")
    parser.add_argument("--list", action="store_true", help="列出所有可用的测试模块")
    parser.add_argument("--all", action="store_true", help="运行全部测试")
    parser.add_argument("--build-dir", help="手动指定构建目录（默认自动查找最新的 Debug 目录）")
    args = parser.parse_args()

    # --list
    if args.list:
        cprint("可用的测试模块:", BOLD)
        for mod, exe in TEST_MODULES.items():
            print(f"  {mod:12s} → {exe}.exe")
        return

    _build._resolve_config()

    # 查找构建目录
    if args.build_dir:
        build_dirs = [args.build_dir]
    else:
        build_dirs = find_build_dirs()

    if not build_dirs:
        cprint("错误: 未找到 Debug 构建目录，请先执行 build.py -Win -d 编译", RED)
        sys.exit(1)

    build_dir = build_dirs[0]
    cprint(f"构建目录: {os.path.relpath(build_dir, PROJECT_ROOT)}", GRAY)

    # 确保 offscreen 平台插件可用
    ensure_platform_plugin(build_dir)

    # 确定要运行的模块列表
    if args.all:
        modules = list(TEST_MODULES.keys())
    elif args.module:
        if args.module not in TEST_MODULES:
            cprint(f"错误: 未知模块 '{args.module}'，可用: {', '.join(TEST_MODULES.keys())}", RED)
            sys.exit(1)
        modules = [args.module]
    else:
        # 无参数时列出可用模块并提示
        cprint("请指定要测试的模块:", BOLD)
        for mod, exe in TEST_MODULES.items():
            exe_path = find_test_exe(build_dir, exe)
            status = f"({exe}.exe)" if exe_path else "(未编译)"
            print(f"  {mod:12s} {status}")
        print("")
        cprint("用法: python tests/run_tests.py <模块名>", GRAY)
        cprint("       python tests/run_tests.py --all", GRAY)
        cprint("       python tests/run_tests.py --list", GRAY)
        return

    # 检查测试文件是否存在
    missing = []
    for mod in modules:
        exe_path = find_test_exe(build_dir, TEST_MODULES[mod])
        if not exe_path:
            missing.append(mod)

    if missing:
        cprint(f"错误: 以下测试未编译: {', '.join(missing)}", RED)
        cprint("请先执行: python build.py -Win -d", GRAY)
        sys.exit(1)

    # 依次运行
    print("")
    total = len(modules)
    passed = 0

    for i, mod in enumerate(modules, 1):
        exe_path = find_test_exe(build_dir, TEST_MODULES[mod])
        cprint(f"[{i}/{total}] {mod} ...", BOLD + CYAN)
        ok, elapsed, stdout, stderr = run_test(exe_path, build_dir)

        if ok:
            cprint(f"  PASS ({elapsed:.1f}s)", GREEN)
            passed += 1
        else:
            cprint(f"  FAIL ({elapsed:.1f}s)", RED)
            if stdout.strip():
                for line in stdout.strip().splitlines():
                    print(f"    {line}")
            if stderr.strip():
                for line in stderr.strip().splitlines():
                    cprint(f"    {line}", RED)

    # 汇总
    print("")
    if passed == total:
        cprint(f"全部通过 ({passed}/{total})", BOLD + GREEN)
    else:
        cprint(f"部分通过 ({passed}/{total})", BOLD + RED)
        sys.exit(1)


if __name__ == "__main__":
    main()
