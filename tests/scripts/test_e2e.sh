#!/bin/bash

# ── macOS / Linux 端到端测试 ─────────────────────────────
# 切到项目根目录（无论从哪里执行此脚本）
cd "$(dirname "$0")/../.."

# 用法: ./tests/scripts/test_e2e.sh [hashchecker路径]
# 默认: ./build/hashchecker

pause_and_exit() {
    local code="$1"
    echo ""
    read -rp "按回车键退出... "
    exit "$code"
}

HASHCHECKER="${1:-./build/hashchecker}"

if [ ! -x "$HASHCHECKER" ]; then
    echo "错误: 找不到 hashchecker (${HASHCHECKER})"
    echo "请先构建: cmake -B build && cmake --build build"
    pause_and_exit 1
fi

TMPDIR=$(mktemp -d /tmp/hc_e2e_XXXXXX)
DIR_A="${TMPDIR}/dir_a"
DIR_B="${TMPDIR}/dir_b"
mkdir -p "$DIR_A" "$DIR_B"

PASS=0
FAIL=0

cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

check() {
    local desc="$1"
    local expected_exit="$2"
    shift 2
    echo -n "  ${desc} ... "
    if "$HASHCHECKER" "$@" > /dev/null 2>&1; then
        actual_exit=0
    else
        actual_exit=$?
    fi
    if [ "$actual_exit" -eq "$expected_exit" ]; then
        echo "PASS"
        PASS=$((PASS + 1))
    else
        echo "FAIL (expected exit ${expected_exit}, got ${actual_exit})"
        FAIL=$((FAIL + 1))
    fi
}

echo "=============================================="
echo " HashChecker 端到端测试"
echo " 平台: $(uname -s)"
echo " 临时目录: $TMPDIR"
echo "=============================================="
echo ""

echo "----------------------------------------------"

# ── 1. 生成 1000 个 2KB 随机文件到 dir_a ──────────────
echo "[1/4] 生成 1000 × 2KB 随机文件..."
for i in $(seq -w 1 1000); do
    dd if=/dev/urandom of="${DIR_A}/file_${i}.bin" bs=2048 count=1 2>/dev/null
done
echo "  完成: 1000 个文件已写入 dir_a"

echo "----------------------------------------------"

# ── 2. 复制 dir_a → dir_b（两个目录完全一致）────────────
echo "[2/4] 复制 dir_a → dir_b..."
cp -a "${DIR_A}/." "${DIR_B}/"
echo "  完成: dir_a 与 dir_b 内容相同"

echo "----------------------------------------------"

# ── 3. 验证三种算法均返回 identical ─────────────────────
echo "[3/4] 验证一致性检测..."
check "MD5     一致目录 → exit 0" 0 -t "$DIR_A" -r "$DIR_B" -a md5 -q
check "SHA1    一致目录 → exit 0" 0 -t "$DIR_A" -r "$DIR_B" -a sha1 -q
check "SHA256  一致目录 → exit 0" 0 -t "$DIR_A" -r "$DIR_B" -a sha256 -q

echo "----------------------------------------------"

# ── 4. 替换 dir_b 为 1000 个不同的随机文件 ──────────────
echo "[4/4] 生成 1000 个不同的随机文件替换 dir_b..."
rm -rf "${DIR_B}"
mkdir -p "${DIR_B}"
for i in $(seq -w 1 1000); do
    dd if=/dev/urandom of="${DIR_B}/file_${i}.bin" bs=2048 count=1 2>/dev/null
done
echo "  完成: dir_b 已被替换为不同内容"

echo "----------------------------------------------"

# ── 5. 验证检测到差异 ────────────────────────────────────
check "MD5     不同目录 → exit 1" 1 -t "$DIR_A" -r "$DIR_B" -a md5 -q
check "SHA1    不同目录 → exit 1" 1 -t "$DIR_A" -r "$DIR_B" -a sha1 -q
check "SHA256  不同目录 → exit 1" 1 -t "$DIR_A" -r "$DIR_B" -a sha256 -q

# ── 6. 验证差异输出包含文件差异 ──────────────────────────
echo ""
echo -n "  差异详情输出 ... "
DIFF_OUTPUT=$("$HASHCHECKER" -t "$DIR_A" -r "$DIR_B" -a sha256 2>&1 || true)
if echo "$DIFF_OUTPUT" | grep "RESULT: DIFFERENCES" > /dev/null; then
    echo "PASS (检测到 DIFFERENCES 标记)"
    PASS=$((PASS + 1))
else
    echo "FAIL (未找到 DIFFERENCES 标记)"
    FAIL=$((FAIL + 1))
fi

MISMATCH_COUNT=$(echo "$DIFF_OUTPUT" | grep -c "MISMATCH" || true)
echo "  检测到 ${MISMATCH_COUNT} 处哈希不匹配"

# ── 结果 ──────────────────────────────────────────────────
echo ""
echo "=============================================="
TOTAL=$((PASS + FAIL))
echo " 结果: ${PASS}/${TOTAL} 通过"
if [ "$FAIL" -eq 0 ]; then
    echo " 状态: 全部通过 ✓"
    echo "=============================================="
    pause_and_exit 0
else
    echo " 状态: ${FAIL} 项失败 ✗"
    echo "=============================================="
    pause_and_exit 1
fi
