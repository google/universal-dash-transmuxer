# Library targets for DashToHls. These minimize external dependencies (such
# as OpenSSL) so other projects can depend on them without bringing in
# unnecessary dependencies.
{
  'target_defaults': {
    'xcode_settings': {
      'CLANG_ENABLE_OBJC_ARC': [ 'YES' ],
    },
  },
  'targets': [
    {
      'target_name': 'DashToHlsLibrary',
      'type': 'static_library',
      'xcode_settings': {
        'GCC_PREFIX_HEADER': 'DashToHls_osx.pch',
        'CLANG_CXX_LIBRARY': 'libc++',
      },
      'direct_dependent_settings': {
        'include_dirs': [
         '../include',
         '..',
        ]
      },
      'include_dirs': [
        '..',
      ],
      'sources': [
        'dash_to_hls_api.cc',
        'utilities.cc',
        'utilities.h',
      ],
      'dependencies': [
        'DashToHlsDash',
        'DashToHlsDefaultDiagnosticCallback',
        'DashToHlsPs',
        'DashToHlsTs',
      ],
    },
    {
      'target_name': 'DashToHlsDash',
      'type': 'static_library',
      'xcode_settings': {
        'GCC_PREFIX_HEADER': 'DashToHls_osx.pch',
        'CLANG_CXX_LIBRARY': 'libc++',
      },
      'include_dirs': [
        '..',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '..',
        ],
      },
      'sources': [
        'utilities.h',
        'utilities.cc',
        '<!@(find dash -type f -name "*.h")',
        '<!@(find dash -type f -name "*.cc" ! -name "*_test.cc")',
      ],
    },
    {
      'target_name': 'DashToHlsPs',
      'type': 'static_library',
      'xcode_settings': {
        'GCC_PREFIX_HEADER': 'DashToHls_osx.pch',
        'CLANG_CXX_LIBRARY': 'libc++',
      },
      'include_dirs': [
        '..',
      ],
      'sources': [
        'utilities.h',
        'utilities.cc',
        '<!@(find ps -type f -name "*.h")',
        '<!@(find ps -type f -name "*.cc" ! -name "*_test.cc")',
      ],
    },
    {
      'target_name': 'DashToHlsTs',
      'type': 'static_library',
      'xcode_settings': {
        'GCC_PREFIX_HEADER': 'DashToHls_osx.pch',
        'CLANG_CXX_LIBRARY': 'libc++',
      },
      'include_dirs': [
        '..',
      ],
      'sources': [
        'ts/transport_stream_out.cc',
        'ts/transport_stream_out.h',
      ],
    },
    # Note: If you depend on any of the sub-libraries here, you either need to
    # depend on this to implement the DashToHlsDefaultDiagnosticCallback
    # function, or, implement it yourself. Otherwise, the project will fail to
    # link.
    {
      'target_name': 'DashToHlsDefaultDiagnosticCallback',
      'type': 'static_library',
      'xcode_settings': {
        'GCC_PREFIX_HEADER': 'DashToHls_osx.pch',
        'CLANG_CXX_LIBRARY': 'libc++',
      },
      'sources': [
        'dash_to_hls_default_diagnostic_callback.mm',
      ],
    }
  ]
}
