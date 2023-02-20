#pragma once

#include <stdint.h>
#include <stdbool.h>


#define GLTF_MAX_INDEX UINT64_MAX
typedef uint64_t gltfIndex;
typedef size_t gltfOffset;
typedef size_t gltfSize;

typedef struct gltfString {
    const char* characters;
    size_t strlen;
}gltfString;

typedef enum gltfComponentType {
    GLTF_COMPONENT_TYPE_BYTE = 5120, // 8-bits
    GLTF_COMPONENT_TYPE_UNSIGNED_BYTE = 5121, // 8-bits
    GLTF_COMPONENT_TYPE_SHORT = 5122, // 16-bits
    GLTF_COMPONENT_TYPE_UNSIGNED_SHORT = 5123, // 16-bits
    GLTF_COMPONENT_TYPE_UNSIGNED_INT = 5125, // 32-bits
    GLTF_COMPONENT_TYPE_FLOAT = 5126, // 32-bits
}gltfComponentType;

typedef struct gltfAccessorSparseIndices {
    gltfIndex bufferView; // The index of the buffer view with sparse indices. The referenced buffer view MUST NOT have its target or byteStride properties defined. The buffer view and the optional byteOffset MUST be aligned to the componentType byte length.
    gltfOffset byteOffset; // The offset relative to the start of the buffer view in bytes. default 0
    gltfComponentType componentType; // The indices data type.
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfAccessorSparseIndices;

typedef struct gltfAccessorSparseValues {
    gltfIndex bufferView;  // The index of the bufferView with sparse values. The referenced buffer view MUST NOT have its target or byteStride properties defined.
    gltfOffset byteOffset; // The offset relative to the start of the bufferView in bytes.
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfAccessorSparseValues;

typedef struct gltfAccessorSparse {
    size_t count; // Number of deviating accessor values stored in the sparse array.
    gltfAccessorSparseIndices indices; // An object pointing to a buffer view containing the indices of deviating accessor values. The number of indices is equal to count. Indices MUST strictly increase.
    gltfAccessorSparseValues values; // An object pointing to a buffer view containing the deviating accessor values.
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfAccessorSparse;

enum gltfAccessorType {
    GLTF_TYPE_ASSERT_ERROR_NONE = 0,
    GLTF_TYPE_SCALAR,
    GLTF_TYPE_VEC2,
    GLTF_TYPE_VEC3,
    GLTF_TYPE_VEC4,
    GLTF_TYPE_MAT2, // column-major
    GLTF_TYPE_MAT3, // column-major
    GLTF_TYPE_MAT4, // column-major
};

typedef struct gltfAccessor {
    gltfIndex bufferView; // The index of the bufferView.
    gltfOffset byteOffset; // The offset relative to the start of the buffer view in bytes.
    gltfComponentType componentType; // The datatype of the accessor's components.
    enum gltfAccessorType type; // Specifies if the accessor's elements are scalars, vectors, or matrices.
    bool normalized; // Specifies whether integer data values are normalized before usage.
    size_t count; // The number of elements referenced by this accessor.
    float max[16]; // Maximum value of each component in this accessor.
    float min[16]; // Minimum value of each component in this accessor.
    gltfAccessorSparse sparse; // Sparse storage of elements that deviate from their initialization value.
    gltfString name; // The user-defined name of this object.
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfAccessor;

enum gltfAnimationChannelTargetPath {
    GLTF_ANIMATION_CHANNEL_TARGET_TRANSLATION,
    GLTF_ANIMATION_CHANNEL_TARGET_ROTATION,
    GLTF_ANIMATION_CHANNEL_TARGET_SCALE,
    GLTF_ANIMATION_CHANNEL_TARGET_WEIGHTS,
};

typedef struct gltfAnimationChannelTarget {
    gltfIndex node; // The index of the node to animate. When undefined, the animated object MAY be defined by an extension.
    /*
    The name of the node's TRS property to animate, or the "weights" of the Morph Targets it instantiates.
    For the "translation" property, the values that are provided by the sampler are the translation along the X, Y, and Z axes.
    For the "rotation" property, the values are a quaternion in the order (x, y, z, w), where w is the scalar.
    For the "scale" property, the values are the scaling factors along the X, Y, and Z axes.
    */
    enum gltfAnimationChannelTargetPath path;
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfAnimationChannelTarget;

typedef struct gltfAnimationChannel {
    gltfIndex sampler; // The index of a sampler in this animation used to compute the value for the target.
    gltfAnimationChannelTarget target; // The descriptor of the animated property.
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfAnimationChannel;

enum gltfAnimSamplerInterpolation {
    GLTF_ANIM_SAMPLER_INTERPOLATION_LINEAR = 0,
    GLTF_ANIM_SAMPLER_INTERPOLATION_STEP,
    GLTF_ANIM_SAMPLER_INTERPOLATION_CUBICSPLINE,
};

typedef struct gltfAnimationSampler {
    gltfIndex input; // The index of an accessor containing keyframe timestamps.
    gltfIndex output; // The index of an accessor, containing keyframe output values.
    enum gltfAnimSamplerInterpolation interpolation; // Interpolation algorithm. default: LINEAR
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfAnimationSampler;

typedef struct gltfAnimation {
    gltfAnimationChannel* channels; // An array of animation channels. An animation channel combines an animation sampler with a target property being animated. Different channels of the same animation MUST NOT have the same targets.
    gltfAnimationSampler* samplers; // An array of animation samplers. An animation sampler combines timestamps with a sequence of output values and defines an interpolation algorithm.
    size_t channel_count;
    size_t sampler_count;
    gltfString name;
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfAnimation;

typedef struct gltfVersion {
    uint32_t major;
    uint32_t minor;
}gltfVersion;

typedef struct gltfAsset {
    gltfString copyright;
    gltfString generator;
    gltfVersion version;
    gltfVersion minVersion;
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfAsset;

enum gltfBufferReferenceType {
    GLTF_BUFFER_REFERENCE_TYPE_NONE = 0,
    GLTF_BUFFER_REFERENCE_TYPE_URI,
    GLTF_BUFFER_REFERENCE_TYPE_DATA,
};

typedef struct gltfBuffer {
    union {
        gltfString uri;
        uint8_t* data; // little-endian data
    };
    enum gltfBufferReferenceType referType;
    gltfSize byteLength;
    gltfString name;
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfBuffer;

enum gltfBufferViewTarget {
    GLTF_BUFFER_VIEW_TARGET_NONE = 0,
    GLTF_BUFFER_VIEW_TARGET_ARRAY_BUFFER = 34962,
    GLTF_BUFFER_VIEW_TARGET_ELEMENT_ARRAY_BUFFER = 34963,
};

typedef struct gltfBufferView {
    gltfIndex buffer;
    gltfOffset byteOffset; // The offset into the buffer in bytes. default: 0
    gltfSize byteLength;
    gltfSize byteStride;
    enum gltfBufferViewTarget target;
    gltfString name;
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfBufferView;

typedef struct gltfOrthographicCamera {
    float xmag; // The floating-point horizontal magnification of the view. This value MUST NOT be equal to zero. This value SHOULD NOT be negative.
    float ymag; // The floating-point vertical magnification of the view. This value MUST NOT be equal to zero. This value SHOULD NOT be negative.
    float zfar; // The floating-point distance to the far clipping plane. This value MUST NOT be equal to zero. zfar MUST be greater than znear.
    float znear; // The floating-point distance to the near clipping plane.
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfOrthographicCamera;

typedef struct gltfPerspectiveCamera {
    float aspectRatio; // The floating-point aspect ratio of the field of view.When undefined(0), the aspect ratio of the rendering viewport MUST be used.
    float yfov; // The floating-point vertical field of view in radians. This value SHOULD be less than §Õ§Ý.
    float zfar; // The floating-point distance to the far clipping plane. When defined, zfar MUST be greater than znear. If zfar is undefined(0), client implementations SHOULD use infinite projection matrix.
    float znear; // The floating-point distance to the near clipping plane.
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfPerspectiveCamera;

enum gltfCameraType {
    GLTF_CAMERA_TYPE_PERSPECTIVE,
    GLTF_CAMERA_TYPE_ORTHOGRAPHIC,
};

typedef struct gltfCamera {
    union {
        gltfOrthographicCamera orthographic;
        gltfPerspectiveCamera perspective;
    };
    enum gltfCameraType type;
    gltfString name;
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfCamera;

enum gltfImageReferenceType {
    GLTF_IMAGE_REFERENCE_TYPE_URI,
    GLTF_IMAGE_REFERENCE_TYPE_BUFFER_VIEW,
    GLTF_IMAGE_REFERENCE_TYPE_PICTURE,
};

enum gltfImageMimeType {
    GLTF_IMAGE_MIME_TYPE_NONE = 0,
    GLTF_IMAGE_MIME_TYPE_IMAGE_JPEG,
    GLTF_IMAGE_MIME_TYPE_IMAGE_PNG,
};

typedef struct gltfImage {
    union {
        gltfIndex bufferView; // The index of the bufferView that contains the image. This field MUST NOT be defined when uri is defined.
        gltfString uri;
        struct {
            uint8_t* data; // little-endian order data
            size_t data_size;
        } picture;
    };
    enum gltfImageReferenceType referType;
    enum gltfImageMimeType mimeType; // The image's media type. This field MUST be defined when bufferView is defined.
    gltfString name;
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfImage;

typedef struct gltfTextureInfo {
    gltfIndex index; // The index of the texture.
    gltfIndex texCoord; // The set index of texture's TEXCOORD attribute used for texture coordinate mapping (e.g. a value of 0 corresponds to TEXCOORD_0). default: 0
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfTextureInfo;

typedef struct gltfMaterialPbrMetallicRoughness {
    gltfTextureInfo baseColorTexture; // The base color texture.
    gltfTextureInfo metallicRoughnessTexture; // The metallic-roughness texture.
    float baseColorFactor[4]; // The factors for the base color of the material. default: [1, 1, 1, 1]
    float metallicFactor; // The factor for the metalness of the material. default: 1
    float roughnessFactor; // The factor for the roughness of the material. default: 1
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfMaterialPbrMetallicRoughness;

typedef struct gltfMaterialNormalTextureInfo {
    gltfIndex index; // The index of the texture.
    gltfIndex texCoord; // The set index of texture's TEXCOORD attribute used for texture coordinate mapping (e.g. a value of 0 corresponds to TEXCOORD_0). default: 0
    float scale; // The scalar parameter applied to each normal vector of the normal texture. default: 1
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfMaterialNormalTextureInfo;

typedef struct MaterialOcclusionTextureInfo {
    gltfIndex index; // The index of the texture.
    gltfIndex texCoord; // The set index of texture's TEXCOORD attribute used for texture coordinate mapping (e.g. a value of 0 corresponds to TEXCOORD_0). default: 0
    float strength; // A scalar multiplier controlling the amount of occlusion applied. default: 1
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}MaterialOcclusionTextureInfo;

enum gltfMaterialAlphaMode {
    GLTF_MATERIAL_ALPHA_MODE_OPAQUE = 0,
    GLTF_MATERIAL_ALPHA_MODE_MASK,
    GLTF_MATERIAL_ALPHA_MODE_BLEND,
};

typedef struct gltfMaterial {
    gltfString name;
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
    gltfMaterialPbrMetallicRoughness pbrMetallicRoughness;
    gltfMaterialNormalTextureInfo normalTexture;
    MaterialOcclusionTextureInfo occlusionTexture;
    gltfTextureInfo emissiveTexture;
    float emissiveFactor[3]; // The factors for the emissive color of the material. default [0, 0, 0]
    enum gltfMaterialAlphaMode alphaMode; // The alpha rendering mode of the material. default: OPAQUE
    float alphaCutoff; // The alpha cutoff value of the material. default: 0.5
    bool doubleSided; // Specifies whether the material is double sided. default: false
}gltfMaterial;

// See specification 3.7.2.1: Meshes.Overview
enum gltfMeshPrimitiveAttributeSemantic {
    // semantics without index (default 0)
    GLTF_MESH_PRIMITIVE_ATTR_SEMANTIC_POSITION = 0,
    GLTF_MESH_PRIMITIVE_ATTR_SEMANTIC_NORMAL,
    GLTF_MESH_PRIMITIVE_ATTR_SEMANTIC_TANGENT,
    // semantics with index
    GLTF_MESH_PRIMITIVE_ATTR_SEMANTIC_TEXCOORD,
    GLTF_MESH_PRIMITIVE_ATTR_SEMANTIC_COLOR,
    GLTF_MESH_PRIMITIVE_ATTR_SEMANTIC_JOINTS,
    GLTF_MESH_PRIMITIVE_ATTR_SEMANTIC_WEIGHTS,

    GLTF_MESH_PRIMITIVE_ATTR_SEMANTIC_MAX,

    GLTF_MESH_PRIMITIVE_ATTR_SEMANTIC_ENUM_MAX = UINT16_MAX,
};

inline uint32_t gltf_mesh_attribute_semantic (enum gltfMeshPrimitiveAttributeSemantic semantic, uint8_t index)
{
    // a value of semantic_index 0 and semantic TEXCOORD corresponds to TEXCOORD_0)
    return (uint32_t)semantic << 8 | index;
}

typedef struct gltfMeshPrimitiveAttribute {
    uint32_t semantic; // Maked by gltf_mesh_attribute_semantic()
    uint8_t index; // The index of accessor
}gltfMeshPrimitiveAttribute;

typedef struct gltfMeshPrimitiveMorphTarget {
    gltfMeshPrimitiveAttribute* attributes;
    size_t attribute_count;
}gltfMeshPrimitiveMorphTarget;

enum gltfMeshPrimitiveMode {
    GLTF_MESH_PRIMITIVE_MODE_POINTS = 0,
    GLTF_MESH_PRIMITIVE_MODE_LINES = 1,
    GLTF_MESH_PRIMITIVE_MODE_LINE_LOOP = 2,
    GLTF_MESH_PRIMITIVE_MODE_LINE_STRIP = 3,
    GLTF_MESH_PRIMITIVE_MODE_TRIANGLES = 4,
    GLTF_MESH_PRIMITIVE_MODE_TRIANGLE_STRIP = 5,
    GLTF_MESH_PRIMITIVE_MODE_TRIANGLE_FAN = 6,
};

typedef struct gltfMeshPrimitive {
    gltfMeshPrimitiveAttribute* attributes;
    gltfIndex indices; // The index of the accessor that contains the vertex indices.
    enum gltfMeshPrimitiveMode mode; // The topology type of primitives to render. default: 4(TRIANGLES)
    gltfIndex material; // The index of the material to apply to this primitive when rendering.
    gltfMeshPrimitiveMorphTarget* targets;
    size_t attribute_count;
    size_t target_count;
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfMeshPrimitive;

typedef struct gltfMesh {
    gltfMeshPrimitive* primitives;
    float* weights;
    size_t primitive_count;
    size_t weight_count;
    gltfString name;
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfMesh;

/*
A node in the node hierarchy. When the node contains skin, all mesh.primitives MUST contain JOINTS_0 and WEIGHTS_0 attributes.
A node MAY have either a matrix or any combination of translation/rotation/scale (TRS) properties.
TRS properties are converted to matrices and postmultiplied in the T * R * S order to compose the transformation matrix;
first the scale is applied to the vertices, then the rotation, and then the translation. If none are provided, the transform is the identity.
When a node is targeted for animation (referenced by an animation.channel.target), matrix MUST NOT be present.
*/
typedef struct gltfNode {
    gltfIndex camera; // The index of the camera referenced by this node.
    gltfIndex skin; // The index of the skin referenced by this node.
    gltfIndex mesh; // The index of the mesh in this node.
    union {
        struct {
            float translation[3]; // The node's translation along the x, y, and z axes. default: [0,0,0]
            float rotation[4]; // The node's unit quaternion rotation in the order (x, y, z, w), where w is the scalar. default: [0,0,0,1]
            float scale[3]; // The node's non-uniform scale, given as the scaling factors along the x, y, and z axes. default: [1,1,1]
        };
        float matrix[16]; // A floating-point 4x4 transformation matrix stored in column-major order. default: [1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]
    };
    enum {
        GLTF_NODE_TRANSFER_TYPE_TRS,
        GLTF_NODE_TRANSFER_TYPE_MATRIX,
    } transfer_type;
    gltfIndex* children; // The indices of this node's children.
    float* weights; // The weights of the instantiated morph target. The number of array elements MUST match the number of morph targets of the referenced mesh. When defined, mesh MUST also be defined.
    size_t children_count;
    size_t weight_count;
    gltfString name;
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfNode;

enum gltfSamplerFilter {
    GLTF_SAMPLER_FILTER_DEFAULT = 0,
    GLTF_SAMPLER_FILTER_NEAREST = 9728,
    GLTF_SAMPLER_FILTER_LINEAR = 9729,
    GLTF_SAMPLER_FILTER_NEAREST_MIPMAP_NEAREST = 9984,
    GLTF_SAMPLER_FILTER_LINEAR_MIPMAP_NEAREST = 9985,
    GLTF_SAMPLER_FILTER_NEAREST_MIPMAP_LINEAR = 9986,
    GLTF_SAMPLER_FILTER_LINEAR_MIPMAP_LINEAR = 9987,
};

enum gltfSamplerWrap {
    GLTF_SAMPLER_WRAP_REPEAT = 10497,
    GLTF_SAMPLER_WRAP_CLAMP_TO_EDGE = 33071,
    GLTF_SAMPLER_WRAP_MIRRORED_REPEAT = 33648,
};

typedef struct gltfSampler {
    enum gltfSamplerFilter magFilter; // NEAREST | LINEAR only
    enum gltfSamplerFilter minFilter;
    enum gltfSamplerWrap wrapS; // S (U) wrapping mode. default: 10497(REPEAT)
    enum gltfSamplerWrap wrapT; // T (V) wrapping mode. default: 10497(REPEAT)
    gltfString name;
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfSampler;

typedef struct gltfScene {
    gltfIndex* nodes; // The indices of each root node.
    size_t node_count;
    gltfString name;
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfScene;

typedef struct gltfSkin {
    gltfIndex inverseBindMatrices;
    gltfIndex skeleton;
    gltfIndex* joints; // Indices of skeleton nodes, used as joints in this skin.
    size_t joint_count;
    gltfString name;
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfSkin;

typedef struct gltfTexture {
    gltfIndex sampler;
    gltfIndex source;
    gltfString name;
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}gltfTexture;

typedef struct glTF_json_t {
    gltfString* extensionsUsed;
    gltfString* extensionsRequired;
    gltfAsset asset;
    gltfAccessor* accessors;
    gltfAnimation* animations;
    gltfBuffer* buffers;
    gltfBufferView* bufferViews;
    gltfCamera* cameras;
    gltfImage* images;
    gltfMaterial* materials;
    gltfMesh* meshes;
    gltfNode* nodes;
    gltfSampler* samplers;
    gltfIndex scene; // The index of the default scene.
    gltfScene* scenes;
    gltfSkin* skins;
    gltfTexture* textures;
    size_t extensionsUsed_count;
    size_t extensionsRequired_count;
    size_t accessor_count;
    size_t animation_count;
    size_t buffer_count;
    size_t bufferView_count;
    size_t camera_count;
    size_t image_count;
    size_t material_count;
    size_t mesh_count;
    size_t node_count;
    size_t sampler_count;
    size_t scene_count;
    size_t skin_count;
    size_t texture_count;
    void* extensions; // JSON object with extension-specific objects.
    void* extras; // Application-specific data.
}glTF_json_t;

typedef struct gltf_binary_data_t {
    uint8_t* data;
    size_t data_size;
}gltf_binary_data_t;

typedef struct glTF_binary_t {
    gltf_binary_data_t* buffers;
    gltf_binary_data_t* images;
}glTF_binary_t;

typedef struct glTF {
    glTF_json_t json;
    glTF_binary_t bin;
}glTF;
