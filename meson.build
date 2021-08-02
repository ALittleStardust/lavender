# Licensed under GPLv2 or later until the library is deemed stable enough for general use, see LICENSE in the source tree.

project (
    'lavender', 
    ['c', 'cpp']
)

subdir ('lavender')

include_directories = [
    include_directories('lavender/include')
]

targets = {
    'platform_get_version_string' : [ 'examples/platform_get_version_string.cpp' ],
    'cpu' : [ 'examples/cpu.cpp' ],
    'os' : [ 'examples/os.cpp' ]
}

foreach target, source : targets
    executable (
        target,
        sources : source,
        cpp_args : cpp_args,
        link_args : link_args,
        include_directories : include_directories,
        link_with : lavender
    )    
endforeach
