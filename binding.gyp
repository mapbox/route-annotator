{
  'includes': [ 'common.gypi' ],
  'variables': {
      'error_on_warnings%':'true',
      # includes we don't want warnings for.
      # As a variable to make easy to pass to
      # cflags (linux) and xcode (mac)
      'system_includes': [
        "-isystem <(module_root_dir)/<!(node -e \"require('nan')\")",
        '-isystem <(module_root_dir)/mason_packages/.link/include/'
      ]
  },
  'targets': [
    {
      'target_name': 'action_before_build',
      'type': 'none',
      'hard_dependency': 1,
      'actions': [
        {
          'action_name': 'install_mason',
          'inputs': ['./install_mason.sh'],
          'outputs': ['./mason_packages'],
          'action': ['./install_mason.sh']
        }
      ]
    },
    {
      'target_name': 'annotator',
      "type": "static_library",
      'hard_dependency': 1,
      'sources': [
        './src/annotator.cpp',
        './src/database.cpp',
        './src/extractor.cpp',
        './src/segment_speed_map.cpp'
      ],
      'cflags': [
          '<@(system_includes)'
      ],
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS': [
            '<@(system_includes)'
        ],
        'GCC_ENABLE_CPP_RTTI': 'YES',
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'MACOSX_DEPLOYMENT_TARGET':'10.8',
        'CLANG_CXX_LIBRARY': 'libc++',
        'CLANG_CXX_LANGUAGE_STANDARD':'c++14',
        'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0'
      }
    },
    {
      'target_name': '<(module_name)',
      'dependencies': [ 'action_before_build', 'annotator' ],
      'product_dir': '<(module_path)',
      'sources': [
        './src/main_bindings.cpp',
        './src/nodejs_bindings.cpp',
        './src/speed_lookup_bindings.cpp'
      ],
      'conditions': [
        ['error_on_warnings == "true"', {
            'cflags_cc' : [ '-Werror' ],
            'xcode_settings': {
              'OTHER_CPLUSPLUSFLAGS': [ '-Werror' ]
            }
        }]
      ],
      "libraries": [
        '<(module_root_dir)/mason_packages/.link/lib/libbz2.a',
        '<(module_root_dir)/mason_packages/.link/lib/libexpat.a',
        '<(module_root_dir)/mason_packages/.link/lib/libboost_iostreams.a',
        # we link to zlib here to fix this error: ../src/extractor.cpp:(.text._ZN6osmium2io16GzipDecompressor4readEv[_ZN6osmium2io16GzipDecompressor4readEv]+0x46): undefined reference to `gzoffset64'
        # because osmium needs a custom zlib that is different that what is statically linked inside node and available on default ubuntu (which don't have gzoffset64`
        '<(module_root_dir)/mason_packages/.link/lib/libz.a'
      ],
      'cflags': [
          '<@(system_includes)'
      ],
      'ldflags': [
        '-Wl,-z,now',
      ],
      'xcode_settings': {
        'OTHER_LDFLAGS':[
          '-Wl,-bind_at_load'
        ],
        'OTHER_CPLUSPLUSFLAGS': [
            '<@(system_includes)'
        ],
        'GCC_ENABLE_CPP_RTTI': 'YES',
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'MACOSX_DEPLOYMENT_TARGET':'10.8',
        'CLANG_CXX_LIBRARY': 'libc++',
        'CLANG_CXX_LANGUAGE_STANDARD':'c++14',
        'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0'
      }
    },
    {
      'target_name': 'bench',
      'dependencies': [ 'annotator' ],
      'type': 'executable',
      'sources': [ './test/bench.cpp' ],
      'include_dirs': [ 'src/' ],
      'conditions': [
        ['error_on_warnings == "true"', {
            'cflags_cc' : [ '-Werror' ],
            'xcode_settings': {
              'OTHER_CPLUSPLUSFLAGS': [ '-Werror' ]
            }
        }]
      ],
      "libraries": [
        '<(module_root_dir)/mason_packages/.link/lib/libbz2.a',
        '<(module_root_dir)/mason_packages/.link/lib/libexpat.a',
        '<(module_root_dir)/mason_packages/.link/lib/libboost_iostreams.a',
        # we link to zlib here to fix this error: ../src/extractor.cpp:(.text._ZN6osmium2io16GzipDecompressor4readEv[_ZN6osmium2io16GzipDecompressor4readEv]+0x46): undefined reference to `gzoffset64'
        # because osmium needs a custom zlib that is different that what is statically linked inside node and available on default ubuntu (which don't have gzoffset64`
        '<(module_root_dir)/mason_packages/.link/lib/libz.a'
      ],
      'cflags': [
          '<@(system_includes)'
      ],
      'ldflags': [
        '-Wl,-z,now',
      ],
      'xcode_settings': {
        'OTHER_LDFLAGS':[
          '-Wl,-bind_at_load'
        ],
        'OTHER_CPLUSPLUSFLAGS': [
            '<@(system_includes)'
        ],
        'GCC_ENABLE_CPP_RTTI': 'YES',
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'MACOSX_DEPLOYMENT_TARGET':'10.8',
        'CLANG_CXX_LIBRARY': 'libc++',
        'CLANG_CXX_LANGUAGE_STANDARD':'c++14',
        'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0'
      }
    },
    {
      'target_name': 'basic-tests',
      'dependencies': [ 'annotator' ],
      'type': 'executable',
      'sources': [
        './test/basic-tests.cpp',
        './test/basic/annotator.cpp',
        './test/basic/database.cpp',
        './test/basic/extractor.cpp',
        './test/basic/rtree.cpp'
      ],
      'include_dirs' : [
        'src/'
      ],
      'conditions': [
        ['error_on_warnings == "true"', {
            'cflags_cc' : [ '-Werror' ],
            'xcode_settings': {
              'OTHER_CPLUSPLUSFLAGS': [ '-Werror' ]
            }
        }]
      ],
      "libraries": [
        '<(module_root_dir)/mason_packages/.link/lib/libbz2.a',
        '<(module_root_dir)/mason_packages/.link/lib/libexpat.a',
        '<(module_root_dir)/mason_packages/.link/lib/libboost_iostreams.a',
        '<(module_root_dir)/mason_packages/.link/lib/libboost_unit_test_framework.a',
        # we link to zlib here to fix this error: ../src/extractor.cpp:(.text._ZN6osmium2io16GzipDecompressor4readEv[_ZN6osmium2io16GzipDecompressor4readEv]+0x46): undefined reference to `gzoffset64'
        # because osmium needs a custom zlib that is different that what is statically linked inside node and available on default ubuntu (which don't have gzoffset64`
        '<(module_root_dir)/mason_packages/.link/lib/libz.a'
      ],
      'cflags': [
          '<@(system_includes)'
      ],
      'ldflags': [
        '-Wl,-z,now',
      ],
      'xcode_settings': {
        'OTHER_LDFLAGS':[
          '-Wl,-bind_at_load'
        ],
        'OTHER_CPLUSPLUSFLAGS': [
            '<@(system_includes)'
        ],
        'GCC_ENABLE_CPP_RTTI': 'YES',
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'MACOSX_DEPLOYMENT_TARGET':'10.8',
        'CLANG_CXX_LIBRARY': 'libc++',
        'CLANG_CXX_LANGUAGE_STANDARD':'c++14',
        'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0'
      }
    },
    {
      'target_name': 'congestion-tests',
      'dependencies': [ 'annotator' ],
      'type': 'executable',
      'sources': [
        './test/congestion-tests.cpp',
        './test/congestion/congestion.cpp'
      ],
      'include_dirs' : [
        'src/'
      ],
      'conditions': [
        ['error_on_warnings == "true"', {
            'cflags_cc' : [ '-Werror' ],
            'xcode_settings': {
              'OTHER_CPLUSPLUSFLAGS': [ '-Werror' ]
            }
        }]
      ],
      "libraries": [
        '<(module_root_dir)/mason_packages/.link/lib/libbz2.a',
        '<(module_root_dir)/mason_packages/.link/lib/libexpat.a',
        '<(module_root_dir)/mason_packages/.link/lib/libboost_iostreams.a',
        '<(module_root_dir)/mason_packages/.link/lib/libboost_unit_test_framework.a',
        # we link to zlib here to fix this error: ../src/extractor.cpp:(.text._ZN6osmium2io16GzipDecompressor4readEv[_ZN6osmium2io16GzipDecompressor4readEv]+0x46): undefined reference to `gzoffset64'
        # because osmium needs a custom zlib that is different that what is statically linked inside node and available on default ubuntu (which don't have gzoffset64`
        '<(module_root_dir)/mason_packages/.link/lib/libz.a'
      ],
      'cflags': [
          '<@(system_includes)'
      ],
      'ldflags': [
        '-Wl,-z,now',
      ],
      'xcode_settings': {
        'OTHER_LDFLAGS':[
          '-Wl,-bind_at_load'
        ],
        'OTHER_CPLUSPLUSFLAGS': [
            '<@(system_includes)'
        ],
        'GCC_ENABLE_CPP_RTTI': 'YES',
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'MACOSX_DEPLOYMENT_TARGET':'10.8',
        'CLANG_CXX_LIBRARY': 'libc++',
        'CLANG_CXX_LANGUAGE_STANDARD':'c++14',
        'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0'
      }
    }
  ]
}
