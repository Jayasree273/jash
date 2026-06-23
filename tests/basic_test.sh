#!/bin/bash
# Basic functionality tests for JASH

JASH="./bin/jash"
TMPDIR="/tmp/jash_test_$$"
PASSED=0
FAILED=0

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

mkdir -p "$TMPDIR"
trap "rm -rf $TMPDIR" EXIT

test_case() {
    local name="$1"
    local cmd="$2"
    local expected="$3"
    
    echo -n "Testing: $name ... "
    
    # Run command through jash, strip ALL occurrences of "jash$ " from everywhere
    result=$(printf "%s\nexit\n" "$cmd" | timeout 2 $JASH 2>&1 | \
             perl -pe 's/jash\$ //g' | sed '/^$/d')
    
    if [[ "$result" == "$expected" ]]; then
        echo -e "${GREEN}✓ PASS${NC}"
        ((PASSED++))
    else
        echo -e "${RED}✗ FAIL${NC}"
        echo "  Expected: '$expected'"
        echo "  Got:      '$result'"
        ((FAILED++))
    fi
}

echo "=========================================="
echo "JASH Basic Functionality Tests"
echo "=========================================="
echo ""

# ========== BUILTIN TESTS ==========
echo "--- Builtins ---"

test_case "echo simple" "echo hello" "hello"
test_case "echo multiple args" "echo hello world" "hello world"
test_case "echo with quotes" "echo 'hello world'" "hello world"

# ========== VARIABLE TESTS ==========
echo ""
echo "--- Variables ---"

test_case "export and expand" "export FOO=hello
echo \$FOO" "hello"

test_case "export with no value" "export BAR
echo \$BAR" ""

test_case "variable in double quotes" "export VAL=test
echo \"prefix_\$VAL\"" "prefix_test"

test_case "variable in single quotes (no expansion)" "export VAL=test
echo 'prefix_\$VAL'" "prefix_\$VAL"

test_case "unset variable shows literal" "export X=value
unset X
echo \$X" "\$X"

# ========== EXPANSION TESTS ==========
echo ""
echo "--- Expansion ---"

test_case "dollar question mark after success" "true
echo \$?" "0"

test_case "dollar question mark after failure" "false
echo \$?" "1"

test_case "braced variable expansion" "export NAME=jash
echo \${NAME}" "jash"

test_case "tilde expansion in pwd" "cd ~
pwd" "$HOME"

# ========== QUOTING TESTS ==========
echo ""
echo "--- Quoting ---"

test_case "single quotes preserve literal" \
    "echo 'hello world'" \
    "hello world"

test_case "double quotes allow expansion" \
    "export MSG=hi
echo \"say \$MSG\"" \
    "say hi"

test_case "single quotes prevent expansion" \
    "export MSG=hi
echo 'say \$MSG'" \
    "say \$MSG"

line2"

# ========== COMPLEX TESTS ==========
echo ""
echo "--- Complex Scenarios ---"

test_case "export then expand" \
    "export GREETING=hello
export NAME=world
echo \$GREETING \$NAME" \
    "hello world"

test_case "multiple variables in string" \
    "export A=foo
export B=bar
export C=baz
echo \$A-\$B-\$C" \
    "foo-bar-baz"

test_case "env shows exported variables" \
    "export TESTVAR=testval
env" \
    "TESTVAR=testval"

test_case "pwd returns home directory" \
    "cd ~
pwd" \
    "$HOME"

# ========== SUMMARY ==========
echo ""
echo "=========================================="
echo "Test Results"
echo "=========================================="
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
echo "Total:  $((PASSED + FAILED))"
echo "=========================================="

if [[ $FAILED -eq 0 ]]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
