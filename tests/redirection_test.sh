#!/bin/bash
# Redirection tests for JASH (manual verification)

JASH="./bin/jash"
TMPDIR="/tmp/jash_redir_$$"
PASSED=0
FAILED=0

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

mkdir -p "$TMPDIR"
trap "rm -rf $TMPDIR" EXIT

test_redirect() {
    local name="$1"
    local script="$2"
    local file="$3"
    local expected="$4"
    
    echo -n "Testing: $name ... "
    
    # Write script to temp file
    echo "$script" > "$TMPDIR/test_script.sh"
    
    # Run jash with script file
    timeout 2 $JASH < "$TMPDIR/test_script.sh" > /dev/null 2>&1
    
    if [[ -f "$file" ]]; then
        result=$(cat "$file")
        if [[ "$result" == "$expected" ]]; then
            echo -e "${GREEN}✓ PASS${NC}"
            ((PASSED++))
        else
            echo -e "${RED}✗ FAIL${NC}"
            echo "  Expected: '$expected'"
            echo "  Got:      '$result'"
            ((FAILED++))
        fi
    else
        echo -e "${RED}✗ FAIL${NC}"
        echo "  File not created: $file"
        ((FAILED++))
    fi
}

echo "=========================================="
echo "JASH Redirection Tests"
echo "=========================================="
echo ""

test_redirect "output >" \
    "echo hello > $TMPDIR/out.txt
exit" \
    "$TMPDIR/out.txt" \
    "hello"

test_redirect "append >>" \
    "echo line1 > $TMPDIR/app.txt
echo line2 >> $TMPDIR/app.txt
exit" \
    "$TMPDIR/app.txt" \
    "line1
line2"

test_redirect "multiple appends" \
    "echo a > $TMPDIR/multi.txt
echo b >> $TMPDIR/multi.txt
echo c >> $TMPDIR/multi.txt
exit" \
    "$TMPDIR/multi.txt" \
    "a
b
c"

test_redirect "input <" \
    "echo 'input data' > $TMPDIR/input.txt
cat < $TMPDIR/input.txt > $TMPDIR/output.txt
exit" \
    "$TMPDIR/output.txt" \
    "input data"

echo ""
echo "=========================================="
echo "Test Results"
echo "=========================================="
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
echo "=========================================="

if [[ $FAILED -eq 0 ]]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    exit 1
fi
