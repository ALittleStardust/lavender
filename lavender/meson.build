# Licensed under GPLv2 or later until the library is deemed stable enough for general use, see LICENSE in the source tree.

source = [
    'src/nt.cpp',
    'src/os/version.cpp',
    'src/os/os.cpp',
    'src/cpu/cpu.cpp',
    'src/process/process.cpp'
]

cxx = meson.get_compiler('cpp')
cxx_id = cxx.get_id()

if cxx_id == 'msvc'
    # Due to cpp_std not being honored (github.com/mesonbuild/meson/issues/5590), for now we'll have to resort to setting C++ standard through cpp_args.
    cpp_args = ['/std:c++17', '/DEBUG:FULL']
    link_args = ['powrprof.lib', 'slwga.lib', 'rpcrt4.lib', 'netapi32.lib', 'ntdll.dll']
elif cxx_id == 'gcc' or cxx_id == 'clang'
    cpp_args = ['-std=c++17', '-g']
    link_args = ['-lpowrprof', '-lslwga', '-lrpcrt4', '-lnetapi32', '-lntdll', '-lstdc++']
else
    error('\nUnidentified compiler: ' + cxx_id + '\n' + meson.project_name() + ' requires either GCC, LLVM/Clang, or MSVC.')
endif

lavender = static_library (
    'lavender',
    sources : source,
    cpp_args : cpp_args,
    link_args : link_args,
    include_directories : [
        include_directories('include')
    ]
)

install_subdir (
    'include',
    install_dir : meson.current_build_dir()
)
