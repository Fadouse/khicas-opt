#!/usr/bin/env python3
"""Exercise actual CG50 file loader's allocation and Bfile error paths with stubs."""

from repository import source_path
from pathlib import Path
import os, subprocess, tempfile
from integration_build import ROOT, function

source = r"""
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
static const int MAX_FILENAME_SIZE=270,MAX_TEXTVIEWER_FILESIZE=65536,READWRITE=1;
static bool inexammode=false,allocation_failure=false;
static int opened=0,closed=0,freed=0,open_result=7,file_size=4,read_size=4;
static std::string filename;
void Bfile_StrToName_ncpy(unsigned short *,const unsigned char *s,int){filename=(const char*)s;}
int Bfile_OpenFile_OS(unsigned short *,int){++opened;return open_result;}
int Bfile_GetFileSize_OS(int){return file_size;}
void Bfile_CloseFile_OS(int){++closed;}
int Bfile_ReadFile_OS(int,unsigned char *s,int n,int){if(read_size>0)memset(s,'a',read_size<n?read_size:n);return read_size;}
void *test_malloc(size_t n){return allocation_failure?nullptr:malloc(n);}
void test_free(void *p){if(p)++freed;free(p);}
#define malloc test_malloc
#define free test_free
"""
source += function((source_path("main.cc")).read_text(), "char * c_load_script(")
source += r"""
#undef malloc
#undef free
void reset(){opened=closed=freed=0;allocation_failure=inexammode=false;open_result=7;file_size=read_size=4;}
int main(){
 reset();assert(!c_load_script(nullptr)&&opened==0);
 assert(!c_load_script(std::string(255,'x').c_str())&&opened==0);
 inexammode=true;assert(!c_load_script("x")&&opened==0);
 reset();open_result=-1;assert(!c_load_script("x")&&opened==1&&closed==0);
 reset();file_size=-1;assert(!c_load_script("x")&&closed==1);
 reset();file_size=65537;assert(!c_load_script("x")&&closed==1);
 reset();allocation_failure=true;assert(!c_load_script("x")&&closed==1);
 for(int n:{-1,0,3,5}){reset();read_size=n;assert(!c_load_script("x")&&closed==1&&freed==1);}
 reset();char *p=c_load_script("CG50TEST.xw");assert(p&&std::string(p)=="aaaa"&&closed==1);
 assert(filename=="\\\\fls0\\CG50TEST.xw");free(p);
 reset();file_size=read_size=0;p=c_load_script("empty");assert(p&&!*p&&closed==1);free(p);
 puts("PASS: target loader path bounds, allocation failure, incomplete reads, handle closure and success");
}
"""
with tempfile.TemporaryDirectory(prefix="khicas-loader-") as tmp:
    p = Path(tmp)
    (p / "test.cc").write_text(source)
    subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++11",
            "-O1",
            "-g",
            "-fsanitize=address,undefined",
            str(p / "test.cc"),
            "-o",
            str(p / "test"),
        ],
        check=True,
    )
    subprocess.run(
        [str(p / "test")],
        env=dict(os.environ, ASAN_OPTIONS="detect_leaks=0"),
        check=True,
        timeout=30,
    )
