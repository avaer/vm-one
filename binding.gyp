{
  'targets': [
    {
      'target_name': 'vm_one',
      'sources': [
        'src/main.cpp',
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
      ],
    },
  ],
}
