#include "WinApplication.h"
#include "opengl.h"
#include <iostream>
#include <vector>
#include "CShaderDirect.hpp"
#include "../OGLApp/shaders/ShaderBase.h"
#include "GameTimer.h"
#include "mathfunc.h"

class CShaderBase;

class OGLgame : public WinApplication
{
private:
	HDC m_deviceContext;
	HGLRC m_renderingContext;

	RECT rc;

	float identityMatrix[16];
	float projectionMatrix[16];
	float viewMM[16];
	float rotAngle;
	unsigned int scWidth, scHeight;

	CShaderBase* directShader;
	GameTimer timer;

	int vertxCount;

	GLuint vao;
	GLuint vbo;
	GLuint vboColor;

	bool AppBegin() override
	{
		//unsigned int w = 0, h = 0;
		GetWindowDims(scWidth, scHeight);

		std::cout << "Iniciando: " << scWidth << "," << scHeight << std::endl;

		InitializeScene();
		InitializeOpenGL(scWidth, scHeight, false);

		BuildObjects();

		timer.Reset();

		return true;
	}

	void SetMoveMatrix(float x, float y, float z)
	{
		GLfloat moveMatrix[16] =
		{
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			x, y, z, 1.0f,
		};

		directShader->setWorldMatrix(moveMatrix);
	}

	bool AppUpdate() override
	{
		timer.Tick();
		float elapsedTime = timer.DeltaTime();

		ProcessInput(elapsedTime);
		RECT rC;
		GetClientRect(GetAppHwnd(), &rC);


		GLfloat identity[16] =
		{
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f,
		};

		rotAngle += 3.1415926f / 180.0f * (10.0f * elapsedTime);
		GLfloat rotationMatrix[16] =
		{
			cosf(rotAngle), 0.0f, sinf(rotAngle), 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			-sinf(rotAngle), 0.0f, cosf(rotAngle), 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f, 
		};


		float perspectiveMatrix[4][4]{ 0 };
		orthoMatrix4x4(-1.0f, 1.0f, -1.3333f, 1.3333f, -0.01f, 100.0f, perspectiveMatrix);

		float viewMatrix[4][4]{ 0 };
		float eye[3] { 5.0f, 5.0f, 5.0f };
		float center[3] { 0.0f, 0.0f, 0.0f };
		float up[3] { 0.0f, 1.0f, 0.0f };

		lookAtRH(eye, center, up, viewMatrix);

		float* viewPtr = reinterpret_cast<float*>(viewMatrix);
		float* perspPtr = reinterpret_cast<float*>(perspectiveMatrix);

		glViewport(0, 0, rC.right - rc.left, rC.bottom - rC.top);
		glClearColor(0.3921f, 0.5843f, 0.9294f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(directShader->getShaderProgram());

		//SetMoveMatrix(0.3f, 0.5f, 0.0f);
		directShader->setWorldMatrix(rotationMatrix);
		directShader->setCamera(viewPtr, perspPtr);

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, vertxCount);

		glFlush();
		SwapBuffers(m_deviceContext);

		return true;
	}
	bool AppEnd() override
	{
		return false;
	}

	bool Size(HWND AppHwnd, WPARAM Wparam, LPARAM Lparam) override
	{
		return false;
	}

	bool DisplayChange(HWND AppHwnd, WPARAM Wparam, LPARAM Lparam) override
	{
		return false;
	}

	bool InitializeScene()
	{
		bool result;
		HWND fakeWND = CreateWindow(
			GetLpszClassName(), L"Fake Window",
			WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			0, 0,
			1, 1,
			NULL, NULL,
			GetAppInstance(), NULL);

		auto effr = GetLastError();
		HDC fakeDC = GetDC(fakeWND);
		if (!fakeDC) {
			return false;
		}

		PIXELFORMATDESCRIPTOR fakePFD;
		ZeroMemory(&fakePFD, sizeof(fakePFD));
		fakePFD.nSize = sizeof(fakePFD);
		fakePFD.nVersion = 1;
		fakePFD.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		fakePFD.iPixelType = PFD_TYPE_RGBA;
		fakePFD.cColorBits = 32;
		fakePFD.cAlphaBits = 8;
		fakePFD.cDepthBits = 24;
		int fakePFDID = ChoosePixelFormat(fakeDC, &fakePFD);
		if (fakePFDID == 0) {
			return false;
		}

		// temporary begin
		{
			int error = SetPixelFormat(fakeDC, fakePFDID, &fakePFD);
			if (error != 1) {
				return false;
			}

			HGLRC fakeRC = wglCreateContext(fakeDC);
			if (!fakeRC) {
				return false;
			}

			error = wglMakeCurrent(fakeDC, fakeRC);
			if (error != 1) {
				return false;
			}

			result = LoadExtensionList();
			if (!result) {
				return false;
			}

			wglMakeCurrent(NULL, NULL);
			wglDeleteContext(fakeRC);
			fakeRC = NULL;
		}
		// Temporary end
		ReleaseDC(fakeWND, fakeDC);
		fakeDC = 0;

		return true;
	}
	bool InitializeOpenGL(int screenWidth, int screenHeight, bool vsync)
	{
		int result;

		m_deviceContext = GetDC(GetAppHwnd());
		if (!m_deviceContext) {
			return false;
		}

		int attributeListInt[19];
		attributeListInt[0] = WGL_SUPPORT_OPENGL_ARB;
		attributeListInt[1] = TRUE;

		attributeListInt[2] = WGL_DRAW_TO_WINDOW_ARB;
		attributeListInt[3] = TRUE;

		attributeListInt[4] = WGL_ACCELERATION_ARB;
		attributeListInt[5] = WGL_FULL_ACCELERATION_ARB;

		attributeListInt[6] = WGL_COLOR_BITS_ARB;
		attributeListInt[7] = 24;

		attributeListInt[8] = WGL_DEPTH_BITS_ARB;
		attributeListInt[9] = 24;

		attributeListInt[10] = WGL_DOUBLE_BUFFER_ARB;
		attributeListInt[11] = TRUE;

		attributeListInt[12] = WGL_SWAP_METHOD_ARB;
		attributeListInt[13] = WGL_SWAP_EXCHANGE_ARB;

		attributeListInt[14] = WGL_PIXEL_TYPE_ARB;
		attributeListInt[15] = WGL_TYPE_RGBA_ARB;

		attributeListInt[16] = WGL_STENCIL_BITS_ARB;
		attributeListInt[17] = 8;

		attributeListInt[18] = 0;

		unsigned int formatCount;
		int pixelFormat[1];
		result = wglChoosePixelFormatARB(m_deviceContext, attributeListInt, NULL, 1, pixelFormat, &formatCount);
		if (result != 1) {
			return false;
		}

		PIXELFORMATDESCRIPTOR pixelFormatDescriptor;
		result = SetPixelFormat(m_deviceContext, pixelFormat[0], &pixelFormatDescriptor);
		if (result != 1) {
			auto effr = GetLastError();
			return false;
		}
		int attributeList[5];
		attributeList[0] = WGL_CONTEXT_MAJOR_VERSION_ARB;
		attributeList[1] = 3;
		attributeList[2] = WGL_CONTEXT_MINOR_VERSION_ARB;
		attributeList[3] = 3;
		attributeList[4] = 0;

		m_renderingContext = wglCreateContextAttribsARB(m_deviceContext, 0, attributeList);
		if (m_renderingContext == NULL) {
			return false;
		}
		result = wglMakeCurrent(m_deviceContext, m_renderingContext);
		if (result != 1) {
			return false;
		}
		glClearDepth(1.0f);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		//glFrontFace(GL_CW);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		char* vendorString = (char*)glGetString(GL_VENDOR);
		char* rendererString = (char*)glGetString(GL_RENDERER);
		char* versionString = (char*)glGetString(GL_VERSION);

		std::cout << vendorString << "-->" << rendererString
			<< " / " << versionString << std::endl;

		if (vsync) { result = wglSwapIntervalEXT(1); }
		else { result = wglSwapIntervalEXT(0); }

		if (result != 1) { return false; }

		return true;
	}

	void BuildObjects()
	{
		directShader = new CShaderDirect();
		directShader->initializeShader("data/coloresae.vxs", "data/coloresae.pxs");

		GLfloat g_vertex_buffer_data[] =
		{
			//frente
			-0.3f, -0.3f, 0.0f,
			0.3f, -0.3f, 0.0f,
			-0.3f, 0.3f, 0.0f,

			0.3f, 0.3f, 0.0f,
			-0.3f, 0.3f, 0.0f,
			0.3f, -0.3f, 0.0f,

			//derecha
			0.3f, -0.3f, -0.0f,
			0.3f, -0.3f, -0.6f,
			0.3f, 0.3f, 0.0f,

			0.3f, 0.3f, -0.6f,
			0.3f, 0.3f, -0.0f,
			0.3f, -0.3f, -0.6f,

			//atras
			0.3f, 0.3f, -0.6f,
			-0.3f, -0.3f, -0.6f,
			-0.3f, 0.3f, -0.6f,

			-0.3f, -0.3f, -0.6f,
			0.3f, 0.3f, -0.6f,
			0.3f, -0.3f, -0.6f,

			//izquierda
			-0.3f, 0.3f, -0.0f,
			-0.3f, 0.3f, -0.6f,
			-0.3f, -0.3f, 0.0f,

			-0.3f, -0.3f, -0.6f,
			-0.3f, -0.3f, -0.0f,
			-0.3f, 0.3f, -0.6f,
		};

		GLfloat colores[] =
		{
			//frente
			1.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 1.0f,

			1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f,
			0.0f, 1.0f, 0.0f,

			//derecha
			1.0f, 0.5f, 1.0f,
			1.0f, 1.0f, 0.5f,
			0.5f, 1.0f, 1.0f,

			1.0f, 0.5f, 1.0f,
			0.5f, 1.0f, 1.0f,
			1.0f, 1.0f, 0.5f,

			//atras
			1.0f, 0.0f, 1.0f,
			0.0f, 1.0f, 1.0f,
			0.0f, 1.0f, 1.0f,

			1.0f, 0.0f, 1.0f,
			0.0f, 1.0f, 1.0f,
			0.0f, 1.0f, 1.0f,

			//izquierda
			0.5f, 1.0f, 0.5f,
			0.5f, 0.5f, 1.0f,
			1.0f, 0.5f, 0.5f,

			0.5f, 1.0f, 0.5f,
			1.0f, 0.5f, 0.5f,
			0.5f, 0.5f, 1.0f,
		};

		int pointCount = sizeof(g_vertex_buffer_data) / sizeof(g_vertex_buffer_data[0]);
		int colorCount = sizeof(colores) / sizeof(colores[0]);

		vertxCount = static_cast<int>(pointCount / 3.0f);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glGenBuffers(1, &vboColor);

		//----------------------------------------------
		GLuint posLoc = directShader->getPosLoc();
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(GLfloat), g_vertex_buffer_data, GL_STATIC_DRAW);

		glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(posLoc);

		//---------------------------------------------
		GLuint colorLoc = directShader->getColorLoc();
		glBindBuffer(GL_ARRAY_BUFFER, vboColor);
		glBufferData(GL_ARRAY_BUFFER, colorCount * sizeof(GLfloat), colores, GL_STATIC_DRAW);

		glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(colorLoc);
	}
	void ReleaseObjects()
	{

	}
	void ProcessInput(float elapsedTime)
	{

	}

	void MouseInput(short x, short y)
	{

	}

public:

	OGLgame(void) 
		: WinApplication()
	{

	}
	virtual ~OGLgame(void)
	{

	}
};