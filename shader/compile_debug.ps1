glslangValidator -I"." -V -g -Od -o debug/line.vert.spv debug/line.vert
glslangValidator -I"." -V -g -Od -o debug/line.frag.spv debug/line.frag

glslangValidator -I"." -V -g -Od -o vdb/soft_renderer.vert.spv vdb/soft_renderer.vert
glslangValidator -I"." -V -g -Od -o vdb/soft_renderer.frag.spv vdb/soft_renderer.frag

glslangValidator -I"." -V -g -Od -o vdb/terrain_gen.comp.spv vdb/terrain_gen.comp
glslangValidator -I"." -V -g -Od -o vdb/lod_gen.comp.spv vdb/lod_gen.comp