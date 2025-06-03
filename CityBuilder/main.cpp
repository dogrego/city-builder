// GLEW
#include <GL/glew.h>

// SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

// ImGui
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

// Standard
#include <iostream>
#include <sstream>

#include "MyApp.h"

int main(int argc, char *args[])
{
	// Step 1: Initialize SDL

	// Set the error logging function
	SDL_LogSetPriority(SDL_LOG_CATEGORY_ERROR, SDL_LOG_PRIORITY_ERROR);

	// Initialize the graphics subsystem, if there is an issue, log and exit
	if (SDL_Init(SDL_INIT_VIDEO) == -1)
	{
		// Log the error and terminate the program
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[SDL initialization] Error during the SDL initialization: %s", SDL_GetError());
		return 1;
	}

	// Ensure SDL quits properly when the program exits, even in case of errors
	std::atexit(SDL_Quit);

	// Step 2: Configure OpenGL settings, create the window, and start OpenGL

	// Step 2a: Configure OpenGL startup settings before creating the window

	// Define the exact OpenGL context to use, if not set, the highest available version will be used
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef _DEBUG
	// If compiled in debug mode, enable OpenGL debug context for debug callbacks
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	// Set the bit depth for storing red, green, blue, and alpha information per pixel
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	// Enable double buffering
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// Define the bit depth of the depth buffer
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// Enable anti-aliasing if needed
	// SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,  1);
	// SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,  2);

	// Create the window
	SDL_Window *win = nullptr;
	win = SDL_CreateWindow("Hello SDL&OpenGL!",																					 // Window title
												 100,																													 // Initial X coordinate of top-left corner
												 100,																													 // Initial Y coordinate of top-left corner
												 800,																													 // Window width
												 600,																													 // Window height
												 SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE); // Display properties

	// If window creation fails, log the error and exit
	if (win == nullptr)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Window creation] Error during the SDL initialization: %s", SDL_GetError());
		return 1;
	}

	// Step 3: Create the OpenGL context, used for rendering

	SDL_GLContext context = SDL_GL_CreateContext(win);
	if (context == nullptr)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[OGL context creation] Error during the creation of the OGL context: %s", SDL_GetError());
		return 1;
	}

	// Enable vsync for rendering
	SDL_GL_SetSwapInterval(1);

	// Initialize GLEW
	GLenum error = glewInit();
	if (error != GLEW_OK)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[GLEW] Error during the initialization of glew.");
		return 1;
	}

	// Retrieve the OpenGL version
	int glVersion[2] = {-1, -1};
	glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
	glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);

	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Running OpenGL %d.%d", glVersion[0], glVersion[1]);

	// If OpenGL version retrieval failed, exit
	if (glVersion[0] == -1 && glVersion[1] == -1)
	{
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(win);

		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[OGL context creation] Error during the initialization of the OGL context! Maybe one of the SDL_GL_SetAttribute(...) calls is erroneous.");
		return 1;
	}

	std::stringstream window_title;
	window_title << "OpenGL " << glVersion[0] << "." << glVersion[1];
	SDL_SetWindowTitle(win, window_title.str().c_str());

	// Initialize ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForOpenGL(win, context);
	ImGui_ImplOpenGL3_Init();

	// Step 4: Start the main event processing loop

	{
		// Should the program terminate?
		bool quit = false;
		// Stores incoming events
		SDL_Event ev;

		// Application instance
		CMyApp app;
		if (!app.Init())
		{
			SDL_GL_DeleteContext(context);
			SDL_DestroyWindow(win);
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[app.Init] Error during the initialization of the application!");
			return 1;
		}

		// Show ImGui window
		bool ShowImGui = true;

		while (!quit)
		{
			// Process incoming events
			while (SDL_PollEvent(&ev))
			{
				ImGui_ImplSDL2_ProcessEvent(&ev);
				bool is_mouse_captured = ImGui::GetIO().WantCaptureMouse;				// Does ImGui need the mouse?
				bool is_keyboard_captured = ImGui::GetIO().WantCaptureKeyboard; // Does ImGui need the keyboard?

				// Handling events
				switch (ev.type)
				{
				case SDL_QUIT:
					quit = true;
					break;
				case SDL_KEYDOWN:
					if (ev.key.keysym.sym == SDLK_ESCAPE)
						quit = true;

					// ALT + ENTER switches to full screen and back
					if ((ev.key.keysym.sym == SDLK_RETURN)														 // Enter is pressed
							&& (ev.key.keysym.mod & KMOD_ALT)															 // Along with ALT
							&& !(ev.key.keysym.mod & (KMOD_SHIFT | KMOD_CTRL | KMOD_GUI))) // No other modifier keys are pressed
					{
						Uint32 FullScreenSwitchFlag = (SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP;
						SDL_SetWindowFullscreen(win, FullScreenSwitchFlag);
						is_keyboard_captured = true; // The application should not receive the ALT+ENTER event
					}

					// CTRL + F1 toggles ImGui visibility
					if ((ev.key.keysym.sym == SDLK_F1)																// F1 is pressed
							&& (ev.key.keysym.mod & KMOD_CTRL)														// Along with CTRL
							&& !(ev.key.keysym.mod & (KMOD_SHIFT | KMOD_ALT | KMOD_GUI))) // No other modifier keys are pressed
					{
						ShowImGui = !ShowImGui;
						is_keyboard_captured = true; // The application should not receive the CTRL+F1 event
					}

					if (!is_keyboard_captured)
						app.KeyboardDown(ev.key);
					break;

				case SDL_KEYUP:
					if (!is_keyboard_captured)
						app.KeyboardUp(ev.key);
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (!is_mouse_captured)
						app.MouseDown(ev.button);
					break;
				case SDL_MOUSEBUTTONUP:
					if (!is_mouse_captured)
						app.MouseUp(ev.button);
					break;
				case SDL_MOUSEWHEEL:
					if (!is_mouse_captured)
						app.MouseWheel(ev.wheel);
					break;
				case SDL_MOUSEMOTION:
					if (!is_mouse_captured)
						app.MouseMove(ev.motion);
					break;
				case SDL_WINDOWEVENT:
					// On some platforms (such as Windows), SIZE_CHANGED is not triggered on the initial display
					// We believe this is a bug in the SDL library
					// Therefore, this case is handled separately,
					// as MyApp may contain window-size-dependent settings such as the camera aspect ratio in the perspective() function
					if ((ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) || (ev.window.event == SDL_WINDOWEVENT_SHOWN))
					{
						int w, h;
						SDL_GetWindowSize(win, &w, &h);
						app.Resize(w, h);
					}
					break;

				default:
					app.OtherEvent(ev);
				}
			}

			// Calculate the necessary update time values
			static Uint32 LastTick = SDL_GetTicks(); // Store the previous tick statically
			Uint32 CurrentTick = SDL_GetTicks();		 // Get the current tick
			SUpdateInfo updateInfo									 // Convert to seconds
					{
							static_cast<float>(CurrentTick) / 1000.0f,
							static_cast<float>(CurrentTick - LastTick) / 1000.0f};
			LastTick = CurrentTick; // Save the current tick as the last tick

			app.Update(updateInfo);
			app.Render();

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplSDL2_NewFrame(); // After this, ImGui commands can be issued until ImGui::Render()

			ImGui::NewFrame();
			if (ShowImGui)
				app.RenderGUI();
			ImGui::Render();

			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			SDL_GL_SwapWindow(win);
		}

		// Cleanup application resources
		app.Clean();
	}

	// Step 5: Exit the program

	// Deinitialize ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(win);

	return 0;
}
