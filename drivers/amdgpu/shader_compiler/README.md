# Shader Compiler

Compiles shaders from GLSL or SPIR-V to RDNA GPU ISA (Instruction Set Architecture).

## Files

- `shader_compiler.c` - Core compiler implementation
- `shader_compiler.h` - Public API

## API

```c
// Initialize compiler
int shader_compiler_init(void);

// Compile shader
int shader_compile(const void *source, size_t source_size,
                   const shader_compile_options_t *options,
                   shader_compile_result_t *result);

// Shutdown
void shader_compiler_fini(void);
```

## Pipeline

```
GLSL Source
    ↓
shader_compile_glsl_to_spirv()  [GLSL → SPIR-V]
    ↓
SPIR-V Binary
    ↓
shader_validate_spirv()         [Validation]
    ↓
spirv_parse_module()            [Instruction parsing]
    ↓
spirv_to_rdna()                 [SPIR-V → RDNA ISA]
    ↓
RDNA ISA Code (machine code)
```

## Supported Shader Types

- Vertex
- Fragment
- Geometry
- Compute
- Tessellation

## Status

✅ SPIR-V validation and parsing working  
✅ RDNA ISA generation framework ready  
⚠️ GLSL compilation is stub (needs glslang linkage)  

## Next Steps

- Link real glslang library for GLSL compilation
- Implement complete SPIR-V to ISA translation
- Add shader caching
- Performance optimization
