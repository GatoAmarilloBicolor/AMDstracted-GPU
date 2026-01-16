/*
 * Shader Compiler Abstraction - Agnóstic SPIR-V to ISA
 * 
 * Provides agnóstic shader compilation interface
 * Supports SPIR-V → RDNA ISA compilation
 * Compatible across Linux, Haiku, FreeBSD
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#ifndef SHADER_COMPILER_H
#define SHADER_COMPILER_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * SHADER TYPES
 * ============================================================================ */

typedef enum {
    SHADER_TYPE_VERTEX = 0,
    SHADER_TYPE_FRAGMENT = 1,
    SHADER_TYPE_GEOMETRY = 2,
    SHADER_TYPE_COMPUTE = 3,
    SHADER_TYPE_TESSELLATION = 4,
} shader_type_t;

typedef enum {
    SHADER_FORMAT_SPIRV = 0,
    SHADER_FORMAT_GLSL = 1,
    SHADER_FORMAT_HLSL = 2,
} shader_input_format_t;

typedef enum {
    ISA_FORMAT_RDNA = 0,      // RDNA/GCN ISA
    ISA_FORMAT_BINARY = 1,    // Binary ISA
} isa_output_format_t;

/* ============================================================================
 * COMPILATION RESULT
 * ============================================================================ */

typedef struct {
    uint32_t success;
    uint32_t code_size;
    uint8_t *code;              // Compiled ISA/binary
    uint32_t register_count;
    uint32_t scratch_memory;
    char error_message[512];
} shader_compile_result_t;

/* ============================================================================
 * COMPILER STATE
 * ============================================================================ */

typedef struct {
    shader_type_t type;
    shader_input_format_t input_format;
    isa_output_format_t output_format;
    uint32_t optimization_level;
    uint32_t target_wave_size;
} shader_compile_options_t;

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

/**
 * Initialize shader compiler
 * 
 * @return 0 on success, -1 on error
 */
int shader_compiler_init(void);

/**
 * Compile shader from SPIR-V or GLSL to ISA
 * 
 * @param source          Shader source or SPIR-V binary
 * @param source_size     Size of source
 * @param options         Compilation options
 * @param result          Output compilation result
 * @return 0 on success, -1 on error
 */
int shader_compile(const void *source, size_t source_size,
                  const shader_compile_options_t *options,
                  shader_compile_result_t *result);

/**
 * Compile SPIR-V to RDNA ISA
 * 
 * @param spirv           SPIR-V binary
 * @param spirv_size      Size in bytes
 * @param shader_type     Shader type
 * @param result          Output ISA
 * @return 0 on success, -1 on error
 */
int shader_compile_spirv_to_isa(const uint32_t *spirv, size_t spirv_size,
                               shader_type_t shader_type,
                               shader_compile_result_t *result);

/**
 * Compile GLSL to SPIR-V
 * 
 * @param glsl            GLSL source code
 * @param glsl_size       Size in bytes
 * @param shader_type     Shader type
 * @param spirv_out       Output SPIR-V binary
 * @param spirv_size_out  Output SPIR-V size
 * @return 0 on success, -1 on error
 */
int shader_compile_glsl_to_spirv(const char *glsl, size_t glsl_size,
                                shader_type_t shader_type,
                                uint32_t **spirv_out, size_t *spirv_size_out);

/**
 * Validate SPIR-V module
 * 
 * @param spirv           SPIR-V binary
 * @param spirv_size      Size in bytes
 * @return 0 if valid, -1 if invalid
 */
int shader_validate_spirv(const uint32_t *spirv, size_t spirv_size);

/**
 * Get compiler capabilities
 * 
 * @param caps            Output capabilities string
 * @param caps_size       Size of caps buffer
 * @return 0 on success
 */
int shader_get_capabilities(char *caps, size_t caps_size);

/**
 * Free compiled shader result
 * 
 * @param result          Result to free
 */
void shader_free_result(shader_compile_result_t *result);

/**
 * Shutdown shader compiler
 */
void shader_compiler_fini(void);

#endif // SHADER_COMPILER_H
