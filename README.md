# oniWorldApp

English | [简体中文](./README.zh-CN.md)

## Introduction
Generate world for the game [Oxygen Not Included](https://www.klei.com/games/oxygen-not-included).  
You can try it on the [website](https://genrwoody.neocities.org/oni-map/index.html).

## Build
Prerequisites: emsdk, cmake, python, node.js, yarn, the emsdk include python and node.js.
```sh
cmake --preset=wasm-release
cmake --build --preset=wasm-release
yarn install
yarn run build
```
or debug
```sh
cmake --preset=wasm-debug
cmake --build  --preset=wasm-debug
yarn install
yarn run start
```
or x86 target
```sh
cmake --preset=x86-release
cmake --build  --preset=x86-release
# run
./out/build/x86-release/src/oniWorldApp
```

## License
This project is licensed under the [MIT License](./LICENSE).  
Third party libraries and their [licenses](./3rdparty/README.md).  
And [Klei](https://www.klei.com), [Oxygen Not Included](https://www.klei.com/games/oxygen-not-included)

