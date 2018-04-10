{
  'targets': [
    {
      'target_name': 'vmOne',
      'sources': [
        'main.cpp',
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ],
    },
  ],
}
