# CustomWriter

一个使用 C++17 文件流写出 XML 的基础工具，同时提供可复用库和命令行示例。

## 功能

- XML 声明、嵌套元素、属性、文本和空元素。
- 文本及属性自动转义。
- 默认两空格缩进，也可在构造 `XmlWriter` 时指定缩进宽度。
- 元素名称、属性名称、重复属性、根元素、嵌套状态和文件 I/O 检查。
- UTF-8 字节和 LF 换行输出，不写 BOM。
- 不使用第三方库。

## 构建

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

当前机器的 PowerShell 默认 PATH 中没有 C++ 编译器。可以使用 Qt 自带的
MinGW 工具链：

```powershell
$env:PATH = "E:\Qt6.8.3\Tools\mingw1310_64\bin;" + $env:PATH
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
ctest --test-dir build --output-on-failure
```

## 运行

```powershell
$env:PATH = "E:\Qt6.8.3\Tools\mingw1310_64\bin;" + $env:PATH
.\build\CustomWriter.exe output.xml
```

命令行程序会生成一个包含书籍信息和空元素的小型目录文档。输出路径可以包含空格。
如果使用其他 MinGW 安装，运行前同样需要保证对应的运行库位于 `PATH` 中。

## 在 VS Code 中调试和跳转

项目已包含 `.vscode/launch.json`、`.vscode/tasks.json` 和 clangd 配置：

- 按 `F5` 使用 `Debug CustomWriter (MinGW GDB)` 配置，自动完成 Debug 构建并启动 GDB。
- 按 `F12` 跳到符号的声明或定义；`Ctrl+F12` 可跳到具体实现。
- `include/custom_writer/xml_writer.hpp` 是头文件，用来声明 `XmlWriter`；具体实现位于 `src/xml_writer.cpp`。
- clangd 通过 `.clangd` 和 `build/compile_commands.json` 识别头文件路径及 C++ 标准库。

首次拉取配置后需要执行一次 `Developer: Reload Window`，让 clangd 和 CMake Tools 重新加载。

## 作为库使用

```cpp
#include "custom_writer/xml_writer.hpp"

custom_writer::XmlWriter writer("output.xml");
writer.writeDeclaration();
writer.startElement("root");
writer.startElement("message");
writer.writeText("Hello & XML");
writer.endElement();
writer.endElement();
writer.close();
```

每个写出操作都返回 `bool`。第一次失败后，后续写出操作返回 `false` 并且
不再写入更多字节；失败原因可通过 `lastError()` 获取。需要检查文档是否完整
写出时，应显式调用并检查 `close()` 的返回值。

## 当前范围

基础版不支持注释、CDATA、原始 XML 片段、同一元素内混合文本和子元素、
Schema 校验、XML 解析或修改 XML 编码。
