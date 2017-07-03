{
  "variables": {
    "module_name": "mitol",
    "module_path": "./bin"
  },
  "targets": [
    {
      "target_name": "mitol",
      "sources": [
        "mns.cc",
        "src/Http.cpp",
        "src/Request.cpp",
        "src/Response.cpp",
        "src/Server.cpp",
        "src/Socket.cpp"
      ],
      "conditions": [
        ["OS==\"linux\" or OS==\"freebsd\" or OS==\"openbsd\" or OS==\"solaris\" or OS==\"aix\"", {
          "cflags_cc": [
            "-std=c++11",
            "-O3",
            "-I ../src",
            "-fPIC",
            "-Wno-format-contains-nul",
            "-Wno-deprecated-declarations"
          ]
        }]
      ]
    }, {
      "target_name": "action_after_build",
      "type": "none",
      "dependencies": [
        "<(module_name)"
      ],
      "copies": [
        {
          "files": [
            "<(PRODUCT_DIR)/<(module_name).node"
          ],
          "destination": "<(module_path)"
        }
      ]
    }
  ]
}