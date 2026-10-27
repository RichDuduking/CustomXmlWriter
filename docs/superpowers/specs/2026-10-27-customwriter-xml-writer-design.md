# CustomWriter 基础 XML 写出器设计

## 目标

CustomWriter 是一个使用 C++17 编写的小型项目，通过 `std::ofstream`
写出 XML 文档。项目同时提供可复用的 `XmlWriter` 库，以及一个将示例
文档写到调用方指定路径的命令行程序。

## 范围

基础版写出器支持：

- 写出 UTF-8 XML 声明。
- 嵌套元素。
- 属性。
- 文本内容。
- 空元素。
- XML 特殊字符转义。
- 可配置缩进，默认使用两个空格。
- 文件打开、写入、嵌套及文档结构错误报告。

基础版不支持注释、CDATA、原始 XML 片段、同一元素内混合文本和子元素、
Schema 校验、XML 解析或修改 XML 编码。无效 Unicode 字符和控制字符由
调用方负责处理。

## 项目结构

```text
CustomWriter/
  CMakeLists.txt
  README.md
  include/custom_writer/xml_writer.hpp
  src/xml_writer.cpp
  src/main.cpp
  tests/xml_writer_test.cpp
  docs/superpowers/specs/
```

`xml_writer.cpp` 会被编译为可复用的 `custom_writer` 库。`main.cpp`
链接该库并提供可直接运行的示例。测试程序同样链接该库，并注册到
CTest。

## 公共接口

```cpp
namespace custom_writer {

struct Attribute {
  std::string_view name;
  std::string_view value;
};

class XmlWriter {
 public:
  explicit XmlWriter(const std::filesystem::path& output_path,
                     std::size_t indent_width = 2);
  ~XmlWriter();

  XmlWriter(const XmlWriter&) = delete;
  XmlWriter& operator=(const XmlWriter&) = delete;
  XmlWriter(XmlWriter&&) = delete;
  XmlWriter& operator=(XmlWriter&&) = delete;

  bool writeDeclaration();
  bool startElement(
      std::string_view name,
      const std::vector<Attribute>& attributes = {});
  bool writeText(std::string_view text);
  bool endElement();
  bool emptyElement(
      std::string_view name,
      const std::vector<Attribute>& attributes = {});
  bool close();

  bool isOpen() const noexcept;
  const std::string& lastError() const noexcept;
};

}  // namespace custom_writer
```

属性名和属性值只在元素调用期间被借用。写出器会在调用返回前立即复制
序列化后的字节。

## 写出器行为

### 文档规则

- `writeDeclaration()` 只能调用一次，并且必须发生在任何元素之前。
- 文档可以不包含 XML 声明。
- 调用 `close()` 前必须写出且只能写出一个根元素。
- 如果仍有元素未闭合，或者没有写出根元素，`close()` 会失败。
- 第一次记录失败后，后续输出调用返回 `false`，且不再写入更多字节。
  `close()` 仍会关闭文件流，但由于写出器处于失败状态，其返回值为
  `false`。
- 析构函数会关闭文件且不抛出异常。调用方若需要获知文档结构错误，必须
  显式调用 `close()`。

### 元素规则

- 元素名和属性名不能为空。
- 写出器会校验 ASCII 名称语法。大于 ASCII 范围的 UTF-8 字节会被接受，
  但不执行 Unicode 规范化或 Unicode 名称校验。
- 同一个元素中不允许出现重复属性名。
- `startElement()` 记录嵌套关系并写出开始标签。
- `writeText()` 要求当前存在未闭合元素，直接写出内联文本，并且可以多次
  调用。
- 同一个元素内不能同时包含内联文本和子元素。
- `endElement()` 关闭最内层尚未闭合的元素。
- `emptyElement()` 写出自闭合元素，也可以作为根元素使用。

### 转义

文本内容会转义 `&`、`<` 和 `>`。

属性值会转义 `&`、`<`、`>`、`"` 和 `'`。属性值中的制表符、换行符和
回车符会分别写为 `&#9;`、`&#10;` 和 `&#13;`。其他字节保持原样。

### 格式

写出 XML 声明时使用：

```xml
<?xml version="1.0" encoding="UTF-8"?>
```

写出器遵循以下格式：

- 除仅包含文本的元素外，每个元素单独占一行。
- 文本位于开始标签和结束标签之间，并保持内联。
- 默认每个嵌套层级缩进两个空格。
- 文件末尾包含一个换行符。
- 不写出字节顺序标记。
- 使用二进制输出流，并统一写入 LF 换行。

示例：

```xml
<?xml version="1.0" encoding="UTF-8"?>
<catalog>
  <book id="42">
    <title>Example &amp; More</title>
  </book>
  <notice/>
</catalog>
```

## 错误报告

构造函数会尝试截断并打开输出文件。如果打开失败，写出器保持关闭状态，
并记录一条描述性错误。

每个会修改输出的操作都会返回是否成功。第一次失败时，写出器会保存错误
信息，调用方可以通过 `lastError()` 获取。I/O 失败、非法名称、重复属性、
尝试混合文本和子元素、缺少根元素、重复根元素、在内容之后请求 XML
声明，以及关闭时的结构错误，都通过这一机制报告。

写入失败后可能留下部分输出文件。写出器不会删除或重写该文件。

## 命令行示例

用法：

```text
CustomWriter <output.xml>
```

程序会写出一个小型目录文档，打印成功后的路径，并返回退出码 0。参数
数量不正确时打印用法并返回 2。写出器或文件打开失败时，将
`lastError()` 的内容打印到标准错误并返回 1。

命令行程序始终创建一个新的演示文档；需要通用写能力的调用方应使用
可复用的库接口。

## 测试

一个无第三方依赖的 CTest 测试程序会校验：

- XML 声明、缩进、属性、嵌套元素、内联文本和空元素能生成预期字节。
- 文本和属性转义能生成有效的 XML 实体。
- 非法名称和重复属性会被拒绝。
- 同一元素内混合文本和子元素会被拒绝。
- 缺少根元素和缺少结束标签会在关闭时失败，重复根元素会在写出第二个
  根元素时被拒绝。
- 文件打开失败会被报告。
- 成功写出后，可以通过 `std::ifstream` 回读内容。

测试使用系统临时目录中的临时文件，并在每个测试用例结束后删除这些
文件。

## 构建与验证

项目使用 CMake 和 C++17，不依赖任何第三方库。

验证命令如下：

```text
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

随后使用一个临时 XML 路径运行示例程序，并回读输出，确认生成了预期
文档。
