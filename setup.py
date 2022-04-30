from distutils.core import setup, Extension
import os, os.path

# lua source filenames for versions 5.1 to 5.4
lua_files = [
    'lapi.c',
    'lauxlib.c',
    'lbaselib.c',
    'lbitlib.c',
    'lcode.c',
    'lcorolib.c',
    'lctype.c',
    'ldblib.c',
    'ldebug.c',
    'ldo.c',
    'ldump.c',
    'lfunc.c',
    'lgc.c',
    'linit.c',
    'liolib.c',
    'llex.c',
    'lmathlib.c',
    'lmem.c',
    'loadlib.c',
    'lobject.c',
    'lopcodes.c',
    'loslib.c',
    'lparser.c',
    'lstate.c',
    'lstring.c',
    'lstrlib.c',
    'ltable.c',
    'ltablib.c',
    'ltm.c',
    'lundump.c',
    'lutf8lib.c',
    'lvm.c',
    'lzio.c'
]

files = [
    'pylua.c',
    'pylua_exceptions.c',
    'pylua_function.c',
    'pylua_hooks.c',
    'pylua_object.c',
    'pylua_protect.c',
    'pylua_python.c',
    'pylua_state.c',
    'pylua_stateinfo.c',
    'pylua_table.c',
    'pylua_thread.c',
    'pylua_userdata.c'
]

sources = []
for file in lua_files:
    path = os.path.join("lua", file)
    if os.path.isfile(path):
        sources.append(path)
        
for file in files:
    sources.append(os.path.join("src", file))
    
module = Extension(
    'pylua',
    sources = sources,
    include_dirs = ['lua']
)

setup (name = 'pylua',
       version = '1.0',
       description = 'pylua',
       ext_modules = [module])
