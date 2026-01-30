glslangValidator -I"." -V -o debug/line.vert.spv debug/line.vert
glslangValidator -I"." -V -o debug/line.frag.spv debug/line.frag

glslangValidator -I"." -V -o vdb/soft_renderer.vert.spv vdb/soft_renderer.vert
glslangValidator -I"." -V -o vdb/soft_renderer.frag.spv vdb/soft_renderer.frag

glslangValidator -I"." -V -o vdb/world_gen.comp.spv vdb/world_gen.comp
glslangValidator -I"." -V -o vdb/lod_gen.comp.spv vdb/lod_gen.comp