# Tools, tests, and utilities for DashToHls.
#
# Do not OpenSource.  The OpenSource packager will run this to generate
# Xcode projects and Makefiles.

{
  'target_defaults': {
    'xcode_settings': {
      'CLANG_ENABLE_OBJC_ARC': [ 'YES' ],
    },
    'conditions': [
      ['OS=="mac"', {
        'xcode_settings': {
          'GCC_PREFIX_HEADER': 'DashToHls_osx.pch',
        },
      }],
      ['OS=="ios"', {
        'xcode_settings': {
          'GCC_PREFIX_HEADER': 'DashToHls_ios.pch',
        },
      }],
    ],
  },
  'conditions': [
    ['OS=="mac"', {
      'target_defaults': {
        'xcode_settings': {
          'ARCHS[sdk=macosx*]': 'x86_64',
          'MACOSX_DEPLOYMENT_TARGET': '10.10',
        },
      },
      'targets': [{
          'target_name': 'DashToHlsTools',
          'type': 'executable',
            'xcode_settings': {
              'INFOPLIST_FILE': 'tools/OSX/tools-Info.plist',
            },
            'mac_bundle': 1,
            'mac_bundle_resources': [
              'tools/OSX/Base.lproj/MainMenu.xib',
              'tools/OSX/tools-Info.plist',
              'tools/OSX/en.lproj/InfoPlist.strings',
            ],
            'sources': [
              'tools/OSX/ToolsAppDelegate.h',
              'tools/OSX/ToolsAppDelegate.mm',
              'tools/OSX/ToolsMpdParser.h',
              'tools/OSX/ToolsMpdParser.mm',
              'tools/OSX/main.m',
            ],
            'include_dirs': [
              '..',
            ],
            'dependencies': [
              'DashToHls.gyp:DashToHlsLibrary',
            ],
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/AVFoundation.framework',
              '$(SDKROOT)/System/Library/Frameworks/Security.framework',
          ],
          }],
        },
      ]],
  'targets': [
    {
      'target_name': 'gtestlib',
      'type': 'static_library',
      'include_dirs': [
        '<(DEPTH)/',
        '<(DEPTH)/third_party/gtest/include',
        '<(DEPTH)/third_party/gmock/include',
      ],
      'all_dependent_settings': {
        'include_dirs': [
          '<(DEPTH)/',
          '<(DEPTH)/third_party/gtest/include',
          '<(DEPTH)/third_party/gmock/include',
        ],
        'xcode_settings': {
          'OTHER_CFLAGS': [
          '-DGTEST_USE_OWN_TR1_TUPLE=1',
          ],
        },
      },
      'xcode_settings': {
        'OTHER_CFLAGS': [
        '-DGTEST_USE_OWN_TR1_TUPLE=1',
        ],
      },
      'sources': [
        '<(DEPTH)/third_party/gtest/src/gtest-all.cc',
        '<(DEPTH)/third_party/gmock/src/gmock-all.cc',
      ]
    },
    {
      'target_name': 'DashToHlsPlayer',
      'type': 'executable',
      'conditions': [
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_PREFIX_HEADER': 'DashToHls_osx.pch',
            'INFOPLIST_FILE': 'player/OSX/player-Info.plist',
          },
          'mac_bundle': 1,
          'mac_bundle_resources': [
            'player/OSX/Base.lproj/MainMenu.xib',
            'player/OSX/player-Info.plist',
            'player/OSX/en.lproj/InfoPlist.strings',
          ],
          'sources': [
            'player/OSX/AppDelegate.h',
            'player/OSX/AppDelegate.mm',
            'player/OSX/main.m',
          ],
          'libraries': [
            '$(SDKROOT)/System/Library/Frameworks/AVKit.framework',
            'libcrypto.dylib',
          ],
        }],
        ['OS=="ios"', {
          'xcode_settings': {
            'GCC_PREFIX_HEADER': 'DashToHls_ios.pch',
            'INFOPLIST_FILE': 'player/iOS/player-Info.plist',
          },
          'mac_bundle': 1,
          'mac_bundle_resources': [
            'player/iOS/Base.lproj/Main_iPad.storyboard',
            'player/iOS/Base.lproj/Main_iPhone.storyboard',
            'player/iOS/en.lproj/InfoPlist.strings',
          ],
          'sources': [
            'player/iOS/AppDelegate.h',
            'player/iOS/AppDelegate.mm',
            'player/iOS/ViewController.h',
            'player/iOS/ViewController.mm',
            'player/iOS/main.m',
          ],
          'dependencies': [
            '<(openssl_dependency)',
          ],
          'libraries': [
            '$(SDKROOT)/System/Library/Frameworks/MediaPlayer.framework',
          ],
        }],
      ],
      'sources': [
        'mac_test_files.mm',
        'player/DashHTTPConnection.h',
        'player/DashHTTPConnection.mm',
      ],
      'mac_bundle_resources': [
        '<@(test_content)',
      ],
      'dependencies': [
        'DashToHls.gyp:DashToHlsLibrary',
        '<(cocoa_http_server_dependency)',
      ],
      'libraries': [
        '$(SDKROOT)/System/Library/Frameworks/AVFoundation.framework',
      ],
      'include_dirs': [
        '..',
      ],
    },
    {
      'target_name': 'DashToHlsTests',
      'type': 'executable',
      'xcode_settings': {
        'GCC_PREFIX_HEADER': 'DashToHls_osx.pch',
        'INFOPLIST_FILE': 'UdtInfo.plist',
      },
      'dependencies': [
        'gtestlib',
        'DashToHls.gyp:DashToHlsLibrary',
      ],
      'include_dirs': [
        '..',
      ],
      'conditions': [
        ['OS=="mac"', {
          'libraries': [
            'libcrypto.dylib',
            '$(SDKROOT)/System/Library/Frameworks/AVFoundation.framework',
            '$(SDKROOT)/System/Library/Frameworks/Security.framework',
          ],
        }],
        ['OS=="ios"', {
          'dependencies': [
            '<(openssl_dependency)',
          ],
          'libraries': [
            '$(SDKROOT)/System/Library/Frameworks/AVFoundation.framework',
            '$(SDKROOT)/System/Library/Frameworks/Security.framework',
          ],
        }],
      ],
      'mac_bundle': 1,
      'mac_bundle_resources': [
        '<@(test_content)',
      ],
      'sources': [
        'dash/box_contents_test.cc',
        'dash/box_test.cc',
        'dash/box_type_test.cc',
        'dash/dash_parser_test.cc',
        'dash_to_hls_api_test.cc',
        'mac_test_files.mm',
        'mac_test_files.h',
        'ps/nalu_test.cc',
        'ps/pes_test.cc',
        'ps/program_stream_out_test.cc',
        'ps/psm_test.cc',
        'ps/system_header_test.cc',
        'ts/transport_stream_out_test.cc',
        'utilities_gmock.h',
        'utilities_test.cc',
        'bit_reader_test.cc',
        '<(gtest_main)',
      ],
    },
  ],
}
