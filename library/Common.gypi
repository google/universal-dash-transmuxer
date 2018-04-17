# Do not OpenSource.  The OpenSource packager will run this to generate
# Xcode projects and Makefiles.

{
  'conditions': [
    ['OS=="mac"', {
      'target_defaults': {
        'xcode_settings': {
          'ARCHS[sdk=macosx*]': 'x86_64',
        },
      },
    }]
  ],
  'includes': [
    '../../../../googlemac/gyp_config/common.gypi',
  ],
  'xcode_settings': {
    'GOOGLE_VERSION_MAJOR': '1',
    'GOOGLE_VERSION_MINOR': '0',
    'GOOGLE_VERSION_FIXLEVEL': '1',
  },
  'variables': {
    'cocoa_http_server_dependency': '<(DEPTH)/third_party/objective_c/CocoaHTTPServer/CocoaHTTPServer.gyp:CocoaHTTPServer',
    'gtest_main': '<(DEPTH)/third_party/gtest/src/gtest_main.cc',
    'openssl_dependency': '<(DEPTH)/third_party/openssl/boringssl/boringssl.gyp:openssl',
    'test_content': ['dash-139.fmp4',
                     'dash-160.fmp4',
                     'mp4box_init.mp4',
                     'sintel-cenc-video-header.mp4',
                     'sintel-cenc-video-segment3.mp4',
                     ],
  }
}
