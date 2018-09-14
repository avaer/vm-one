{
  'targets': [
    {
      'target_name': 'vm_one',
      'sources': [
        'main.cpp',
        'src/vm.cpp',
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
        "src"
      ],
      'conditions': [
        ['"<!(echo $LUMIN)"=="1"', {
          'defines': ['LUMIN'],
        }],
        ['"<!(echo $ANDROID)"=="1"', {
          'defines': ['ANDROID'],
        }],
      ],
    },
  ],
}
