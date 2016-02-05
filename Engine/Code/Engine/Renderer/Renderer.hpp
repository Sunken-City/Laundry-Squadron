#pragma once
#include <string>
#include "Engine/Renderer/RGBA.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/Vector4.hpp"
#include "Engine/Math/Matrix4x4.hpp"

//-----------------------------------------------------------------------------------------------
#define UNUSED(x) (void)(x);

//---------------------------------------------------------------------------------
// FIXMEs / TODOs / NOTE macros from http://www.flipcode.com/archives/FIXME_TODO_Notes_As_Warnings_In_Compiler_Output.shtml
#define _QUOTE(x) # x
#define QUOTE(x) _QUOTE(x)
#define __FILE__LINE__ __FILE__ "(" QUOTE(__LINE__) ") : "

#define NOTE( x )  message( x )
#define FILE_LINE  message( __FILE__LINE__ )

#define TODO( x )  message( __FILE__LINE__"\n"           \
        " ------------------------------------------------\n" \
        "|  TODO :   " #x "\n" \
        " -------------------------------------------------\n" )
#define FIXME( x )  message(  __FILE__LINE__"\n"           \
        " ------------------------------------------------\n" \
        "|  FIXME :  " #x "\n" \
        " -------------------------------------------------\n" )
#define todo( x )  message( __FILE__LINE__" TODO :   " #x "\n" ) 
#define fixme( x )  message( __FILE__LINE__" FIXME:   " #x "\n" ) 

//-----------------------------------------------------------------------------------
#define CHECK_GL_ERRORS
#define GL_CHECK_ERROR()  Renderer::GLCheckError(__FILE__, __LINE__);


//-----------------------------------------------------------------------------------
class AABB2;
class AABB3;
class Texture;
class Face;
class BitmapFont;
class ShaderProgram;
struct Vertex_PCT;

class Renderer
{
public:
    //ENUMS//////////////////////////////////////////////////////////////////////////
    enum DrawMode
    {
        QUADS,
        QUAD_STRIP,
        POINTS,
        LINES,
        LINE_LOOP,
        POLYGON,
        TRIANGLES,
        NUM_DRAW_MODES
    };

    //CONSTRUCTORS//////////////////////////////////////////////////////////////////////////
    Renderer();
    ~Renderer();

    //FUNCTIONS//////////////////////////////////////////////////////////////////////////
    void ClearScreen(float red, float green, float blue);
    void ClearScreen(const RGBA& color);
    void PushMatrix();
    void PopMatrix();
    void Translate(float x, float y, float z);
    void Translate(const Vector2& xy);
    void Translate(const Vector3& xyz);
    void Rotate(float rotationDegrees);
    void Rotate(float rotationDegrees, float x, float y, float z);
    void Scale(float x, float y, float z);
    unsigned char GetDrawMode(DrawMode mode);

    //STATE MODIFICATION//////////////////////////////////////////////////////////////////////////
    void EnableAdditiveBlending();
    void EnableAlphaBlending();
    void EnableInvertedBlending();
    void EnableDepthTest(bool enabled);
    void EnableFaceCulling(bool enabled);
    void BindTexture(const Texture& texture);
    void UnbindTexture();
    void SetOrtho(const Vector2& bottomLeft, const Vector2& topRight);
    void SetPerspective(float fovDegreesY, float aspect, float nearDist, float farDist); 
    void SetColor(float red, float green, float blue, float alpha);
    void SetColor(const RGBA& color);
    void SetPointSize(float size);
    void SetLineWidth(float width);

    //BUFFERS//////////////////////////////////////////////////////////////////////////
    int GenerateBufferID();
    void DeleteBuffers(int vboID);
    void BindAndBufferVBOData(int vboID, const Vertex_PCT* vertexes, int numVerts);
    void DrawVertexArray(const Vertex_PCT* vertexes, int numVertexes, DrawMode drawMode = QUADS, Texture* texture = nullptr); 
    void DrawVBO_PCT(unsigned int vboID, int numVerts, DrawMode drawMode = QUADS, Texture* texture = nullptr);

    //DRAWING//////////////////////////////////////////////////////////////////////////
    void DrawPoint(const Vector2& point, const RGBA& color = RGBA::WHITE, float pointSize = 1.0f);
    void DrawPoint(const Vector3& point, const RGBA& color = RGBA::WHITE, float pointSize = 1.0f);
    void DrawPoint(float x, float y, const RGBA& color = RGBA::WHITE, float pointSize = 1.0f);
    void DrawLine(const Vector2& start, const Vector2& end, const RGBA& color = RGBA::WHITE, float lineThickness = 1.0f);
    void DrawLine(const Vector3& start, const Vector3& end, const RGBA& color = RGBA::WHITE, float lineThickness = 1.0f);
    void DrawAABB(const AABB2& bounds, const RGBA& color = RGBA::WHITE);
    void DrawAABB(const AABB3& bounds, const RGBA& color = RGBA::WHITE);
    void DrawAABBBoundingBox(const AABB3& bounds, const RGBA& color = RGBA::WHITE);
    void DrawTexturedAABB3(const AABB3& bounds, const RGBA& color = RGBA::WHITE, const Vector2& texCoordMins = Vector2::ZERO, const Vector2& texCoordMaxs = Vector2::ONE, Texture* texture = nullptr);
    void DrawTexturedAABB(const AABB2& bounds, const Vector2& texCoordMins, const Vector2& texCoordMaxs, Texture* texture = nullptr, const RGBA& color = RGBA::WHITE);
    void DrawTexturedFace(const Face& face, const Vector2& texCoordMins, const Vector2& texCoordMaxs, Texture* texture = nullptr, const RGBA& color = RGBA::WHITE);
    void DrawPolygonOutline(const Vector2& center, float radius, int numSides, float radianOffset, const RGBA& color = RGBA::WHITE);
    void DrawPolygon(const Vector2& center, float radius, int numSides, float radianOffset, const RGBA& color = RGBA::WHITE);
    void DrawText2D(const Vector2& startBottomLeft, const std::string& asciiText, float cellWidth, float cellHeight, const RGBA& tint = RGBA::WHITE, bool drawShadow = false, const BitmapFont* font = nullptr);
    void DrawText2D(const Vector2& position, const std::string& asciiText, float scale, const RGBA& tint = RGBA::WHITE, bool drawShadow = false, const BitmapFont* font = nullptr, const Vector2& right = Vector2::UNIT_X, const Vector2& up = Vector2::UNIT_Y);

    //TEMPORARY SECTION FOR IN-CLASS LECTURE
    //All of these functions were coded as part of a lecture, and the refactor pass is a later lecture.
                                    typedef unsigned int GLuint;
                                    typedef int GLint;
                                    typedef int GLsizei;
                                    typedef unsigned int GLenum;
                                    typedef bool GLboolean;
                                    
                                    void BindMeshToVAO(GLuint vao, GLuint vbo, GLuint ibo, ShaderProgram* program);
                                    GLuint RenderBufferCreate(void* data, size_t count, size_t elementSize, GLenum usage/* = GL_STATIC_DRAW*/);
                                    void RenderBufferDestroy(GLuint buffer);                                    
                                    int CreateSampler(GLenum min_filter, GLenum magFilter, GLenum uWrap, GLenum vWrap);
                                    static void GLCheckError(const char* file, size_t line);
    //END TEMPORARY SECTION FOR CLASS
                            
    //CONSTANTS//////////////////////////////////////////////////////////////////////////
    static const int CIRCLE_SIDES = 50;
    static const int HEXAGON_SIDES = 6;
    static const unsigned char plainWhiteTexel[3];

    //MEMBER VARIABLES//////////////////////////////////////////////////////////////////////////
    static Renderer* instance;
private:
    BitmapFont* m_defaultFont;
    Texture* m_defaultTexture;
};