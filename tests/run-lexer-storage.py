#!/usr/bin/env python3
"""Compare lexer preprocessing and check scanner-owned diagnostic text lifetime.

The actual preprocessing functions use a deterministic Flex buffer stub. This
is not a substitute for executing the complete target parser on the calculator.
"""

from repository import source_path
import json, os, shlex, subprocess, tempfile
from pathlib import Path
from integration_build import ROOT, function

old = subprocess.check_output(
    ["git", "show", "checkpoint/integration-cycle2-2026a:input_lexer.ll"],
    cwd=ROOT,
    text=True,
)
new = (source_path("input_lexer.ll")).read_text()


# The generic function extractor counts braces in strings; preprocessing has
# unmatched bracket literals, so slice at explicit adjacent function signatures.
def get(s):
    a = s.index("    YY_BUFFER_STATE set_lexer_string(")
    b = s.index("    int delete_lexer_string(", a)
    return s[a:b]


assert get(new) == get((source_path("input_lexer.cc")).read_text()), (
    "Regenerated lexer is out of sync"
)
source = r"""
#include "giacPCH.h"
#include "input_lexer.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include <fstream>
struct yy_buffer_state {std::string text; char *yy_ch_buf;};
typedef void *yyscan_t;
struct FakeScanner {const giac::context *ctx;};
int confirm(const char *,const char *,bool){return 0;}
int giac_yylex_init(yyscan_t *s){*s=new FakeScanner;return 0;}
void giac_yyset_extra(const giac::context *c,yyscan_t s){static_cast<FakeScanner*>(s)->ctx=c;}
const giac::context *giac_yyget_extra(yyscan_t s){return static_cast<FakeScanner*>(s)->ctx;}
YY_BUFFER_STATE giac_yy_scan_string(const char *t,yyscan_t){
 auto *b=new yy_buffer_state;b->text=t;b->yy_ch_buf=&b->text[0];return b;
}
void yy_delete_buffer(YY_BUFFER_STATE b,yyscan_t){delete b;}
void yylex_destroy(yyscan_t s){delete static_cast<FakeScanner*>(s);}
namespace giac {
"""
source += get(old).replace("set_lexer_string(", "set_lexer_string_before(", 1)
source += get(new)
source += function(new, "    int delete_lexer_string(")
source += r"""
}
int main(int argc,char **argv){
 using namespace giac;assert(argc==2);context c;const context *contextptr=&c;
 std::ifstream in(argv[1]);std::string line;unsigned count=0;
 while(std::getline(in,line)){
  for(int mode=0;mode<=3;++mode){
   xcas_mode(contextptr)=mode;
   for(int limit:{0,9000}){
    void *before,*after;
    auto a=set_lexer_string_before(line,before,contextptr,limit);
    std::string expected=a->text;yy_delete_buffer(a,before);yylex_destroy(before);
    auto b=set_lexer_string(line,after,contextptr,limit);
    assert(b->text==expected);
    assert(currently_scanned(contextptr)==b->yy_ch_buf);
    // Destroy stack-local scratch before inspecting error-reporting input.
    volatile char scratch[8192];for(unsigned i=0;i<sizeof(scratch);++i)scratch[i]='!';
    assert(std::string(currently_scanned(contextptr))==expected);
    delete_lexer_string(b,after);assert(!*currently_scanned(contextptr));++count;
   }
  }
 }
 std::cout<<"PASS: "<<count<<" lexer normalization comparisons and diagnostic lifetimes\n";
}
"""
inputs = [
    "",
    "x",
    "1+2",
    "sin(x",
    "[1,2",
    "x)",
    "x]",
    "x:=1:;:;",
    '"a..b"',
    "x\\\ny",
    "1e−3",
    "a→b",
    "x≤3",
    '"escaped\\"text"',
    "/* comment */x",
    "// comment\rx+1",
]
for corpus in (
    "calculus-corpus.json",
    "generalization-corpus.json",
    "generalization-cycle2.json",
    "generalization-cycle3.json",
):
    inputs += [
        c["f"] for c in json.loads((ROOT / "tests" / corpus).read_text())["cases"]
    ]
inputs += ["x" * n for n in (6143, 6144, 8191, 8192, 9000)]
with tempfile.TemporaryDirectory(prefix="khicas-lexer-") as tmp:
    p = Path(tmp)
    (p / "test.cc").write_text(source)
    (p / "inputs.txt").write_text("\n".join(inputs) + "\n")
    flags = [
        os.environ.get("CXX", "c++"),
        "-std=c++11",
        "-O1",
        "-g",
        "-fsanitize=address,undefined",
        "-fno-omit-frame-pointer",
        "-DHAVE_CONFIG_H",
        "-DGIAC_GENERIC_CONSTANTS",
        "-Wno-deprecated-declarations",
        "-I",
        os.environ.get("GIAC_INCLUDE", "/usr/include/giac"),
    ] + shlex.split(os.environ.get("CXXFLAGS", ""))
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac"]
    subprocess.run(
        flags + [str(p / "test.cc")] + libs + ["-o", str(p / "test")], check=True
    )
    env = dict(
        os.environ, ASAN_OPTIONS="detect_leaks=0:detect_stack_use_after_return=1"
    )
    subprocess.run(
        [str(p / "test"), str(p / "inputs.txt")], env=env, check=True, timeout=60
    )
