# CustomWriter Basic XML Writer Design

## Purpose

CustomWriter is a small C++17 project that writes XML documents through
`std::ofstream`. It provides a reusable `XmlWriter` library and a command-line
example that writes a sample document to a caller-selected path.

## Scope

The basic writer supports:

- UTF-8 XML declaration output.
- Nested elements.
- Attributes.
- Text content.
- Empty elements.
- XML escaping.
- Configurable indentation with a two-space default.
- File-open, write, nesting, and document-structure error reporting.

The writer does not support comments, CDATA, raw XML fragments, mixed text and
child-element content, schema validation, XML parsing, or changes to XML
encoding. Invalid Unicode and control characters are the caller's
responsibility.

## Project Structure

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

`xml_writer.cpp` is compiled into a reusable `custom_writer` library.
`main.cpp` links that library and acts as a working example. The test
executable also links the library and is registered with CTest.

## Public API

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

Attribute names and values are borrowed only for the duration of an element
call. The writer copies the serialized bytes immediately.

## Writer Behavior

### Document rules

- `writeDeclaration()` may be called once, before any element.
- A document may omit the declaration.
- Exactly one root element must be written before `close()`.
- `close()` fails if elements remain open or if no root element was written.
- After the first recorded failure, subsequent output-producing calls return
  `false` without writing more bytes. `close()` still closes the stream but
  returns `false` because the writer is in a failed state.
- Destructor closes the file without throwing. Callers that need to observe
  document-structure errors must call `close()` explicitly.

### Element rules

- Element and attribute names must be non-empty.
- ASCII name syntax is validated. UTF-8 bytes above ASCII are accepted without
  Unicode normalization or Unicode name validation.
- Duplicate attribute names on one element are rejected.
- `startElement()` records nesting and emits the opening tag.
- `writeText()` requires an open element, writes inline text, and may be called
  multiple times.
- Child elements and inline text cannot be mixed inside one element.
- `endElement()` closes the innermost open element.
- `emptyElement()` writes a self-closing element and may be used as the root.

### Escaping

Text content escapes `&`, `<`, and `>`.

Attribute values escape `&`, `<`, `>`, `"`, and `'`. Attribute tab, newline,
and carriage-return characters are written as `&#9;`, `&#10;`, and `&#13;`.
All other bytes are written unchanged.

### Formatting

The declaration, when present, is:

```xml
<?xml version="1.0" encoding="UTF-8"?>
```

The writer emits:

- One element per line, except text-only elements.
- Inline text between its opening and closing tags.
- Two spaces per nesting level by default.
- A final newline.
- No byte-order mark.
- LF line endings in a binary output stream.

Example:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<catalog>
  <book id="42">
    <title>Example &amp; More</title>
  </book>
  <notice/>
</catalog>
```

## Error Reporting

The constructor attempts to truncate and open the output file. If opening
fails, the writer remains closed and records a descriptive error.

Each mutating operation returns whether it succeeded. The first failure stores
an error message retrievable through `lastError()`. I/O failures, invalid
names, duplicate attributes, attempts to mix text and child elements, missing
or duplicate roots, a declaration attempted after content, and close-time
structure errors are all reported this way.

A failed write may leave a partial output file. The writer does not delete or
rewrite that file.

## Command-Line Example

Usage:

```text
CustomWriter <output.xml>
```

The program writes a small catalog document, prints a success path, and returns
exit code 0. Wrong arguments print usage and return 2. Writer/open failures
print `lastError()` to standard error and return 1.

The command-line example always creates a new demonstration document; the
reusable library is the general-purpose API for other callers.

## Testing

One dependency-free CTest executable verifies:

- Declaration, indentation, attributes, nested elements, inline text, and
  empty elements produce the expected bytes.
- Text and attribute escaping produces valid XML entities.
- Invalid names and duplicate attributes are rejected.
- Text and child-element mixing is rejected.
- Missing or duplicate roots and missing end tags produce close failures.
- File-open failure is reported.
- Successful output can be read back through `std::ifstream`.

Tests use temporary files below the system temporary directory and remove
them after each case.

## Build and Verification

The project uses CMake and C++17 with no third-party dependencies.

Verification consists of:

```text
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The example executable is then run with a temporary XML path, and its output is
read back to confirm that the expected document was produced.
