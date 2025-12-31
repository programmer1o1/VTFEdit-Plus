# Precompiled Linux Libraries

## Compressonator (AMD)
- **Version**: 4.5 (GPUOpen-Tools fork)
- **Built on**: Fedora 43, GCC 15.2.1
- **Architecture**: x86_64
- **Notes**: AVX512 disabled due to compilation issues. SSE4 and AVX2 variants included.

### Files:
- `libCompressonator.a` - Main DXT compression library
- `libCMP_Core.a` - Core compression functions
- `libCMP_Core_SSE.a` - SSE4 optimized
- `libCMP_Core_AVX.a` - AVX2 optimized

### Build Date
December 30, 2025

### To rebuild (if needed):
See main README.md Linux build instructions. Warning: Complex build process!
