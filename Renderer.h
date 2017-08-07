
struct RenderState
{
   GLuint fbo;
   GLuint mainColorTexture;
   GLuint blurTexture;
   GLuint depthBuffer;

   GLuint fullScreenProgram;
   GLuint blurProgram;

   GLuint horizontalFbo;
   GLuint horizontalColorBuffer;

   GLuint verticalFbo;
   GLuint verticalColorBuffer;
};
