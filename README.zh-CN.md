# oniWorldApp

[English](./README.md) | 简体中文

## 介绍
生成[缺氧](https://www.klei.com/games/oxygen-not-included)游戏的世界, 在线使用[网页版](https://genrwoody.neocities.org/oni-map/index.html).

## 构建
需要的工具: emsdk, cmake, python, node.js, yarn, emsdk中带有python和node.js
```sh
cmake --preset=wasm-release
cmake --build --preset=wasm-release
yarn install
yarn run build
```
或者debug模式
```sh
cmake --preset=wasm-debug
cmake --build --preset=wasm-debug
yarn install
yarn run start
```
或者pc版
```sh
cmake --preset=x86-release
cmake --build --preset=x86-release
# run
./out/build/x86-release/src/oniWorldApp
```

## license
本项目遵循[MIT License](./LICENSE), 第三方库及它们的[License](./3rdparty/README.md), 以及[科雷](https://www.klei.com), [缺氧](https://www.klei.com/games/oxygen-not-included)

