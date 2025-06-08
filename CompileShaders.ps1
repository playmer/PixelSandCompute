.\shadercross.exe Shaders/FillTexture.comp.hlsl --dest MSL --stage compute --output Shaders/Compiled/MSL/FillTexture.comp.msl
.\shadercross.exe Shaders/FillTexture.comp.hlsl --dest DXIL --stage compute --output Shaders/Compiled/DXIL/FillTexture.comp.dxil
.\shadercross.exe Shaders/FillTexture.comp.hlsl --dest SPIRV --stage compute --output Shaders/Compiled/SPIRV/FillTexture.comp.spv

.\shadercross.exe Shaders/TexturedQuad.vert.hlsl --dest MSL --stage vertex --output Shaders/Compiled/MSL/TexturedQuad.vert.msl
.\shadercross.exe Shaders/TexturedQuad.vert.hlsl --dest DXIL --stage vertex --output Shaders/Compiled/DXIL/TexturedQuad.vert.dxil
.\shadercross.exe Shaders/TexturedQuad.vert.hlsl --dest SPIRV --stage vertex --output Shaders/Compiled/SPIRV/TexturedQuad.vert.spv

.\shadercross.exe Shaders/TexturedQuad.frag.hlsl --dest MSL --stage fragment --output Shaders/Compiled/MSL/TexturedQuad.frag.msl
.\shadercross.exe Shaders/TexturedQuad.frag.hlsl --dest DXIL --stage fragment --output Shaders/Compiled/DXIL/TexturedQuad.frag.dxil
.\shadercross.exe Shaders/TexturedQuad.frag.hlsl --dest SPIRV --stage fragment --output Shaders/Compiled/SPIRV/TexturedQuad.frag.spv
