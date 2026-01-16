# Test Suite

Comprehensive testing of all driver components.

## Running Tests

```bash
# Run all tests
./test_components

# Run specific test suite
./test_components [suite_name]
```

## Test Coverage

### Component Tests (70 total)

**Shader Compiler Tests** (12)
- SPIR-V validation
- Module parsing
- ISA generation
- Shader type support

**RADV Backend Tests** (14)
- GEM allocator
- Limits checking
- Ring buffer
- Device management

**Zink Layer Tests** (15)
- Context management
- Draw state tracking
- Command queueing
- Resource creation

**DRM Shim Tests** (13)
- Device contexts
- Version query
- IPC routing
- FD management

**Integration Tests** (16)
- End-to-end flow
- Memory management
- Shader pipeline
- Full system integration

## Test Organization

```
tests/
├── unit/               # Component unit tests
│   ├── test_shader_compiler.c
│   ├── test_radv_backend.c
│   ├── test_zink_layer.c
│   └── test_drm_shim.c
├── integration/        # End-to-end tests
│   ├── test_ipc_flow.c
│   ├── test_memory_management.c
│   └── test_shader_pipeline.c
└── test_components.c   # Consolidated suite
```

## Results

✅ **70/70 tests PASSING**

| Category | Tests | Status |
|----------|-------|--------|
| Shader Compiler | 12 | ✓ PASS |
| RADV Backend | 14 | ✓ PASS |
| Zink Layer | 15 | ✓ PASS |
| DRM Shim | 13 | ✓ PASS |
| Integration | 16 | ✓ PASS |
| **Total** | **70** | **✓ PASS** |

## Coverage

Current: ~55%

## Adding New Tests

1. Create test function in appropriate category
2. Add assertions using TEST() macro
3. Update test suite list
4. Run full suite to verify

## Performance Benchmarks

Current (simulation mode):
- Memory allocation: <1ms
- Shader compilation: <10ms
- Command submission: <1ms
- Context creation: <5ms

## Next Steps

- Add stress tests (high load)
- Add memory leak detection
- Performance regression testing
- Hardware-specific tests
