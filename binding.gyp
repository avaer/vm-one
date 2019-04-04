{
  'targets': [
    {
      'target_name': 'vm_one',
      'sources': [
        'src/main.cpp',
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
        "src"
      ],
      'conditions': [
        ['"<!(echo $ANDROID)"=="1"', {
          'defines': ['ANDROID'],
        }],
        ['"<!(echo $LUMIN)"=="1"', {
          'defines': ['LUMIN'],
        }],
      ],
    },
    {
      'target_name': 'vm_one2',
      'sources': [
        'src/child.cpp',
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
        "src"
      ],
      'conditions': [
        ['"<!(echo $ANDROID)"=="1"', {
          'defines': ['ANDROID'],
        }],
        ['"<!(echo $LUMIN)"=="1"', {
          'defines': ['LUMIN'],
        }],
      ],
    },
  ],
}
