import path from "path";
import * as webpack from "webpack";
import HtmlWebpackPlugin from "html-webpack-plugin";
import CopyWebpackPlugin from "copy-webpack-plugin";
import packageJson from "./package.json";

export default (
    _env: any,
    argv: webpack.WebpackOptionsNormalized
): webpack.Configuration => {
    var wasmBuildPath = "out/build/wasm-release";
    if (argv.mode === "development") {
        wasmBuildPath = "out/build/wasm-debug";
    }
    const config: webpack.Configuration = {
        entry: "./src/index.tsx",
        module: {
            rules: [
                {
                    test: /\.ts(x)?$/,
                    use: "ts-loader",
                    exclude: /node_modules/,
                },
                {
                    test: /\.css$/,
                    use: ["style-loader", "css-loader"],
                },
                {
                    test: /\.png$/,
                    type: "asset/resource",
                },
            ],
        },
        output: {
            clean: true,
            path: path.resolve(__dirname, "out/html"),
            filename: "bundle.js",
        },
        resolve: {
            extensions: [".tsx", ".ts", ".js"],
        },
        performance: {
            hints: false,
        },
        plugins: [
            new HtmlWebpackPlugin({
                template: "./src/index.html",
                filename: "index.html",
                inject: "body",
                minify: {
                    collapseWhitespace: true,
                    removeComments: true,
                },
                hash: true,
            }),
            new CopyWebpackPlugin({
                patterns: [
                    { from: "oniWorldApp.js", context: wasmBuildPath + "/src"},
                    { from: "wasm.bin", context: wasmBuildPath + "/src"},
                    { from: "data.bin", context: wasmBuildPath + "/asset"},
                    { from: "*", context: "public" },
                ],
            }),
            new webpack.DefinePlugin({
                "process.env.VERSION": JSON.stringify(packageJson.version),
            }),
        ],
    };
    return config;
};
