#pragma once

#include <string>
#include <cstdlib>

#define LOG_INFO(FMT,...) printf("[INFO] " FMT "\n",__VA_ARGS__)
#define LOG_WARN(FMT,...) printf("[WARN] " FMT "\n",__VA_ARGS__)
#define LOG_ERROR(FMT,...) printf("[ERRO] " FMT "\n",__VA_ARGS__)

#define _GL_CHECK_FOR_ERRORS() __CheckForGLErrors(__FILE__, __LINE__,__FUNCTION__)

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef NDEBUG
#define _GL_WRAP0(GLFUNC) GLFUNC(); __CheckForGLErrors(__FILE__, __LINE__,#GLFUNC)
#define _GL_WRAP1(GLFUNC,a) GLFUNC((a)); __CheckForGLErrors(__FILE__, __LINE__,#GLFUNC)
#define _GL_WRAP2(GLFUNC,a,b) GLFUNC((a),(b)); __CheckForGLErrors(__FILE__, __LINE__,#GLFUNC)
#define _GL_WRAP3(GLFUNC,a,b,c) GLFUNC((a),(b),(c)); __CheckForGLErrors(__FILE__, __LINE__,#GLFUNC)
#define _GL_WRAP4(GLFUNC,a,b,c,d) GLFUNC((a),(b),(c),(d)); __CheckForGLErrors(__FILE__, __LINE__,#GLFUNC)
#define _GL_WRAP5(GLFUNC,a,b,c,d,e) GLFUNC((a),(b),(c),(d),(e)); __CheckForGLErrors(__FILE__, __LINE__,#GLFUNC)
#define _GL_WRAP6(GLFUNC,a,b,c,d,e,f) GLFUNC((a),(b),(c),(d),(e),(f)); __CheckForGLErrors(__FILE__, __LINE__,#GLFUNC)
#define _GL_WRAP7(GLFUNC,a,b,c,d,e,f,g) GLFUNC((a),(b),(c),(d),(e),(f),(g)); __CheckForGLErrors(__FILE__, __LINE__,#GLFUNC)
#define _GL_WRAP8(GLFUNC,a,b,c,d,e,f,g,h) GLFUNC((a),(b),(c),(d),(e),(f),(g),(h)); __CheckForGLErrors(__FILE__, __LINE__,#GLFUNC)
#define _GL_WRAP9(GLFUNC,a,b,c,d,e,f,g,h,i) GLFUNC((a),(b),(c),(d),(e),(f),(g),(h),(i)); __CheckForGLErrors(__FILE__, __LINE__,#GLFUNC)
#else
#define _GL_WRAP0(GLFUNC) GLFUNC();
#define _GL_WRAP1(GLFUNC,a) GLFUNC((a));
#define _GL_WRAP2(GLFUNC,a,b) GLFUNC((a),(b));
#define _GL_WRAP3(GLFUNC,a,b,c) GLFUNC((a),(b),(c));
#define _GL_WRAP4(GLFUNC,a,b,c,d) GLFUNC((a),(b),(c),(d));
#define _GL_WRAP5(GLFUNC,a,b,c,d,e) GLFUNC((a),(b),(c),(d),(e));
#define _GL_WRAP6(GLFUNC,a,b,c,d,e,f) GLFUNC((a),(b),(c),(d),(e),(f));
#define _GL_WRAP7(GLFUNC,a,b,c,d,e,f,g) GLFUNC((a),(b),(c),(d),(e),(f),(g));
#define _GL_WRAP8(GLFUNC,a,b,c,d,e,f,g,h) GLFUNC((a),(b),(c),(d),(e),(f),(g),(h));
#define _GL_WRAP9(GLFUNC,a,b,c,d,e,f,g,h,i) GLFUNC((a),(b),(c),(d),(e),(f),(g),(h),(i));
#endif

std::string ReadFile(const char* path);

void __CheckForGLErrors(const char* file, int line, const char* function);
