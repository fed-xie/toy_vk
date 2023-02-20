print("Asset init");

scene = {
	models = {
		"assets/models/box.gltf",
	},
	shaders = {},
	nodes = {},
};

require("assets/scripts/utils");


scenes = {
	scene,
};

entry_scene = 1;

print_r(scenes);