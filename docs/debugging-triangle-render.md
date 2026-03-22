# bgfx Android 三角形渲染问题修复记录

## 问题描述

应用启动后屏幕全黑，无法渲染出预期的彩色三角形。

## 修复过程

### 1. Shader 数据问题

**问题**：bgfx 的 shader 不是标准的 GLSL 源码，而是特殊的二进制格式。直接传入 GLSL 源码导致 `ShaderGL::create` 崩溃。

**原因**：`bgfx::createShader()` 需要 bgfx 特定的 shader 字节码格式（包含 magic header、hash 等元数据），不接受原始 GLSL 字符串。

**解决**：
- 从 bgfx 官方 examples 的预编译 shader (`examples/runtime/shaders/essl/`) 复制 `vs_mesh.bin` 和 `fs_mesh.bin`
- 使用 `xxd -i` 或 Python 将二进制文件转换为 C 字节数组
- 在代码中通过 `bgfx::copy()` 传递数据

```cpp
// 错误做法：传入 GLSL 源码字符串
const char* glsl = "attribute vec3 a_position; ...";
const bgfx::Memory* mem = bgfx::makeRef(glsl, strlen(glsl)); // 会崩溃

// 正确做法：使用预编译的 bgfx shader 字节码
const bgfx::Memory* vsMem = bgfx::copy(s_vsMeshData, sizeof(s_vsMeshData));
bgfx::ShaderHandle vsShader = bgfx::createShader(vsMem);
```

### 2. makeRef vs copy 问题

**问题**：即使使用正确的 shader 数据，`makeRef()` 仍然失败。

**原因**：`makeRef()` 只是引用原始数据，bgfx 内部可能持有指针引用。而 shader 数据是 ROM 数据（嵌入式字节数组），在某些情况下引用可能无效。

**解决**：使用 `bgfx::copy()` 替代 `makeRef()`，强制复制数据到 bgfx 管理的内存：

```cpp
// makeRef：仅引用（失败）
const bgfx::Memory* vsMem = bgfx::makeRef(s_vsMeshData, sizeof(s_vsMeshData));

// copy：复制数据（成功）
const bgfx::Memory* vsMem = bgfx::copy(s_vsMeshData, sizeof(s_vsMeshData));
```

### 3. 渲染后端自动选择

**问题**：强制指定 `RendererType::OpenGLES` 导致 shader 格式可能不匹配。

**解决**：让 bgfx 自动检测渲染器类型：

```cpp
// 强制指定（可能导致问题）
init.type = bgfx::RendererType::OpenGLES;

// 自动检测（推荐）
init.type = bgfx::RendererType::Count;
```

## 最终代码结构

```
app/src/main/cpp/
├── CMakeLists.txt              # CMake 构建配置
├── bgfx_renderer.h             # BgfxRenderer 头文件
├── bgfx_renderer.cpp           # BgfxRenderer 实现（核心渲染逻辑）
├── jni_binder.cpp              # JNI 绑定层
├── native-lib.cpp               # 入口点
└── shaders/essl/
    ├── vs_mesh.bin              # 预编译顶点着色器
    ├── fs_mesh.bin              # 预编译片元着色器
    └── shaders_essl.h           # 嵌入式 shader 数据（由 bin 文件转换）
```

## 关键教训

1. **bgfx shader 不是 GLSL**：`bgfx::createShader()` 不接受 GLSL 源码，必须使用 bgfx 特有的二进制格式

2. **预编译 shader 获取方式**：
   - bgfx 官方 examples 已包含预编译的 shader（`examples/runtime/shaders/essl/`）
   - 或使用 shaderc 工具自行编译（需要完整构建环境）

3. **数据生命周期**：`makeRef()` vs `copy()` 的选择取决于数据来源：
   - `makeRef()`：适用于临时创建的数据
   - `copy()`：适用于 ROM/嵌入式数据，确保数据被正确复制

4. **渲染后端**：`RendererType::Count` 让 bgfx 自动选择，通常比强制指定更可靠

## 相关文件

- `app/src/main/cpp/bgfx_renderer.cpp` - 渲染器实现
- `app/src/main/cpp/shaders/essl/shaders_essl.h` - 嵌入式 shader 数据
- `app/src/main/cpp/CMakeLists.txt` - CMake 构建配置

## 参考

- [bgfx Shader Manual](https://bkaradzic.github.io/bgfx/shader.html)
- [bgfx Examples](https://github.com/bkaradzic/bgfx/tree/master/examples)
