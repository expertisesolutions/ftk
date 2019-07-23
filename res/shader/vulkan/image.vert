#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in mat4 transformMatrix;

layout(location = 0) out vec2 fragTexCoord;

layout(location = 3) out flat vec2 borderxy;

// vec4 background_position[6]
// =
//   {
//    {-1.0f, -1.0f, 0.0f, 1.0f}
//    ,  {1.0f, -1.0f, 0.0f, 1.0f}
//    ,  {1.0f,  1.0f, 0.0f, 1.0f}
//    ,  {1.0f,  1.0f, 0.0f, 1.0f}
//    , {-1.0f,  1.0f, 0.0f, 1.0f}
//    , {-1.0f, -1.0f, 0.0f, 1.0f}
//   };
// vec2 background_texcoord[6]
// =
//   {
//       {0.0f,  0.0f}
//    ,  {1.0f,  0.0f}
//    ,  {1.0f,  1.0f}
//    ,  {1.0f,  1.0f}
//    ,  {0.0f,  1.0f}
//    ,  {0.0f,  0.0f}
//   };

void main() {
  borderxy = inTexCoord;
  //gl_Position = background_position[gl_VertexIndex] * transformMatrix;
  // if (transformMatrix[0].r == 0.0)
  //   gl_Position = vec4 (positions[gl_VertexIndex], 0.0, 1.0);
  // else
  //   gl_Position = vec4 (positions2[gl_VertexIndex], 0.0, 1.0);
  gl_Position = inPosition;
  fragTexCoord = inTexCoord;
  //fragTexCoord = background_texcoord[gl_VertexIndex];
}
