# Project1：多项式与表达式求值计算器

使用 C++17 实现计算模块，使用 Qt 6 Widgets 实现桌面 GUI，同时提供命令行入口、静态库和自动化测试。界面直接调用公开接口，计算模块不读取终端输入、不打印结果，也没有全局计算状态。

## 启动桌面 GUI

仓库提供源代码。克隆后可通过 Qt Creator 或下方命令编译运行，需 C++17 编译器、CMake 和 Qt 6 开发库。Windows、Linux 和 macOS 使用各自平台的 Qt 开发环境编译。

在 macOS 上执行打包脚本后，生成的应用位于 `dist/Project1.app`，可在 Finder 中双击打开，并包含所需 Qt 运行库。`build/`、`build-gui/` 和 `dist/` 由 `.gitignore` 排除，不随源代码提交。

### 用 Qt Creator 编译

1. 在 Qt Creator 中打开项目根目录的 `CMakeLists.txt`。
2. 选择桌面 Qt Kit，例如本机已安装的 **Qt 6.12.0 macOS**，并配置项目。
3. 在运行配置中选择 `project1_gui`（程序名 `Project1`），编译后点击运行。

`calculator_cli` 是终端入口，`calculator_tests` 和 `calculator_gui_tests` 是测试程序。Qt Creator 中 GUI 的生成位置取决于所选构建目录。

### 在本机终端编译

在项目目录运行：

```sh
./scripts/run-gui.sh
```

脚本使用本机 `~/Qt/6.12.0/macos` 的开发库和 Qt 安装时附带的 CMake，构建到 `build-gui/` 并打开应用。其他安装路径可以指定 `QT_ROOT`：

```sh
QT_ROOT='/你的Qt目录/版本/macos' ./scripts/run-gui.sh
```

标准 CMake 方式（适用于已将 cmake 加入 PATH 的环境）：

```sh
cmake -S . -B build-gui -DCMAKE_PREFIX_PATH='/Qt安装目录/版本/平台' -DCMAKE_BUILD_TYPE=Release
cmake --build build-gui --parallel 2
ctest --test-dir build-gui --output-on-failure
```

编译后，macOS 打开 `build-gui/Project1.app`，Windows 运行对应构建目录中的 `Project1.exe`，Linux 运行 `build-gui/Project1`。纯后端构建可以加入 `-DPROJECT1_BUILD_GUI=OFF`，无需 Qt。

重新生成包含 Qt 运行库的 macOS 应用：

```sh
./scripts/package-macos.sh
```

该脚本将最新构建复制到 `dist/Project1.app`，使用 Qt 自带的 `macdeployqt` 打包。CMake 与部署方式可参考 [Qt 的 CMake 文档](https://doc.qt.io/qt-6/cmake-get-started.html) 和 [macOS 部署文档](https://doc.qt.io/qt-6/macos-deployment.html)。

### GUI 使用方式

- **表达式计算**：键盘输入或点击计算器按键，按 Enter 或点击「计算」；支持输入 `×`、`÷`、`−`，界面会转换成后端运算符。
- **演算过程**：默认记录完整过程；选择表格中的一行，或点击上一步/下一步，查看该步完成后的两栈状态。鼠标悬停在操作文字上可查看完整内容与栈快照。
- **多项式计算**：输入规定的整数序列，实时预览代数式；下拉选择加、减、乘、求导或求值。求值时输入实数 `x`。
- **结果操作**：复制结果，查看结果整数序列和系数/指数明细，或将多项式结果作为 A 继续计算。参考示例可以一键载入并计算。
- **错误提示**：直接显示在输入附近，输入错误会高亮对应字符；表达式计算中断时保留此前过程。修改输入会清除旧结果，避免误认为旧结果对应新输入。
- GUI 输入框最多接受 4096 个字符，演算表格按需滚动；较大的整数结果会自动缩小字号以完整显示。

## 已实现功能

- 一元稀疏多项式：序列输入、代数式/序列输出、加、减、乘、求导、给定实数 `x` 求值。
- 表达式：多位非负整数、`+ - * /`、括号，以及扩展的乘方 `^`、一元正负号。
- 双栈过程记录：每一步提供运算符栈、运算数栈、当前输入符号/整数、输入位置和操作说明。
- 异常处理：非法输入、括号不匹配、除零、负乘方指数、整数/指数溢出和非有限浮点数。
- Qt 桌面界面：功能切换、计算器按键、输入预览、逐步查看双栈、结果复制、继续计算及参考示例。

多项式使用**自定义单链表**，两个栈使用**自定义动态数组栈**。没有使用 `std::vector`、`std::list`、`std::stack`、`std::map` 等 STL 容器，也没有使用 STL 迭代器或算法。`std::string` 仅用于输入输出文本，`std::unique_ptr<T[]>` 仅用于管理自定义栈数组的内存。

## 文件结构

```text
include/calculator/
  polynomial.hpp       多项式公开接口
  expression.hpp       表达式求值与过程回调接口
  stack.hpp            自定义顺序栈
  error.hpp            统一异常类型、错误码、输入位置
  checked_integer.hpp  64 位整数安全运算
src/
  polynomial.cpp       有序链表、多项式运算、稀疏 Horner 求值
  expression.cpp       算符优先关系表、词法识别、双栈归约
  main.cpp             命令行验证入口
tests/
  test_calculator.cpp  样例、边界、随机对照和过程回调测试
  test_gui.cpp         真实控件交互、界面状态和结果验证
gui/
  calculator_window.*  两个计算页面及界面/后端调用
  fitted_result_label.hpp  整数结果字号自适应
  style.*              统一界面主题
  main.cpp             Qt 应用入口
scripts/
  run-gui.sh           本机 GUI 构建与启动
  package-macos.sh     打包 macOS 应用及 Qt 运行库
CMakeLists.txt
Makefile
```

## 编译与验证

在 `Project1` 目录执行（macOS / Linux，需 C++17 编译器和 make）：

```sh
make
make test
make sanitize
```

`make` 生成 `build/calculator` 和 `build/libcalculator.a`。`make sanitize` 使用 AddressSanitizer 和 UndefinedBehaviorSanitizer 检查内存访问及未定义行为；需编译器支持。指定编译器可以使用 `make CXX=clang++` 或 `make CXX=g++`。

也可以直接编译，无需 make：

```sh
c++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/polynomial.cpp src/expression.cpp src/main.cpp -o calculator
```

## 命令行示例

参数中的表达式和多项式序列需要加引号。

```sh
./build/calculator expr '((2+3)*4+1)/3' --trace
# 逐步显示两栈及操作，最后结果为 7

./build/calculator expr '2^3^2'
# 结果：512

./build/calculator expr '3*(-2)'
# 结果：-6

./build/calculator poly show '2 2 3 5 1'
# 结果：2x^3 + 5x
# 序列：2 2 3 5 1

./build/calculator poly add '2 2 3 5 1' '2 3 3 4 0'
# 结果：5x^3 + 5x + 4

./build/calculator poly sub '1 1 2' '2 1 2 1 1'
# 结果：-x

./build/calculator poly mul '2 1 1 1 0' '2 1 1 -1 0'
# 结果：x^2 - 1

./build/calculator poly diff '3 3 3 2 2 7 0'
# 结果：9x^2 + 4x

./build/calculator poly eval '1 2 2' 2
# 结果：8
```

`./build/calculator --help` 查看完整命令。验证入口成功返回 `0`，输入或计算错误返回 `2`，其他运行异常返回 `3`。

## 给界面调用的接口

### 多项式

```cpp
#include "calculator/polynomial.hpp"

auto a = calculator::Polynomial::fromSequence("2 2 3 5 1");
auto b = calculator::Polynomial::fromSequence("2 3 3 4 0");
auto sum = a + b;
auto difference = a - b;
auto product = a * b;
auto derivative = a.derivative();
long double value = a.evaluate(2.0L);

auto display = sum.toString();    // "5x^3 + 5x + 4"
auto sequence = sum.toSequence(); // "3 5 3 5 1 4 0"
auto singleTerm = calculator::Polynomial::monomial(2, 3);
auto coefficient = sum.coefficientAt(3); // 5
```

所有运算返回新多项式，保持输入多项式不变。支持深拷贝和移动，节点由对象自动释放。默认构造得到零多项式。

### 表达式与过程记录

```cpp
#include "calculator/expression.hpp"

void onStep(const calculator::TraceStep& step, void* context) {
    // 在这里更新界面的过程记录。
    // step.operatorStack / operandStack：从栈底到栈顶的文本快照。
    // step.inputToken：当前读入的完整整数或符号。
    // step.action：该步操作说明。
    // step.position：输入中从 0 开始的字节位置。
    // context：调用方传入的指针，例如界面对象。
    (void)step;
    (void)context;
}

auto result = calculator::evaluateExpression("10-2*3", onStep, nullptr);
auto resultWithoutTrace = calculator::evaluateExpression("10-2*3");
```

回调在求值过程中同步执行；快照表示**操作完成后的状态**。`u+`、`u-` 表示一元正负号。执行归约时 `inputToken` 是触发归约的当前输入，具体计算见 `action`。自动补入的 `#` 位于 `input.size()`。需要保存记录时，应在回调内复制 `step` 的内容；引用只在当前回调期间有效。未提供回调时不会生成快照文本。

### 错误处理

```cpp
#include "calculator/error.hpp"
#include "calculator/expression.hpp"

try {
    auto result = calculator::evaluateExpression("10/(3-3)");
    (void)result;
} catch (const calculator::Error& error) {
    // error.what()：中文错误说明
    // error.code()：可供界面分类处理的 ErrorCode
    // error.position()：错误对应的输入字节位置
    // Error::noPosition：没有对应的输入位置，例如多项式结果溢出
}
```

外部程序编译时加入 `-Iinclude`，链接 `build/libcalculator.a` 即可；或者将 `src/polynomial.cpp` 和 `src/expression.cpp` 加入已有工程。

## 输入及运算规则

### 多项式规则

- 输入格式为 `n c1 e1 ... cn en`，整数之间用空白分隔；系数可正、可负、可为零。
- 项数 `n` 为非负整数；指数在 `[0, INT_MAX]` 内，必须严格降序且不重复。项数与实际系数、指数对数量必须一致。
- 零系数项会被移除；空多项式和完全抵消后的多项式均输出 `0`。
- 系数采用 `std::int64_t`，范围为 `[-9223372036854775808, 9223372036854775807]`。加减乘和求导检测系数溢出；乘法检测指数相加溢出。
- `x` 和求值结果采用 `long double`，支持小数与负数；结果受平台浮点精度影响。拒绝 `NaN`、无穷大和计算过程中出现的非有限结果。

### 表达式规则

- 输入整数文字只能为非负整数，最大为 `9223372036854775807`；负数通过一元负号产生。
- 忽略空白，支持可选的末尾终止符 `#`。不支持小数、变量、隐式乘法或其他字符。
- 基础七种符号的比较使用作业原表；扩展符号遵循下列规则：

| 运算符 | 优先级（由低到高） | 结合方式 |
| --- | --- | --- |
| 二元 `+ -` | 1 | 左结合 |
| `* /` | 2 | 左结合 |
| 一元 `+ -` | 3 | 从右向左作用 |
| `^` | 4 | 右结合 |

- 括号改变运算顺序。`2^3^2 = 512`，`-2^2 = -4`，`(-2)^2 = 4`，`3*(-2) = -6`。
- 连续前缀符号合法，如 `--3 = 3`、`1--2 = 3`。
- 除法向零截断，如 `7/3 = 2`、`-7/3 = -2`；除零报错。
- 乘方指数必须为非负整数，`0^0` 定义为 `1`；负指数报错。
- 每一步整数运算必须在 64 位有符号范围内，即使后续运算可能抵消，中间结果溢出也会报错。

## 核心算法与验证范围

- 加减：同时遍历两条有序链表，时间 `O(m+n)`。
- 求导：遍历链表，时间 `O(m)`。
- 乘法：两两相乘并在结果链表中有序插入、合并，时间 `O(mnk)`，其中 `k` 为结果链表的最大项数，最坏为 `O((mn)^2)`。
- 求值：稀疏 Horner 法配合快速幂，不建立稠密指数数组。每个指数间隔的快速幂为对数时间。
- 表达式：逐字符识别，使用运算符优先关系控制入栈、括号匹配及归约；每个符号至多入栈、出栈一次，关闭过程记录时按输入长度线性扫描。开启过程记录时额外复制每一步的两栈快照。

测试覆盖 PDF 中全部参考数据、运算符优先级及结合性、深括号、长表达式、拷贝与移动、零项抵消、错误位置和数值边界。另使用 300 组随机多项式与固定数组实现的参考结果对照，以及 800 组随机表达式与独立计算结果对照。

GUI 测试使用 Qt Test 操作实际控件，覆盖页面切换、键盘/计算器按键、Unicode 运算符、演算步骤导航、过程记录开关、输入更新时旧结果清除、复制结果、错误字符定位、参考示例、结果复用和实数求值。CMake 中通过 offscreen 平台运行这些测试，无需手动点击窗口。
